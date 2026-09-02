#include "inflate.h"
#include "pdf_reader.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

void test_zlib_inflate_stored_block() {
    // zlib header (0x78,0x01) + one raw DEFLATE "stored" block containing "hello"
    // + a dummy 4-byte Adler32 trailer that zlib_inflate never needs to read.
    const std::vector<unsigned char> zlib_data = {
        0x78, 0x01,
        0x01, 0x05, 0x00, 0xFA, 0xFF,
        'h', 'e', 'l', 'l', 'o',
        0, 0, 0, 0
    };
    std::string out;
    const bool ok = docs::zlib_inflate(zlib_data, out);
    assert(ok);
    assert(out == "hello");
}

void test_extract_pdf_text_uncompressed_stream() {
    const std::string pdf =
        "1 0 obj\n<< /Length 44 >>\nstream\n"
        "BT /F1 12 Tf 100 700 Td (Hello World) Tj ET\n"
        "endstream\nendobj\n";
    const std::vector<unsigned char> bytes(pdf.begin(), pdf.end());
    const std::string text = docs::extract_pdf_text_from_bytes(bytes);
    assert(text.find("Hello World") != std::string::npos);
}

void test_extract_pdf_text_array_form() {
    const std::string pdf =
        "1 0 obj\n<< /Length 10 >>\nstream\n"
        "BT [(Bal) -250 (ance)] TJ ET\n"
        "endstream\nendobj\n";
    const std::vector<unsigned char> bytes(pdf.begin(), pdf.end());
    const std::string text = docs::extract_pdf_text_from_bytes(bytes);
    assert(text.find("Bal ance") != std::string::npos);
}

} // namespace

int main() {
    test_zlib_inflate_stored_block();
    test_extract_pdf_text_uncompressed_stream();
    test_extract_pdf_text_array_form();

    std::cout << "All pdf_reader tests passed." << std::endl;
    return 0;
}
