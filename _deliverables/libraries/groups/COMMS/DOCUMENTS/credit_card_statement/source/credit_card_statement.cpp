#include "credit_card_statement.h"

#include <regex>
#include <sstream>

namespace docs {

namespace {

double parse_amount(std::string raw) {
    bool negative = false;
    if (raw.size() >= 2 && raw.front() == '(' && raw.back() == ')') {
        negative = true;
        raw = raw.substr(1, raw.size() - 2);
    }
    std::string cleaned;
    for (const char c : raw) {
        if (c == '$' || c == ',') continue;
        if (c == '-') { negative = true; continue; }
        cleaned.push_back(c);
    }
    double value = 0.0;
    try { value = std::stod(cleaned); } catch (...) { value = 0.0; }
    return negative ? -value : value;
}

} // namespace

std::vector<Transaction> parse_transactions(const std::string& statement_text) {
    static const std::regex line_pattern(
        R"(^\s*(\d{1,2}/\d{1,2}(?:/\d{2,4})?)\s+(.+?)\s+(\(?-?\$?[0-9][0-9,]*\.[0-9]{2}\)?)\s*$)");

    std::vector<Transaction> transactions;
    std::istringstream stream(statement_text);
    std::string line;
    while (std::getline(stream, line)) {
        std::smatch match;
        if (!std::regex_match(line, match, line_pattern)) continue;

        Transaction tx;
        tx.date = match[1].str();
        tx.description = match[2].str();
        tx.amount = parse_amount(match[3].str());
        transactions.push_back(tx);
    }
    return transactions;
}

} // namespace docs
