#ifndef ML_DATAFORMATS_HTTP_ROUTE_H
#define ML_DATAFORMATS_HTTP_ROUTE_H

#include <string>
#include <functional>
#include "networking/http/request_response.h"

namespace dataformats {
namespace http {

namespace nhh = networking::http;

struct Route {
    std::string method;
    std::string path;
    std::string description;
    std::function<nhh::Response(const nhh::Request&)> handler;

    Route(const std::string& m, const std::string& p, const std::string& d,
          std::function<nhh::Response(const nhh::Request&)> h)
        : method(m), path(p), description(d), handler(h) {}
};

} // namespace http
} // namespace dataformats

#endif // ML_DATAFORMATS_HTTP_ROUTE_H
