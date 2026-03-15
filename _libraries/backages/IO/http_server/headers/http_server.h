#ifndef IO_HTTP_SERVER_HTTP_SERVER_H
#define IO_HTTP_SERVER_HTTP_SERVER_H

#include "request_handle.h"
#include "../../servlets/headers/http_servlet_base.h"
#include "../../advanced_logging/headers/advanced_logging.h"
#include "../../../SECURITY/auth/headers/auth_system.h"
#include "../../../MISC/thread_pool/thread_pool.h"
#include <string>
#include <memory>
#include <vector>

namespace io {
namespace http_server {

struct ProtectedPathRule {
	std::string path_prefix;
	auth::UserRole required_role = auth::UserRole::USER;
};

class HttpServer {
public:
	HttpServer(int port, size_t num_threads, advanced_logging::Logger* logger, std::shared_ptr<networking::servlets::HttpServletBase> servlet);
	~HttpServer();

	void start();
	void stop();
	void display_banner() const;
	std::string get_version() const;
	void set_auth_system(std::shared_ptr<auth::AuthSystem> auth_system);
	void protect_path_prefix(const std::string& path_prefix, auth::UserRole required_role = auth::UserRole::USER);
	void clear_protected_paths();
	void set_auth_endpoints(const std::string& login_path,
		const std::string& logout_path = "/auth/logout",
		const std::string& me_path = "/auth/me");

	// Add stubs for handler registration
	void add_request_handler(const RequestHandle&);
	template<typename... Args>
	void add_request_handler_group(Args&&...) {}

private:
	int port_;
	size_t num_threads_;
	advanced_logging::Logger* logger_;
	std::shared_ptr<networking::servlets::HttpServletBase> servlet_;
	std::shared_ptr<auth::AuthSystem> auth_system_;
	std::vector<ProtectedPathRule> protected_paths_;
	std::string login_endpoint_ = "/auth/login";
	std::string logout_endpoint_ = "/auth/logout";
	std::string me_endpoint_ = "/auth/me";
	std::unique_ptr<ThreadPool> thread_pool_;
	bool running_ = false;

	Response handle_request_with_auth(const Request& request);
};

} // namespace http_server
} // namespace io

#endif // IO_HTTP_SERVER_HTTP_SERVER_H
