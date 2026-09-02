#pragma once
#include <string>
#include <vector>

namespace docs {

// Extracts the embedded text layer from a PDF file by decompressing page
// content streams (FlateDecode via a built-in DEFLATE decoder) and reading
// the Tj/TJ/'/" text-showing operators. This only recovers text that the PDF
// already stores as text; scanned/image-only pages produce no text here and
// need a real OCR backend (see ocr_engine.h) to be handled.
// Returns the extracted text (one line per detected text-positioning move)
// and throws std::runtime_error if the file cannot be read/parsed at all.
std::string extract_pdf_text(const std::string& pdf_path);
std::string extract_pdf_text_from_bytes(const std::vector<unsigned char>& pdf_bytes);

} // namespace docs
