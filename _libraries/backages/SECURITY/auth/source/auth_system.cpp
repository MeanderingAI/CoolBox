#include "../headers/auth_system.h"

#include "../../../MISC/hash/headers/password_hash.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cctype>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>

#if defined(COOLBOX_AUTH_USE_OPENSSL_PROVIDER)
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif

namespace auth {

namespace {

std::string legacy_simple_hash(const std::string& input) {
    std::hash<std::string> hasher;
    std::size_t h = hasher(input);
    h ^= (h >> 16);
    h *= 0x85ebca6b;
    h ^= (h >> 13);
    h *= 0xc2b2ae35;
    h ^= (h >> 16);

    std::ostringstream os;
    os << std::hex << std::setfill('0') << std::setw(16) << h;
    return os.str();
}

std::string trim_copy(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool starts_with_case_insensitive(const std::string& text, const std::string& prefix) {
    if (text.size() < prefix.size()) {
        return false;
    }
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(text[i]))) !=
            static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(prefix[i])))) {
            return false;
        }
    }
    return true;
}

std::string base64url_encode(const std::string& input) {
    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    std::uint32_t accumulator = 0;
    int bits_collected = 0;
    for (unsigned char ch : input) {
        accumulator = (accumulator << 8) | ch;
        bits_collected += 8;
        while (bits_collected >= 6) {
            bits_collected -= 6;
            output.push_back(kAlphabet[(accumulator >> bits_collected) & 0x3fu]);
        }
    }
    if (bits_collected > 0) {
        accumulator <<= (6 - bits_collected);
        output.push_back(kAlphabet[accumulator & 0x3fu]);
    }
    return output;
}

std::string base64url_decode(const std::string& input) {
    static const std::array<int, 256> decode_table = []() {
        std::array<int, 256> table{};
        table.fill(-1);
        const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        for (std::size_t i = 0; i < alphabet.size(); ++i) {
            table[static_cast<unsigned char>(alphabet[i])] = static_cast<int>(i);
        }
        return table;
    }();

    std::string output;
    output.reserve((input.size() * 3) / 4);
    std::uint32_t accumulator = 0;
    int bits_collected = 0;
    for (unsigned char ch : input) {
        const int value = decode_table[ch];
        if (value < 0) {
            throw std::runtime_error("Invalid base64url data");
        }
        accumulator = (accumulator << 6) | static_cast<std::uint32_t>(value);
        bits_collected += 6;
        while (bits_collected >= 8) {
            bits_collected -= 8;
            output.push_back(static_cast<char>((accumulator >> bits_collected) & 0xffu));
        }
    }
    return output;
}

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

std::string json_unescape(const std::string& value) {
    std::string output;
    output.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            output.push_back(value[i]);
            continue;
        }
        const char escaped = value[++i];
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

std::string extract_json_string_field(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"((?:\\\\.|[^\\\"])*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return "";
    }
    return json_unescape(match[1].str());
}

std::int64_t extract_json_integer_field(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        throw std::runtime_error("Missing JSON integer field: " + key);
    }
    return std::stoll(match[1].str());
}

#if defined(COOLBOX_AUTH_USE_OPENSSL_PROVIDER)
std::string hmac_sha256(const std::string& key, const std::string& message) {
    unsigned int output_length = 0;
    unsigned char output[EVP_MAX_MD_SIZE];
    if (HMAC(EVP_sha256(),
             key.data(),
             static_cast<int>(key.size()),
             reinterpret_cast<const unsigned char*>(message.data()),
             message.size(),
             output,
             &output_length) == nullptr) {
        throw std::runtime_error("HMAC-SHA256 failed");
    }
    return std::string(reinterpret_cast<const char*>(output), output_length);
}

std::string build_jwt_token(const std::string& secret,
                            const std::string& issuer,
                            int lifetime_seconds,
                            const Session& session,
                            const User& user) {
    const auto now = std::chrono::system_clock::now();
    const auto issued_at = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const auto expires_at = issued_at + lifetime_seconds;

    const std::string header = R"({"alg":"HS256","typ":"JWT"})";
    std::ostringstream payload;
    payload << '{'
            << "\"sub\":\"" << json_escape(user.username) << "\","
            << "\"role\":\"" << json_escape(role_to_string(user.role)) << "\","
            << "\"sid\":\"" << json_escape(session.session_id) << "\","
            << "\"iss\":\"" << json_escape(issuer) << "\","
            << "\"iat\":" << issued_at << ','
            << "\"exp\":" << expires_at
            << '}';

    const std::string encoded_header = base64url_encode(header);
    const std::string encoded_payload = base64url_encode(payload.str());
    const std::string signing_input = encoded_header + "." + encoded_payload;
    const std::string signature = base64url_encode(hmac_sha256(secret, signing_input));
    return signing_input + "." + signature;
}
#endif

std::string extract_bearer_token(const std::string& authorization_header) {
    const std::string trimmed = trim_copy(authorization_header);
    if (!starts_with_case_insensitive(trimmed, "Bearer ")) {
        return "";
    }
    return trim_copy(trimmed.substr(7));
}

bool is_modern_password_hash(const std::string& hash) {
    return hash.rfind("$argon2id$", 0) == 0 ||
           hash.rfind("$coolbox-scrypt$", 0) == 0 ||
           hash.rfind("$coolbox-pbkdf2-sha256$", 0) == 0 ||
           hash.rfind("$2", 0) == 0;
}

} // namespace

AuthSystem::AuthSystem()
    : session_timeout_(3600)
    , max_login_attempts_(5)
    , password_min_length_(8)
    , jwt_lifetime_(3600)
    , jwt_secret_(generate_session_id() + generate_session_id())
    , jwt_issuer_("coolbox-http-server") {}

bool AuthSystem::create_user(const std::string& username, const std::string& password,
                             const std::string& email, UserRole role) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (users_.count(username)) return false;
    if (static_cast<int>(password.size()) < password_min_length_) return false;

    User u;
    u.username = username;
    u.password_hash = hash_password(password);
    u.email = email;
    u.role = role;
    u.created_at = std::chrono::system_clock::now();
    u.last_login = {};
    u.is_active = true;
    users_[username] = u;
    return true;
}

bool AuthSystem::delete_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    return users_.erase(username) > 0;
}

bool AuthSystem::update_user(const std::string& username, const User& user_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(username);
    if (it == users_.end()) return false;
    it->second.email = user_data.email;
    it->second.role = user_data.role;
    it->second.is_active = user_data.is_active;
    it->second.metadata = user_data.metadata;
    return true;
}

User* AuthSystem::get_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(username);
    return it != users_.end() ? &it->second : nullptr;
}

std::vector<std::string> AuthSystem::list_users() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(users_.size());
    for (const auto& [username, _] : users_) {
        names.push_back(username);
    }
    return names;
}

AuthResult AuthSystem::login(const std::string& username, const std::string& password,
                             const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(mutex_);
    AuthResult result{false, "", "", "", {}};

    if (is_account_locked(username)) {
        result.message = "Account is locked due to too many failed attempts";
        return result;
    }

    auto it = users_.find(username);
    if (it == users_.end()) {
        record_login_attempt(username);
        result.message = "Invalid username or password";
        return result;
    }

    if (!it->second.is_active) {
        result.message = "Account is disabled";
        return result;
    }

    if (!verify_password(password, it->second.password_hash)) {
        record_login_attempt(username);
        result.message = "Invalid username or password";
        return result;
    }

    login_attempts_.erase(username);
    const auto now = std::chrono::system_clock::now();
    it->second.last_login = now;

    Session s;
    s.session_id = generate_session_id();
    s.username = username;
    s.role = it->second.role;
    s.created_at = now;
    s.expires_at = now + std::chrono::seconds(session_timeout_);
    s.ip_address = ip_address;
    sessions_[s.session_id] = s;

    result.success = true;
    result.message = "Login successful";
    result.session_id = s.session_id;
    result.user = it->second;

    try {
#if defined(COOLBOX_AUTH_USE_OPENSSL_PROVIDER)
        result.jwt_token = build_jwt_token(jwt_secret_, jwt_issuer_, jwt_lifetime_, s, it->second);
#else
        throw std::runtime_error("JWT support requires OpenSSL-compatible crypto support");
#endif
    } catch (const std::exception& ex) {
        sessions_.erase(s.session_id);
        result.success = false;
        result.message = ex.what();
        result.session_id.clear();
        result.jwt_token.clear();
    }

    return result;
}

bool AuthSystem::logout(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.erase(session_id) > 0;
}

bool AuthSystem::change_password(const std::string& username,
                                 const std::string& old_password,
                                 const std::string& new_password) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(username);
    if (it == users_.end()) return false;
    if (!verify_password(old_password, it->second.password_hash)) return false;
    if (static_cast<int>(new_password.size()) < password_min_length_) return false;
    it->second.password_hash = hash_password(new_password);
    return true;
}

bool AuthSystem::jwt_available() const {
#if defined(COOLBOX_AUTH_USE_OPENSSL_PROVIDER)
    return true;
#else
    return false;
#endif
}

void AuthSystem::set_jwt_secret(const std::string& secret) {
    std::lock_guard<std::mutex> lock(mutex_);
    jwt_secret_ = secret;
}

std::string AuthSystem::generate_jwt_for_session(const std::string& session_id) const {
#if defined(COOLBOX_AUTH_USE_OPENSSL_PROVIDER)
    std::lock_guard<std::mutex> lock(mutex_);
    auto session_it = sessions_.find(session_id);
    if (session_it == sessions_.end() || is_session_expired(session_it->second)) {
        throw std::runtime_error("Cannot issue JWT for an invalid or expired session");
    }
    auto user_it = users_.find(session_it->second.username);
    if (user_it == users_.end()) {
        throw std::runtime_error("Cannot issue JWT for an unknown user");
    }
    return build_jwt_token(jwt_secret_, jwt_issuer_, jwt_lifetime_, session_it->second, user_it->second);
#else
    (void)session_id;
    throw std::runtime_error("JWT support requires OpenSSL-compatible crypto support");
#endif
}

JwtValidationResult AuthSystem::validate_jwt(const std::string& token) const {
    JwtValidationResult result{false, "Invalid token", {}};

#if defined(COOLBOX_AUTH_USE_OPENSSL_PROVIDER)
    const std::size_t first_dot = token.find('.');
    const std::size_t second_dot = token.find('.', first_dot == std::string::npos ? first_dot : first_dot + 1);
    if (first_dot == std::string::npos || second_dot == std::string::npos || token.find('.', second_dot + 1) != std::string::npos) {
        result.message = "JWT must contain exactly three sections";
        return result;
    }

    try {
        const std::string encoded_header = token.substr(0, first_dot);
        const std::string encoded_payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
        const std::string encoded_signature = token.substr(second_dot + 1);
        const std::string signing_input = encoded_header + "." + encoded_payload;
        const std::string expected_signature = base64url_encode(hmac_sha256(jwt_secret_, signing_input));
        if (expected_signature != encoded_signature) {
            result.message = "JWT signature verification failed";
            return result;
        }

        const std::string header_json = base64url_decode(encoded_header);
        const std::string payload_json = base64url_decode(encoded_payload);
        if (extract_json_string_field(header_json, "alg") != "HS256") {
            result.message = "Unsupported JWT algorithm";
            return result;
        }

        JwtClaims claims;
        claims.subject = extract_json_string_field(payload_json, "sub");
        claims.session_id = extract_json_string_field(payload_json, "sid");
        claims.issuer = extract_json_string_field(payload_json, "iss");
        claims.role = string_to_role(extract_json_string_field(payload_json, "role"));
        claims.issued_at = extract_json_integer_field(payload_json, "iat");
        claims.expires_at = extract_json_integer_field(payload_json, "exp");

        if (claims.subject.empty() || claims.session_id.empty()) {
            result.message = "JWT is missing required claims";
            return result;
        }
        if (claims.issuer != jwt_issuer_) {
            result.message = "JWT issuer mismatch";
            return result;
        }

        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (claims.expires_at <= now) {
            result.message = "JWT has expired";
            return result;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto session_it = sessions_.find(claims.session_id);
        if (session_it == sessions_.end() || is_session_expired(session_it->second)) {
            result.message = "JWT session is no longer valid";
            return result;
        }
        if (session_it->second.username != claims.subject || session_it->second.role != claims.role) {
            result.message = "JWT session claims do not match the active session";
            return result;
        }

        result.valid = true;
        result.message = "JWT is valid";
        result.claims = claims;
        return result;
    } catch (const std::exception& ex) {
        result.message = ex.what();
        return result;
    }
#else
    (void)token;
    result.message = "JWT support requires OpenSSL-compatible crypto support";
    return result;
#endif
}

JwtValidationResult AuthSystem::validate_authorization_header(const std::string& authorization_header) const {
    const std::string token = extract_bearer_token(authorization_header);
    if (token.empty()) {
        return {false, "Authorization header must use the Bearer scheme", {}};
    }
    return validate_jwt(token);
}

Session* AuthSystem::get_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) return nullptr;
    if (is_session_expired(it->second)) {
        sessions_.erase(it);
        return nullptr;
    }
    return &it->second;
}

bool AuthSystem::validate_session(const std::string& session_id) {
    return get_session(session_id) != nullptr;
}

bool AuthSystem::refresh_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) return false;
    if (is_session_expired(it->second)) {
        sessions_.erase(it);
        return false;
    }
    it->second.expires_at = std::chrono::system_clock::now() + std::chrono::seconds(session_timeout_);
    return true;
}

void AuthSystem::cleanup_expired_sessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (is_session_expired(it->second)) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<Session> AuthSystem::get_active_sessions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Session> active;
    const auto now = std::chrono::system_clock::now();
    for (const auto& [_, session] : sessions_) {
        if (session.expires_at > now) {
            active.push_back(session);
        }
    }
    return active;
}

size_t AuthSystem::get_active_sessions_count() const {
    return get_active_sessions().size();
}

PermissionResult AuthSystem::check_permission(const std::string& session_id,
                                              const std::string& resource,
                                              const std::string& action) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || is_session_expired(it->second)) {
        return {false, "Invalid or expired session"};
    }

    if (it->second.role == UserRole::ADMIN) {
        return {true, "Admin access"};
    }

    if (it->second.role == UserRole::GUEST) {
        if (action == "read") {
            return {true, "Guest read access"};
        }
        return {false, "Guest accounts have read-only access"};
    }

    if (action == "read" || action == "write" || action == "update") {
        return {true, "User access granted"};
    }

    if (action == "delete" || action == "admin") {
        return {false, "Insufficient permissions for " + action + " on " + resource};
    }

    return {true, "Access granted"};
}

bool AuthSystem::has_role(const std::string& session_id, UserRole required_role) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || is_session_expired(it->second)) return false;
    return static_cast<int>(it->second.role) <= static_cast<int>(required_role);
}

std::string AuthSystem::hash_password(const std::string& password) const {
    try {
        if (utils::hash::supports_argon2id()) {
            return utils::hash::argon2id_hash(password);
        }
        if (utils::hash::supports_scrypt()) {
            return utils::hash::scrypt_hash(password);
        }
        if (utils::hash::supports_bcrypt()) {
            return utils::hash::bcrypt_hash(password);
        }
        if (utils::hash::supports_pbkdf2_sha256()) {
            return utils::hash::pbkdf2_sha256_hash(password);
        }
    } catch (const std::exception&) {
    }

    return legacy_simple_hash("salt_" + password + "_pepper");
}

bool AuthSystem::verify_password(const std::string& password, const std::string& hash) const {
    try {
        if (hash.rfind("$argon2id$", 0) == 0) {
            return utils::hash::verify_argon2id(password, hash);
        }
        if (hash.rfind("$coolbox-scrypt$", 0) == 0) {
            return utils::hash::verify_scrypt(password, hash);
        }
        if (hash.rfind("$coolbox-pbkdf2-sha256$", 0) == 0) {
            return utils::hash::verify_pbkdf2_sha256(password, hash);
        }
        if (hash.rfind("$2", 0) == 0) {
            return utils::hash::verify_bcrypt(password, hash);
        }
    } catch (const std::exception&) {
        return false;
    }

    if (!is_modern_password_hash(hash)) {
        return legacy_simple_hash("salt_" + password + "_pepper") == hash;
    }
    return false;
}

std::string AuthSystem::generate_session_id() const {
    static std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<std::uint64_t> dist;
    std::ostringstream os;
    os << std::hex << dist(rng) << dist(rng);
    return os.str();
}

bool AuthSystem::is_session_expired(const Session& session) const {
    return std::chrono::system_clock::now() > session.expires_at;
}

void AuthSystem::record_login_attempt(const std::string& username) {
    login_attempts_[username]++;
}

bool AuthSystem::is_account_locked(const std::string& username) const {
    const auto it = login_attempts_.find(username);
    return it != login_attempts_.end() && it->second >= max_login_attempts_;
}

std::string role_to_string(UserRole role) {
    switch (role) {
        case UserRole::ADMIN: return "admin";
        case UserRole::USER: return "user";
        case UserRole::GUEST: return "guest";
    }
    return "unknown";
}

UserRole string_to_role(const std::string& role_str) {
    if (role_str == "admin") return UserRole::ADMIN;
    if (role_str == "user") return UserRole::USER;
    return UserRole::GUEST;
}

} // namespace auth
