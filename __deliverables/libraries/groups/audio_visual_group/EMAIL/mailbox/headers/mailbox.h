#pragma once

#include <map>
#include <string>
#include <vector>

namespace trekker {
namespace email {

struct EmailMessage {
    std::string id;
    std::string from;
    std::string to;
    std::string subject;
    std::string body;
    std::string timestamp;
    bool read = false;
};

class MailboxStore {
public:
    bool create_mailbox(const std::string& mailbox);
    bool delete_mailbox(const std::string& mailbox);
    bool has_mailbox(const std::string& mailbox) const;

    bool append_message(const std::string& mailbox, const EmailMessage& message);
    std::vector<EmailMessage> list_messages(const std::string& mailbox) const;
    bool mark_read(const std::string& mailbox, const std::string& message_id);

    std::size_t mailbox_count() const;
    std::size_t total_messages() const;
    std::size_t unread_count(const std::string& mailbox) const;

private:
    std::map<std::string, std::vector<EmailMessage>> mailboxes_;
};

} // namespace email
} // namespace trekker
