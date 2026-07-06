#pragma once

#include <map>
#include <string>

namespace trekker {
namespace email {

class MailboxStore;

struct EmailAccount {
    std::string address;
    std::string display_name;
    std::string outbound_host;
    int outbound_port = 587;
};

class SmtpServer {
public:
    bool register_account(const EmailAccount& account);
    bool unregister_account(const std::string& address);
    bool has_account(const std::string& address) const;
    std::size_t account_count() const;

    bool send_mail(
        const std::string& from_account,
        const std::string& mailbox,
        const std::string& to,
        const std::string& subject,
        const std::string& body,
        MailboxStore& store,
        std::string* error = nullptr
    ) const;

private:
    std::map<std::string, EmailAccount> accounts_;
};

} // namespace email
} // namespace trekker
