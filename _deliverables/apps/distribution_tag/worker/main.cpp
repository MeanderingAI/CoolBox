// distribution_tag — Worker Node
//
// A self-contained worker that seeds its own local job queue and processes
// every job using the DISTRIBUTED library primitives.  In a production
// system the coordination state (TaskQueue, MessageBus, ServiceRegistry, etc.)
// would be accessed over a network transport such as gRPC or NATS; here they
// are instantiated in-process to keep the demo self-contained.
//
// Run: ./distribution_tag_worker [worker_id [num_jobs]]
//   worker_id  — optional ID string,   default "worker-standalone"
//   num_jobs   — optional job count,   default 10

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "circuit_breaker.h"
#include "distributed_lock.h"
#include "message_bus.h"
#include "service_registry.h"
#include "task_queue.h"

#include "job.h"

using namespace distributed;
using namespace distribution_tag;
using namespace std::chrono_literals;

// ── Worker class ──────────────────────────────────────────────────────────────

class Worker {
public:
    Worker(std::string        id,
           TaskQueue<Job>&    job_queue,
           TaskQueue<JobResult>& result_queue,
           MessageBus&        bus,
           ServiceRegistry&   registry,
           DistributedLock&   write_lock,
           CircuitBreaker&    cb)
        : id_(std::move(id)),
          job_queue_(job_queue),
          result_queue_(result_queue),
          bus_(bus),
          registry_(registry),
          write_lock_(write_lock),
          cb_(cb) {}

    void run() {
        // ── Registration ──────────────────────────────────────────────────────
        registry_.register_service("workers", id_,
                                    {"localhost", 0, "role=worker"});
        log("Registered. Listening for jobs...\n");

        // ── Subscribe to job-available notifications ───────────────────────────
        auto sub_id = bus_.subscribe(
            "jobs", [this](const std::string&, const std::string& jid) {
                log("Wake-up signal for " + jid + "\n");
            });

        // ── Process loop ──────────────────────────────────────────────────────
        std::mt19937                      rng(std::hash<std::string>{}(id_));
        std::uniform_int_distribution<int> work_ms(25, 100);
        std::uniform_int_distribution<int> fail_roll(1, 10);

        while (true) {
            registry_.heartbeat("workers", id_);

            auto job = job_queue_.dequeue_for(400ms);
            if (!job.has_value()) {
                log("Queue drained. Shutting down.\n");
                break;
            }

            log("Processing " + job->id + " [" + job->tag + "] " + job->payload + "\n");

            const auto t0 = std::chrono::steady_clock::now();

            JobResult result;
            result.job_id    = job->id;
            result.worker_id = id_;
            result.tag       = job->tag;

            try {
                cb_.call([&] {
                    if (fail_roll(rng) == 1)
                        throw std::runtime_error("transient I/O fault");

                    std::this_thread::sleep_for(std::chrono::milliseconds(work_ms(rng)));
                    result.output  = "tagged as [" + job->tag + "] → " + job->payload;
                    result.success = true;
                });
            } catch (const std::exception& e) {
                result.success = false;
                result.output  = std::string("FAILED: ") + e.what();
                log(result.output + "\n");
            }

            const auto t1 = std::chrono::steady_clock::now();
            result.duration_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

            // Acquire the distributed write lock before committing the result.
            if (write_lock_.acquire(id_, 2000ms, 500ms)) {
                result_queue_.enqueue(result);
                write_lock_.release(id_);
            }

            log(job->id + " → " + (result.success ? "OK" : "FAIL") + " ("
                + std::to_string(result.duration_ms) + " ms)\n");
        }

        // ── Cleanup ───────────────────────────────────────────────────────────
        bus_.unsubscribe("jobs", sub_id);
        registry_.deregister("workers", id_);
        log("Deregistered.\n");
    }

private:
    void log(const std::string& msg) const {
        std::cout << "[" << id_ << "] " << msg;
    }

    std::string           id_;
    TaskQueue<Job>&       job_queue_;
    TaskQueue<JobResult>& result_queue_;
    MessageBus&           bus_;
    ServiceRegistry&      registry_;
    DistributedLock&      write_lock_;
    CircuitBreaker&       cb_;
};

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    const std::string worker_id = (argc > 1) ? argv[1] : "worker-standalone";
    const int         num_jobs  = (argc > 2) ? std::atoi(argv[2]) : 10;

    std::cout << "╔══════════════════════════════════════════╗\n"
              << "║  distribution_tag  ·  WORKER NODE        ║\n"
              << "╚══════════════════════════════════════════╝\n\n";
    std::cout << "Worker ID : " << worker_id << "\n";
    std::cout << "Job count : " << num_jobs  << "\n\n";

    // ── Build coordination layer ──────────────────────────────────────────────
    TaskQueue<Job>       job_queue;
    TaskQueue<JobResult> result_queue;
    MessageBus           bus;
    ServiceRegistry      registry;
    DistributedLock      write_lock;

    CircuitBreakerConfig cb_cfg;
    cb_cfg.failure_threshold = 3;
    cb_cfg.success_threshold = 2;
    cb_cfg.timeout           = 300ms;
    CircuitBreaker cb(cb_cfg);

    // ── Seed jobs (simulates what the master would dispatch) ──────────────────
    const std::vector<std::pair<std::string, std::string>> samples = {
        {"IMAGE",    "photo_001.jpg"},
        {"DOCUMENT", "report.pdf"},
        {"VIDEO",    "clip.mp4"},
        {"AUDIO",    "track.mp3"},
        {"IMAGE",    "banner.png"},
        {"DOCUMENT", "notes.txt"},
        {"VIDEO",    "promo.mov"},
        {"AUDIO",    "sfx.wav"},
        {"IMAGE",    "icon_set.svg"},
        {"DOCUMENT", "spec_sheet.docx"},
    };

    for (int i = 0; i < num_jobs; ++i) {
        const auto& [tag, payload] = samples[i % static_cast<int>(samples.size())];
        Job job;
        job.id      = "job-" + std::to_string(i + 1);
        job.tag     = tag;
        job.payload = payload;
        job_queue.enqueue(job);
        bus.publish("jobs", job.id);
    }
    std::cout << "Seeded " << num_jobs << " jobs into local queue.\n\n";

    // ── Run ───────────────────────────────────────────────────────────────────
    Worker worker(worker_id, job_queue, result_queue, bus, registry, write_lock, cb);
    worker.run();

    // ── Collect results ───────────────────────────────────────────────────────
    std::vector<JobResult> results;
    while (auto r = result_queue.try_dequeue()) results.push_back(*r);

    // ── Summary table ─────────────────────────────────────────────────────────
    int ok = 0, fail = 0;
    std::cout << "\n─── Worker Summary (" << results.size() << "/" << num_jobs
              << ") ──────────────────────────────\n\n";
    std::cout << std::left
              << std::setw(9) << "Job"
              << std::setw(7) << "Status"
              << std::setw(7) << "ms"
              << "Output\n";
    std::cout << std::string(60, '-') << '\n';

    for (const auto& r : results) {
        r.success ? ++ok : ++fail;
        std::cout << std::left
                  << std::setw(9) << r.job_id
                  << std::setw(7) << (r.success ? "OK" : "FAIL")
                  << std::setw(7) << r.duration_ms
                  << r.output << '\n';
    }

    std::cout << '\n';
    std::cout << "OK     : " << ok   << '\n';
    std::cout << "Failed : " << fail << '\n';
    std::cout << "Circuit: " << cb.state_name()
              << " (failures=" << cb.failure_count() << ")\n";

    std::cout << "\n=== Worker done ===\n";
    return fail > 0 ? 1 : 0;
}
