#include <stdexcept>
#include <string>

#include "tyst_framework.hpp"

namespace {

class CounterFixture : public tyst::framework::Test {
protected:
    void SetUp() override {
        value = 41;
    }

    int value = 0;
};

TYST_TEST(TystFrameworkTest, SupportsBasicExpectations) {
    TYST_EXPECT_EQ(2 + 2, 4);
    TYST_EXPECT_TRUE(true);
    TYST_EXPECT_FALSE(false);
    TYST_EXPECT_STREQ("tyst", "tyst");
    TYST_EXPECT_NEAR(3.14159, 3.1416, 1e-3);
}

TYST_TEST(TystFrameworkTest, SupportsExceptionAssertions) {
    TYST_EXPECT_THROW(throw std::runtime_error("boom"), std::runtime_error);
    TYST_EXPECT_NO_THROW(static_cast<void>(std::string("safe")));
    tyst::framework::expect_no_throw([]() {
        const int value = 7;
        EXPECT_EQ(value, 7);
    });
}

TYST_TEST_F(CounterFixture, SupportsFixtures) {
    TYST_ASSERT_EQ(value, 41);
    value += 1;
    TYST_EXPECT_EQ(value, 42);
}

} // namespace