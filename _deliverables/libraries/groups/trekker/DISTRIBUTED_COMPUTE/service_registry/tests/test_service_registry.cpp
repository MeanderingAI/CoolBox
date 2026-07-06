#include "tyst_framework.hpp"

#include "service_registry.h"

#include <chrono>
#include <thread>

using namespace distributed;

TEST(ServiceRegistryTest, RegisterAndLookupHealthyInstance) {
    ServiceRegistry reg;
    EXPECT_TRUE(reg.register_service("auth", "i1", {"localhost", 8080, ""}));
    auto results = reg.lookup("auth");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].instance_id, "i1");
    EXPECT_EQ(results[0].endpoint.port, 8080);
}

TEST(ServiceRegistryTest, DuplicateInstanceIdRejected) {
    ServiceRegistry reg;
    EXPECT_TRUE(reg.register_service("svc", "i1", {"h", 1, ""}));
    EXPECT_FALSE(reg.register_service("svc", "i1", {"h", 2, ""}));
    EXPECT_EQ(reg.instance_count("svc"), 1u);
}

TEST(ServiceRegistryTest, DeregisterRemovesInstance) {
    ServiceRegistry reg;
    reg.register_service("svc", "i1", {"h", 1, ""});
    EXPECT_TRUE(reg.deregister("svc", "i1"));
    EXPECT_EQ(reg.lookup("svc").size(), 0u);
}

TEST(ServiceRegistryTest, DeregisterNonExistentReturnsFalse) {
    ServiceRegistry reg;
    EXPECT_FALSE(reg.deregister("svc", "ghost"));
}

TEST(ServiceRegistryTest, HeartbeatKeepsInstanceHealthy) {
    ServiceRegistry reg;
    reg.register_service("svc", "i1", {"h", 1, ""});
    EXPECT_TRUE(reg.heartbeat("svc", "i1"));
    EXPECT_EQ(reg.lookup("svc").size(), 1u);
}

TEST(ServiceRegistryTest, ExpireStaleMarksInstanceUnhealthy) {
    ServiceRegistry reg;
    reg.register_service("svc", "i1", {"h", 1, ""});
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    std::size_t expired = reg.expire_stale(std::chrono::milliseconds(30));
    EXPECT_EQ(expired, 1u);
    EXPECT_EQ(reg.lookup("svc").size(), 0u); // lookup returns healthy only
    EXPECT_EQ(reg.instance_count("svc"), 1u); // record still present
}

TEST(ServiceRegistryTest, HeartbeatRestoresHealthAfterExpiry) {
    ServiceRegistry reg;
    reg.register_service("svc", "i1", {"h", 1, ""});
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    reg.expire_stale(std::chrono::milliseconds(30));
    EXPECT_EQ(reg.lookup("svc").size(), 0u);
    reg.heartbeat("svc", "i1");
    EXPECT_EQ(reg.lookup("svc").size(), 1u);
}

TEST(ServiceRegistryTest, GetInstanceReturnsMetadata) {
    ServiceRegistry reg;
    reg.register_service("db", "primary", {"db-host", 5432, "role=primary"});
    auto rec = reg.get_instance("db", "primary");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->endpoint.metadata, "role=primary");
    EXPECT_EQ(rec->endpoint.host, "db-host");
}

TEST(ServiceRegistryTest, LookupUnknownServiceReturnsEmpty) {
    ServiceRegistry reg;
    EXPECT_TRUE(reg.lookup("nonexistent").empty());
}

TEST(ServiceRegistryTest, MultipleServicesAreIsolated) {
    ServiceRegistry reg;
    reg.register_service("svcA", "i1", {"a", 1, ""});
    reg.register_service("svcB", "i1", {"b", 2, ""});
    EXPECT_EQ(reg.lookup("svcA").size(), 1u);
    EXPECT_EQ(reg.lookup("svcB").size(), 1u);
    reg.deregister("svcA", "i1");
    EXPECT_EQ(reg.lookup("svcA").size(), 0u);
    EXPECT_EQ(reg.lookup("svcB").size(), 1u);
}
