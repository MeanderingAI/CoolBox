#ifndef COOLBOX__LIBRARIES_PACKAGES_IO_DATAFORMATS_WEB_HEADERS_MJS_PROCESSOR_H
#define COOLBOX__LIBRARIES_PACKAGES_IO_DATAFORMATS_WEB_HEADERS_MJS_PROCESSOR_H

namespace networking {
namespace document {

class MJSProcessor {
public:
    MJSProcessor();
    ~MJSProcessor();
    void process(const std::string& mjs_code);
};

} // namespace document
} // namespace networking

#endif  // COOLBOX__LIBRARIES_PACKAGES_IO_DATAFORMATS_WEB_HEADERS_MJS_PROCESSOR_H
