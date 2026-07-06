#include "smtp_server.h"

#include <atomic>
#include <chrono>
#include <sstream>

#include "mailbox.h"

namespace trekker {
namespace email {

namespace {

std::string generate_message_id() {
    static std::atomic<unsigned long> sequence{1};
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::ostringstream oss;
    oss << "msg-" << now_ms << "-" << sequence.fetch_add(1);
    return oss.str();
}

std::string now_iso8601_like() {
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    return std::to_string(now_ms);
}

} // namespace

bool SmtpServer::register_account(const EmailAccount& account) {
    if (account.address.empty()) {
        return false;
    }
    accounts_[account.address] = account;
    return true;
}

bool SmtpServer::unregister_account(const std::string& address) {
    return accounts_.erase(address) > 0;
}

bool SmtpServer::has_account(const std::string& address) const {
    return accounts_.find(address) != accounts_.end();
}

std::size_t SmtpServer::account_count() const {
    return accounts_.size();
}

bool SmtpServer::send_mail(
    const std::string& from_account,
    const std::string& mailbox,
    const std::string& to,
    const std::string& subject,
    const std::string& body,
    MailboxStore& store,
    std::string* error
) const {
    if (!has_account(from_account)) {
        if (error) {
            *error = "from_account is not registered";
        }
        return false;
    }
    if (to.empty()) {
        if (error) {
            *error = "to is required";
        }
        return false;
    }
    if (mailbox.empty() || !store.has_mailbox(mailbox)) {
        if (error) {
            *error = "mailbox does not exist";
        }
        return false;
    }

    EmailMessage message;
    message.id = generate_message_id();
    message.from = from_account;
    message.to = to;
    message.subject = subject;
    message.body = body;
    message.timestamp = now_iso8601_like();
    message.read = false;

    if (!store.append_message(mailbox, message)) {
        if (error) {
            *error = "failed to persist message";
        }
        return false;
    }

    return true;
}

} // namespace email
} // namespace trekker
