#pragma once

#include <cstdint>
#include <string>

namespace distribution_tag {

// A unit of tagging work dispatched by the master and consumed by a worker.
struct Job {
    std::string   id;        // globally unique job ID
    std::string   tag;       // classification label to apply (IMAGE, DOCUMENT, etc.)
    std::string   payload;   // name / URI of the asset being tagged
    std::uint32_t priority{1};
};

// The result produced by a worker after processing one Job.
struct JobResult {
    std::string job_id;
    std::string worker_id;
    std::string tag;
    bool        success{false};
    std::string output;
    long long   duration_ms{0};
};

} // namespace distribution_tag
