#include "xlsx_writer.h"
#include "zip_writer.h"

#include <sstream>

namespace docs {

namespace {

std::string xml_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (const char c : value) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

std::string column_letter(size_t index) {
    std::string letters;
    ++index;
    while (index > 0) {
        const size_t rem = (index - 1) % 26;
        letters.insert(letters.begin(), static_cast<char>('A' + rem));
        index = (index - 1) / 26;
    }
    return letters;
}

void append_row_xml(std::ostringstream& xml, size_t row_number, const std::vector<XlsxCell>& cells) {
    xml << "<row r=\"" << row_number << "\">";
    for (size_t col = 0; col < cells.size(); ++col) {
        const std::string ref = column_letter(col) + std::to_string(row_number);
        const XlsxCell& cell = cells[col];
        if (cell.is_number) {
            xml << "<c r=\"" << ref << "\"><v>" << cell.number << "</v></c>";
        } else {
            xml << "<c r=\"" << ref << "\" t=\"inlineStr\"><is><t>" << xml_escape(cell.text) << "</t></is></c>";
        }
    }
    xml << "</row>";
}

std::vector<XlsxCell> to_cells(const std::vector<std::string>& headers) {
    std::vector<XlsxCell> cells;
    cells.reserve(headers.size());
    for (const auto& h : headers) cells.push_back(XlsxCell::text_value(h));
    return cells;
}

} // namespace

XlsxCell XlsxCell::text_value(std::string value) {
    XlsxCell cell;
    cell.is_number = false;
    cell.text = std::move(value);
    return cell;
}

XlsxCell XlsxCell::number_value(double value) {
    XlsxCell cell;
    cell.is_number = true;
    cell.number = value;
    return cell;
}

void XlsxWorkbook::set_header(std::vector<std::string> headers) {
    header_ = std::move(headers);
}

void XlsxWorkbook::add_row(std::vector<XlsxCell> cells) {
    rows_.push_back(std::move(cells));
}

std::string XlsxWorkbook::build_sheet_xml() const {
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
    xml << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetData>";

    size_t row_number = 1;
    if (!header_.empty()) {
        append_row_xml(xml, row_number, to_cells(header_));
        ++row_number;
    }
    for (const auto& row : rows_) {
        append_row_xml(xml, row_number, row);
        ++row_number;
    }

    xml << "</sheetData></worksheet>";
    return xml.str();
}

bool XlsxWorkbook::save(const std::string& path) const {
    static const char* content_types =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
        "</Types>";

    static const char* root_rels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
        "</Relationships>";

    static const char* workbook_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets><sheet name=\"Sheet1\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
        "</workbook>";

    static const char* workbook_rels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
        "</Relationships>";

    const std::vector<ZipEntry> entries = {
        {"[Content_Types].xml", content_types},
        {"_rels/.rels", root_rels},
        {"xl/workbook.xml", workbook_xml},
        {"xl/_rels/workbook.xml.rels", workbook_rels},
        {"xl/worksheets/sheet1.xml", build_sheet_xml()},
    };

    return write_zip(path, entries);
}

} // namespace docs
