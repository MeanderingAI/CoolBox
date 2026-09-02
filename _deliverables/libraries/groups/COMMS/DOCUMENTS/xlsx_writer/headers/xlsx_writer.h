#pragma once
#include <string>
#include <vector>

namespace docs {

struct XlsxCell {
    bool is_number = false;
    std::string text;
    double number = 0.0;

    static XlsxCell text_value(std::string value);
    static XlsxCell number_value(double value);
};

// Builds a single-sheet .xlsx workbook (OOXML SpreadsheetML) with no external
// XML/zip dependencies; see zip_writer.h for the container format used.
class XlsxWorkbook {
public:
    void set_header(std::vector<std::string> headers);
    void add_row(std::vector<XlsxCell> cells);

    bool save(const std::string& path) const;

private:
    std::vector<std::string> header_;
    std::vector<std::vector<XlsxCell>> rows_;

    std::string build_sheet_xml() const;
};

} // namespace docs
