#pragma once

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
