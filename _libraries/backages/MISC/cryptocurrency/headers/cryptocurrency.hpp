#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace utils {
namespace cryptocurrency {

using HashString = std::string;

struct KeyPair {
    std::string private_key;
    std::string public_key;

    bool empty() const {
        return private_key.empty() || public_key.empty();
    }
};

KeyPair generate_key_pair();
std::string sign_message(const std::string& message, const std::string& private_key);
bool verify_message_signature(const std::string& message,
                              const std::string& signature,
                              const std::string& public_key);
std::string address_from_public_key(const std::string& public_key);

struct Transaction {
    std::string sender;
    std::string sender_public_key;
    std::string recipient;
    double amount = 0.0;
    double fee = 0.0;
    std::string payload;
    std::uint64_t timestamp = 0;
    std::string transaction_id;
    std::string signature;

    Transaction() = default;
    Transaction(std::string sender,
                std::string recipient,
                double amount,
                double fee = 0.0,
                std::string payload = "",
                std::uint64_t timestamp = 0);

    bool is_reward_transaction() const;
    bool is_valid() const;
    std::string serialize() const;
    std::string signing_payload() const;
    std::string calculate_id() const;
    void sign(const std::string& private_key);
    bool verify_signature() const;
};

class MerkleTree {
public:
    static std::vector<HashString> leaf_hashes(const std::vector<Transaction>& transactions);
    static std::vector<std::vector<HashString>> levels(const std::vector<Transaction>& transactions);
    static HashString root(const std::vector<Transaction>& transactions);
};

class Block {
public:
    Block() = default;
    Block(std::size_t index,
          HashString previous_hash,
          std::vector<Transaction> transactions = {},
          std::uint64_t timestamp = 0);

    std::size_t index() const { return index_; }
    std::uint64_t timestamp() const { return timestamp_; }
    const HashString& previous_hash() const { return previous_hash_; }
    const HashString& merkle_root() const { return merkle_root_; }
    const HashString& hash() const { return hash_; }
    std::uint64_t nonce() const { return nonce_; }
    const std::vector<Transaction>& transactions() const { return transactions_; }

    void add_transaction(const Transaction& transaction);
    void set_previous_hash(const HashString& previous_hash);
    void update_merkle_root();
    HashString calculate_hash() const;
    void mine(std::size_t leading_zeroes);
    bool has_valid_hash(std::size_t leading_zeroes) const;
    bool is_valid(std::size_t leading_zeroes, const HashString& expected_previous_hash) const;

private:
    std::size_t index_ = 0;
    std::uint64_t timestamp_ = 0;
    HashString previous_hash_;
    std::vector<Transaction> transactions_;
    HashString merkle_root_;
    HashString hash_;
    std::uint64_t nonce_ = 0;
};

class Blockchain {
public:
    explicit Blockchain(std::size_t difficulty = 3, double mining_reward = 50.0);

    const std::vector<Block>& chain() const { return chain_; }
    const std::vector<Transaction>& pending_transactions() const { return pending_transactions_; }
    const Block& latest_block() const;

    std::size_t difficulty() const { return difficulty_; }
    double mining_reward() const { return mining_reward_; }

    void set_difficulty(std::size_t difficulty) { difficulty_ = difficulty; }
    void set_mining_reward(double reward) { mining_reward_ = reward; }

    bool add_transaction(const Transaction& transaction);
    Block mine_pending_transactions(const std::string& miner_address);
    double get_balance(const std::string& address) const;
    double get_pending_balance(const std::string& address) const;
    bool has_sufficient_balance(const Transaction& transaction) const;
    bool is_valid() const;
    std::uint64_t cumulative_work() const;
    bool can_replace_chain(const std::vector<Block>& candidate_chain) const;
    bool replace_chain(const std::vector<Block>& candidate_chain);

    static bool is_valid_chain(const std::vector<Block>& chain, std::size_t difficulty);
    static std::uint64_t cumulative_work(const std::vector<Block>& chain);

private:
    std::vector<Block> chain_;
    std::vector<Transaction> pending_transactions_;
    std::size_t difficulty_ = 0;
    double mining_reward_ = 0.0;

    static Block create_genesis_block(std::size_t difficulty);
};

HashString compute_hash_hex(const std::string& value);

} // namespace cryptocurrency
} // namespace utils
