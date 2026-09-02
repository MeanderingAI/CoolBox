#include "freeswitch_adapter.h"

#include <cctype>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace trekker {
namespace voip {

namespace {

bool socket_send_all(int sock, const std::string& payload) {
    std::size_t sent = 0;
    while (sent < payload.size()) {
#ifdef _WIN32
        const int rc = send(static_cast<SOCKET>(sock), payload.data() + sent, static_cast<int>(payload.size() - sent), 0);
#else
        const ssize_t rc = send(sock, payload.data() + sent, payload.size() - sent, 0);
#endif
        if (rc <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(rc);
    }
    return true;
}

std::string socket_recv_until(int sock, const std::string& delimiter, int max_bytes = 8192) {
    std::string data;
    data.reserve(1024);
    char buffer[512];
    while (static_cast<int>(data.size()) < max_bytes) {
#ifdef _WIN32
        const int rc = recv(static_cast<SOCKET>(sock), buffer, static_cast<int>(sizeof(buffer)), 0);
#else
        const ssize_t rc = recv(sock, buffer, sizeof(buffer), 0);
#endif
        if (rc <= 0) {
            break;
        }
        data.append(buffer, buffer + rc);
        if (data.find(delimiter) != std::string::npos) {
            break;
        }
    }
    return data;
}

std::string extract_job_uuid(const std::string& payload) {
    const std::string key = "Job-UUID:";
    const auto pos = payload.find(key);
    if (pos == std::string::npos) {
        return "";
    }
    auto end = payload.find('\n', pos);
    if (end == std::string::npos) {
        end = payload.size();
    }
    std::string value = payload.substr(pos + key.size(), end - (pos + key.size()));
    const auto first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

} // namespace

bool FreeSwitchAdapter::configure(const FreeSwitchConfig& config, std::string* error) {
    if (trim_copy(config.event_socket_host).empty()) {
        if (error) {
            *error = "event_socket_host is required";
        }
        return false;
    }
    if (config.event_socket_port <= 0 || config.event_socket_port > 65535) {
        if (error) {
            *error = "event_socket_port must be 1..65535";
        }
        return false;
    }
    if (trim_copy(config.event_socket_password).empty()) {
        if (error) {
            *error = "event_socket_password is required";
        }
        return false;
    }
    if (trim_copy(config.sip_profile).empty()) {
        if (error) {
            *error = "sip_profile is required";
        }
        return false;
    }
    if (trim_copy(config.gateway).empty()) {
        if (error) {
            *error = "gateway is required";
        }
        return false;
    }

    config_ = config;
    return true;
}

FreeSwitchConfig FreeSwitchAdapter::config() const {
    return config_;
}

bool FreeSwitchAdapter::can_originate(std::string* error) const {
    if (trim_copy(config_.gateway).empty()) {
        if (error) {
            *error = "gateway is not configured";
        }
        return false;
    }
    if (trim_copy(config_.caller_id_number).empty()) {
        if (error) {
            *error = "caller_id_number is not configured";
        }
        return false;
    }
    return true;
}

FreeSwitchOriginateResult FreeSwitchAdapter::build_originate(const FreeSwitchOriginateRequest& request) const {
    FreeSwitchOriginateResult result;
    result.call_id = trim_copy(request.call_id);

    std::string error;
    if (!can_originate(&error)) {
        result.error = error;
        return result;
    }

    const std::string normalized = normalize_e164(request.to_e164);
    if (normalized.empty()) {
        result.error = "to_e164 must contain a valid E.164 number";
        return result;
    }

    const std::string timeout = trim_copy(request.timeout_seconds).empty() ? "30" : trim_copy(request.timeout_seconds);

    std::ostringstream bridge_uri;
    bridge_uri << "sofia/gateway/" << config_.gateway << "/" << normalized.substr(1);

    std::ostringstream originate;
    originate << "originate {origination_caller_id_number=" << config_.caller_id_number
              << ",origination_uuid=" << result.call_id
              << ",ignore_early_media=true"
              << ",call_timeout=" << timeout
              << "}" << bridge_uri.str()
              << " &park()";

    result.success = true;
    result.normalized_to_e164 = normalized;
    result.bridge_uri = bridge_uri.str();
    result.originate_command = originate.str();
    return result;
}

FreeSwitchOriginateResult FreeSwitchAdapter::originate_via_event_socket(const FreeSwitchOriginateRequest& request) const {
    FreeSwitchOriginateResult result = build_originate(request);
    if (!result.success) {
        return result;
    }

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        result.success = false;
        result.error = "WSAStartup failed";
        return result;
    }
#endif

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* resolved = nullptr;
    const std::string port = std::to_string(config_.event_socket_port);
    const int gai_rc = getaddrinfo(config_.event_socket_host.c_str(), port.c_str(), &hints, &resolved);
    if (gai_rc != 0 || resolved == nullptr) {
        result.success = false;
        result.error = "failed to resolve FreeSWITCH event socket host";
#ifdef _WIN32
        WSACleanup();
#endif
        return result;
    }

    int sock = -1;
    for (addrinfo* it = resolved; it != nullptr; it = it->ai_next) {
#ifdef _WIN32
        const SOCKET candidate = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (candidate == INVALID_SOCKET) {
            continue;
        }
        if (connect(candidate, it->ai_addr, static_cast<int>(it->ai_addrlen)) == 0) {
            sock = static_cast<int>(candidate);
            break;
        }
        closesocket(candidate);
#else
        const int candidate = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (candidate < 0) {
            continue;
        }
        if (connect(candidate, it->ai_addr, it->ai_addrlen) == 0) {
            sock = candidate;
            break;
        }
        close(candidate);
#endif
    }
    freeaddrinfo(resolved);

    if (sock < 0) {
        result.success = false;
        result.error = "failed to connect to FreeSWITCH event socket";
#ifdef _WIN32
        WSACleanup();
#endif
        return result;
    }

    const std::string greeting = socket_recv_until(sock, "\n\n");
    if (greeting.find("auth/request") == std::string::npos) {
        result.success = false;
        result.error = "unexpected FreeSWITCH greeting";
        result.event_socket_reply = greeting;
#ifdef _WIN32
        closesocket(static_cast<SOCKET>(sock));
        WSACleanup();
#else
        close(sock);
#endif
        return result;
    }

    const std::string auth = "auth " + config_.event_socket_password + "\n\n";
    if (!socket_send_all(sock, auth)) {
        result.success = false;
        result.error = "failed to send auth command";
#ifdef _WIN32
        closesocket(static_cast<SOCKET>(sock));
        WSACleanup();
#else
        close(sock);
#endif
        return result;
    }

    const std::string auth_reply = socket_recv_until(sock, "\n\n");
    if (auth_reply.find("+OK accepted") == std::string::npos) {
        result.success = false;
        result.error = "FreeSWITCH auth failed";
        result.event_socket_reply = auth_reply;
#ifdef _WIN32
        closesocket(static_cast<SOCKET>(sock));
        WSACleanup();
#else
        close(sock);
#endif
        return result;
    }

    const std::string command = "bgapi " + result.originate_command + "\n\n";
    if (!socket_send_all(sock, command)) {
        result.success = false;
        result.error = "failed to send bgapi originate command";
#ifdef _WIN32
        closesocket(static_cast<SOCKET>(sock));
        WSACleanup();
#else
        close(sock);
#endif
        return result;
    }

    const std::string command_reply = socket_recv_until(sock, "\n\n");
    result.event_socket_reply = command_reply;
    result.bgapi_job_uuid = extract_job_uuid(command_reply);
    if (command_reply.find("+OK Job-UUID") == std::string::npos && result.bgapi_job_uuid.empty()) {
        result.success = false;
        result.error = "FreeSWITCH bgapi originate rejected";
    }

#ifdef _WIN32
    closesocket(static_cast<SOCKET>(sock));
    WSACleanup();
#else
    close(sock);
#endif

    return result;
}

std::string FreeSwitchAdapter::trim_copy(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

std::string FreeSwitchAdapter::normalize_e164(const std::string& raw_number) {
    const std::string trimmed = trim_copy(raw_number);
    if (trimmed.empty()) {
        return "";
    }

    std::string digits;
    digits.reserve(trimmed.size());
    bool has_plus = false;

    for (std::size_t i = 0; i < trimmed.size(); ++i) {
        const char ch = trimmed[i];
        if (i == 0 && ch == '+') {
            has_plus = true;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
            digits.push_back(ch);
            continue;
        }
        if (ch == ' ' || ch == '-' || ch == '(' || ch == ')' || ch == '.') {
            continue;
        }
        return "";
    }

    if (digits.size() < 8 || digits.size() > 15) {
        return "";
    }

    if (!has_plus) {
        return "+" + digits;
    }
    return "+" + digits;
}

} // namespace voip
} // namespace trekker
