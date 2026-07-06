#include "../headers/cryptocurrency.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#if defined(COOLBOX_CRYPTOCURRENCY_USE_OPENSSL_PROVIDER)
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/obj_mac.h>
#endif

namespace utils {
namespace cryptocurrency {
namespace {

constexpr double kAmountTolerance = 1e-9;

std::uint64_t current_timestamp() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string to_hex(std::uint64_t value) {
    std::ostringstream stream;
    stream << std::hex << std::setw(16) << std::setfill('0') << value;
    return stream.str();
}

std::uint64_t fnv1a64(const std::string& value, std::uint64_t seed) {
    std::uint64_t hash = 1469598103934665603ull ^ seed;
    for (unsigned char ch : value) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ull;
        hash ^= (hash >> 32);
    }
    return hash;
}

bool has_leading_zeroes(const std::string& hash, std::size_t leading_zeroes) {
    return hash.size() >= leading_zeroes &&
           std::all_of(hash.begin(), hash.begin() + static_cast<std::ptrdiff_t>(leading_zeroes),
                       [](char ch) { return ch == '0'; });
}

std::size_t leading_zero_count(const std::string& hash) {
    std::size_t count = 0;
    while (count < hash.size() && hash[count] == '0') {
        ++count;
    }
    return count;
}

std::string bytes_to_hex(const unsigned char* data, std::size_t size) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < size; ++i) {
        stream << std::setw(2) << static_cast<int>(data[i]);
    }
    return stream.str();
}

std::string bytes_to_hex(const std::string& value) {
    return bytes_to_hex(reinterpret_cast<const unsigned char*>(value.data()), value.size());
}

std::vector<unsigned char> hex_to_bytes(const std::string& hex) {
    if ((hex.size() % 2) != 0) {
        throw std::invalid_argument("Hex string must have an even length");
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const auto high = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(hex[i])));
        const auto low = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(hex[i + 1])));

        auto nibble = [](unsigned char ch) -> int {
            if (ch >= '0' && ch <= '9') {
                return ch - '0';
            }
            if (ch >= 'A' && ch <= 'F') {
                return 10 + (ch - 'A');
            }
            throw std::invalid_argument("Invalid hex digit");
        };

        bytes.push_back(static_cast<unsigned char>((nibble(high) << 4) | nibble(low)));
    }
    return bytes;
}

std::string random_secret_material() {
    std::random_device rd;
    std::mt19937_64 generator(rd());
    std::uniform_int_distribution<std::uint64_t> distribution;

    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (int i = 0; i < 4; ++i) {
        stream << std::setw(16) << distribution(generator);
    }
    return stream.str();
}

#if defined(COOLBOX_CRYPTOCURRENCY_USE_OPENSSL_PROVIDER)
std::string bio_to_string(BIO* bio) {
    BUF_MEM* buffer = nullptr;
    BIO_get_mem_ptr(bio, &buffer);
    if (buffer == nullptr || buffer->data == nullptr) {
        return "";
    }
    return std::string(buffer->data, buffer->length);
}

EVP_PKEY* read_private_key(const std::string& pem) {
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (bio == nullptr) {
        return nullptr;
    }
    EVP_PKEY* key = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return key;
}

EVP_PKEY* read_public_key(const std::string& pem) {
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (bio == nullptr) {
        return nullptr;
    }
    EVP_PKEY* key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return key;
}
#endif

} // namespace

HashString compute_hash_hex(const std::string& value) {
#if defined(COOLBOX_CRYPTOCURRENCY_USE_OPENSSL_PROVIDER)
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr) {
        throw std::runtime_error("Failed to create OpenSSL digest context");
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_length = 0;

    const bool ok = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
                    EVP_DigestUpdate(context, value.data(), value.size()) == 1 &&
                    EVP_DigestFinal_ex(context, digest.data(), &digest_length) == 1;
    EVP_MD_CTX_free(context);

    if (!ok) {
        throw std::runtime_error("SHA-256 digest computation failed");
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_length; ++i) {
        stream << std::setw(2) << static_cast<int>(digest[i]);
    }
    return stream.str();
#else
    static constexpr std::array<std::uint64_t, 4> seeds = {
        0x243f6a8885a308d3ull,
        0x13198a2e03707344ull,
        0xa4093822299f31d0ull,
        0x082efa98ec4e6c89ull,
    };

    std::string digest;
    digest.reserve(64);
    for (std::uint64_t seed : seeds) {
        digest += to_hex(fnv1a64(value, seed));
    }
    return digest;
#endif
}

KeyPair generate_key_pair() {
#if defined(COOLBOX_CRYPTOCURRENCY_USE_OPENSSL_PROVIDER)
    EVP_PKEY_CTX* parameter_context = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if (parameter_context == nullptr) {
        throw std::runtime_error("Failed to allocate EC parameter context");
    }

    EVP_PKEY* parameters = nullptr;
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* key_context = nullptr;
    const bool ok = EVP_PKEY_paramgen_init(parameter_context) == 1 &&
                    EVP_PKEY_CTX_set_ec_paramgen_curve_nid(parameter_context, NID_X9_62_prime256v1) == 1 &&
                    EVP_PKEY_CTX_set_ec_param_enc(parameter_context, OPENSSL_EC_NAMED_CURVE) == 1 &&
                    EVP_PKEY_paramgen(parameter_context, &parameters) == 1;
    if (!ok) {
        EVP_PKEY_CTX_free(parameter_context);
        EVP_PKEY_free(parameters);
        throw std::runtime_error("Failed to generate EC parameters");
    }
    EVP_PKEY_CTX_free(parameter_context);

    key_context = EVP_PKEY_CTX_new(parameters, nullptr);
    if (key_context == nullptr || EVP_PKEY_keygen_init(key_context) != 1 || EVP_PKEY_keygen(key_context, &pkey) != 1) {
        EVP_PKEY_CTX_free(key_context);
        EVP_PKEY_free(parameters);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to generate EC key pair");
    }
    EVP_PKEY_CTX_free(key_context);
    EVP_PKEY_free(parameters);

    BIO* private_bio = BIO_new(BIO_s_mem());
    BIO* public_bio = BIO_new(BIO_s_mem());
    if (private_bio == nullptr || public_bio == nullptr ||
        PEM_write_bio_PrivateKey(private_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1 ||
        PEM_write_bio_PUBKEY(public_bio, pkey) != 1) {
        BIO_free(private_bio);
        BIO_free(public_bio);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to serialize key pair");
    }

    KeyPair pair{bio_to_string(private_bio), bio_to_string(public_bio)};
    BIO_free(private_bio);
    BIO_free(public_bio);
    EVP_PKEY_free(pkey);
    return pair;
#else
    const std::string secret = random_secret_material();
    return KeyPair{secret, secret};
#endif
}

std::string sign_message(const std::string& message, const std::string& private_key) {
#if defined(COOLBOX_CRYPTOCURRENCY_USE_OPENSSL_PROVIDER)
    EVP_PKEY* key = read_private_key(private_key);
    if (key == nullptr) {
        throw std::runtime_error("Unable to parse private key");
    }

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr) {
        EVP_PKEY_free(key);
        throw std::runtime_error("Unable to allocate signing context");
    }

    std::size_t signature_size = 0;
    bool ok = EVP_DigestSignInit(context, nullptr, EVP_sha256(), nullptr, key) == 1 &&
              EVP_DigestSignUpdate(context, message.data(), message.size()) == 1 &&
              EVP_DigestSignFinal(context, nullptr, &signature_size) == 1;

    std::vector<unsigned char> signature(signature_size);
    if (ok) {
        ok = EVP_DigestSignFinal(context, signature.data(), &signature_size) == 1;
    }

    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);

    if (!ok) {
        throw std::runtime_error("Failed to sign message");
    }

    signature.resize(signature_size);
    return bytes_to_hex(signature.data(), signature.size());
#else
    return compute_hash_hex(message + "|" + private_key);
#endif
}

bool verify_message_signature(const std::string& message,
                              const std::string& signature,
                              const std::string& public_key) {
    if (signature.empty() || public_key.empty()) {
        return false;
    }

#if defined(COOLBOX_CRYPTOCURRENCY_USE_OPENSSL_PROVIDER)
    EVP_PKEY* key = read_public_key(public_key);
    if (key == nullptr) {
        return false;
    }

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr) {
        EVP_PKEY_free(key);
        return false;
    }

    std::vector<unsigned char> signature_bytes;
    try {
        signature_bytes = hex_to_bytes(signature);
    } catch (const std::exception&) {
        EVP_MD_CTX_free(context);
        EVP_PKEY_free(key);
        return false;
    }

    const bool ok = EVP_DigestVerifyInit(context, nullptr, EVP_sha256(), nullptr, key) == 1 &&
                    EVP_DigestVerifyUpdate(context, message.data(), message.size()) == 1 &&
                    EVP_DigestVerifyFinal(context, signature_bytes.data(), signature_bytes.size()) == 1;

    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);
    return ok;
#else
    return compute_hash_hex(message + "|" + public_key) == signature;
#endif
}

std::string address_from_public_key(const std::string& public_key) {
    if (public_key.empty()) {
        return "";
    }
    return compute_hash_hex("address|" + public_key).substr(0, 40);
}

Transaction::Transaction(std::string sender_value,
                         std::string recipient_value,
                         double amount_value,
                         double fee_value,
                         std::string payload_value,
                         std::uint64_t timestamp_value)
    : sender(std::move(sender_value))
    , recipient(std::move(recipient_value))
    , amount(amount_value)
    , fee(fee_value)
    , payload(std::move(payload_value))
    , timestamp(timestamp_value == 0 ? current_timestamp() : timestamp_value) {
    transaction_id = calculate_id();
}

bool Transaction::is_reward_transaction() const {
    return sender.empty() || sender == "COINBASE" || sender == "SYSTEM";
}

bool Transaction::is_valid() const {
    if (recipient.empty()) {
        return false;
    }
    if (!std::isfinite(amount) || amount <= 0.0) {
        return false;
    }
    if (!std::isfinite(fee) || fee < 0.0) {
        return false;
    }
    if (!is_reward_transaction() && sender.empty()) {
        return false;
    }
    if (calculate_id() != transaction_id) {
        return false;
    }
    if (is_reward_transaction()) {
        return signature.empty();
    }
    if (sender_public_key.empty() || signature.empty()) {
        return false;
    }
    if (sender != address_from_public_key(sender_public_key)) {
        return false;
    }
    return verify_signature();
}

std::string Transaction::serialize() const {
    std::ostringstream stream;
    stream << sender << '|'
           << sender_public_key << '|'
           << recipient << '|'
           << std::fixed << std::setprecision(8) << amount << '|'
           << std::fixed << std::setprecision(8) << fee << '|'
           << timestamp << '|'
           << payload;
    return stream.str();
}

std::string Transaction::signing_payload() const {
    return serialize();
}

std::string Transaction::calculate_id() const {
    return compute_hash_hex(signing_payload());
}

void Transaction::sign(const std::string& private_key) {
    if (is_reward_transaction()) {
        signature.clear();
        transaction_id = calculate_id();
        return;
    }

    transaction_id = calculate_id();
    signature = sign_message(signing_payload(), private_key);
}

bool Transaction::verify_signature() const {
    if (is_reward_transaction()) {
        return signature.empty();
    }
    return verify_message_signature(signing_payload(), signature, sender_public_key);
}

std::vector<HashString> MerkleTree::leaf_hashes(const std::vector<Transaction>& transactions) {
    std::vector<HashString> leaves;
    leaves.reserve(transactions.size());
    for (const auto& transaction : transactions) {
        leaves.push_back(transaction.transaction_id.empty() ? transaction.calculate_id() : transaction.transaction_id);
    }
    if (leaves.empty()) {
        leaves.push_back(compute_hash_hex("empty-merkle-tree"));
    }
    return leaves;
}

std::vector<std::vector<HashString>> MerkleTree::levels(const std::vector<Transaction>& transactions) {
    std::vector<std::vector<HashString>> tree_levels;
    tree_levels.push_back(leaf_hashes(transactions));

    while (tree_levels.back().size() > 1) {
        const auto& current = tree_levels.back();
        std::vector<HashString> next;
        next.reserve((current.size() + 1) / 2);

        for (std::size_t i = 0; i < current.size(); i += 2) {
            const HashString& left = current[i];
            const HashString& right = (i + 1 < current.size()) ? current[i + 1] : current[i];
            next.push_back(compute_hash_hex(left + right));
        }

        tree_levels.push_back(std::move(next));
    }

    return tree_levels;
}

HashString MerkleTree::root(const std::vector<Transaction>& transactions) {
    const auto tree_levels = levels(transactions);
    return tree_levels.back().front();
}

Block::Block(std::size_t index_value,
             HashString previous_hash_value,
             std::vector<Transaction> transactions_value,
             std::uint64_t timestamp_value)
    : index_(index_value)
    , timestamp_(timestamp_value == 0 ? current_timestamp() : timestamp_value)
    , previous_hash_(std::move(previous_hash_value))
    , transactions_(std::move(transactions_value)) {
    update_merkle_root();
    hash_ = calculate_hash();
}

void Block::add_transaction(const Transaction& transaction) {
    transactions_.push_back(transaction);
    update_merkle_root();
    hash_ = calculate_hash();
}

void Block::set_previous_hash(const HashString& previous_hash) {
    previous_hash_ = previous_hash;
    hash_ = calculate_hash();
}

void Block::update_merkle_root() {
    merkle_root_ = MerkleTree::root(transactions_);
}

HashString Block::calculate_hash() const {
    std::ostringstream stream;
    stream << index_ << '|'
           << previous_hash_ << '|'
           << timestamp_ << '|'
           << nonce_ << '|'
           << merkle_root_;
    return compute_hash_hex(stream.str());
}

void Block::mine(std::size_t leading_zeroes) {
    hash_ = calculate_hash();
    while (!has_leading_zeroes(hash_, leading_zeroes)) {
        ++nonce_;
        hash_ = calculate_hash();
    }
}

bool Block::has_valid_hash(std::size_t leading_zeroes) const {
    return hash_ == calculate_hash() && has_leading_zeroes(hash_, leading_zeroes);
}

bool Block::is_valid(std::size_t leading_zeroes, const HashString& expected_previous_hash) const {
    if (previous_hash_ != expected_previous_hash) {
        return false;
    }
    for (const auto& transaction : transactions_) {
        if (!transaction.is_valid()) {
            return false;
        }
    }
    if (merkle_root_ != MerkleTree::root(transactions_)) {
        return false;
    }
    return has_valid_hash(leading_zeroes);
}

Blockchain::Blockchain(std::size_t difficulty, double mining_reward)
    : difficulty_(difficulty)
    , mining_reward_(mining_reward) {
    chain_.push_back(create_genesis_block(difficulty_));
}

const Block& Blockchain::latest_block() const {
    return chain_.back();
}

bool Blockchain::add_transaction(const Transaction& transaction) {
    if (!transaction.is_valid() || transaction.is_reward_transaction()) {
        return false;
    }
    if (!has_sufficient_balance(transaction)) {
        return false;
    }
    pending_transactions_.push_back(transaction);
    return true;
}

Block Blockchain::mine_pending_transactions(const std::string& miner_address) {
    if (miner_address.empty()) {
        throw std::invalid_argument("Miner address must not be empty");
    }

    double total_fees = 0.0;
    for (const auto& transaction : pending_transactions_) {
        total_fees += transaction.fee;
    }

    std::vector<Transaction> transactions = pending_transactions_;
    transactions.emplace_back("COINBASE",
                              miner_address,
                              mining_reward_ + total_fees,
                              0.0,
                              "mining reward");

    Block block(chain_.size(), latest_block().hash(), std::move(transactions));
    block.mine(difficulty_);
    chain_.push_back(block);
    pending_transactions_.clear();
    return block;
}

double Blockchain::get_balance(const std::string& address) const {
    if (address.empty()) {
        return 0.0;
    }

    double balance = 0.0;
    for (const auto& block : chain_) {
        for (const auto& transaction : block.transactions()) {
            if (transaction.sender == address && !transaction.is_reward_transaction()) {
                balance -= (transaction.amount + transaction.fee);
            }
            if (transaction.recipient == address) {
                balance += transaction.amount;
            }
        }
    }
    return balance;
}

double Blockchain::get_pending_balance(const std::string& address) const {
    double balance = get_balance(address);
    for (const auto& transaction : pending_transactions_) {
        if (transaction.sender == address && !transaction.is_reward_transaction()) {
            balance -= (transaction.amount + transaction.fee);
        }
        if (transaction.recipient == address) {
            balance += transaction.amount;
        }
    }
    return balance;
}

bool Blockchain::has_sufficient_balance(const Transaction& transaction) const {
    if (transaction.is_reward_transaction()) {
        return true;
    }
    return get_pending_balance(transaction.sender) + kAmountTolerance >= (transaction.amount + transaction.fee);
}

bool Blockchain::is_valid() const {
    return is_valid_chain(chain_, difficulty_);
}

std::uint64_t Blockchain::cumulative_work() const {
    return cumulative_work(chain_);
}

bool Blockchain::can_replace_chain(const std::vector<Block>& candidate_chain) const {
    return is_valid_chain(candidate_chain, difficulty_) && cumulative_work(candidate_chain) > cumulative_work();
}

bool Blockchain::replace_chain(const std::vector<Block>& candidate_chain) {
    if (!can_replace_chain(candidate_chain)) {
        return false;
    }
    chain_ = candidate_chain;
    pending_transactions_.clear();
    return true;
}

bool Blockchain::is_valid_chain(const std::vector<Block>& chain, std::size_t difficulty) {
    if (chain.empty()) {
        return false;
    }
    if (!chain.front().is_valid(difficulty, "0")) {
        return false;
    }

    for (std::size_t i = 1; i < chain.size(); ++i) {
        if (!chain[i].is_valid(difficulty, chain[i - 1].hash())) {
            return false;
        }
    }
    return true;
}

std::uint64_t Blockchain::cumulative_work(const std::vector<Block>& chain) {
    std::uint64_t work = 0;
    for (const auto& block : chain) {
        const std::size_t zeroes = leading_zero_count(block.hash());
        const std::size_t shift = std::min<std::size_t>(zeroes, 20);
        work += (1ull << shift);
    }
    return work;
}

Block Blockchain::create_genesis_block(std::size_t difficulty) {
    Block genesis(0, "0", {}, 1700000000ull);
    genesis.mine(difficulty);
    return genesis;
}

} // namespace cryptocurrency
} // namespace utils
