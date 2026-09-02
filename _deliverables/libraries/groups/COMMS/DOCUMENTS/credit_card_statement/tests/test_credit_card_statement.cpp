#include "credit_card_statement.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

bool nearly_equal(double a, double b) { return std::fabs(a - b) < 1e-9; }

void test_basic_purchase_line() {
    const auto txs = docs::parse_transactions("03/14 AMAZON.COM MARKETPLACE SEATTLE WA 45.99\n");
    assert(txs.size() == 1);
    assert(txs[0].date == "03/14");
    assert(txs[0].description == "AMAZON.COM MARKETPLACE SEATTLE WA");
    assert(nearly_equal(txs[0].amount, 45.99));
}

void test_negative_payment_line() {
    const auto txs = docs::parse_transactions("03/15/2026 PAYMENT THANK YOU -250.00\n");
    assert(txs.size() == 1);
    assert(nearly_equal(txs[0].amount, -250.00));
}

void test_parenthesized_credit_line() {
    const auto txs = docs::parse_transactions("03/16 REFUND FROM MERCHANT (12.34)\n");
    assert(txs.size() == 1);
    assert(nearly_equal(txs[0].amount, -12.34));
}

void test_multiple_lines_and_ignored_noise() {
    const std::string text =
        "Statement for account ending 1234\n"
        "03/14 AMAZON.COM MARKETPLACE 45.99\n"
        "not a transaction line\n"
        "03/15 GROCERY STORE #9 1,234.56\n";
    const auto txs = docs::parse_transactions(text);
    assert(txs.size() == 2);
    assert(nearly_equal(txs[1].amount, 1234.56));
}

} // namespace

int main() {
    test_basic_purchase_line();
    test_negative_payment_line();
    test_parenthesized_credit_line();
    test_multiple_lines_and_ignored_noise();

    std::cout << "All credit_card_statement tests passed." << std::endl;
    return 0;
}
