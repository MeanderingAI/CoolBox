#pragma once

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
