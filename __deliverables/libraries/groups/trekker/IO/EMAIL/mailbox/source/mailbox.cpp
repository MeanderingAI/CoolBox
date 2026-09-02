#include "mailbox.h"

namespace trekker {
namespace email {

bool MailboxStore::create_mailbox(const std::string& mailbox) {
    if (mailbox.empty()) {
        return false;
    }
    return mailboxes_.emplace(mailbox, std::vector<EmailMessage>{}).second;
}

bool MailboxStore::delete_mailbox(const std::string& mailbox) {
    return mailboxes_.erase(mailbox) > 0;
}

bool MailboxStore::has_mailbox(const std::string& mailbox) const {
    return mailboxes_.find(mailbox) != mailboxes_.end();
}

bool MailboxStore::append_message(const std::string& mailbox, const EmailMessage& message) {
    auto it = mailboxes_.find(mailbox);
    if (it == mailboxes_.end()) {
        return false;
    }
    it->second.push_back(message);
    return true;
}

std::vector<EmailMessage> MailboxStore::list_messages(const std::string& mailbox) const {
    auto it = mailboxes_.find(mailbox);
    if (it == mailboxes_.end()) {
        return {};
    }
    return it->second;
}

bool MailboxStore::mark_read(const std::string& mailbox, const std::string& message_id) {
    auto it = mailboxes_.find(mailbox);
    if (it == mailboxes_.end()) {
        return false;
    }
    for (auto& message : it->second) {
        if (message.id == message_id) {
            message.read = true;
            return true;
        }
    }
    return false;
}

std::size_t MailboxStore::mailbox_count() const {
    return mailboxes_.size();
}

std::size_t MailboxStore::total_messages() const {
    std::size_t total = 0;
    for (const auto& entry : mailboxes_) {
        total += entry.second.size();
    }
    return total;
}

std::size_t MailboxStore::unread_count(const std::string& mailbox) const {
    auto it = mailboxes_.find(mailbox);
    if (it == mailboxes_.end()) {
        return 0;
    }
    std::size_t unread = 0;
    for (const auto& message : it->second) {
        if (!message.read) {
            ++unread;
        }
    }
    return unread;
}

} // namespace email
} // namespace trekker
