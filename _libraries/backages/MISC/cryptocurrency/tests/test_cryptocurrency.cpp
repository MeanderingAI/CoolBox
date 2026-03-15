#include <gtest/gtest.h>

#include "cryptocurrency.hpp"

namespace {

utils::cryptocurrency::Transaction make_signed_transaction(const utils::cryptocurrency::KeyPair& sender_keys,
                                                           const std::string& recipient_address,
                                                           double amount,
                                                           double fee,
                                                           const std::string& payload,
                                                           std::uint64_t timestamp) {
    utils::cryptocurrency::Transaction transaction(
        utils::cryptocurrency::address_from_public_key(sender_keys.public_key),
        recipient_address,
        amount,
        fee,
        payload,
        timestamp);
    transaction.sender_public_key = sender_keys.public_key;
    transaction.transaction_id = transaction.calculate_id();
    transaction.sign(sender_keys.private_key);
    return transaction;
}

} // namespace

TEST(CryptocurrencyTransactionTest, TransactionIdIsStableForSameContent) {
    const auto sender_keys = utils::cryptocurrency::generate_key_pair();
    const std::string sender_address = utils::cryptocurrency::address_from_public_key(sender_keys.public_key);
    const utils::cryptocurrency::Transaction tx1 = make_signed_transaction(sender_keys, "bob", 3.5, 0.1, "invoice-7", 1700000100ull);
    const utils::cryptocurrency::Transaction tx2 = make_signed_transaction(sender_keys, "bob", 3.5, 0.1, "invoice-7", 1700000100ull);

    EXPECT_EQ(tx1.transaction_id, tx2.transaction_id);
    EXPECT_TRUE(tx1.is_valid());
    EXPECT_EQ(tx1.sender, sender_address);
}

TEST(CryptocurrencyTransactionTest, SignedTransactionsVerifyAndTamperingFails) {
    const auto sender_keys = utils::cryptocurrency::generate_key_pair();
    auto tx = make_signed_transaction(sender_keys, "merchant", 1.25, 0.05, "coffee", 1700000111ull);

    EXPECT_TRUE(tx.verify_signature());
    tx.amount = 2.25;
    EXPECT_FALSE(tx.verify_signature());
}

TEST(CryptocurrencyMerkleTreeTest, RootIsDeterministicForOddLeafCount) {
    const auto alice_keys = utils::cryptocurrency::generate_key_pair();
    const auto bob_keys = utils::cryptocurrency::generate_key_pair();
    const auto charlie_keys = utils::cryptocurrency::generate_key_pair();
    const std::vector<utils::cryptocurrency::Transaction> transactions = {
        make_signed_transaction(alice_keys, utils::cryptocurrency::address_from_public_key(bob_keys.public_key), 1.0, 0.01, "a", 1700000001ull),
        make_signed_transaction(bob_keys, utils::cryptocurrency::address_from_public_key(charlie_keys.public_key), 0.5, 0.02, "b", 1700000002ull),
        make_signed_transaction(charlie_keys, "dana", 0.25, 0.01, "c", 1700000003ull),
    };

    const auto root1 = utils::cryptocurrency::MerkleTree::root(transactions);
    const auto root2 = utils::cryptocurrency::MerkleTree::root(transactions);

    EXPECT_FALSE(root1.empty());
    EXPECT_EQ(root1, root2);
}

TEST(CryptocurrencyBlockTest, MiningProducesLeadingZeroHash) {
    const auto sender_keys = utils::cryptocurrency::generate_key_pair();
    utils::cryptocurrency::Block block(
        1,
        "0000previous",
        {make_signed_transaction(sender_keys, "bob", 2.0, 0.05, "payment", 1700000100ull)});

    block.mine(2);

    EXPECT_TRUE(block.has_valid_hash(2));
    EXPECT_EQ(block.hash().substr(0, 2), "00");
}

TEST(CryptocurrencyBlockchainTest, MiningRewardsMinerAndTracksBalances) {
    utils::cryptocurrency::Blockchain chain(2, 25.0);
    const auto alice_keys = utils::cryptocurrency::generate_key_pair();
    const std::string alice_address = utils::cryptocurrency::address_from_public_key(alice_keys.public_key);

    chain.mine_pending_transactions(alice_address);

    EXPECT_TRUE(chain.add_transaction(make_signed_transaction(alice_keys, "bob", 5.0, 0.5, "payment", 1700000200ull)));
    const auto mined = chain.mine_pending_transactions("miner-1");

    EXPECT_TRUE(mined.has_valid_hash(2));
    EXPECT_TRUE(chain.is_valid());
    EXPECT_DOUBLE_EQ(chain.get_balance("miner-1"), 25.5);
    EXPECT_DOUBLE_EQ(chain.get_balance("bob"), 5.0);
    EXPECT_DOUBLE_EQ(chain.get_balance(alice_address), 19.5);
}

TEST(CryptocurrencyBlockchainTest, RejectsRewardTransactionsAsPendingUserTransactions) {
    utils::cryptocurrency::Blockchain chain(1, 10.0);
    EXPECT_FALSE(chain.add_transaction({"COINBASE", "miner", 10.0, 0.0, "reward", 1700000300ull}));
    EXPECT_TRUE(chain.pending_transactions().empty());
}

TEST(CryptocurrencyBlockchainTest, RejectsOverspendingTransactions) {
    utils::cryptocurrency::Blockchain chain(1, 10.0);
    const auto alice_keys = utils::cryptocurrency::generate_key_pair();
    EXPECT_FALSE(chain.add_transaction(make_signed_transaction(alice_keys, "vendor", 4.0, 0.1, "overspend", 1700000400ull)));
}

TEST(CryptocurrencyBlockchainTest, StrongerValidChainCanReplaceCurrentChain) {
    const auto miner_keys = utils::cryptocurrency::generate_key_pair();
    const std::string miner_address = utils::cryptocurrency::address_from_public_key(miner_keys.public_key);

    utils::cryptocurrency::Blockchain original(1, 10.0);
    original.mine_pending_transactions(miner_address);

    utils::cryptocurrency::Blockchain candidate(1, 10.0);
    candidate.mine_pending_transactions(miner_address);
    candidate.mine_pending_transactions(miner_address);

    EXPECT_TRUE(original.can_replace_chain(candidate.chain()));
    EXPECT_TRUE(original.replace_chain(candidate.chain()));
    EXPECT_EQ(original.chain().size(), candidate.chain().size());
    EXPECT_TRUE(original.is_valid());
}
