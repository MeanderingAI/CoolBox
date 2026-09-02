#include "credit_card_statement.h"
#include "ocr_engine.h"
#include "pdf_reader.h"
#include "xlsx_writer.h"

#include <iostream>
#include <numeric>
#include <string>

namespace {

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <input.pdf> [output.xlsx]\n"
              << "  Extracts transactions from a credit card bill PDF and writes them to an Excel spreadsheet.\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const std::string input_path = argv[1];
    const std::string output_path = argc >= 3 ? argv[2] : "statement.xlsx";

    std::string text;
    try {
        text = docs::extract_pdf_text(input_path);
    } catch (const std::exception& ex) {
        std::cerr << "[pdf_cell] failed to read PDF: " << ex.what() << std::endl;
        return 1;
    }

    // A page with no embedded text is most likely a scanned image; fall back
    // to the pluggable OCR engine (unconfigured by default, see ocr_engine.h).
    if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
        try {
            auto ocr = docs::default_ocr_engine();
            text = ocr->recognize_text({});
        } catch (const std::exception& ex) {
            std::cerr << "[pdf_cell] " << ex.what() << std::endl;
            return 1;
        }
    }

    const std::vector<docs::Transaction> transactions = docs::parse_transactions(text);
    if (transactions.empty()) {
        std::cerr << "[pdf_cell] warning: no transaction lines were recognized in " << input_path << std::endl;
    }

    docs::XlsxWorkbook workbook;
    workbook.set_header({"Date", "Description", "Amount"});
    double total = 0.0;
    for (const auto& tx : transactions) {
        workbook.add_row({
            docs::XlsxCell::text_value(tx.date),
            docs::XlsxCell::text_value(tx.description),
            docs::XlsxCell::number_value(tx.amount),
        });
        total += tx.amount;
    }
    workbook.add_row({
        docs::XlsxCell::text_value(""),
        docs::XlsxCell::text_value("Total"),
        docs::XlsxCell::number_value(total),
    });

    if (!workbook.save(output_path)) {
        std::cerr << "[pdf_cell] failed to write " << output_path << std::endl;
        return 1;
    }

    std::cout << "[pdf_cell] wrote " << transactions.size() << " transaction(s) to " << output_path << std::endl;
    return 0;
}
