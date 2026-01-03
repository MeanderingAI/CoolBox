#include "networking/rest_api/http_server_base.h"
#include "networking/rest_api/http1_server.h"
#include "networking/rest_api/http2_server.h"
#include "networking/rest_api/http3_server.h"

namespace networking {
namespace rest_api {

// =========================================================================
// HttpServerBase Implementation
// =========================================================================

nhh::Response HttpServerBase::process_routes(const nhh::Request& request) {
    // Find matching route
    for (auto& route : routes_) {
        if (route->matches(request.path(), request.method())) {
            // Extract path parameters
            auto params = route->extract_params(request.path());
            nhh::Request modified_request = request;
            modified_request.set_path_params(params);
            
            // Apply middleware and handle
            return apply_middleware(modified_request, [&route](const nhh::Request& req) {
                return route->handle(req);
            });
        }
    }
    
    // No route found
    nhh::Response response;
    response.set_status(nhh::HttpStatus::NOT_FOUND);
    response.set_json("{\"error\": \"Not Found\"}");
    return response;
}

nhh::Response HttpServerBase::apply_middleware(const nhh::Request& request,
                                         std::function<nhh::Response(const nhh::Request&)> handler) {
    // Apply middleware in order
    std::function<nhh::Response(const nhh::Request&)> chain = handler;
    
    for (auto it = middleware_.rbegin(); it != middleware_.rend(); ++it) {
        auto middleware = *it;
        auto next_chain = chain;
        chain = [middleware, next_chain](const nhh::Request& req) {
            return middleware(req, next_chain);
        };
    }
    
    nhh::Response response = chain(request);
    
    // Add CORS headers if enabled
    if (cors_enabled_) {
        response.set_header("Access-Control-Allow-Origin", cors_origin_);
        response.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
        response.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
    
    return response;
}

// =========================================================================
// HttpServerFactory Implementation
// =========================================================================

std::unique_ptr<HttpServerBase> HttpServerFactory::create(
    HttpVersion version,
    int port,
    size_t num_threads
) {
    switch (version) {
        case HttpVersion::HTTP_1_0:
        case HttpVersion::HTTP_1_1:
            return create_http1(port, num_threads);
        case HttpVersion::HTTP_2:
            return create_http2(port, num_threads);
        case HttpVersion::HTTP_3:
            return create_http3(port, num_threads);
    }
    
    return create_http1(port, num_threads);
}

std::unique_ptr<HttpServerBase> HttpServerFactory::create_http1(int port, size_t num_threads) {
    return std::make_unique<Http1Server>(port, num_threads);
}

std::unique_ptr<HttpServerBase> HttpServerFactory::create_http2(int port, size_t num_threads) {
    return std::make_unique<Http2Server>(port, num_threads);
}

std::unique_ptr<HttpServerBase> HttpServerFactory::create_http3(int port, size_t num_threads) {
    return std::make_unique<Http3Server>(port, num_threads);
}

} // namespace rest_api
} // namespace ml
