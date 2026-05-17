#ifndef TREKKER_MISC_UUID_GENERATION_HPP
#define TREKKER_MISC_UUID_GENERATION_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace trekker {
namespace misc {
namespace uuid_generation {

struct Uuid {
    std::array<std::uint8_t, 16> bytes{};

    std::string to_string(bool uppercase = false) const;
    std::string to_guid_string(bool uppercase = true) const;
};

namespace namespaces {
Uuid dns();
Uuid url();
Uuid oid();
Uuid x500();
}

Uuid parse_uuid(const std::string& value);

Uuid generate_v1();
Uuid generate_v2(std::uint32_t local_identifier, std::uint8_t local_domain = 0);
Uuid generate_v3(const Uuid& namespace_uuid, const std::string& name);
Uuid generate_v4();
Uuid generate_v5(const Uuid& namespace_uuid, const std::string& name);
Uuid generate_v6();
Uuid generate_v7();
Uuid generate_v8(const std::vector<std::uint8_t>& custom_entropy);

std::string generate_guid();

} // namespace uuid_generation
} // namespace misc
} // namespace trekker

#endif // TREKKER_MISC_UUID_GENERATION_HPP
