#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace docs {

uint32_t crc32(const unsigned char* data, size_t len);

struct ZipEntry {
    std::string name;
    std::string content;
};

// Writes a minimal ZIP archive using the "stored" (uncompressed) method,
// which is valid per the ZIP spec and needs no compression dependency.
bool write_zip(const std::string& path, const std::vector<ZipEntry>& entries);

} // namespace docs
