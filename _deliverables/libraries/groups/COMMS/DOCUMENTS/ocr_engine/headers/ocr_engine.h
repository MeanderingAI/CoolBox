#pragma once
#include <memory>
#include <string>
#include <vector>

namespace docs {

// Pluggable text-recognition backend for scanned/image-only PDF pages.
// pdf_reader only recovers text that a PDF already stores as a text layer;
// image-based pages need a real OCR implementation wired in through this
// interface (e.g. an adapter around Tesseract or a cloud OCR API).
class OcrEngine {
public:
    virtual ~OcrEngine() = default;
    virtual std::string recognize_text(const std::vector<unsigned char>& image_bytes) = 0;
};

// Default backend used when no real OCR engine has been configured. It
// always throws, with a message explaining what needs to be wired in.
class UnavailableOcrEngine : public OcrEngine {
public:
    std::string recognize_text(const std::vector<unsigned char>& image_bytes) override;
};

std::shared_ptr<OcrEngine> default_ocr_engine();

} // namespace docs
