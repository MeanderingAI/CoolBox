#include "coolbox/coolbox_c.h"

#include "uuid_generation.hpp"

#include <cctype>
#include <string>
#include <vector>

namespace {

thread_local std::string g_uuid_value;

const char* store(const std::string& value) {
    g_uuid_value = value;
    return g_uuid_value.c_str();
}

std::vector<std::uint8_t> parse_hex_bytes(const char* value) {
    std::vector<std::uint8_t> out;
    if (value == nullptr) {
        return out;
    }

    std::string compact;
    for (const char* it = value; *it != '\0'; ++it) {
        if (std::isxdigit(static_cast<unsigned char>(*it)) != 0) {
            compact.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(*it))));
        }
    }

    if (compact.empty()) {
        return out;
    }

    if ((compact.size() % 2U) != 0U) {
        compact.insert(compact.begin(), '0');
    }

    out.reserve(compact.size() / 2U);
    for (std::size_t i = 0; i < compact.size(); i += 2U) {
        auto hex_to_int = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            return -1;
        };
        const int hi = hex_to_int(compact[i]);
        const int lo = hex_to_int(compact[i + 1U]);
        if (hi < 0 || lo < 0) {
            return {};
        }
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }

    return out;
}

const char* fail() {
    return store("");
}

} // namespace

extern "C" {

const char* coolbox_c_uuid_v1(void) {
    try {
        return store(trekker::misc::uuid_generation::generate_v1().to_string());
    } catch (...) {
        return fail();
    }
}

const char* coolbox_c_uuid_v2(unsigned int local_identifier, unsigned int local_domain) {
    try {
        return store(
            trekker::misc::uuid_generation::generate_v2(
                static_cast<std::uint32_t>(local_identifier),
                static_cast<std::uint8_t>(local_domain & 0xffU))
                .to_string());
    } catch (...) {
        return fail();
    }
}

const char* coolbox_c_uuid_v3(const char* namespace_uuid, const char* name) {
    try {
        const auto ns = trekker::misc::uuid_generation::parse_uuid(namespace_uuid != nullptr ? namespace_uuid : "");
        const std::string n = name != nullptr ? std::string(name) : std::string();
        return store(trekker::misc::uuid_generation::generate_v3(ns, n).to_string());
    } catch (...) {
        return fail();
    }
}

const char* coolbox_c_uuid_v4(void) {
    try {
        return store(trekker::misc::uuid_generation::generate_v4().to_string());
    } catch (...) {
        return fail();
    }
}

const char* coolbox_c_uuid_v5(const char* namespace_uuid, const char* name) {
    try {
        const auto ns = trekker::misc::uuid_generation::parse_uuid(namespace_uuid != nullptr ? namespace_uuid : "");
        const std::string n = name != nullptr ? std::string(name) : std::string();
        return store(trekker::misc::uuid_generation::generate_v5(ns, n).to_string());
    } catch (...) {
        return fail();
    }
}

const char* coolbox_c_uuid_v6(void) {
    try {
        return store(trekker::misc::uuid_generation::generate_v6().to_string());
    } catch (...) {
        return fail();
    }
}

const char* coolbox_c_uuid_v8(const char* custom_entropy_hex) {
    try {
        return store(trekker::misc::uuid_generation::generate_v8(parse_hex_bytes(custom_entropy_hex)).to_string());
    } catch (...) {
        return fail();
    }
}

const char* coolbox_c_guid(void) {
    try {
        return store(trekker::misc::uuid_generation::generate_guid());
    } catch (...) {
        return fail();
    }
}

} // extern "C"
