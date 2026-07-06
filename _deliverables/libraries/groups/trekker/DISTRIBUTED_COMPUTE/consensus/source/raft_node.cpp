#include "../headers/raft_node.h"

#include <algorithm>

namespace distributed {

RaftNode::RaftNode(std::uint32_t id, std::uint32_t cluster_size)
    : id_(id),
      cluster_size_(cluster_size),
      role_(RaftRole::Follower),
      current_term_(0),
      commit_index_(0),
      last_applied_(0),
      votes_received_(0) {}

RaftRole RaftNode::role() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return role_;
}

std::uint64_t RaftNode::current_term() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_term_;
}

std::uint64_t RaftNode::log_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<std::uint64_t>(log_.size());
}

std::uint64_t RaftNode::commit_index() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return commit_index_;
}

void RaftNode::start_election() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++current_term_;
    role_           = RaftRole::Candidate;
    voted_for_      = id_;
    votes_received_ = 1; // vote for self
}

VoteResponse RaftNode::handle_vote_request(const VoteRequest& req) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (req.term > current_term_) {
        current_term_ = req.term;
        role_         = RaftRole::Follower;
        voted_for_.reset();
    }

    if (req.term < current_term_) {
        return {current_term_, false};
    }

    const bool can_vote = !voted_for_.has_value() || *voted_for_ == req.candidate_id;
    const bool log_ok   = log_is_up_to_date(req.last_log_index, req.last_log_term);

    if (can_vote && log_ok) {
        voted_for_ = req.candidate_id;
        return {current_term_, true};
    }

    return {current_term_, false};
}

bool RaftNode::record_vote(std::uint32_t /*voter_id*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (role_ != RaftRole::Candidate) return false;
    ++votes_received_;
    if (votes_received_ > cluster_size_ / 2) {
        role_ = RaftRole::Leader;
        return true;
    }
    return false;
}

bool RaftNode::append_command(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (role_ != RaftRole::Leader) return false;
    log_.push_back({current_term_, command});
    return true;
}

AppendResponse RaftNode::handle_append_entries(const AppendRequest& req) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (req.term > current_term_) {
        current_term_ = req.term;
        role_         = RaftRole::Follower;
        voted_for_.reset();
    }

    if (req.term < current_term_) {
        return {current_term_, false};
    }

    // Heartbeat — no entries to replicate.
    if (req.entries.empty()) {
        if (req.leader_commit > commit_index_) {
            commit_index_ = std::min(req.leader_commit,
                                     static_cast<std::uint64_t>(log_.size()));
        }
        return {current_term_, true};
    }

    // Log consistency check.
    if (req.prev_log_index > 0) {
        if (static_cast<std::uint64_t>(log_.size()) < req.prev_log_index)
            return {current_term_, false};
        if (log_[req.prev_log_index - 1].term != req.prev_log_term)
            return {current_term_, false};
    }

    // Truncate and append.
    log_.resize(req.prev_log_index);
    for (const auto& e : req.entries) log_.push_back(e);

    if (req.leader_commit > commit_index_) {
        commit_index_ = std::min(req.leader_commit,
                                 static_cast<std::uint64_t>(log_.size()));
    }

    return {current_term_, true};
}

bool RaftNode::update_term(std::uint64_t new_term) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (new_term > current_term_) {
        current_term_ = new_term;
        role_         = RaftRole::Follower;
        voted_for_.reset();
        return true;
    }
    return false;
}

bool RaftNode::log_is_up_to_date(std::uint64_t last_index,
                                  std::uint64_t last_term) const {
    if (log_.empty()) return true;
    const auto& my_last = log_.back();
    if (last_term != my_last.term)
        return last_term > my_last.term;
    return last_index >= static_cast<std::uint64_t>(log_.size());
}

} // namespace distributed
