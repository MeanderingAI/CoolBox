#include "ocr_engine.h"

#include <stdexcept>

namespace docs {

std::string UnavailableOcrEngine::recognize_text(const std::vector<unsigned char>& /*image_bytes*/) {
    throw std::runtime_error(
        "No OCR backend is configured. This page appears to be a scanned image with no "
        "embedded text layer; plug a real OcrEngine implementation (e.g. a Tesseract "
        "adapter) into pdf_cell to recognize it.");
}

std::shared_ptr<OcrEngine> default_ocr_engine() {
    return std::make_shared<UnavailableOcrEngine>();
}

} // namespace docs
