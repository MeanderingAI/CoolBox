// distribution_tag — Master Node
//
// Demonstrates all six DISTRIBUTED library primitives in a realistic
// content-tagging pipeline:
//
//   TaskQueue<Job>      — bounded work buffer; master enqueues, workers dequeue
//   MessageBus          — pub/sub "job available" wake-up signals
//   ServiceRegistry     — workers register on startup; master monitors health
//   DistributedLock     — mutual exclusion around the shared result store
//   CircuitBreaker      — wraps each worker call; opens on repeated failures
//   (RaftNode lives in consensus/ and is not needed for a coordinator-based design)
//
// Run: ./distribution_tag_master [num_workers [http_port]]
//   num_workers  — optional, default 4
//   http_port    — optional, default 7700  (GET /cluster/topology)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#  include <process.h>
#  define GET_PID() static_cast<int>(_getpid())
#else
#  include <unistd.h>
#  define GET_PID() static_cast<int>(getpid())
#endif

#include "circuit_breaker.h"
#include "distributed_lock.h"
#include "message_bus.h"
#include "service_registry.h"
#include "task_queue.h"

#include "http_server.h"
#include "advanced_logging.h"

#include "job.h"

using namespace distributed;
using namespace distribution_tag;
using namespace std::chrono_literals;

// ── HTTP topology servlet ─────────────────────────────────────────────────────
// Implements GET /cluster/topology returning live JSON.
// GET /health returns a simple 200 OK for readiness checks.

struct TopologyState {
    const ServiceRegistry&    registry;
    const TaskQueue<Job>&     job_queue;
    const CircuitBreaker&     breaker;
    const std::atomic<int>&   jobs_done;
    int                       total_jobs;
    int                       num_workers;
    int                       pid;
    std::chrono::steady_clock::time_point started_at;
};

class TopologyServlet : public networking::servlets::HttpServletBase {
public:
    explicit TopologyServlet(const TopologyState& s) : state_(s) {}

    std::string get_version() const override { return "HTTP/1.1"; }

    Response handle_request(const Request& req) override {
        // CORS preflight
        if (req.method == "OPTIONS") {
            Response r;
            r.status_code = 204;
            r.headers[std::string("Access-Control-Allow-Origin")]  = "*";
            r.headers[std::string("Access-Control-Allow-Methods")] = "GET, OPTIONS";
            r.headers[std::string("Access-Control-Allow-Headers")] = "Content-Type";
            r.body = "";
            return r;
        }

        if (req.uri == "/health") {
            return cors(Response::ok("{\"status\":\"ok\"}"));
        }

        if (req.uri == "/cluster/topology" || req.uri == "/cluster/topology/") {
            return cors(build_topology());
        }

        return cors(Response::not_found());
    }

private:
    const TopologyState& state_;

    static Response cors(Response r) {
        r.headers[HeaderKey::ContentType]              = "application/json";
        r.headers[std::string("Access-Control-Allow-Origin")] = "*";
        return r;
    }

    Response build_topology() const {
        using Clock = std::chrono::steady_clock;
        const auto now     = Clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                 now - state_.started_at).count();
        const int  done    = state_.jobs_done.load();
        const double jps   = elapsed > 0 ? static_cast<double>(done) / elapsed : 0.0;
        const std::size_t qdepth = state_.job_queue.size();

        // Master block
        std::ostringstream js;
        js << "{";
        js << "\"master\":{";
        js << "\"status\":\"running\",";
        js << "\"pid\":" << state_.pid << ",";
        js << "\"jobs_per_sec\":" << std::fixed << std::setprecision(1) << jps << ",";
        js << "\"queue_depth\":" << qdepth;
        js << "},";

        // Workers — all instances (healthy and unhealthy)
        // lookup() returns healthy ones; instance_count() gives total
        const auto healthy_workers = state_.registry.lookup("workers");
        const std::size_t total_registered = state_.registry.instance_count("workers");

        js << "\"workers\":[";
        // Emit healthy workers
        bool first_w = true;
        for (const auto& rec : healthy_workers) {
            if (!first_w) js << ",";
            first_w = false;
            js << "{";
            js << "\"id\":\"" << rec.instance_id << "\",";
            js << "\"status\":\"running\",";
            js << "\"jobs_done\":0,";
            js << "\"errors\":0";
            js << "}";
        }
        // Fill remaining slots as stopped
        const std::size_t healthy_count = healthy_workers.size();
        for (std::size_t i = healthy_count + 1; i <= static_cast<std::size_t>(state_.num_workers); ++i) {
            if (!first_w) js << ",";
            first_w = false;
            js << "{";
            js << "\"id\":\"W-" << i << "\",";
            js << "\"status\":\"stopped\",";
            js << "\"jobs_done\":0,";
            js << "\"errors\":0";
            js << "}";
        }
        js << "],";

        // Primitives
        const std::string cb_state = state_.breaker.state_name();
        const uint32_t    cb_fails = state_.breaker.failure_count();
        const bool cb_ok = (cb_state == "Closed");

        js << "\"primitives\":[";
        js << "{\"name\":\"TaskQueue\",\"status\":\"ok\",\"detail\":\"depth: " << qdepth << ", processed: " << done << "\"},";
        js << "{\"name\":\"MessageBus\",\"status\":\"ok\",\"detail\":\"" << state_.registry.instance_count("workers") << " subscribers\"},";
        js << "{\"name\":\"ServiceRegistry\",\"status\":\"ok\",\"detail\":\"" << total_registered << " nodes registered\"},";
        js << "{\"name\":\"DistributedLock\",\"status\":\"ok\",\"detail\":\"0 contended\"},";
        js << "{\"name\":\"CircuitBreaker\",\"status\":\"" << (cb_ok ? "ok" : "warn") << "\",\"detail\":\"state: " << cb_state << ", trips: " << cb_fails << "\"}";
        js << "],";

        // Stats
        js << "\"stats\":{";
        js << "\"jobs_processed\":" << done << ",";
        js << "\"uptime_seconds\":" << elapsed << ",";
        js << "\"circuit_trips\":" << cb_fails;
        js << "}";

        js << "}";

        return Response::ok(js.str());
    }
};

// ─────────────────────────────────────────────────────────────────────────────

// ── Shared coordination layer ─────────────────────────────────────────────────
// In a real distributed system these would be accessed via a network transport.
// Here they are shared across threads within the same process to demonstrate
// the API and coordination semantics.

static TaskQueue<Job>       g_job_queue(/*max_size=*/0); // unbounded
static TaskQueue<JobResult> g_result_queue;
static MessageBus           g_bus;
static ServiceRegistry      g_registry;
static DistributedLock      g_write_lock;

// ── Worker coroutine ──────────────────────────────────────────────────────────

static void run_worker(const std::string& worker_id, CircuitBreaker& cb,
                        std::atomic<int>& jobs_attempted) {
    // Register with the coordinator.
    g_registry.register_service("workers", worker_id,
                                 {"localhost", 0, "role=worker"});

    // Subscribe to "job-available" wake-up signals published by the master.
    // In practice the handler would wake a blocking receive; here dequeue_for
    // already handles the blocking, so we just log.
    auto sub_id = g_bus.subscribe(
        "jobs", [&worker_id](const std::string&, const std::string& jid) {
            (void)jid;
        });

    std::mt19937                      rng(std::hash<std::string>{}(worker_id));
    std::uniform_int_distribution<int> work_ms(30, 120);
    std::uniform_int_distribution<int> fail_roll(1, 8); // ~12.5% chance of failure

    while (true) {
        // Periodic heartbeat keeps this worker marked healthy.
        g_registry.heartbeat("workers", worker_id);

        // Block for up to 400 ms waiting for the next job.
        auto job = g_job_queue.dequeue_for(400ms);
        if (!job.has_value()) break; // queue closed and drained

        ++jobs_attempted;

        const auto t0 = std::chrono::steady_clock::now();

        JobResult result;
        result.job_id    = job->id;
        result.worker_id = worker_id;
        result.tag       = job->tag;

        try {
            cb.call([&] {
                // Simulate a transient fault so the circuit breaker gets exercised.
                if (fail_roll(rng) == 1)
                    throw std::runtime_error("transient I/O fault");

                // Simulate classification work.
                std::this_thread::sleep_for(std::chrono::milliseconds(work_ms(rng)));
                result.output  = "tagged as [" + job->tag + "] → " + job->payload;
                result.success = true;
            });
        } catch (const std::exception& e) {
            result.success = false;
            result.output  = std::string("FAILED: ") + e.what();
        }

        const auto t1 = std::chrono::steady_clock::now();
        result.duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        // Acquire the write lock before committing the result, simulating
        // mutual exclusion over a shared result store.
        if (g_write_lock.acquire(worker_id, 2000ms, 500ms)) {
            g_result_queue.enqueue(result);
            g_write_lock.release(worker_id);
        }
    }

    g_bus.unsubscribe("jobs", sub_id);
    g_registry.deregister("workers", worker_id);
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    const int num_workers = (argc > 1) ? std::atoi(argv[1]) : 4;
    const int http_port   = (argc > 2) ? std::atoi(argv[2]) : 7700;

    std::cout << "╔══════════════════════════════════════════╗\n"
              << "║  distribution_tag  ·  MASTER NODE        ║\n"
              << "╚══════════════════════════════════════════╝\n\n";

    // ── Register master in the service registry ───────────────────────────────
    g_registry.register_service("master", "master-0",
                                 {"localhost", 9000, "role=master"});

    // ── Build the job catalogue ───────────────────────────────────────────────
    const std::vector<std::pair<std::string, std::string>> assets = {
        {"IMAGE",    "sunset_photo.jpg"},
        {"IMAGE",    "city_skyline.png"},
        {"IMAGE",    "product_shot.png"},
        {"IMAGE",    "team_portrait.jpg"},
        {"IMAGE",    "infographic.svg"},
        {"IMAGE",    "thumbnail_batch.zip"},
        {"DOCUMENT", "quarterly_report.pdf"},
        {"DOCUMENT", "meeting_notes.txt"},
        {"DOCUMENT", "user_manual.docx"},
        {"DOCUMENT", "contract_draft.pdf"},
        {"DOCUMENT", "research_paper.pdf"},
        {"DOCUMENT", "changelog.md"},
        {"VIDEO",    "product_demo.mp4"},
        {"VIDEO",    "tutorial_01.mov"},
        {"VIDEO",    "webinar_recording.mp4"},
        {"VIDEO",    "intro_animation.gif"},
        {"AUDIO",    "podcast_episode.mp3"},
        {"AUDIO",    "ambient_track.wav"},
        {"AUDIO",    "voiceover.ogg"},
        {"AUDIO",    "jingle.mp3"},
    };

    int job_idx = 0;
    for (const auto& [tag, payload] : assets) {
        Job job;
        job.id      = "job-" + std::to_string(++job_idx);
        job.tag     = tag;
        job.payload = payload;
        g_job_queue.enqueue(job);
        // Publish a wake-up signal so workers unblock immediately.
        g_bus.publish("jobs", job.id);
    }
    const int total_jobs = job_idx;

    std::cout << "Enqueued " << total_jobs << " tagging jobs.\n";
    std::cout << "Spawning " << num_workers << " worker threads...\n\n";

    // ── Circuit breaker (shared across all workers) ───────────────────────────
    CircuitBreakerConfig cb_cfg;
    cb_cfg.failure_threshold = 4;
    cb_cfg.success_threshold = 2;
    cb_cfg.timeout           = 300ms;
    CircuitBreaker cb(cb_cfg);

    // ── HTTP topology server ───────────────────────────────────────────────────
    std::atomic<int> jobs_attempted{0};

    const auto started_at = std::chrono::steady_clock::now();
    TopologyState tstate{
        g_registry, g_job_queue, cb, jobs_attempted,
        total_jobs, num_workers, GET_PID(), started_at
    };
    advanced_logging::Logger topo_log;
    auto servlet = std::make_shared<TopologyServlet>(tstate);
    io::http_server::HttpServer http_srv(
        http_port, 2, &topo_log, servlet);
    std::thread http_thread([&http_srv]() { http_srv.start(); });
    std::cout << "[http]   GET http://127.0.0.1:" << http_port
              << "/cluster/topology\n\n";

    // ── Spawn worker threads ──────────────────────────────────────────────────
    std::vector<std::thread> threads;
    threads.reserve(num_workers);
    for (int i = 1; i <= num_workers; ++i) {
        threads.emplace_back(run_worker, "worker-" + std::to_string(i),
                              std::ref(cb), std::ref(jobs_attempted));
    }

    // ── Periodic health monitor on the main thread ────────────────────────────
    // Expire workers whose last heartbeat is older than 5 s.
    std::thread monitor([&] {
        while (jobs_attempted.load() < total_jobs) {
            std::this_thread::sleep_for(500ms);
            const auto stale = g_registry.expire_stale(5000ms);
            if (stale > 0)
                std::cout << "[monitor] Marked " << stale << " stale worker(s) unhealthy.\n";
        }
    });

    for (auto& t : threads) t.join();
    monitor.join();

    // ── Stop HTTP server ──────────────────────────────────────────────────────
    http_srv.stop();
    http_thread.join();

    // ── Collect results ───────────────────────────────────────────────────────
    std::vector<JobResult> results;
    while (auto r = g_result_queue.try_dequeue()) results.push_back(*r);

    // Sort by job number for readable output.
    std::sort(results.begin(), results.end(), [](const JobResult& a, const JobResult& b) {
        return a.job_id < b.job_id;
    });

    // ── Print results table ───────────────────────────────────────────────────
    int successes = 0, failures = 0;

    std::cout << "\n─── Results (" << results.size() << "/" << total_jobs
              << " processed) ─────────────────────────────────\n\n";
    std::cout << std::left
              << std::setw(9)  << "Job"
              << std::setw(13) << "Worker"
              << std::setw(7)  << "Status"
              << std::setw(7)  << "ms"
              << "Output\n";
    std::cout << std::string(72, '-') << '\n';

    for (const auto& r : results) {
        if (r.success) ++successes; else ++failures;
        std::cout << std::left
                  << std::setw(9)  << r.job_id
                  << std::setw(13) << r.worker_id
                  << std::setw(7)  << (r.success ? "OK" : "FAIL")
                  << std::setw(7)  << r.duration_ms
                  << r.output << '\n';
    }

    // ── Summary ───────────────────────────────────────────────────────────────
    std::cout << '\n';
    std::cout << "Success : " << successes << '\n';
    std::cout << "Failed  : " << failures  << '\n';
    std::cout << "Circuit : " << cb.state_name()
              << " (failures=" << cb.failure_count() << ")\n";

    const auto workers_alive =
        g_registry.lookup("workers").size(); // should be 0 — all deregistered
    std::cout << "Registry: " << workers_alive << " workers still registered\n";

    std::cout << "\n=== Master complete ===\n";
    return failures > 0 ? 1 : 0;
}
