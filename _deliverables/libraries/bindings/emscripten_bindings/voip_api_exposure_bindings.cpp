#include <emscripten/bind.h>

#include "api_exposure.h"

#include <string>

using namespace emscripten;
using namespace trekker::voip;

namespace {

std::string js_voip_rest_description() {
    ApiExposure exposure(ApiExposureType::Rest);
    return exposure.describe();
}

std::string js_voip_websocket_description() {
    ApiExposure exposure(ApiExposureType::WebSocket);
    return exposure.describe();
}

} // namespace

EMSCRIPTEN_BINDINGS(voip_api_exposure_module) {
    function("voip_rest_description", &js_voip_rest_description);
    function("voip_websocket_description", &js_voip_websocket_description);
}
