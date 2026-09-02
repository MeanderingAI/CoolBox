#include "zip_writer.h"

#include <array>
#include <fstream>

namespace docs {

namespace {

void put_u16(std::string& out, uint16_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
}

void put_u32(std::string& out, uint32_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
    out.push_back(static_cast<char>((v >> 16) & 0xFF));
    out.push_back(static_cast<char>((v >> 24) & 0xFF));
}

std::array<uint32_t, 256> build_crc_table() {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int k = 0; k < 8; ++k) {
            c = (c & 1u) != 0 ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        table[i] = c;
    }
    return table;
}

} // namespace

uint32_t crc32(const unsigned char* data, size_t len) {
    static const std::array<uint32_t, 256> table = build_crc_table();
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

bool write_zip(const std::string& path, const std::vector<ZipEntry>& entries) {
    std::string out;
    std::vector<std::string> central_headers;

    for (const auto& entry : entries) {
        const uint32_t offset = static_cast<uint32_t>(out.size());

        const auto* data = reinterpret_cast<const unsigned char*>(entry.content.data());
        const uint32_t crc = crc32(data, entry.content.size());
        const uint32_t size = static_cast<uint32_t>(entry.content.size());

        std::string local;
        put_u32(local, 0x04034b50u);
        put_u16(local, 20);
        put_u16(local, 0);
        put_u16(local, 0); // stored, no compression
        put_u16(local, 0);
        put_u16(local, 0);
        put_u32(local, crc);
        put_u32(local, size);
        put_u32(local, size);
        put_u16(local, static_cast<uint16_t>(entry.name.size()));
        put_u16(local, 0);
        local += entry.name;
        local += entry.content;
        out += local;

        std::string central;
        put_u32(central, 0x02014b50u);
        put_u16(central, 20);
        put_u16(central, 20);
        put_u16(central, 0);
        put_u16(central, 0);
        put_u16(central, 0);
        put_u16(central, 0);
        put_u32(central, crc);
        put_u32(central, size);
        put_u32(central, size);
        put_u16(central, static_cast<uint16_t>(entry.name.size()));
        put_u16(central, 0);
        put_u16(central, 0);
        put_u16(central, 0);
        put_u16(central, 0);
        put_u32(central, 0);
        put_u32(central, offset);
        central += entry.name;
        central_headers.push_back(central);
    }

    const uint32_t central_start = static_cast<uint32_t>(out.size());
    uint32_t central_size = 0;
    for (const auto& header : central_headers) {
        out += header;
        central_size += static_cast<uint32_t>(header.size());
    }

    std::string end_record;
    put_u32(end_record, 0x06054b50u);
    put_u16(end_record, 0);
    put_u16(end_record, 0);
    put_u16(end_record, static_cast<uint16_t>(entries.size()));
    put_u16(end_record, static_cast<uint16_t>(entries.size()));
    put_u32(end_record, central_size);
    put_u32(end_record, central_start);
    put_u16(end_record, 0);
    out += end_record;

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(out.data(), static_cast<std::streamsize>(out.size()));
    return true;
}

} // namespace docs
