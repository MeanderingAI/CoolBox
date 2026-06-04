#pragma once

#include <string>

namespace trekker {
namespace voip {

struct FreeSwitchConfig {
    std::string event_socket_host = "127.0.0.1";
    int event_socket_port = 8021;
    std::string event_socket_password = "ClueCon";
    std::string sip_profile = "external";
    std::string gateway = "";
    std::string caller_id_number = "+10000000000";
};

struct FreeSwitchOriginateRequest {
    std::string call_id;
    std::string from_uri;
    std::string to_e164;
    std::string timeout_seconds = "30";
};

struct FreeSwitchOriginateResult {
    bool success = false;
    std::string call_id;
    std::string normalized_to_e164;
    std::string bridge_uri;
    std::string originate_command;
    std::string event_socket_reply;
    std::string bgapi_job_uuid;
    std::string error;
};

class FreeSwitchAdapter {
public:
    bool configure(const FreeSwitchConfig& config, std::string* error = nullptr);
    FreeSwitchConfig config() const;

    bool can_originate(std::string* error = nullptr) const;

    FreeSwitchOriginateResult build_originate(const FreeSwitchOriginateRequest& request) const;
    FreeSwitchOriginateResult originate_via_event_socket(const FreeSwitchOriginateRequest& request) const;

private:
    static std::string trim_copy(const std::string& value);
    static std::string normalize_e164(const std::string& raw_number);

    FreeSwitchConfig config_{};
};

} // namespace voip
} // namespace trekker
