#include "http3_servlet.h"

namespace networking {
namespace servlets {

Http3Servlet::Http3Servlet()
	: handler_()
	, exchange_(std::make_shared<quic::LoopbackExchange>()) {}

Http3Servlet::Http3Servlet(std::shared_ptr<RequestHandler> handler,
						   std::shared_ptr<quic::LoopbackExchange> exchange)
	: handler_(std::move(handler))
	, exchange_(std::move(exchange)) {
	if (!exchange_) {
		exchange_ = std::make_shared<quic::LoopbackExchange>();
	}
}

Response Http3Servlet::handle_request(const Request& request) {
	exchange_->routes().set_fallback([this](const quic::Http3Request& http3_request) {
		Request translated_request = quic::request_to_http1(http3_request);

		Response response;
		if (handler_) {
			response = handler_->handle(translated_request);
		} else {
			response.status_code = 501;
			response.body = "HTTP/3 servlet handler is not configured";
			response.headers[HeaderKey::ContentLength] = std::to_string(response.body.size());
		}

		return quic::response_from_http1(response, http3_request.stream_id);
	});

	quic::Http3Request http3_request = quic::request_from_http1(request, 0);
	const quic::Http3Response http3_response = exchange_->perform(std::move(http3_request));
	if (http3_response.status_code != 0) {
		return quic::response_to_http1(http3_response);
	}

	Response response;
	response.status_code = 500;
	response.body = "HTTP/3 exchange failed to produce a response";
	response.headers[HeaderKey::ContentLength] = std::to_string(response.body.size());
	return response;
}

} // namespace servlets
} // namespace networking
