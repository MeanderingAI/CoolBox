#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "api_exposure.h"
#include "http_server.h"
#include "imap_server.h"
#include "mailbox.h"
#include "smtp_server.h"

namespace {

using trekker::email::ApiExposure;
using trekker::email::ApiExposureType;
using trekker::email::EmailAccount;
using trekker::email::EmailMessage;
using trekker::email::ImapServer;
using trekker::email::MailboxStore;
using trekker::email::SmtpServer;

std::string json_escape(const std::string& value) {
    std::ostringstream escaped;
    for (unsigned char ch : value) {
        switch (ch) {
            case '\\': escaped << "\\\\"; break;
            case '"': escaped << "\\\""; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default: escaped << static_cast<char>(ch); break;
        }
    }
    return escaped.str();
}

std::string trim_copy(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

std::string extract_json_string_field(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"((?:\\\\.|[^\\\"])*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return "";
    }

    const std::string raw = match[1].str();
    std::string output;
    output.reserve(raw.size());

    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] != '\\' || i + 1 >= raw.size()) {
            output.push_back(raw[i]);
            continue;
        }

        const char escaped = raw[++i];
        switch (escaped) {
            case '\\': output.push_back('\\'); break;
            case '"': output.push_back('"'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            default: output.push_back(escaped); break;
        }
    }

    return output;
}

int extract_json_int_field(const std::string& json, const std::string& key, int fallback = 0) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return fallback;
    }
    try {
        return std::stoi(match[1].str());
    } catch (...) {
        return fallback;
    }
}

std::string path_without_query(const std::string& uri) {
    const auto pos = uri.find('?');
    if (pos == std::string::npos) {
        return uri;
    }
    return uri.substr(0, pos);
}

std::string query_value(const std::string& uri, const std::string& key) {
    const auto pos = uri.find('?');
    if (pos == std::string::npos || pos + 1 >= uri.size()) {
        return "";
    }

    std::istringstream iss(uri.substr(pos + 1));
    std::string token;
    while (std::getline(iss, token, '&')) {
        const auto eq = token.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        if (token.substr(0, eq) == key) {
            std::string value = token.substr(eq + 1);
            std::string decoded;
            decoded.reserve(value.size());
            for (std::size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '+') {
                    decoded.push_back(' ');
                    continue;
                }
                if (value[i] == '%' && i + 2 < value.size()) {
                    const std::string hex = value.substr(i + 1, 2);
                    char* end = nullptr;
                    const long code = std::strtol(hex.c_str(), &end, 16);
                    if (end && *end == '\0') {
                        decoded.push_back(static_cast<char>(code));
                        i += 2;
                        continue;
                    }
                }
                decoded.push_back(value[i]);
            }
            return decoded;
        }
    }
    return "";
}

Response json_response(int status, const std::string& body) {
    Response response;
    response.status_code = status;
    response.body = body;
    response.headers[HeaderKey::ContentType] = "application/json";
    response.headers[HeaderKey::ContentLength] = std::to_string(body.size());
    response.headers[std::string("Access-Control-Allow-Origin")] = "*";
    response.headers[std::string("Access-Control-Allow-Methods")] = "GET, POST, OPTIONS";
    response.headers[std::string("Access-Control-Allow-Headers")] = "Content-Type";
    return response;
}

std::string messages_to_json(const std::vector<EmailMessage>& messages) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const auto& message = messages[i];
        out << "{";
        out << "\"id\":\"" << json_escape(message.id) << "\",";
        out << "\"from\":\"" << json_escape(message.from) << "\",";
        out << "\"to\":\"" << json_escape(message.to) << "\",";
        out << "\"subject\":\"" << json_escape(message.subject) << "\",";
        out << "\"body\":\"" << json_escape(message.body) << "\",";
        out << "\"timestamp\":\"" << json_escape(message.timestamp) << "\",";
        out << "\"read\":" << (message.read ? "true" : "false");
        out << "}";
    }
    out << "]";
    return out.str();
}

class EmailCenterServlet : public networking::servlets::HttpServletBase {
public:
    EmailCenterServlet()
        : exposure_(ApiExposureType::Rest),
          imap_(mailbox_store_) {
        mailbox_store_.create_mailbox("inbox");
    }

    std::string get_version() const override {
        return "HTTP/1.1";
    }

    Response handle_request(const Request& req) override {
        const std::string path = path_without_query(req.uri);

        if (req.method == "OPTIONS") {
            return json_response(204, "");
        }

        if (req.method == "GET" && (path == "/health" || path == "/api/email/health")) {
            return json_response(200, "{\"status\":\"ok\"}");
        }

        if (req.method == "GET" && path == "/api/email/status") {
            return handle_status();
        }

        if (req.method == "POST" && path == "/api/email/smtp/register") {
            return handle_smtp_register(req.body);
        }

        if (req.method == "POST" && path == "/api/email/smtp/unregister") {
            return handle_smtp_unregister(req.body);
        }

        if (req.method == "POST" && path == "/api/email/mailbox/create") {
            return handle_mailbox_create(req.body);
        }

        if (req.method == "POST" && path == "/api/email/mailbox/delete") {
            return handle_mailbox_delete(req.body);
        }

        if (req.method == "GET" && path == "/api/email/messages") {
            return handle_messages_list(req.uri);
        }

        if (req.method == "POST" && path == "/api/email/messages/read") {
            return handle_messages_read(req.body);
        }

        if (req.method == "POST" && path == "/api/email/send") {
            return handle_send_mail(req.body);
        }

        if (req.method == "POST" && path == "/api/email/exposure") {
            return handle_exposure(req.body);
        }

        return json_response(404, "{\"success\":false,\"error\":\"not found\"}");
    }

private:
    mutable std::mutex mutex_;
    SmtpServer smtp_;
    MailboxStore mailbox_store_;
    ApiExposure exposure_;
    ImapServer imap_;
    std::string exposure_mode_ = "rest";

    Response handle_status() {
        std::lock_guard<std::mutex> guard(mutex_);

        std::ostringstream body;
        body << "{";
        body << "\"smtp_accounts\":" << smtp_.account_count() << ",";
        body << "\"mailboxes\":" << mailbox_store_.mailbox_count() << ",";
        body << "\"messages\":" << mailbox_store_.total_messages() << ",";
        body << "\"inbox_unread\":" << mailbox_store_.unread_count("inbox") << ",";
        body << "\"exposure\":\"" << json_escape(exposure_mode_) << "\",";
        body << "\"description\":\"" << json_escape(exposure_.describe()) << "\"";
        body << "}";

        return json_response(200, body.str());
    }

    Response handle_smtp_register(const std::string& json) {
        EmailAccount account;
        account.address = trim_copy(extract_json_string_field(json, "address"));
        account.display_name = trim_copy(extract_json_string_field(json, "display_name"));
        account.outbound_host = trim_copy(extract_json_string_field(json, "outbound_host"));
        account.outbound_port = extract_json_int_field(json, "outbound_port", 587);

        if (account.address.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"address is required\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        const bool ok = smtp_.register_account(account);
        return json_response(
            ok ? 200 : 400,
            ok ? "{\"success\":true}" : "{\"success\":false,\"error\":\"failed to register account\"}"
        );
    }

    Response handle_smtp_unregister(const std::string& json) {
        const std::string address = trim_copy(extract_json_string_field(json, "address"));
        if (address.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"address is required\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        const bool ok = smtp_.unregister_account(address);
        return json_response(
            ok ? 200 : 404,
            ok ? "{\"success\":true}" : "{\"success\":false,\"error\":\"account not found\"}"
        );
    }

    Response handle_mailbox_create(const std::string& json) {
        const std::string mailbox = trim_copy(extract_json_string_field(json, "mailbox"));
        if (mailbox.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"mailbox is required\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        const bool ok = mailbox_store_.create_mailbox(mailbox);
        return json_response(
            ok ? 200 : 409,
            ok ? "{\"success\":true}" : "{\"success\":false,\"error\":\"mailbox already exists\"}"
        );
    }

    Response handle_mailbox_delete(const std::string& json) {
        const std::string mailbox = trim_copy(extract_json_string_field(json, "mailbox"));
        if (mailbox.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"mailbox is required\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        const bool ok = mailbox_store_.delete_mailbox(mailbox);
        return json_response(
            ok ? 200 : 404,
            ok ? "{\"success\":true}" : "{\"success\":false,\"error\":\"mailbox not found\"}"
        );
    }

    Response handle_messages_list(const std::string& uri) {
        const std::string mailbox = trim_copy(query_value(uri, "mailbox"));
        if (mailbox.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"mailbox query is required\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        std::vector<EmailMessage> messages;
        if (!imap_.fetch_mailbox(mailbox, &messages)) {
            return json_response(404, "{\"success\":false,\"error\":\"mailbox not found\"}");
        }

        std::ostringstream body;
        body << "{";
        body << "\"success\":true,";
        body << "\"mailbox\":\"" << json_escape(mailbox) << "\",";
        body << "\"messages\":" << messages_to_json(messages);
        body << "}";

        return json_response(200, body.str());
    }

    Response handle_messages_read(const std::string& json) {
        const std::string mailbox = trim_copy(extract_json_string_field(json, "mailbox"));
        const std::string id = trim_copy(extract_json_string_field(json, "id"));

        if (mailbox.empty() || id.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"mailbox and id are required\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        const bool ok = imap_.mark_read(mailbox, id);
        return json_response(
            ok ? 200 : 404,
            ok ? "{\"success\":true}" : "{\"success\":false,\"error\":\"message not found\"}"
        );
    }

    Response handle_send_mail(const std::string& json) {
        const std::string from_account = trim_copy(extract_json_string_field(json, "from_account"));
        const std::string mailbox = trim_copy(extract_json_string_field(json, "mailbox"));
        const std::string to = trim_copy(extract_json_string_field(json, "to"));
        const std::string subject = trim_copy(extract_json_string_field(json, "subject"));
        const std::string body_text = extract_json_string_field(json, "body");

        std::string error;
        std::lock_guard<std::mutex> guard(mutex_);
        const bool ok = smtp_.send_mail(from_account, mailbox, to, subject, body_text, mailbox_store_, &error);

        if (!ok) {
            return json_response(400, "{\"success\":false,\"error\":\"" + json_escape(error) + "\"}");
        }
        return json_response(200, "{\"success\":true}");
    }

    Response handle_exposure(const std::string& json) {
        const std::string exposure = trim_copy(extract_json_string_field(json, "exposure"));
        if (exposure != "rest" && exposure != "websocket") {
            return json_response(400, "{\"success\":false,\"error\":\"exposure must be 'rest' or 'websocket'\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        if (exposure == "websocket") {
            exposure_ = ApiExposure(ApiExposureType::WebSocket);
        } else {
            exposure_ = ApiExposure(ApiExposureType::Rest);
        }
        exposure_mode_ = exposure;

        return json_response(200, "{\"success\":true,\"mode\":\"" + json_escape(exposure_mode_) + "\",\"description\":\"" + json_escape(exposure_.describe()) + "\"}");
    }
};

std::optional<int> parse_port_from_args(int argc, char** argv) {
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--port") {
            try {
                return std::stoi(argv[i + 1]);
            } catch (...) {
                return std::nullopt;
            }
        }
    }
    return std::nullopt;
}

volatile std::sig_atomic_t stop_flag = 0;

void signal_handler(int) {
    stop_flag = 1;
}

} // namespace

int main(int argc, char** argv) {
    const int port = parse_port_from_args(argc, argv).value_or(8022);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto servlet = std::make_shared<EmailCenterServlet>();
    io::http_server::HttpServer server(port, 4, nullptr, servlet);

    std::thread server_thread([&server]() {
        server.start();
    });

    std::cout << "[email_center_api] listening on port " << port << std::endl;

    while (!stop_flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[email_center_api] shutting down" << std::endl;
    server.stop();

    if (server_thread.joinable()) {
        server_thread.join();
    }

    return 0;
}
