#pragma once

#include <string>
#include <vector>

namespace trekker {
namespace email {

class MailboxStore;
struct EmailMessage;

class ImapServer {
public:
    explicit ImapServer(MailboxStore& store);

    bool fetch_mailbox(const std::string& mailbox, std::vector<EmailMessage>* out) const;
    bool fetch_by_id(const std::string& mailbox, const std::string& message_id, EmailMessage* out) const;
    bool mark_read(const std::string& mailbox, const std::string& message_id);

private:
    MailboxStore& store_;
};

} // namespace email
} // namespace trekker
