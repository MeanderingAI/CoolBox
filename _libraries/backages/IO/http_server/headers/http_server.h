#ifndef IO_HTTP_SERVER_HTTP_SERVER_H
#define IO_HTTP_SERVER_HTTP_SERVER_H

#include "request_handle.h"
#include "../../servlets/headers/http_servlet_base.h"
#include "../../advanced_logging/headers/advanced_logging.h"
#include "../../../MISC/thread_pool/thread_pool.h"
#include <string>
#include <memory>

namespace io {
namespace http_server {

class HttpServer {
public:
	HttpServer(int port, size_t num_threads, advanced_logging::Logger* logger, std::shared_ptr<networking::servlets::HttpServletBase> servlet);
	~HttpServer();

	void start();
	void stop();
	void display_banner() const;
	std::string get_version() const;

	// Add stubs for handler registration
	void add_request_handler(const RequestHandle&);
	template<typename... Args>
	void add_request_handler_group(Args&&...) {}

private:
	int port_;
	size_t num_threads_;
	advanced_logging::Logger* logger_;
	std::shared_ptr<networking::servlets::HttpServletBase> servlet_;
	std::unique_ptr<ThreadPool> thread_pool_;
	bool running_ = false;
};

} // namespace http_server
} // namespace io

#endif // IO_HTTP_SERVER_HTTP_SERVER_H
