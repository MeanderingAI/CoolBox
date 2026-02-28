#include "../headers/auth_system.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>
#include <functional>

namespace auth {

// Simple hash (NOT cryptographic – for demo/educational purposes only)
static std::string simple_hash(const std::string& input) {
    std::hash<std::string> hasher;
    size_t h = hasher(input);
    // Mix bits
    h ^= (h >> 16);
    h *= 0x85ebca6b;
    h ^= (h >> 13);
    h *= 0xc2b2ae35;
    h ^= (h >> 16);

    std::ostringstream os;
    os << std::hex << std::setfill('0') << std::setw(16) << h;
    return os.str();
}

AuthSystem::AuthSystem()
    : session_timeout_(3600)
    , max_login_attempts_(5)
    , password_min_length_(8)
{}

// ── User management ────────────────────────────────────────────────

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
    for (auto& [k, _] : users_) names.push_back(k);
    return names;
}

// ── Authentication ─────────────────────────────────────────────────

AuthResult AuthSystem::login(const std::string& username, const std::string& password,
                             const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(mutex_);
    AuthResult result{false, "", "", {}};

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

    // Success
    login_attempts_.erase(username);
    auto now = std::chrono::system_clock::now();
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

// ── Session management ─────────────────────────────────────────────

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
    it->second.expires_at = std::chrono::system_clock::now()
                            + std::chrono::seconds(session_timeout_);
    return true;
}

void AuthSystem::cleanup_expired_sessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (is_session_expired(it->second))
            it = sessions_.erase(it);
        else
            ++it;
    }
}

std::vector<Session> AuthSystem::get_active_sessions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Session> active;
    auto now = std::chrono::system_clock::now();
    for (auto& [_, s] : sessions_)
        if (s.expires_at > now)
            active.push_back(s);
    return active;
}

size_t AuthSystem::get_active_sessions_count() const {
    return get_active_sessions().size();
}

// ── Permissions ────────────────────────────────────────────────────

PermissionResult AuthSystem::check_permission(const std::string& session_id,
                                              const std::string& resource,
                                              const std::string& action) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || is_session_expired(it->second))
        return {false, "Invalid or expired session"};

    // Admin can do anything
    if (it->second.role == UserRole::ADMIN)
        return {true, "Admin access"};

    // Guests can only read
    if (it->second.role == UserRole::GUEST) {
        if (action == "read")
            return {true, "Guest read access"};
        return {false, "Guest accounts have read-only access"};
    }

    // Regular users: read + write on most resources
    if (action == "read" || action == "write" || action == "update")
        return {true, "User access granted"};

    if (action == "delete" || action == "admin")
        return {false, "Insufficient permissions for " + action + " on " + resource};

    return {true, "Access granted"};
}

bool AuthSystem::has_role(const std::string& session_id, UserRole required_role) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || is_session_expired(it->second)) return false;
    // ADMIN >= USER >= GUEST
    return static_cast<int>(it->second.role) <= static_cast<int>(required_role);
}

// ── Private helpers ────────────────────────────────────────────────

std::string AuthSystem::hash_password(const std::string& password) const {
    // Simple salted hash (NOT production-grade)
    return simple_hash("salt_" + password + "_pepper");
}

bool AuthSystem::verify_password(const std::string& password, const std::string& hash) const {
    return hash_password(password) == hash;
}

std::string AuthSystem::generate_session_id() const {
    static std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<uint64_t> dist;
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
    auto it = login_attempts_.find(username);
    return it != login_attempts_.end() && it->second >= max_login_attempts_;
}

// ── Free functions ─────────────────────────────────────────────────

std::string role_to_string(UserRole role) {
    switch (role) {
        case UserRole::ADMIN: return "admin";
        case UserRole::USER:  return "user";
        case UserRole::GUEST: return "guest";
    }
    return "unknown";
}

UserRole string_to_role(const std::string& role_str) {
    if (role_str == "admin") return UserRole::ADMIN;
    if (role_str == "user")  return UserRole::USER;
    return UserRole::GUEST;
}

} // namespace auth
