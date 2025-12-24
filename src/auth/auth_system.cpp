#include "auth/auth_system.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <algorithm>
#include <cstring>

namespace auth {

std::string role_to_string(UserRole role) {
    switch (role) {
        case UserRole::ADMIN: return "admin";
        case UserRole::USER: return "user";
        case UserRole::GUEST: return "guest";
        default: return "unknown";
    }
}

UserRole string_to_role(const std::string& role_str) {
    if (role_str == "admin") return UserRole::ADMIN;
    if (role_str == "user") return UserRole::USER;
    if (role_str == "guest") return UserRole::GUEST;
    return UserRole::GUEST;
}

AuthSystem::AuthSystem()
    : session_timeout_(3600),  // 1 hour
      max_login_attempts_(5),
      password_min_length_(8) {
    
    // Create default admin user
    create_user("admin", "admin123", "admin@localhost", UserRole::ADMIN);
    create_user("user", "user123", "user@localhost", UserRole::USER);
}

std::string AuthSystem::hash_password(const std::string& password) const {
    // Simple hash for demo (use proper bcrypt/scrypt in production)
    std::hash<std::string> hasher;
    size_t hash = hasher(password + "salt_value_12345");
    
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

bool AuthSystem::verify_password(const std::string& password, const std::string& hash) const {
    return hash_password(password) == hash;
}

std::string AuthSystem::generate_session_id() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "sess_";
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);
    }
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    ss << "_" << std::hex << timestamp;
    
    return ss.str();
}

bool AuthSystem::create_user(const std::string& username, const std::string& password,
                             const std::string& email, UserRole role) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (users_.find(username) != users_.end()) {
        return false;  // User already exists
    }
    
    if (password.length() < static_cast<size_t>(password_min_length_)) {
        return false;  // Password too short
    }
    
    User user;
    user.username = username;
    user.password_hash = hash_password(password);
    user.email = email;
    user.role = role;
    user.created_at = std::chrono::system_clock::now();
    user.is_active = true;
    
    users_[username] = user;
    return true;
}

bool AuthSystem::delete_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    return users_.erase(username) > 0;
}

User* AuthSystem::get_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(username);
    if (it != users_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> AuthSystem::list_users() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> usernames;
    for (const auto& [username, user] : users_) {
        usernames.push_back(username);
    }
    return usernames;
}

AuthResult AuthSystem::login(const std::string& username, const std::string& password,
                             const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AuthResult result;
    result.success = false;
    
    std::cout << "[AuthSystem] Login request for username: '" << username << "'\n";
    std::cout << "[AuthSystem] Password length: " << password.length() << "\n";
    
    // Check if account is locked
    if (is_account_locked(username)) {
        result.message = "Account is locked due to too many failed login attempts";
        std::cout << "[AuthSystem] Account locked\n";
        return result;
    }
    
    // Find user
    auto it = users_.find(username);
    if (it == users_.end()) {
        result.message = "Invalid username or password";
        std::cout << "[AuthSystem] User not found\n";
        record_login_attempt(username);
        return result;
    }
    
    User& user = it->second;
    std::cout << "[AuthSystem] User found: " << user.username << "\n";
    std::cout << "[AuthSystem] User is active: " << user.is_active << "\n";
    
    // Check if user is active
    if (!user.is_active) {
        result.message = "Account is disabled";
        std::cout << "[AuthSystem] Account disabled\n";
        return result;
    }
    
    // Verify password
    std::cout << "[AuthSystem] Verifying password...\n";
    std::cout << "[AuthSystem] Input password hash: " << hash_password(password) << "\n";
    std::cout << "[AuthSystem] Stored password hash: " << user.password_hash << "\n";
    
    if (!verify_password(password, user.password_hash)) {
        result.message = "Invalid username or password";
        std::cout << "[AuthSystem] Password verification failed\n";
        record_login_attempt(username);
        return result;
    }
    
    std::cout << "[AuthSystem] Password verified successfully\n";
    
    // Create session
    Session session;
    session.session_id = generate_session_id();
    session.username = username;
    session.role = user.role;
    session.created_at = std::chrono::system_clock::now();
    session.expires_at = session.created_at + std::chrono::seconds(session_timeout_);
    session.ip_address = ip_address;
    
    sessions_[session.session_id] = session;
    
    // Update user last login
    user.last_login = std::chrono::system_clock::now();
    
    // Clear login attempts
    login_attempts_.erase(username);
    
    result.success = true;
    result.message = "Login successful";
    result.session_id = session.session_id;
    result.user = user;
    
    std::cout << "[AuthSystem] Login successful, session created\n";
    
    return result;
}

bool AuthSystem::logout(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.erase(session_id) > 0;
}

bool AuthSystem::change_password(const std::string& username, const std::string& old_password,
                                const std::string& new_password) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        return false;
    }
    
    User& user = it->second;
    
    if (!verify_password(old_password, user.password_hash)) {
        return false;
    }
    
    if (new_password.length() < static_cast<size_t>(password_min_length_)) {
        return false;
    }
    
    user.password_hash = hash_password(new_password);
    return true;
}

Session* AuthSystem::get_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool AuthSystem::validate_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }
    
    if (is_session_expired(it->second)) {
        sessions_.erase(it);
        return false;
    }
    
    return true;
}

bool AuthSystem::refresh_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }
    
    it->second.expires_at = std::chrono::system_clock::now() + std::chrono::seconds(session_timeout_);
    return true;
}

void AuthSystem::cleanup_expired_sessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sessions_.begin();
    while (it != sessions_.end()) {
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
    for (const auto& [id, session] : sessions_) {
        if (!is_session_expired(session)) {
            active.push_back(session);
        }
    }
    return active;
}

PermissionResult AuthSystem::check_permission(const std::string& session_id,
                                             const std::string& resource,
                                             const std::string& action) {
    PermissionResult result;
    result.allowed = false;
    
    if (!validate_session(session_id)) {
        result.reason = "Invalid or expired session";
        return result;
    }
    
    auto* session = get_session(session_id);
    if (!session) {
        result.reason = "Session not found";
        return result;
    }
    
    // Admin has all permissions
    if (session->role == UserRole::ADMIN) {
        result.allowed = true;
        return result;
    }
    
    // Regular users can read most resources
    if (action == "read" && session->role == UserRole::USER) {
        result.allowed = true;
        return result;
    }
    
    // Users can write to their own resources
    if (action == "write" && resource.find(session->username) != std::string::npos) {
        result.allowed = true;
        return result;
    }
    
    result.reason = "Insufficient permissions";
    return result;
}

bool AuthSystem::has_role(const std::string& session_id, UserRole required_role) {
    auto* session = get_session(session_id);
    if (!session) return false;
    
    // Admin has all roles
    if (session->role == UserRole::ADMIN) return true;
    
    return session->role == required_role;
}

size_t AuthSystem::get_active_sessions_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (const auto& [id, session] : sessions_) {
        if (!is_session_expired(session)) {
            count++;
        }
    }
    return count;
}

bool AuthSystem::is_session_expired(const Session& session) const {
    return std::chrono::system_clock::now() > session.expires_at;
}

void AuthSystem::record_login_attempt(const std::string& username) {
    login_attempts_[username]++;
}

bool AuthSystem::is_account_locked(const std::string& username) const {
    auto it = login_attempts_.find(username);
    if (it == login_attempts_.end()) return false;
    return it->second >= max_login_attempts_;
}

} // namespace auth
