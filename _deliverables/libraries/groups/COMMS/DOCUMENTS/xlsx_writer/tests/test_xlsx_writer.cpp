#include "xlsx_writer.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void test_save_produces_valid_zip_with_expected_content() {
    docs::XlsxWorkbook workbook;
    workbook.set_header({"Date", "Description", "Amount"});
    workbook.add_row({
        docs::XlsxCell::text_value("03/14"),
        docs::XlsxCell::text_value("Hello & <World>"),
        docs::XlsxCell::number_value(45.99),
    });

    const std::string path = "test_xlsx_writer_output.xlsx";
    const bool ok = workbook.save(path);
    assert(ok);

    const std::string bytes = read_file(path);
    assert(bytes.size() > 4);
    assert(bytes[0] == 'P' && bytes[1] == 'K'); // ZIP local file header signature
    assert(bytes.find("[Content_Types].xml") != std::string::npos);
    assert(bytes.find("xl/workbook.xml") != std::string::npos);
    assert(bytes.find("Hello &amp; &lt;World&gt;") != std::string::npos);

    std::remove(path.c_str());
}

} // namespace

int main() {
    test_save_produces_valid_zip_with_expected_content();

    std::cout << "All xlsx_writer tests passed." << std::endl;
    return 0;
}
