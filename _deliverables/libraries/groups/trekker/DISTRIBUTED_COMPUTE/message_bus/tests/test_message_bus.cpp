#include "tyst_framework.hpp"

#include "message_bus.h"

#include <atomic>
#include <string>
#include <thread>

using namespace distributed;

TEST(MessageBusTest, SubscribeAndPublishDeliversPayload) {
    MessageBus bus;
    std::string got_topic, got_payload;

    bus.subscribe("events", [&](const std::string& t, const std::string& p) {
        got_topic   = t;
        got_payload = p;
    });

    bus.publish("events", "hello");
    EXPECT_EQ(got_topic,   "events");
    EXPECT_EQ(got_payload, "hello");
}

TEST(MessageBusTest, PublishToUnknownTopicReturnsZero) {
    MessageBus bus;
    EXPECT_EQ(bus.publish("none", "data"), 0u);
}

TEST(MessageBusTest, MultipleSubscribersAllReceive) {
    MessageBus bus;
    std::atomic<int> count{0};

    bus.subscribe("tick", [&](const std::string&, const std::string&) { ++count; });
    bus.subscribe("tick", [&](const std::string&, const std::string&) { ++count; });

    std::size_t delivered = bus.publish("tick", "");
    EXPECT_EQ(delivered, 2u);
    EXPECT_EQ(count.load(), 2);
}

TEST(MessageBusTest, UnsubscribeStopsDelivery) {
    MessageBus bus;
    std::atomic<int> count{0};

    auto id = bus.subscribe("data", [&](const std::string&, const std::string&) { ++count; });
    bus.publish("data", "first");
    bus.unsubscribe("data", id);
    bus.publish("data", "second");

    EXPECT_EQ(count.load(), 1);
}

TEST(MessageBusTest, SubscriberCountIsAccurate) {
    MessageBus bus;
    EXPECT_EQ(bus.subscriber_count("x"), 0u);
    bus.subscribe("x", [](const std::string&, const std::string&) {});
    bus.subscribe("x", [](const std::string&, const std::string&) {});
    EXPECT_EQ(bus.subscriber_count("x"), 2u);
}

TEST(MessageBusTest, ClearTopicRemovesAllSubscribers) {
    MessageBus bus;
    std::atomic<int> count{0};
    bus.subscribe("z", [&](const std::string&, const std::string&) { ++count; });
    bus.clear_topic("z");
    bus.publish("z", "msg");
    EXPECT_EQ(count.load(), 0);
}

TEST(MessageBusTest, ClearAllRemovesEverything) {
    MessageBus bus;
    std::atomic<int> count{0};
    bus.subscribe("a", [&](const std::string&, const std::string&) { ++count; });
    bus.subscribe("b", [&](const std::string&, const std::string&) { ++count; });
    bus.clear_all();
    bus.publish("a", "x");
    bus.publish("b", "x");
    EXPECT_EQ(count.load(), 0);
}

TEST(MessageBusTest, TopicsAreIndependent) {
    MessageBus bus;
    std::string result_a, result_b;
    bus.subscribe("a", [&](const std::string&, const std::string& p) { result_a = p; });
    bus.subscribe("b", [&](const std::string&, const std::string& p) { result_b = p; });
    bus.publish("a", "alpha");
    bus.publish("b", "beta");
    EXPECT_EQ(result_a, "alpha");
    EXPECT_EQ(result_b, "beta");
}
