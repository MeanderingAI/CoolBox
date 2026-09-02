#ifndef DISTRIBUTED_RAFT_NODE_H
#define DISTRIBUTED_RAFT_NODE_H

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace distributed {

enum class RaftRole { Follower, Candidate, Leader };

struct LogEntry {
    std::uint64_t term;
    std::string   command;
};

struct VoteRequest {
    std::uint64_t term;
    std::uint32_t candidate_id;
    std::uint64_t last_log_index;
    std::uint64_t last_log_term;
};

struct VoteResponse {
    std::uint64_t term;
    bool          granted;
};

struct AppendRequest {
    std::uint64_t         term;
    std::uint32_t         leader_id;
    std::uint64_t         prev_log_index;
    std::uint64_t         prev_log_term;
    std::vector<LogEntry> entries;
    std::uint64_t         leader_commit;
};

struct AppendResponse {
    std::uint64_t term;
    bool          success;
};

// Simplified Raft consensus node implementing leader election and log
// replication logic. RPC transport is handled by the caller; this class
// models the state-machine transitions and voting rules only.
class RaftNode {
public:
    explicit RaftNode(std::uint32_t id, std::uint32_t cluster_size);

    // ── Election ─────────────────────────────────────────────────────────────
    void         start_election();
    VoteResponse handle_vote_request(const VoteRequest& req);
    // Record a vote received from another node; returns true when majority won.
    bool         record_vote(std::uint32_t voter_id);

    // ── Log replication ───────────────────────────────────────────────────────
    bool           append_command(const std::string& command); // leader only
    AppendResponse handle_append_entries(const AppendRequest& req);

    // ── Accessors ─────────────────────────────────────────────────────────────
    std::uint32_t id()           const { return id_; }
    RaftRole      role()         const;
    std::uint64_t current_term() const;
    std::uint64_t log_size()     const;
    std::uint64_t commit_index() const;

    // Step down to follower if new_term is greater. Returns true if updated.
    bool update_term(std::uint64_t new_term);

private:
    bool log_is_up_to_date(std::uint64_t last_index, std::uint64_t last_term) const;

    mutable std::mutex       mutex_;
    std::uint32_t            id_;
    std::uint32_t            cluster_size_;
    RaftRole                 role_;
    std::uint64_t            current_term_;
    std::optional<uint32_t>  voted_for_;
    std::vector<LogEntry>    log_;
    std::uint64_t            commit_index_;
    std::uint64_t            last_applied_;
    std::uint32_t            votes_received_;
};

} // namespace distributed

#endif // DISTRIBUTED_RAFT_NODE_H
