#pragma once

#include <map>
#include <string>

namespace trekker {
namespace voip {

struct SipEndpoint {
    std::string uri;
    std::string transport;
};

class SipServer {
public:
    bool register_endpoint(const SipEndpoint& endpoint);
    bool unregister_endpoint(const std::string& uri);
    bool has_endpoint(const std::string& uri) const;
    std::size_t endpoint_count() const;

private:
    std::map<std::string, SipEndpoint> endpoints_;
};

} // namespace voip
} // namespace trekker
