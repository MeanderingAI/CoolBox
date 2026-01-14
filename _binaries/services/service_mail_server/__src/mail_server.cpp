#include "mail_server.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace services {
namespace mail_server {

// EmailMessage implementation
std::string EmailMessage::to_rfc822() const {
    std::stringstream ss;
    
    // Standard headers
    ss << "Message-ID: " << message_id << "\r\n";
    ss << "From: " << from << "\r\n";
    
    if (!to.empty()) {
        ss << "To: ";
        for (size_t i = 0; i < to.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << to[i];
        }
        ss << "\r\n";
    }
    
    if (!cc.empty()) {
        ss << "Cc: ";
        for (size_t i = 0; i < cc.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << cc[i];
        }
        ss << "\r\n";
    }
    
    ss << "Subject: " << subject << "\r\n";
    ss << "Date: " << MailUtils::format_date_rfc822(timestamp) << "\r\n";
    
    // Additional headers
    for (const auto& [key, value] : headers) {
        ss << key << ": " << value << "\r\n";
    }
    
    // Empty line before body
    ss << "\r\n";
    ss << body;
    
    return ss.str();
}

EmailMessage EmailMessage::from_rfc822(const std::string& rfc822_data) {
    EmailMessage msg;
    std::istringstream stream(rfc822_data);
    std::string line;
    bool in_body = false;
    std::stringstream body_stream;
    
    while (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (in_body) {
            body_stream << line << "\n";
            continue;
        }
        
        if (line.empty()) {
            in_body = true;
            continue;
        }
        
        // Parse header
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            
            if (key == "From") {
                msg.from = value;
            } else if (key == "To") {
                msg.to.push_back(value);
            } else if (key == "Subject") {
                msg.subject = value;
            } else if (key == "Message-ID") {
                msg.message_id = value;
            } else {
                msg.headers[key] = value;
            }
        }
    }
    
    msg.body = body_stream.str();
    msg.size = rfc822_data.size();
    
    return msg;
}

// Mailbox implementation
Mailbox::Mailbox(const std::string& username) : username_(username) {}

bool Mailbox::add_message(const EmailMessage& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_[message.message_id] = message;
    return true;
}

bool Mailbox::delete_message(const std::string& message_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = messages_.find(message_id);
    if (it != messages_.end()) {
        it->second.is_deleted = true;
        return true;
    }
    return false;
}

bool Mailbox::mark_as_read(const std::string& message_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = messages_.find(message_id);
    if (it != messages_.end()) {
        it->second.is_read = true;
        return true;
    }
    return false;
}

bool Mailbox::mark_as_unread(const std::string& message_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = messages_.find(message_id);
    if (it != messages_.end()) {
        it->second.is_read = false;
        return true;
    }
    return false;
}

std::vector<EmailMessage> Mailbox::get_all_messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<EmailMessage> result;
    for (const auto& [id, msg] : messages_) {
        if (!msg.is_deleted) {
            result.push_back(msg);
        }
    }
    return result;
}

std::vector<EmailMessage> Mailbox::get_unread_messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<EmailMessage> result;
    for (const auto& [id, msg] : messages_) {
        if (!msg.is_deleted && !msg.is_read) {
            result.push_back(msg);
        }
    }
    return result;
}

EmailMessage* Mailbox::get_message(const std::string& message_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = messages_.find(message_id);
    if (it != messages_.end() && !it->second.is_deleted) {
        return &it->second;
    }
    return nullptr;
}

size_t Mailbox::get_message_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, msg] : messages_) {
        if (!msg.is_deleted) {
            count++;
        }
    }
    return count;
}

size_t Mailbox::get_unread_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, msg] : messages_) {
        if (!msg.is_deleted && !msg.is_read) {
            count++;
        }
    }
    return count;
}

size_t Mailbox::get_total_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& [id, msg] : messages_) {
        if (!msg.is_deleted) {
            total += msg.size;
        }
    }
    return total;
}

void Mailbox::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
}

// SMTPServer implementation
SMTPServer::SMTPServer(int port, const std::string& domain)
    : port_(port), domain_(domain), running_(false), 
      max_message_size_(10 * 1024 * 1024), require_auth_(false),
      messages_sent_(0), messages_received_(0), active_connections_(0) {}

SMTPServer::~SMTPServer() {
    stop();
}

bool SMTPServer::start() {
    if (running_) return true;
    running_ = true;
    return true;
}

void SMTPServer::stop() {
    running_ = false;
}

void SMTPServer::add_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    users_[username] = password;
}

void SMTPServer::remove_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    users_.erase(username);
}

void SMTPServer::set_message_handler(std::function<void(const EmailMessage&)> handler) {
    message_handler_ = handler;
}

void SMTPServer::set_relay_handler(std::function<bool(const std::string&)> handler) {
    relay_handler_ = handler;
}

bool SMTPServer::authenticate_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    auto it = users_.find(username);
    return it != users_.end() && it->second == password;
}

// POP3Server implementation
POP3Server::POP3Server(int port)
    : port_(port), running_(false), active_connections_(0) {}

POP3Server::~POP3Server() {
    stop();
}

bool POP3Server::start() {
    if (running_) return true;
    running_ = true;
    return true;
}

void POP3Server::stop() {
    running_ = false;
}

void POP3Server::add_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    users_[username] = password;
}

void POP3Server::remove_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    users_.erase(username);
}

bool POP3Server::authenticate(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    auto it = users_.find(username);
    return it != users_.end() && it->second == password;
}

void POP3Server::add_mailbox(const std::string& username, std::shared_ptr<Mailbox> mailbox) {
    std::lock_guard<std::mutex> lock(mailboxes_mutex_);
    mailboxes_[username] = mailbox;
}

std::shared_ptr<Mailbox> POP3Server::get_mailbox(const std::string& username) {
    std::lock_guard<std::mutex> lock(mailboxes_mutex_);
    auto it = mailboxes_.find(username);
    return it != mailboxes_.end() ? it->second : nullptr;
}

// IMAPServer implementation
IMAPServer::IMAPServer(int port)
    : port_(port), running_(false), active_connections_(0) {}

IMAPServer::~IMAPServer() {
    stop();
}

bool IMAPServer::start() {
    if (running_) return true;
    running_ = true;
    return true;
}

void IMAPServer::stop() {
    running_ = false;
}

void IMAPServer::add_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    users_[username] = password;
}

void IMAPServer::remove_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    users_.erase(username);
}

void IMAPServer::add_mailbox(const std::string& username, std::shared_ptr<Mailbox> mailbox) {
    std::lock_guard<std::mutex> lock(mutex_);
    mailboxes_[username] = mailbox;
}

// MailClient implementation
MailClient::MailClient(const std::string& smtp_server, int smtp_port)
    : smtp_server_(smtp_server), smtp_port_(smtp_port), 
      connected_(false), socket_fd_(-1) {}

bool MailClient::connect() {
    connected_ = true;
    return true;
}

void MailClient::disconnect() {
    connected_ = false;
}

void MailClient::set_credentials(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
}

bool MailClient::send_email(const EmailMessage& message) {
    if (!connected_) {
        last_error_ = "Not connected";
        return false;
    }
    
    // In a real implementation, this would send via SMTP
    return true;
}

bool MailClient::send_email(const std::string& from, const std::string& to,
                           const std::string& subject, const std::string& body) {
    EmailMessage msg;
    msg.from = from;
    msg.to.push_back(to);
    msg.subject = subject;
    msg.body = body;
    msg.message_id = MailUtils::generate_message_id(smtp_server_);
    
    return send_email(msg);
}

// MailServer implementation
MailServer::MailServer(int smtp_port, int pop3_port)
    : smtp_server_(std::make_unique<SMTPServer>(smtp_port)),
      pop3_server_(std::make_unique<POP3Server>(pop3_port)),
      allow_relay_(false) {
    
    // Set up message handler
    smtp_server_->set_message_handler([this](const EmailMessage& msg) {
        on_message_received(msg);
    });
    
    smtp_server_->set_relay_handler([this](const std::string& recipient) {
        return should_relay(recipient);
    });
}

MailServer::~MailServer() {
    stop();
}

bool MailServer::start() {
    bool smtp_started = smtp_server_->start();
    bool pop3_started = pop3_server_->start();
    return smtp_started && pop3_started;
}

void MailServer::stop() {
    smtp_server_->stop();
    pop3_server_->stop();
}

bool MailServer::is_running() const {
    return smtp_server_->is_running() && pop3_server_->is_running();
}

void MailServer::add_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    users_[username] = password;
    
    // Create mailbox
    auto mailbox = std::make_shared<Mailbox>(username);
    mailboxes_[username] = mailbox;
    
    // Add to servers
    smtp_server_->add_user(username, password);
    pop3_server_->add_user(username, password);
    pop3_server_->add_mailbox(username, mailbox);
}

void MailServer::remove_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    users_.erase(username);
    mailboxes_.erase(username);
    
    smtp_server_->remove_user(username);
    pop3_server_->remove_user(username);
}

std::shared_ptr<Mailbox> MailServer::get_mailbox(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = mailboxes_.find(username);
    return it != mailboxes_.end() ? it->second : nullptr;
}

void MailServer::set_domain(const std::string& domain) {
    smtp_server_->set_domain(domain);
}

void MailServer::add_relay_domain(const std::string& domain) {
    relay_domains_.insert(domain);
}

size_t MailServer::get_total_users() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return users_.size();
}

size_t MailServer::get_total_messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& [username, mailbox] : mailboxes_) {
        total += mailbox->get_message_count();
    }
    return total;
}

size_t MailServer::get_messages_sent() const {
    return smtp_server_->get_messages_sent();
}

size_t MailServer::get_messages_received() const {
    return smtp_server_->get_messages_received();
}

void MailServer::on_message_received(const EmailMessage& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Deliver to local mailboxes
    for (const auto& recipient : message.to) {
        std::string username = MailUtils::parse_email_address(recipient);
        auto it = mailboxes_.find(username);
        if (it != mailboxes_.end()) {
            it->second->add_message(message);
        }
    }
}

bool MailServer::should_relay(const std::string& recipient) {
    if (!allow_relay_) return false;
    
    std::string domain = recipient.substr(recipient.find('@') + 1);
    return relay_domains_.find(domain) != relay_domains_.end();
}

// MailUtils implementation
std::string MailUtils::generate_message_id(const std::string& domain) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "<";
    
    // Generate random hex string
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << dis(gen);
    }
    
    ss << "@" << domain << ">";
    return ss.str();
}

std::string MailUtils::encode_base64(const std::vector<char>& data) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string encoded;
    int val = 0;
    int valb = -6;
    
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    
    return encoded;
}

std::vector<char> MailUtils::decode_base64(const std::string& encoded) {
    static const int decode_table[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
    };
    
    std::vector<char> decoded;
    int val = 0;
    int valb = -8;
    
    for (unsigned char c : encoded) {
        if (decode_table[c] == -1) break;
        val = (val << 6) + decode_table[c];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return decoded;
}

std::string MailUtils::format_email_address(const std::string& name, const std::string& email) {
    if (name.empty()) {
        return email;
    }
    return "\"" + name + "\" <" + email + ">";
}

std::string MailUtils::parse_email_address(const std::string& formatted) {
    size_t start = formatted.find('<');
    size_t end = formatted.find('>');
    
    if (start != std::string::npos && end != std::string::npos) {
        return formatted.substr(start + 1, end - start - 1);
    }
    
    return formatted;
}

std::string MailUtils::format_date_rfc822(const std::chrono::system_clock::time_point& time) {
    time_t tt = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::gmtime(&tt);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S +0000");
    return ss.str();
}

std::chrono::system_clock::time_point MailUtils::parse_date_rfc822(const std::string& date) {
    // Simplified parsing
    return std::chrono::system_clock::now();
}

bool MailUtils::is_valid_email(const std::string& email) {
    size_t at_pos = email.find('@');
    if (at_pos == std::string::npos || at_pos == 0 || at_pos == email.size() - 1) {
        return false;
    }
    
    size_t dot_pos = email.find('.', at_pos);
    return dot_pos != std::string::npos && dot_pos < email.size() - 1;
}

std::string MailUtils::sanitize_header(const std::string& header) {
    std::string sanitized = header;
    // Remove any CRLF
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\r'), sanitized.end());
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\n'), sanitized.end());
    return sanitized;
}

} // namespace mail_server
} // namespace services
