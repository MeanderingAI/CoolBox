#include "api_exposure.h"

namespace trekker {
namespace voip {

ApiExposure::ApiExposure(ApiExposureType type) : type_(type) {}

std::string ApiExposure::describe() const {
    if (type_ == ApiExposureType::WebSocket) {
        return "voip signaling exposed via WebSocket";
    }
    return "voip signaling exposed via REST API";
}

} // namespace voip
} // namespace trekker
