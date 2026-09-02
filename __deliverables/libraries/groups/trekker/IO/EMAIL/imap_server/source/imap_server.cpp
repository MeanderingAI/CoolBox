#include "imap_server.h"

#include "mailbox.h"

namespace trekker {
namespace email {

ImapServer::ImapServer(MailboxStore& store) : store_(store) {}

bool ImapServer::fetch_mailbox(const std::string& mailbox, std::vector<EmailMessage>* out) const {
    if (!out) {
        return false;
    }
    if (!store_.has_mailbox(mailbox)) {
        return false;
    }
    *out = store_.list_messages(mailbox);
    return true;
}

bool ImapServer::fetch_by_id(const std::string& mailbox, const std::string& message_id, EmailMessage* out) const {
    if (!out || !store_.has_mailbox(mailbox)) {
        return false;
    }
    const auto messages = store_.list_messages(mailbox);
    for (const auto& message : messages) {
        if (message.id == message_id) {
            *out = message;
            return true;
        }
    }
    return false;
}

bool ImapServer::mark_read(const std::string& mailbox, const std::string& message_id) {
    return store_.mark_read(mailbox, message_id);
}

} // namespace email
} // namespace trekker
