#pragma once

#include <string>

namespace trekker {
namespace voip {

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

} // namespace voip
} // namespace trekker
