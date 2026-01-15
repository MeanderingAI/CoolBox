#ifndef ML_DATAFORMATS_HTTP_ROUTE_H
#define ML_DATAFORMATS_HTTP_ROUTE_H

#include <string>
#include <functional>
#include "IO/dataformats/http/request_response.h"

namespace dataformats {
namespace http {

struct Route {
    std::string method;
    std::string path;
    std::string description;
    std::function<Response(const Request&)> handler;

    Route(const std::string& m, const std::string& p, const std::string& d,
          std::function<Response(const Request&)> h)
        : method(m), path(p), description(d), handler(h) {}
};

} // namespace http
} // namespace dataformats

#endif // ML_DATAFORMATS_HTTP_ROUTE_H
