#include <emscripten/bind.h>

#include "uuid_generation.hpp"

#include <cctype>
#include <string>
#include <vector>

using namespace emscripten;
using namespace trekker::misc::uuid_generation;

namespace {

std::vector<std::uint8_t> hex_to_bytes(const std::string& value) {
    std::string compact;
    compact.reserve(value.size());

    for (char c : value) {
        if (std::isxdigit(static_cast<unsigned char>(c)) != 0) {
            compact.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }

    if ((compact.size() % 2U) != 0U) {
        compact.insert(compact.begin(), '0');
    }

    std::vector<std::uint8_t> out;
    out.reserve(compact.size() / 2U);

    auto hex_to_int = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return -1;
    };

    for (std::size_t i = 0; i + 1U < compact.size(); i += 2U) {
        const int hi = hex_to_int(compact[i]);
        const int lo = hex_to_int(compact[i + 1U]);
        if (hi < 0 || lo < 0) {
            return {};
        }
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }

    return out;
}

std::string js_uuid_v1() {
    return generate_v1().to_string();
}

std::string js_uuid_v2(std::uint32_t local_identifier, std::uint32_t local_domain) {
    return generate_v2(local_identifier, static_cast<std::uint8_t>(local_domain & 0xffU)).to_string();
}

std::string js_uuid_v3(const std::string& namespace_uuid, const std::string& name) {
    return generate_v3(parse_uuid(namespace_uuid), name).to_string();
}

std::string js_uuid_v4() {
    return generate_v4().to_string();
}

std::string js_uuid_v5(const std::string& namespace_uuid, const std::string& name) {
    return generate_v5(parse_uuid(namespace_uuid), name).to_string();
}

std::string js_uuid_v6() {
    return generate_v6().to_string();
}

std::string js_uuid_v7() {
    return generate_v7().to_string();
}

std::string js_uuid_v8(const std::string& entropy_hex) {
    return generate_v8(hex_to_bytes(entropy_hex)).to_string();
}

std::string js_guid() {
    return generate_guid();
}

std::string js_cuid() {
    return generate_cuid();
}

std::string ns_dns() { return namespaces::dns().to_string(); }
std::string ns_url() { return namespaces::url().to_string(); }
std::string ns_oid() { return namespaces::oid().to_string(); }
std::string ns_x500() { return namespaces::x500().to_string(); }

} // namespace

EMSCRIPTEN_BINDINGS(uuid_generation_module) {
    function("uuid_v1", &js_uuid_v1);
    function("uuid_v2", &js_uuid_v2);
    function("uuid_v3", &js_uuid_v3);
    function("uuid_v4", &js_uuid_v4);
    function("uuid_v5", &js_uuid_v5);
    function("uuid_v6", &js_uuid_v6);
    function("uuid_v7", &js_uuid_v7);
    function("uuid_v8", &js_uuid_v8);
    function("guid", &js_guid);
    function("cuid", &js_cuid);

    function("namespace_dns", &ns_dns);
    function("namespace_url", &ns_url);
    function("namespace_oid", &ns_oid);
    function("namespace_x500", &ns_x500);
}
