#ifndef COOLBOX__LIBRARIES_PACKAGES_IO_DATAFORMATS_WEB_HEADERS_CSS_PROCESSOR_H
#define COOLBOX__LIBRARIES_PACKAGES_IO_DATAFORMATS_WEB_HEADERS_CSS_PROCESSOR_H

namespace networking {
namespace document {

class CSSProcessor {
public:
    CSSProcessor();
    ~CSSProcessor();
    void process(const std::string& css_code);
};

} // namespace document
} // namespace networking

#endif  // COOLBOX__LIBRARIES_PACKAGES_IO_DATAFORMATS_WEB_HEADERS_CSS_PROCESSOR_H
