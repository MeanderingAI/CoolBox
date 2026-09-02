#include "../headers/message_bus.h"

#include <algorithm>

namespace distributed {

SubscriberId MessageBus::subscribe(const std::string& topic, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    const SubscriberId id = ++next_id_;
    topics_[topic].push_back({id, std::move(handler)});
    return id;
}

bool MessageBus::unsubscribe(const std::string& topic, SubscriberId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = topics_.find(topic);
    if (it == topics_.end()) return false;

    auto& subs = it->second;
    auto  pos  = std::remove_if(subs.begin(), subs.end(),
                                [id](const Subscription& s) { return s.id == id; });
    if (pos == subs.end()) return false;
    subs.erase(pos, subs.end());
    return true;
}

std::size_t MessageBus::publish(const std::string& topic, const std::string& payload) {
    // Take a snapshot so the handlers can safely mutate subscriptions.
    std::vector<Subscription> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = topics_.find(topic);
        if (it == topics_.end()) return 0;
        snapshot = it->second;
    }
    for (const auto& sub : snapshot) sub.handler(topic, payload);
    return snapshot.size();
}

std::size_t MessageBus::subscriber_count(const std::string& topic) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = topics_.find(topic);
    return (it == topics_.end()) ? 0 : it->second.size();
}

void MessageBus::clear_topic(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    topics_.erase(topic);
}

void MessageBus::clear_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    topics_.clear();
}

} // namespace distributed
