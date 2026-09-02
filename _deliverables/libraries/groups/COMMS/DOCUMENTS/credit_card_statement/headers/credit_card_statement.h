#pragma once
#include <string>
#include <vector>

namespace docs {

struct Transaction {
    std::string date;        // as printed, e.g. "03/14" or "03/14/2026"
    std::string description;
    double amount = 0.0;     // negative for credits/payments shown in parentheses
};

// Scans OCR/PDF-extracted statement text line by line and pulls out rows that
// look like "<date>  <description>  <amount>", which is how most credit card
// statements print transactions. Lines that don't match are ignored.
std::vector<Transaction> parse_transactions(const std::string& statement_text);

} // namespace docs
