#ifndef COOLBOX__LIBRARIES_GROUPS_APP_BUILDER_OS_GENERICS_XML_SCREEN_DESCRIPTOR_PARSER_HEADERS_XML_SCREEN_DESCRIPTOR_PARSER_HPP
#define COOLBOX__LIBRARIES_GROUPS_APP_BUILDER_OS_GENERICS_XML_SCREEN_DESCRIPTOR_PARSER_HEADERS_XML_SCREEN_DESCRIPTOR_PARSER_HPP

#include <string>
#include <vector>

#include "components.hpp"
#include "windows.hpp"

namespace os_generics {
namespace xml_screen_descriptor {

struct ScreenDescriptor : public ::graphics::GraphicsObject {
    std::string title = "Screen";
    std::size_t width = 960;
    std::size_t height = 640;
    ::graphics::windows::PlatformStyle platform = ::graphics::windows::PlatformStyle::Windows;
    ::graphics::components::ComponentHolder layout =
        ::graphics::components::ComponentHolder::vertical();

    std::string graphics_object_kind() const override { return "screenDescriptor"; }
    std::string graphics_object_name() const override { return title; }
};

struct ParseResult {
    ScreenDescriptor descriptor;
    std::vector<std::string> errors;

    bool ok() const { return errors.empty(); }
};

class XmlScreenDescriptorParser {
public:
    ParseResult parse_string(const std::string& xml_text) const;
    ParseResult parse_file(const std::string& path) const;
};

} // namespace xml_screen_descriptor
} // namespace os_generics

#endif
