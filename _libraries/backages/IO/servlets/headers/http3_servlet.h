#ifndef COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP3_SERVLET_H
#define COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP3_SERVLET_H
#include "http_servlet_base.h"

#include <memory>

#include "quic_transport.hpp"

namespace networking {
namespace servlets {

class Http3Servlet : public HttpServletBase {
public:
    Http3Servlet();
    explicit Http3Servlet(std::shared_ptr<RequestHandler> handler,
                          std::shared_ptr<quic::LoopbackExchange> exchange = nullptr);
    ~Http3Servlet() override = default;
    std::string get_version() const override { return "HTTP/3"; }
    Response handle_request(const Request& request) override;

    const quic::Session& session() const { return exchange_->server().session(); }
    const quic::LoopbackExchange& exchange() const { return *exchange_; }

private:
    std::shared_ptr<RequestHandler> handler_;
    std::shared_ptr<quic::LoopbackExchange> exchange_;
};

} // namespace servlets
} // namespace networking

#endif  // COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP3_SERVLET_H
