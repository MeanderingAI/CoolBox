#include "tyst_framework.hpp"

#include "raft_node.h"

using namespace distributed;

TEST(RaftNodeTest, InitialStateIsFollower) {
    RaftNode node(1, 3);
    EXPECT_EQ(node.role(), RaftRole::Follower);
    EXPECT_EQ(node.current_term(), 0u);
    EXPECT_EQ(node.log_size(), 0u);
}

TEST(RaftNodeTest, StartElectionBecomesCandidate) {
    RaftNode node(1, 3);
    node.start_election();
    EXPECT_EQ(node.role(), RaftRole::Candidate);
    EXPECT_EQ(node.current_term(), 1u);
}

TEST(RaftNodeTest, MajorityVoteElectsLeader) {
    RaftNode node(1, 3);
    node.start_election();                  // self-vote counted
    bool became_leader = node.record_vote(2); // second vote = majority
    EXPECT_TRUE(became_leader);
    EXPECT_EQ(node.role(), RaftRole::Leader);
}

TEST(RaftNodeTest, SingleVoteNotSufficientInClusterOfFive) {
    RaftNode node(1, 5);
    node.start_election();
    bool won = node.record_vote(2); // 2 of 5 — not majority
    EXPECT_FALSE(won);
    EXPECT_EQ(node.role(), RaftRole::Candidate);
}

TEST(RaftNodeTest, VoteGrantedToFirstRequester) {
    RaftNode node(2, 3);
    VoteRequest req{1, 1, 0, 0};
    auto resp = node.handle_vote_request(req);
    EXPECT_TRUE(resp.granted);
}

TEST(RaftNodeTest, VoteRejectedForStaleTerm) {
    RaftNode node(1, 3);
    node.start_election(); // term = 1
    VoteRequest req{0, 2, 0, 0}; // stale term
    auto resp = node.handle_vote_request(req);
    EXPECT_FALSE(resp.granted);
}

TEST(RaftNodeTest, VoteRejectedWhenAlreadyVoted) {
    RaftNode node(1, 3);
    VoteRequest req1{1, 2, 0, 0};
    VoteRequest req2{1, 3, 0, 0};
    node.handle_vote_request(req1);
    auto resp = node.handle_vote_request(req2);
    EXPECT_FALSE(resp.granted);
}

TEST(RaftNodeTest, AppendCommandRequiresLeaderRole) {
    RaftNode node(1, 3);
    EXPECT_FALSE(node.append_command("set x 1")); // follower
    node.start_election();
    node.record_vote(2);                            // become leader
    EXPECT_TRUE(node.append_command("set x 1"));
    EXPECT_EQ(node.log_size(), 1u);
}

TEST(RaftNodeTest, HeartbeatAlwaysSucceeds) {
    RaftNode node(1, 3);
    AppendRequest req{1, 2, 0, 0, {}, 0};
    auto resp = node.handle_append_entries(req);
    EXPECT_TRUE(resp.success);
}

TEST(RaftNodeTest, AppendEntriesReplicatesLog) {
    RaftNode node(1, 3);
    AppendRequest req{1, 2, 0, 0, {{1, "set x 1"}, {1, "set y 2"}}, 2};
    auto resp = node.handle_append_entries(req);
    EXPECT_TRUE(resp.success);
    EXPECT_EQ(node.log_size(), 2u);
    EXPECT_EQ(node.commit_index(), 2u);
}

TEST(RaftNodeTest, UpdateTermStepsDownFromCandidate) {
    RaftNode node(1, 3);
    node.start_election(); // term = 1, candidate
    bool updated = node.update_term(5);
    EXPECT_TRUE(updated);
    EXPECT_EQ(node.role(), RaftRole::Follower);
    EXPECT_EQ(node.current_term(), 5u);
}

TEST(RaftNodeTest, UpdateTermIgnoresSameTerm) {
    RaftNode node(1, 3);
    node.start_election(); // term = 1
    bool updated = node.update_term(1);
    EXPECT_FALSE(updated);
    EXPECT_EQ(node.role(), RaftRole::Candidate);
}
