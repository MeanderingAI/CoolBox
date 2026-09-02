#pragma once
#include <string>
#include <vector>

namespace docs {

// Minimal, dependency-free RFC1951 DEFLATE decoder plus a zlib-wrapper
// convenience function, used to decompress /FlateDecode PDF streams.
bool inflate_raw(const std::vector<unsigned char>& compressed, std::string& out);
bool zlib_inflate(const std::vector<unsigned char>& zlib_data, std::string& out);

} // namespace docs
