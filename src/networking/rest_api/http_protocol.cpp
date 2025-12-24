#include "networking/rest_api/http_protocol.h"
#include <sstream>
#include <iomanip>

namespace ml {
namespace rest_api {

// =========================================================================
// Helper Functions
// =========================================================================

std::string http_version_to_string(HttpVersion version) {
    switch (version) {
        case HttpVersion::HTTP_1_0: return "HTTP/1.0";
        case HttpVersion::HTTP_1_1: return "HTTP/1.1";
        case HttpVersion::HTTP_2: return "HTTP/2";
        case HttpVersion::HTTP_3: return "HTTP/3";
    }
    return "HTTP/1.1";
}

ProtocolCapabilities ProtocolCapabilities::for_version(HttpVersion version) {
    ProtocolCapabilities caps;
    
    switch (version) {
        case HttpVersion::HTTP_1_0:
        case HttpVersion::HTTP_1_1:
            caps.supports_multiplexing = false;
            caps.supports_server_push = false;
            caps.supports_header_compression = false;
            caps.supports_prioritization = false;
            caps.is_encrypted = false;
            caps.is_udp_based = false;
            break;
            
        case HttpVersion::HTTP_2:
            caps.supports_multiplexing = true;
            caps.supports_server_push = true;
            caps.supports_header_compression = true;
            caps.supports_prioritization = true;
            caps.is_encrypted = true;
            caps.is_udp_based = false;
            break;
            
        case HttpVersion::HTTP_3:
            caps.supports_multiplexing = true;
            caps.supports_server_push = true;
            caps.supports_header_compression = true;
            caps.supports_prioritization = true;
            caps.is_encrypted = true;
            caps.is_udp_based = true;
            break;
    }
    
    return caps;
}

// =========================================================================
// HPACK Implementation (HTTP/2 header compression)
// =========================================================================

std::vector<uint8_t> HPACKEncoder::encode(const std::map<std::string, std::string>& headers) {
    std::vector<uint8_t> result;
    
    // Simplified HPACK encoding (indexed literals)
    for (const auto& [key, value] : headers) {
        // Literal header with incremental indexing
        result.push_back(0x40);  // 01000000
        
        // Encode key length and key
        result.push_back(static_cast<uint8_t>(key.length()));
        for (char c : key) {
            result.push_back(static_cast<uint8_t>(c));
        }
        
        // Encode value length and value
        result.push_back(static_cast<uint8_t>(value.length()));
        for (char c : value) {
            result.push_back(static_cast<uint8_t>(c));
        }
    }
    
    return result;
}

std::map<std::string, std::string> HPACKEncoder::decode(const std::vector<uint8_t>& data) {
    std::map<std::string, std::string> headers;
    
    size_t pos = 0;
    while (pos < data.size()) {
        // Check header type
        uint8_t byte = data[pos++];
        
        if ((byte & 0x40) != 0) {  // Literal with incremental indexing
            // Read key length
            if (pos >= data.size()) break;
            uint8_t key_len = data[pos++];
            
            // Read key
            if (pos + key_len > data.size()) break;
            std::string key(data.begin() + pos, data.begin() + pos + key_len);
            pos += key_len;
            
            // Read value length
            if (pos >= data.size()) break;
            uint8_t value_len = data[pos++];
            
            // Read value
            if (pos + value_len > data.size()) break;
            std::string value(data.begin() + pos, data.begin() + pos + value_len);
            pos += value_len;
            
            headers[key] = value;
        }
    }
    
    return headers;
}

// =========================================================================
// QPACK Implementation (HTTP/3 header compression)
// =========================================================================

std::vector<uint8_t> QPACKEncoder::encode(const std::map<std::string, std::string>& headers) {
    std::vector<uint8_t> result;
    
    // QPACK encoding (similar to HPACK but with dynamic table tracking)
    // Simplified implementation
    for (const auto& [key, value] : headers) {
        // Insert with name reference
        result.push_back(0x50);  // 01010000
        
        // Encode key
        result.push_back(static_cast<uint8_t>(key.length()));
        for (char c : key) {
            result.push_back(static_cast<uint8_t>(c));
        }
        
        // Encode value
        result.push_back(static_cast<uint8_t>(value.length()));
        for (char c : value) {
            result.push_back(static_cast<uint8_t>(c));
        }
    }
    
    return result;
}

std::map<std::string, std::string> QPACKEncoder::decode(const std::vector<uint8_t>& data) {
    std::map<std::string, std::string> headers;
    
    size_t pos = 0;
    while (pos < data.size()) {
        if (pos >= data.size()) break;
        uint8_t byte = data[pos++];
        
        if ((byte & 0x50) != 0) {
            // Read key
            if (pos >= data.size()) break;
            uint8_t key_len = data[pos++];
            
            if (pos + key_len > data.size()) break;
            std::string key(data.begin() + pos, data.begin() + pos + key_len);
            pos += key_len;
            
            // Read value
            if (pos >= data.size()) break;
            uint8_t value_len = data[pos++];
            
            if (pos + value_len > data.size()) break;
            std::string value(data.begin() + pos, data.begin() + pos + value_len);
            pos += value_len;
            
            headers[key] = value;
        }
    }
    
    return headers;
}

} // namespace rest_api
} // namespace ml
