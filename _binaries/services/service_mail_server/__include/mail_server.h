#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <atomic>
#include <set>

namespace services {
namespace mail_server {

// Email message structure
struct EmailMessage {
    std::string message_id;
    std::string from;
    std::vector<std::string> to;
    std::vector<std::string> cc;
    std::vector<std::string> bcc;
    std::string subject;
    std::string body;
    std::map<std::string, std::string> headers;
    std::chrono::system_clock::time_point timestamp;
    size_t size;
    bool is_read;
    bool is_deleted;
    
    EmailMessage() : size(0), is_read(false), is_deleted(false) {
        timestamp = std::chrono::system_clock::now();
    }
    
    std::string to_rfc822() const;
    static EmailMessage from_rfc822(const std::string& rfc822_data);
};

// Email attachment
struct EmailAttachment {
    std::string filename;
    std::string content_type;
    std::vector<char> data;
    std::string encoding;
    
    EmailAttachment() : encoding("base64") {}
};

// Mailbox for storing user emails
class Mailbox {
public:
    Mailbox(const std::string& username);
    
    bool add_message(const EmailMessage& message);
    bool delete_message(const std::string& message_id);
    bool mark_as_read(const std::string& message_id);
    bool mark_as_unread(const std::string& message_id);
    
    std::vector<EmailMessage> get_all_messages() const;
    std::vector<EmailMessage> get_unread_messages() const;
    EmailMessage* get_message(const std::string& message_id);
    
    size_t get_message_count() const;
    size_t get_unread_count() const;
    size_t get_total_size() const;
    
    void clear();
    
private:
    std::string username_;
    std::map<std::string, EmailMessage> messages_;
    mutable std::mutex mutex_;
};

// SMTP Server - handles sending emails
class SMTPServer {
public:
    SMTPServer(int port = 25, const std::string& domain = "localhost");
    ~SMTPServer();
    
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // Configuration
    void set_domain(const std::string& domain) { domain_ = domain; }
    void set_max_message_size(size_t size) { max_message_size_ = size; }
    void enable_authentication(bool enable) { require_auth_ = enable; }
    void add_user(const std::string& username, const std::string& password);
    void remove_user(const std::string& username);
    
    // Callbacks
    void set_message_handler(std::function<void(const EmailMessage&)> handler);
    void set_relay_handler(std::function<bool(const std::string&)> handler);
    
    // Statistics
    size_t get_messages_sent() const { return messages_sent_; }
    size_t get_messages_received() const { return messages_received_; }
    size_t get_active_connections() const { return active_connections_; }
    
private:
    int port_;
    std::string domain_;
    std::atomic<bool> running_;
    size_t max_message_size_;
    bool require_auth_;
    
    // User authentication
    std::map<std::string, std::string> users_;
    mutable std::mutex users_mutex_;
    
    // Statistics
    std::atomic<size_t> messages_sent_;
    std::atomic<size_t> messages_received_;
    std::atomic<size_t> active_connections_;
    
    // Callbacks
    std::function<void(const EmailMessage&)> message_handler_;
    std::function<bool(const std::string&)> relay_handler_;
    
    // Server logic
    void accept_connections();
    void handle_client(int client_socket);
    bool authenticate_user(const std::string& username, const std::string& password);
    bool process_smtp_command(const std::string& command, std::string& response);
};

// POP3 Server - handles retrieving emails
class POP3Server {
public:
    POP3Server(int port = 110);
    ~POP3Server();
    
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // User management
    void add_user(const std::string& username, const std::string& password);
    void remove_user(const std::string& username);
    bool authenticate(const std::string& username, const std::string& password);
    
    // Mailbox access
    void add_mailbox(const std::string& username, std::shared_ptr<Mailbox> mailbox);
    std::shared_ptr<Mailbox> get_mailbox(const std::string& username);
    
    // Statistics
    size_t get_active_connections() const { return active_connections_; }
    
private:
    int port_;
    std::atomic<bool> running_;
    
    // User authentication
    std::map<std::string, std::string> users_;
    mutable std::mutex users_mutex_;
    
    // Mailboxes
    std::map<std::string, std::shared_ptr<Mailbox>> mailboxes_;
    mutable std::mutex mailboxes_mutex_;
    
    // Statistics
    std::atomic<size_t> active_connections_;
    
    // Server logic
    void accept_connections();
    void handle_client(int client_socket);
    bool process_pop3_command(const std::string& command, 
                              const std::string& username,
                              Mailbox* mailbox,
                              std::string& response);
};

// IMAP Server (simplified) - more advanced email retrieval
class IMAPServer {
public:
    IMAPServer(int port = 143);
    ~IMAPServer();
    
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // User management
    void add_user(const std::string& username, const std::string& password);
    void remove_user(const std::string& username);
    
    // Mailbox access
    void add_mailbox(const std::string& username, std::shared_ptr<Mailbox> mailbox);
    
private:
    int port_;
    std::atomic<bool> running_;
    
    std::map<std::string, std::string> users_;
    std::map<std::string, std::shared_ptr<Mailbox>> mailboxes_;
    mutable std::mutex mutex_;
    
    std::atomic<size_t> active_connections_;
    
    void accept_connections();
    void handle_client(int client_socket);
};

// Mail client for sending emails
class MailClient {
public:
    MailClient(const std::string& smtp_server, int smtp_port = 25);
    
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_; }
    
    // Authentication
    void set_credentials(const std::string& username, const std::string& password);
    
    // Send email
    bool send_email(const EmailMessage& message);
    bool send_email(const std::string& from, const std::string& to,
                   const std::string& subject, const std::string& body);
    
    // Get last error
    std::string get_last_error() const { return last_error_; }
    
private:
    std::string smtp_server_;
    int smtp_port_;
    std::string username_;
    std::string password_;
    bool connected_;
    std::string last_error_;
    
    int socket_fd_;
    
    bool send_command(const std::string& command, std::string& response);
    bool read_response(std::string& response);
};

// Complete mail server combining SMTP and POP3
class MailServer {
public:
    MailServer(int smtp_port = 25, int pop3_port = 110);
    ~MailServer();
    
    bool start();
    void stop();
    bool is_running() const;
    
    // User management
    void add_user(const std::string& username, const std::string& password);
    void remove_user(const std::string& username);
    
    // Access mailboxes
    std::shared_ptr<Mailbox> get_mailbox(const std::string& username);
    
    // Configuration
    void set_domain(const std::string& domain);
    void enable_relay(bool enable) { allow_relay_ = enable; }
    void add_relay_domain(const std::string& domain);
    
    // Statistics
    size_t get_total_users() const;
    size_t get_total_messages() const;
    size_t get_messages_sent() const;
    size_t get_messages_received() const;
    
private:
    std::unique_ptr<SMTPServer> smtp_server_;
    std::unique_ptr<POP3Server> pop3_server_;
    
    // Shared user database
    std::map<std::string, std::string> users_;
    std::map<std::string, std::shared_ptr<Mailbox>> mailboxes_;
    mutable std::mutex mutex_;
    
    bool allow_relay_;
    std::set<std::string> relay_domains_;
    
    void on_message_received(const EmailMessage& message);
    bool should_relay(const std::string& recipient);
};

// Utility functions
class MailUtils {
public:
    static std::string generate_message_id(const std::string& domain);
    static std::string encode_base64(const std::vector<char>& data);
    static std::vector<char> decode_base64(const std::string& encoded);
    static std::string format_email_address(const std::string& name, const std::string& email);
    static std::string parse_email_address(const std::string& formatted);
    static std::string format_date_rfc822(const std::chrono::system_clock::time_point& time);
    static std::chrono::system_clock::time_point parse_date_rfc822(const std::string& date);
    static bool is_valid_email(const std::string& email);
    static std::string sanitize_header(const std::string& header);
};

} // namespace mail_server
} // namespace services
