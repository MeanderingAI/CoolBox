#include "services/mail_server/mail_server.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace services::mail_server;

void print_separator(const std::string& title = "") {
    std::cout << "\n" << std::string(60, '=') << "\n";
    if (!title.empty()) {
        std::cout << "  " << title << "\n";
        std::cout << std::string(60, '=') << "\n";
    }
}

void demo_basic_email() {
    print_separator("Basic Email Operations");
    
    // Create a simple email message
    EmailMessage msg;
    msg.message_id = MailUtils::generate_message_id("example.com");
    msg.from = "alice@example.com";
    msg.to.push_back("bob@example.com");
    msg.subject = "Hello, Bob!";
    msg.body = "This is a test email message.\n\nBest regards,\nAlice";
    msg.timestamp = std::chrono::system_clock::now();
    
    std::cout << "Created email:\n";
    std::cout << "  Message-ID: " << msg.message_id << "\n";
    std::cout << "  From: " << msg.from << "\n";
    std::cout << "  To: " << msg.to[0] << "\n";
    std::cout << "  Subject: " << msg.subject << "\n";
    std::cout << "  Body length: " << msg.body.length() << " bytes\n";
    
    // Convert to RFC822 format
    std::string rfc822 = msg.to_rfc822();
    std::cout << "\nRFC822 Format:\n";
    std::cout << rfc822 << "\n";
}

void demo_mailbox() {
    print_separator("Mailbox Operations");
    
    // Create a mailbox
    Mailbox mailbox("alice");
    std::cout << "Created mailbox for user: alice\n";
    
    // Add some messages
    for (int i = 1; i <= 5; ++i) {
        EmailMessage msg;
        msg.message_id = MailUtils::generate_message_id("example.com");
        msg.from = "sender" + std::to_string(i) + "@example.com";
        msg.to.push_back("alice@example.com");
        msg.subject = "Test Email #" + std::to_string(i);
        msg.body = "This is test email number " + std::to_string(i);
        msg.size = msg.to_rfc822().size();
        
        mailbox.add_message(msg);
    }
    
    std::cout << "\nMailbox Statistics:\n";
    std::cout << "  Total messages: " << mailbox.get_message_count() << "\n";
    std::cout << "  Unread messages: " << mailbox.get_unread_count() << "\n";
    std::cout << "  Total size: " << mailbox.get_total_size() << " bytes\n";
    
    // Get all messages
    auto messages = mailbox.get_all_messages();
    std::cout << "\nMessages in mailbox:\n";
    for (const auto& msg : messages) {
        std::cout << "  • " << msg.subject << " from " << msg.from 
                  << " (" << (msg.is_read ? "read" : "unread") << ")\n";
    }
    
    // Mark first message as read
    if (!messages.empty()) {
        mailbox.mark_as_read(messages[0].message_id);
        std::cout << "\nMarked first message as read\n";
        std::cout << "  Unread messages: " << mailbox.get_unread_count() << "\n";
    }
}

void demo_mail_server() {
    print_separator("Full Mail Server");
    
    // Create mail server
    MailServer server(2525, 1110);  // Use non-standard ports
    server.set_domain("example.com");
    
    std::cout << "Created mail server:\n";
    std::cout << "  SMTP Port: 2525\n";
    std::cout << "  POP3 Port: 1110\n";
    std::cout << "  Domain: example.com\n";
    
    // Add users
    server.add_user("alice", "password123");
    server.add_user("bob", "secret456");
    server.add_user("charlie", "pass789");
    
    std::cout << "\nAdded users:\n";
    std::cout << "  • alice@example.com\n";
    std::cout << "  • bob@example.com\n";
    std::cout << "  • charlie@example.com\n";
    
    // Start server
    if (server.start()) {
        std::cout << "\nMail server started successfully!\n";
        
        // Simulate sending emails
        auto alice_mailbox = server.get_mailbox("alice");
        auto bob_mailbox = server.get_mailbox("bob");
        
        if (alice_mailbox && bob_mailbox) {
            // Send email to Alice
            EmailMessage msg1;
            msg1.message_id = MailUtils::generate_message_id("example.com");
            msg1.from = "bob@example.com";
            msg1.to.push_back("alice@example.com");
            msg1.subject = "Meeting Tomorrow";
            msg1.body = "Hi Alice,\n\nDon't forget about our meeting tomorrow at 10 AM.\n\nBob";
            msg1.size = msg1.to_rfc822().size();
            alice_mailbox->add_message(msg1);
            
            // Send email to Bob
            EmailMessage msg2;
            msg2.message_id = MailUtils::generate_message_id("example.com");
            msg2.from = "alice@example.com";
            msg2.to.push_back("bob@example.com");
            msg2.subject = "Re: Meeting Tomorrow";
            msg2.body = "Hi Bob,\n\nThanks for the reminder! I'll be there.\n\nAlice";
            msg2.size = msg2.to_rfc822().size();
            bob_mailbox->add_message(msg2);
            
            std::cout << "\nEmails sent!\n";
        }
        
        // Display server statistics
        std::cout << "\nServer Statistics:\n";
        std::cout << "  Total users: " << server.get_total_users() << "\n";
        std::cout << "  Total messages: " << server.get_total_messages() << "\n";
        std::cout << "  Server running: " << (server.is_running() ? "Yes" : "No") << "\n";
        
        // Check individual mailboxes
        std::cout << "\nMailbox Status:\n";
        for (const std::string& username : {"alice", "bob", "charlie"}) {
            auto mailbox = server.get_mailbox(username);
            if (mailbox) {
                std::cout << "  " << username << ": " << mailbox->get_message_count() 
                         << " messages (" << mailbox->get_unread_count() << " unread)\n";
            }
        }
        
        server.stop();
        std::cout << "\nMail server stopped.\n";
    } else {
        std::cout << "\nFailed to start mail server!\n";
    }
}

void demo_email_utils() {
    print_separator("Email Utilities");
    
    // Email validation
    std::vector<std::string> test_emails = {
        "user@example.com",
        "invalid.email",
        "no@domain",
        "@nodomain.com",
        "valid.user+tag@subdomain.example.com"
    };
    
    std::cout << "Email validation:\n";
    for (const auto& email : test_emails) {
        bool valid = MailUtils::is_valid_email(email);
        std::cout << "  " << email << " -> " << (valid ? "VALID" : "INVALID") << "\n";
    }
    
    // Email address formatting
    std::cout << "\nEmail address formatting:\n";
    std::string formatted = MailUtils::format_email_address("John Doe", "john@example.com");
    std::cout << "  Formatted: " << formatted << "\n";
    std::string parsed = MailUtils::parse_email_address(formatted);
    std::cout << "  Parsed: " << parsed << "\n";
    
    // Base64 encoding
    std::cout << "\nBase64 encoding:\n";
    std::string original = "Hello, World!";
    std::vector<char> data(original.begin(), original.end());
    std::string encoded = MailUtils::encode_base64(data);
    std::vector<char> decoded = MailUtils::decode_base64(encoded);
    std::string decoded_str(decoded.begin(), decoded.end());
    
    std::cout << "  Original: " << original << "\n";
    std::cout << "  Encoded: " << encoded << "\n";
    std::cout << "  Decoded: " << decoded_str << "\n";
    std::cout << "  Match: " << (original == decoded_str ? "YES" : "NO") << "\n";
    
    // Date formatting
    std::cout << "\nDate formatting (RFC822):\n";
    auto now = std::chrono::system_clock::now();
    std::string date = MailUtils::format_date_rfc822(now);
    std::cout << "  Current time: " << date << "\n";
}

void demo_smtp_client() {
    print_separator("SMTP Client");
    
    // Create mail client
    MailClient client("localhost", 2525);
    client.set_credentials("alice", "password123");
    
    std::cout << "Created SMTP client:\n";
    std::cout << "  Server: localhost:2525\n";
    std::cout << "  Username: alice\n";
    
    // Connect
    if (client.connect()) {
        std::cout << "\nConnected to SMTP server\n";
        
        // Send email
        bool sent = client.send_email(
            "alice@example.com",
            "bob@example.com",
            "Test from Client",
            "This is a test email sent using the SMTP client."
        );
        
        if (sent) {
            std::cout << "Email sent successfully!\n";
        } else {
            std::cout << "Failed to send email: " << client.get_last_error() << "\n";
        }
        
        client.disconnect();
        std::cout << "Disconnected from server\n";
    } else {
        std::cout << "\nFailed to connect to SMTP server\n";
    }
}

void demo_message_threading() {
    print_separator("Message Threading & Conversations");
    
    Mailbox mailbox("user");
    
    // Create a conversation thread
    std::string thread_id = "thread-001";
    
    // Original message
    EmailMessage msg1;
    msg1.message_id = MailUtils::generate_message_id("example.com");
    msg1.from = "alice@example.com";
    msg1.to.push_back("user@example.com");
    msg1.subject = "Project Discussion";
    msg1.body = "Let's discuss the new project timeline.";
    msg1.headers["Thread-ID"] = thread_id;
    msg1.size = msg1.to_rfc822().size();
    mailbox.add_message(msg1);
    
    // Reply 1
    EmailMessage msg2;
    msg2.message_id = MailUtils::generate_message_id("example.com");
    msg2.from = "bob@example.com";
    msg2.to.push_back("user@example.com");
    msg2.subject = "Re: Project Discussion";
    msg2.body = "I think we should extend the deadline by a week.";
    msg2.headers["Thread-ID"] = thread_id;
    msg2.headers["In-Reply-To"] = msg1.message_id;
    msg2.size = msg2.to_rfc822().size();
    mailbox.add_message(msg2);
    
    // Reply 2
    EmailMessage msg3;
    msg3.message_id = MailUtils::generate_message_id("example.com");
    msg3.from = "alice@example.com";
    msg3.to.push_back("user@example.com");
    msg3.subject = "Re: Project Discussion";
    msg3.body = "Agreed. I'll update the schedule.";
    msg3.headers["Thread-ID"] = thread_id;
    msg3.headers["In-Reply-To"] = msg2.message_id;
    msg3.size = msg3.to_rfc822().size();
    mailbox.add_message(msg3);
    
    std::cout << "Created conversation thread with 3 messages:\n";
    auto messages = mailbox.get_all_messages();
    for (size_t i = 0; i < messages.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << messages[i].subject 
                  << " from " << messages[i].from << "\n";
        if (messages[i].headers.count("In-Reply-To")) {
            std::cout << "     (Reply to previous message)\n";
        }
    }
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║           Mail Server Demo - ToolBox Library            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    try {
        demo_basic_email();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demo_mailbox();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demo_email_utils();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demo_mail_server();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demo_smtp_client();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        demo_message_threading();
        
        print_separator();
        std::cout << "\n✓ All demos completed successfully!\n\n";
        
        std::cout << "Mail Server Features:\n";
        std::cout << "  • SMTP server for sending emails\n";
        std::cout << "  • POP3 server for retrieving emails\n";
        std::cout << "  • Mailbox management with read/unread tracking\n";
        std::cout << "  • RFC822 email format support\n";
        std::cout << "  • Base64 encoding/decoding\n";
        std::cout << "  • Email validation and parsing\n";
        std::cout << "  • Multi-user support with authentication\n";
        std::cout << "  • Message threading and conversations\n";
        std::cout << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
