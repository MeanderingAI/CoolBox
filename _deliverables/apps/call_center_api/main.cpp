#include <algorithm>
#include <atomic>
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
#include <unordered_map>
#include <utility>
#include <vector>

#include "api_exposure.h"
#include "freeswitch_adapter.h"
#include "http_server.h"
#include "mcu.h"
#include "nat_traversal.h"
#include "sip_server.h"

namespace {

using trekker::voip::ApiExposure;
using trekker::voip::ApiExposureType;
using trekker::voip::FreeSwitchAdapter;
using trekker::voip::FreeSwitchConfig;
using trekker::voip::FreeSwitchOriginateRequest;
using trekker::voip::FreeSwitchOriginateResult;
using trekker::voip::McuBridge;
using trekker::voip::NatCandidate;
using trekker::voip::NatTraversal;
using trekker::voip::SipEndpoint;
using trekker::voip::SipServer;

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

std::string trim_copy(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
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
    std::string query = uri.substr(pos + 1);
    std::istringstream iss(query);
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
                if (value[i] == '+' ) {
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
    Response r;
    r.status_code = status;
    r.body = body;
    r.headers[HeaderKey::ContentType] = "application/json";
    r.headers[HeaderKey::ContentLength] = std::to_string(body.size());
    r.headers[std::string("Access-Control-Allow-Origin")] = "*";
    r.headers[std::string("Access-Control-Allow-Methods")] = "GET, POST, OPTIONS";
    r.headers[std::string("Access-Control-Allow-Headers")] = "Content-Type";
    return r;
}

std::string generate_call_id() {
    static std::atomic<unsigned long> sequence{1};
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::ostringstream oss;
    oss << "pstn-" << now_ms << "-" << sequence.fetch_add(1);
    return oss.str();
}

struct SignalMessage {
    std::string type;
    std::string from;
    std::string to;
    std::string call_id;
    std::string sdp;
    std::string candidate;
    std::string reason;
};

class CallCenterServlet : public networking::servlets::HttpServletBase {
public:
    std::string get_version() const override {
        return "HTTP/1.1";
    }

    Response handle_request(const Request& req) override {
        const std::string path = path_without_query(req.uri);

        if (req.method == "OPTIONS") {
            return json_response(204, "");
        }

        if (req.method == "GET" && (path == "/health" || path == "/api/voip/health")) {
            return json_response(200, "{\"status\":\"ok\"}");
        }

        if (req.method == "GET" && path == "/api/voip/status") {
            return handle_status();
        }

        if (req.method == "GET" && path == "/api/voip/sip/endpoints") {
            return handle_sip_endpoints();
        }

        if (req.method == "POST" && path == "/api/voip/sip/register") {
            return handle_sip_register(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/sip/unregister") {
            return handle_sip_unregister(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/mcu/join") {
            return handle_mcu_join(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/mcu/leave") {
            return handle_mcu_leave(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/nat/select") {
            return handle_nat_select(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/exposure") {
            return handle_exposure(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/presence/online") {
            return handle_presence_online(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/presence/offline") {
            return handle_presence_offline(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/signals/send") {
            return handle_signal_send(req.body);
        }

        if (req.method == "GET" && path == "/api/voip/signals/poll") {
            return handle_signal_poll(req.uri);
        }

        if (req.method == "GET" && path == "/api/voip/pstn/config") {
            return handle_pstn_config_get();
        }

        if (req.method == "POST" && path == "/api/voip/pstn/config") {
            return handle_pstn_config_set(req.body);
        }

        if (req.method == "POST" && path == "/api/voip/pstn/call") {
            return handle_pstn_call(req.body);
        }

        return json_response(404, "{\"success\":false,\"error\":\"not found\"}");
    }

private:
    SipServer sip_server_;
    McuBridge mcu_;
    NatTraversal nat_;
    ApiExposure exposure_{ApiExposureType::Rest};
    FreeSwitchAdapter freeswitch_;

    mutable std::mutex mutex_;
    std::map<std::string, SipEndpoint> endpoints_;
    std::map<std::string, std::vector<SignalMessage>> pending_signals_;
    std::map<std::string, std::string> calls_;
    std::map<std::string, std::string> pstn_jobs_;
    std::map<std::string, std::map<std::string, std::string>> rooms_;
    std::vector<std::string> online_endpoints_;
    std::string exposure_mode_ = "rest";
    bool freeswitch_configured_ = false;

    Response handle_status() {
        std::lock_guard<std::mutex> guard(mutex_);

        std::ostringstream room_sizes;
        room_sizes << "{";
        bool first = true;
        for (const auto& item : rooms_) {
            if (!first) {
                room_sizes << ",";
            }
            first = false;
            room_sizes << "\"" << json_escape(item.first) << "\":" << item.second.size();
        }
        room_sizes << "}";

        std::ostringstream online;
        online << "[";
        for (std::size_t i = 0; i < online_endpoints_.size(); ++i) {
            if (i > 0) {
                online << ",";
            }
            online << "\"" << json_escape(online_endpoints_[i]) << "\"";
        }
        online << "]";

        std::ostringstream body;
        body << "{";
        body << "\"sip_endpoints\":" << endpoints_.size() << ",";
        body << "\"rooms\":" << room_sizes.str() << ",";
        body << "\"exposure\":\"" << json_escape(exposure_mode_) << "\",";
        body << "\"active_calls\":" << calls_.size() << ",";
        body << "\"pstn_configured\":" << (freeswitch_configured_ ? "true" : "false") << ",";
        body << "\"active_pstn_jobs\":" << pstn_jobs_.size() << ",";
        body << "\"online_endpoints\":" << online.str();
        body << "}";

        return json_response(200, body.str());
    }

    Response handle_sip_endpoints() {
        std::lock_guard<std::mutex> guard(mutex_);
        std::ostringstream endpoints_json;
        endpoints_json << "[";
        std::size_t index = 0;
        for (const auto& [uri, endpoint] : endpoints_) {
            if (index++ > 0) {
                endpoints_json << ",";
            }
            endpoints_json << "{";
            endpoints_json << "\"uri\":\"" << json_escape(endpoint.uri) << "\",";
            endpoints_json << "\"transport\":\"" << json_escape(endpoint.transport) << "\"";
            endpoints_json << "}";
        }
        endpoints_json << "]";

        std::ostringstream online;
        online << "[";
        for (std::size_t i = 0; i < online_endpoints_.size(); ++i) {
            if (i > 0) {
                online << ",";
            }
            online << "\"" << json_escape(online_endpoints_[i]) << "\"";
        }
        online << "]";

        std::ostringstream body;
        body << "{";
        body << "\"endpoints\":" << endpoints_json.str() << ",";
        body << "\"online\":" << online.str();
        body << "}";
        return json_response(200, body.str());
    }

    Response handle_sip_register(const std::string& json) {
        const std::string uri = trim_copy(extract_json_string_field(json, "uri"));
        std::string transport = trim_copy(extract_json_string_field(json, "transport"));
        if (transport.empty()) {
            transport = "udp";
        }

        if (uri.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"uri is required\"}");
        }

        SipEndpoint endpoint{uri, transport};
        const bool ok = sip_server_.register_endpoint(endpoint);
        if (!ok) {
            return json_response(400, "{\"success\":false,\"error\":\"invalid endpoint\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        endpoints_[uri] = endpoint;
        std::ostringstream body;
        body << "{\"success\":true,\"count\":" << endpoints_.size() << "}";
        return json_response(200, body.str());
    }

    Response handle_sip_unregister(const std::string& json) {
        const std::string uri = trim_copy(extract_json_string_field(json, "uri"));
        if (uri.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"uri is required\"}");
        }

        const bool removed = sip_server_.unregister_endpoint(uri);

        std::lock_guard<std::mutex> guard(mutex_);
        endpoints_.erase(uri);
        online_endpoints_.erase(
            std::remove(online_endpoints_.begin(), online_endpoints_.end(), uri),
            online_endpoints_.end()
        );

        std::ostringstream body;
        body << "{\"success\":" << (removed ? "true" : "false") << ",\"count\":" << endpoints_.size() << "}";
        return json_response(200, body.str());
    }

    Response handle_mcu_join(const std::string& json) {
        const std::string room = trim_copy(extract_json_string_field(json, "room"));
        const std::string participant = trim_copy(extract_json_string_field(json, "participant"));
        if (room.empty() || participant.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"room and participant are required\"}");
        }

        const bool ok = mcu_.join_room(room, participant);
        if (!ok) {
            return json_response(400, "{\"success\":false,\"participants\":0}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        rooms_[room][participant] = participant;
        std::ostringstream body;
        body << "{\"success\":true,\"participants\":" << rooms_[room].size() << "}";
        return json_response(200, body.str());
    }

    Response handle_mcu_leave(const std::string& json) {
        const std::string room = trim_copy(extract_json_string_field(json, "room"));
        const std::string participant = trim_copy(extract_json_string_field(json, "participant"));
        if (room.empty() || participant.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"room and participant are required\"}");
        }

        const bool removed = mcu_.leave_room(room, participant);

        std::lock_guard<std::mutex> guard(mutex_);
        auto room_it = rooms_.find(room);
        if (room_it != rooms_.end()) {
            room_it->second.erase(participant);
            if (room_it->second.empty()) {
                rooms_.erase(room_it);
            }
        }

        const std::size_t participants = rooms_.count(room) ? rooms_[room].size() : 0;
        std::ostringstream body;
        body << "{\"success\":" << (removed ? "true" : "false") << ",\"participants\":" << participants << "}";
        return json_response(200, body.str());
    }

    Response handle_nat_select(const std::string& json) {
        NatCandidate local;
        local.address = trim_copy(extract_json_string_field(json, "address"));
        local.port = static_cast<unsigned short>(extract_json_int_field(json, "port", 0));
        local.protocol = trim_copy(extract_json_string_field(json, "protocol"));

        NatCandidate reflexive;
        const std::regex reflexive_pattern("\\\"reflexive\\\"\\s*:\\s*\\{([^}]*)\\}");
        const std::regex local_pattern("\\\"local\\\"\\s*:\\s*\\{([^}]*)\\}");
        std::smatch match;

        std::string local_blob = json;
        std::string reflexive_blob = json;

        if (std::regex_search(json, match, local_pattern)) {
            local_blob = match[1].str();
        }
        if (std::regex_search(json, match, reflexive_pattern)) {
            reflexive_blob = match[1].str();
        }

        local.address = trim_copy(extract_json_string_field(local_blob, "address"));
        local.port = static_cast<unsigned short>(extract_json_int_field(local_blob, "port", 0));
        local.protocol = trim_copy(extract_json_string_field(local_blob, "protocol"));

        reflexive.address = trim_copy(extract_json_string_field(reflexive_blob, "address"));
        reflexive.port = static_cast<unsigned short>(extract_json_int_field(reflexive_blob, "port", 0));
        reflexive.protocol = trim_copy(extract_json_string_field(reflexive_blob, "protocol"));

        if (local.protocol.empty()) {
            local.protocol = "udp";
        }
        if (reflexive.protocol.empty()) {
            reflexive.protocol = "udp";
        }

        const NatCandidate chosen = nat_.best_candidate(local, reflexive);

        std::ostringstream body;
        body << "{\"candidate\":{";
        body << "\"address\":\"" << json_escape(chosen.address) << "\",";
        body << "\"port\":" << chosen.port << ",";
        body << "\"protocol\":\"" << json_escape(chosen.protocol) << "\"";
        body << "}}";
        return json_response(200, body.str());
    }

    Response handle_exposure(const std::string& json) {
        const std::string mode = trim_copy(extract_json_string_field(json, "exposure"));
        if (mode != "rest" && mode != "websocket") {
            return json_response(400, "{\"success\":false,\"error\":\"exposure must be rest or websocket\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        exposure_mode_ = mode;
        exposure_ = ApiExposure(mode == "websocket" ? ApiExposureType::WebSocket : ApiExposureType::Rest);

        std::ostringstream body;
        body << "{";
        body << "\"success\":true,";
        body << "\"exposure\":\"" << json_escape(exposure_mode_) << "\",";
        body << "\"description\":\"" << json_escape(exposure_.describe()) << "\"";
        body << "}";
        return json_response(200, body.str());
    }

    Response handle_presence_online(const std::string& json) {
        const std::string uri = trim_copy(extract_json_string_field(json, "uri"));
        if (uri.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"uri is required\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        if (std::find(online_endpoints_.begin(), online_endpoints_.end(), uri) == online_endpoints_.end()) {
            online_endpoints_.push_back(uri);
            std::sort(online_endpoints_.begin(), online_endpoints_.end());
        }
        return json_response(200, "{\"success\":true}");
    }

    Response handle_presence_offline(const std::string& json) {
        const std::string uri = trim_copy(extract_json_string_field(json, "uri"));
        if (uri.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"uri is required\"}");
        }

        std::lock_guard<std::mutex> guard(mutex_);
        online_endpoints_.erase(
            std::remove(online_endpoints_.begin(), online_endpoints_.end(), uri),
            online_endpoints_.end()
        );
        return json_response(200, "{\"success\":true}");
    }

    Response handle_signal_send(const std::string& json) {
        SignalMessage msg;
        msg.type = trim_copy(extract_json_string_field(json, "type"));
        msg.from = trim_copy(extract_json_string_field(json, "from"));
        msg.to = trim_copy(extract_json_string_field(json, "to"));
        msg.call_id = trim_copy(extract_json_string_field(json, "call_id"));
        msg.sdp = trim_copy(extract_json_string_field(json, "sdp"));
        msg.candidate = trim_copy(extract_json_string_field(json, "candidate"));
        msg.reason = trim_copy(extract_json_string_field(json, "reason"));

        if (msg.type.empty() || msg.from.empty() || msg.to.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"type, from, and to are required\"}");
        }

        if (msg.call_id.empty()) {
            msg.call_id = msg.from + "->" + msg.to;
        }

        if (msg.type == "offer") {
            std::lock_guard<std::mutex> guard(mutex_);
            calls_[msg.call_id] = "ringing";
        } else if (msg.type == "answer") {
            std::lock_guard<std::mutex> guard(mutex_);
            calls_[msg.call_id] = "active";
        } else if (msg.type == "hangup" || msg.type == "reject") {
            std::lock_guard<std::mutex> guard(mutex_);
            calls_.erase(msg.call_id);
        }

        {
            std::lock_guard<std::mutex> guard(mutex_);
            pending_signals_[msg.to].push_back(msg);
        }

        return json_response(200, "{\"success\":true}");
    }

    Response handle_signal_poll(const std::string& uri) {
        const std::string endpoint = trim_copy(query_value(uri, "uri"));
        if (endpoint.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"uri query parameter is required\"}");
        }

        std::vector<SignalMessage> messages;
        {
            std::lock_guard<std::mutex> guard(mutex_);
            auto it = pending_signals_.find(endpoint);
            if (it != pending_signals_.end()) {
                messages = std::move(it->second);
                pending_signals_.erase(it);
            }
        }

        std::ostringstream body;
        body << "{\"messages\":[";
        for (std::size_t i = 0; i < messages.size(); ++i) {
            const auto& msg = messages[i];
            if (i > 0) {
                body << ",";
            }
            body << "{";
            body << "\"type\":\"signal." << json_escape(msg.type) << "\",";
            body << "\"from\":\"" << json_escape(msg.from) << "\",";
            body << "\"to\":\"" << json_escape(msg.to) << "\",";
            body << "\"call_id\":\"" << json_escape(msg.call_id) << "\",";
            body << "\"sdp\":\"" << json_escape(msg.sdp) << "\",";
            body << "\"candidate\":\"" << json_escape(msg.candidate) << "\",";
            body << "\"reason\":\"" << json_escape(msg.reason) << "\"";
            body << "}";
        }
        body << "]}";
        return json_response(200, body.str());
    }

    Response handle_pstn_config_get() {
        std::lock_guard<std::mutex> guard(mutex_);
        const FreeSwitchConfig config = freeswitch_.config();
        std::ostringstream body;
        body << "{";
        body << "\"success\":true,";
        body << "\"configured\":" << (freeswitch_configured_ ? "true" : "false") << ",";
        body << "\"event_socket_host\":\"" << json_escape(config.event_socket_host) << "\",";
        body << "\"event_socket_port\":" << config.event_socket_port << ",";
        body << "\"event_socket_password_set\":" << (config.event_socket_password.empty() ? "false" : "true") << ",";
        body << "\"sip_profile\":\"" << json_escape(config.sip_profile) << "\",";
        body << "\"gateway\":\"" << json_escape(config.gateway) << "\",";
        body << "\"caller_id_number\":\"" << json_escape(config.caller_id_number) << "\"";
        body << "}";
        return json_response(200, body.str());
    }

    Response handle_pstn_config_set(const std::string& json) {
        std::lock_guard<std::mutex> guard(mutex_);
        FreeSwitchConfig config = freeswitch_.config();

        const std::string host = trim_copy(extract_json_string_field(json, "event_socket_host"));
        const std::string password = trim_copy(extract_json_string_field(json, "event_socket_password"));
        const std::string profile = trim_copy(extract_json_string_field(json, "sip_profile"));
        const std::string gateway = trim_copy(extract_json_string_field(json, "gateway"));
        const std::string caller = trim_copy(extract_json_string_field(json, "caller_id_number"));
        const int port = extract_json_int_field(json, "event_socket_port", 0);

        if (!host.empty()) {
            config.event_socket_host = host;
        }
        if (port > 0) {
            config.event_socket_port = port;
        }
        if (!password.empty()) {
            config.event_socket_password = password;
        }
        if (!profile.empty()) {
            config.sip_profile = profile;
        }
        if (!gateway.empty()) {
            config.gateway = gateway;
        }
        if (!caller.empty()) {
            config.caller_id_number = caller;
        }

        std::string error;
        if (!freeswitch_.configure(config, &error)) {
            return json_response(400, "{\"success\":false,\"error\":\"" + json_escape(error) + "\"}");
        }
        freeswitch_configured_ = true;

        std::ostringstream body;
        body << "{";
        body << "\"success\":true,";
        body << "\"configured\":true,";
        body << "\"event_socket_host\":\"" << json_escape(config.event_socket_host) << "\",";
        body << "\"event_socket_port\":" << config.event_socket_port << ",";
        body << "\"sip_profile\":\"" << json_escape(config.sip_profile) << "\",";
        body << "\"gateway\":\"" << json_escape(config.gateway) << "\",";
        body << "\"caller_id_number\":\"" << json_escape(config.caller_id_number) << "\"";
        body << "}";
        return json_response(200, body.str());
    }

    Response handle_pstn_call(const std::string& json) {
        FreeSwitchOriginateRequest request;
        request.call_id = trim_copy(extract_json_string_field(json, "call_id"));
        request.from_uri = trim_copy(extract_json_string_field(json, "from_uri"));
        request.to_e164 = trim_copy(extract_json_string_field(json, "to_number"));
        const int timeout_seconds = extract_json_int_field(json, "timeout_seconds", 30);
        request.timeout_seconds = timeout_seconds > 0 ? std::to_string(timeout_seconds) : "30";

        if (request.call_id.empty()) {
            request.call_id = generate_call_id();
        }
        if (request.to_e164.empty()) {
            return json_response(400, "{\"success\":false,\"error\":\"to_number is required\"}");
        }

        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (!freeswitch_configured_) {
                return json_response(400, "{\"success\":false,\"error\":\"FreeSWITCH is not configured; POST /api/voip/pstn/config first\"}");
            }
        }

        FreeSwitchOriginateResult result;
        {
            std::lock_guard<std::mutex> guard(mutex_);
            result = freeswitch_.originate_via_event_socket(request);
            if (result.success) {
                calls_[request.call_id] = "dialing-pstn";
                pstn_jobs_[request.call_id] = result.bgapi_job_uuid;
            }
        }

        if (!result.success) {
            std::ostringstream body;
            body << "{";
            body << "\"success\":false,";
            body << "\"call_id\":\"" << json_escape(request.call_id) << "\",";
            body << "\"error\":\"" << json_escape(result.error) << "\",";
            body << "\"originate_command\":\"" << json_escape(result.originate_command) << "\"";
            body << "}";
            return json_response(502, body.str());
        }

        std::ostringstream body;
        body << "{";
        body << "\"success\":true,";
        body << "\"call_id\":\"" << json_escape(result.call_id) << "\",";
        body << "\"to_number\":\"" << json_escape(result.normalized_to_e164) << "\",";
        body << "\"bridge_uri\":\"" << json_escape(result.bridge_uri) << "\",";
        body << "\"job_uuid\":\"" << json_escape(result.bgapi_job_uuid) << "\"";
        body << "}";
        return json_response(200, body.str());
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
    const int port = parse_port_from_args(argc, argv).value_or(8011);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto servlet = std::make_shared<CallCenterServlet>();
    io::http_server::HttpServer server(port, 4, nullptr, servlet);

    std::thread server_thread([&server]() {
        server.start();
    });

    std::cout << "[call_center_api] listening on port " << port << std::endl;

    while (!stop_flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[call_center_api] shutting down" << std::endl;
    server.stop();

    if (server_thread.joinable()) {
        server_thread.join();
    }

    return 0;
}
