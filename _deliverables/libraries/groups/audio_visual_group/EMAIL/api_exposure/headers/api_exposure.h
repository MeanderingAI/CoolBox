#pragma once

#include <string>

namespace trekker {
namespace email {

enum class ApiExposureType {
    WebSocket,
    Rest
};

class ApiExposure {
public:
    explicit ApiExposure(ApiExposureType type);
    std::string describe() const;

private:
    ApiExposureType type_;
};

} // namespace email
} // namespace trekker
