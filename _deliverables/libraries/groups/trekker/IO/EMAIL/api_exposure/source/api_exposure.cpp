#include "api_exposure.h"

namespace trekker {
namespace email {

ApiExposure::ApiExposure(ApiExposureType type) : type_(type) {}

std::string ApiExposure::describe() const {
    if (type_ == ApiExposureType::WebSocket) {
        return "email control exposed via WebSocket";
    }
    return "email control exposed via REST API";
}

} // namespace email
} // namespace trekker
