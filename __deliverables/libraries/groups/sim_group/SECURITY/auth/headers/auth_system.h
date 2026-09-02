#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace auth {

enum class UserRole {
    ADMIN,
    USER,
    GUEST
};

struct User {
    std::string username;
    std::string password_hash;
    std::string email;
    UserRole role;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_login;
    bool is_active;
    std::map<std::string, std::string> metadata;
};

struct Session {
    std::string session_id;
    std::string username;
    UserRole role;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::string ip_address;
    std::map<std::string, std::string> data;
};

struct AuthResult {
    bool success;
    std::string message;
    std::string session_id;
    std::string jwt_token;
    User user;
};

struct JwtClaims {
    std::string subject;
    std::string session_id;
    UserRole role = UserRole::GUEST;
    std::string issuer;
    std::int64_t issued_at = 0;
    std::int64_t expires_at = 0;
};

struct JwtValidationResult {
    bool valid;
    std::string message;
    JwtClaims claims;
};

struct PermissionResult {
    bool allowed;
    std::string reason;
};

class AuthSystem {
public:
    AuthSystem();
    ~AuthSystem() = default;

    bool create_user(const std::string& username, const std::string& password,
                     const std::string& email, UserRole role = UserRole::USER);
    bool delete_user(const std::string& username);
    bool update_user(const std::string& username, const User& user_data);
    User* get_user(const std::string& username);
    std::vector<std::string> list_users() const;

    AuthResult login(const std::string& username, const std::string& password,
                     const std::string& ip_address = "");
    bool logout(const std::string& session_id);
    bool change_password(const std::string& username, const std::string& old_password,
                         const std::string& new_password);

    bool jwt_available() const;
    void set_jwt_secret(const std::string& secret);
    void set_jwt_issuer(const std::string& issuer) { jwt_issuer_ = issuer; }
    void set_jwt_lifetime(int seconds) { jwt_lifetime_ = seconds; }
    int get_jwt_lifetime() const { return jwt_lifetime_; }
    std::string generate_jwt_for_session(const std::string& session_id) const;
    JwtValidationResult validate_jwt(const std::string& token) const;
    JwtValidationResult validate_authorization_header(const std::string& authorization_header) const;

    Session* get_session(const std::string& session_id);
    bool validate_session(const std::string& session_id);
    bool refresh_session(const std::string& session_id);
    void cleanup_expired_sessions();
    std::vector<Session> get_active_sessions() const;

    PermissionResult check_permission(const std::string& session_id,
                                      const std::string& resource,
                                      const std::string& action);
    bool has_role(const std::string& session_id, UserRole required_role);

    void set_session_timeout(int seconds) { session_timeout_ = seconds; }
    void set_max_login_attempts(int attempts) { max_login_attempts_ = attempts; }
    void set_password_min_length(int length) { password_min_length_ = length; }

    size_t get_total_users() const { return users_.size(); }
    size_t get_active_sessions_count() const;

private:
    std::map<std::string, User> users_;
    std::map<std::string, Session> sessions_;
    std::map<std::string, int> login_attempts_;
    mutable std::mutex mutex_;

    int session_timeout_;
    int max_login_attempts_;
    int password_min_length_;
    int jwt_lifetime_;
    std::string jwt_secret_;
    std::string jwt_issuer_;

    std::string hash_password(const std::string& password) const;
    bool verify_password(const std::string& password, const std::string& hash) const;
    std::string generate_session_id() const;
    bool is_session_expired(const Session& session) const;
    void record_login_attempt(const std::string& username);
    bool is_account_locked(const std::string& username) const;
};

std::string role_to_string(UserRole role);
UserRole string_to_role(const std::string& role_str);

} // namespace auth
