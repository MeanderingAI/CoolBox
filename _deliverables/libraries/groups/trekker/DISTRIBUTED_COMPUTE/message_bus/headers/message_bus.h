#ifndef DISTRIBUTED_MESSAGE_BUS_H
#define DISTRIBUTED_MESSAGE_BUS_H

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace distributed {

using SubscriberId   = std::uint32_t;
using MessageHandler = std::function<void(const std::string& topic,
                                          const std::string& payload)>;

// In-process publish/subscribe message bus.
// Thread-safe; handlers are invoked synchronously on the publishing thread.
// Snapshot-then-call ensures subscriptions mutated during publish are safe.
class MessageBus {
public:
    MessageBus() = default;

    // Subscribe to a topic. Returns a subscriber ID for later unsubscription.
    SubscriberId subscribe(const std::string& topic, MessageHandler handler);

    // Unsubscribe by subscriber ID. Returns false if not found.
    bool unsubscribe(const std::string& topic, SubscriberId id);

    // Deliver payload to all current subscribers. Returns subscriber count called.
    std::size_t publish(const std::string& topic, const std::string& payload);

    std::size_t subscriber_count(const std::string& topic) const;

    void clear_topic(const std::string& topic);
    void clear_all();

private:
    struct Subscription {
        SubscriberId   id;
        MessageHandler handler;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<Subscription>> topics_;
    std::uint32_t next_id_{0};
};

} // namespace distributed

#endif // DISTRIBUTED_MESSAGE_BUS_H
