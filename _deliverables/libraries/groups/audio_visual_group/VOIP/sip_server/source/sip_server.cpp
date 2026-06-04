#include "sip_server.h"

namespace trekker {
namespace voip {

bool SipServer::register_endpoint(const SipEndpoint& endpoint) {
    if (endpoint.uri.empty()) {
        return false;
    }
    endpoints_[endpoint.uri] = endpoint;
    return true;
}

bool SipServer::unregister_endpoint(const std::string& uri) {
    return endpoints_.erase(uri) > 0;
}

bool SipServer::has_endpoint(const std::string& uri) const {
    return endpoints_.find(uri) != endpoints_.end();
}

std::size_t SipServer::endpoint_count() const {
    return endpoints_.size();
}

} // namespace voip
} // namespace trekker
