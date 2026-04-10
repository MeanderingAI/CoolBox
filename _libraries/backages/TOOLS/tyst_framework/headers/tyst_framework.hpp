#pragma once

#include <gtest/gtest.h>

#include <utility>

namespace tyst {
namespace framework {

using Test = ::testing::Test;
using Environment = ::testing::Environment;

inline void init(int* argc, char** argv) {
    ::testing::InitGoogleTest(argc, argv);
}

template <typename Callable>
void expect_no_throw(Callable&& callable) {
    EXPECT_NO_THROW(std::forward<Callable>(callable)());
}

template <typename Callable>
void assert_no_throw(Callable&& callable) {
    ASSERT_NO_THROW(std::forward<Callable>(callable)());
}

} // namespace framework
} // namespace tyst

#define TYST_TEST(test_suite_name, test_name) TEST(test_suite_name, test_name)
#define TYST_TEST_F(test_fixture, test_name) TEST_F(test_fixture, test_name)
#define TYST_EXPECT_TRUE(condition) EXPECT_TRUE(condition)
#define TYST_EXPECT_FALSE(condition) EXPECT_FALSE(condition)
#define TYST_EXPECT_EQ(lhs, rhs) EXPECT_EQ(lhs, rhs)
#define TYST_EXPECT_NE(lhs, rhs) EXPECT_NE(lhs, rhs)
#define TYST_EXPECT_LT(lhs, rhs) EXPECT_LT(lhs, rhs)
#define TYST_EXPECT_LE(lhs, rhs) EXPECT_LE(lhs, rhs)
#define TYST_EXPECT_GT(lhs, rhs) EXPECT_GT(lhs, rhs)
#define TYST_EXPECT_GE(lhs, rhs) EXPECT_GE(lhs, rhs)
#define TYST_EXPECT_NEAR(lhs, rhs, abs_error) EXPECT_NEAR(lhs, rhs, abs_error)
#define TYST_EXPECT_DOUBLE_EQ(lhs, rhs) EXPECT_DOUBLE_EQ(lhs, rhs)
#define TYST_EXPECT_FLOAT_EQ(lhs, rhs) EXPECT_FLOAT_EQ(lhs, rhs)
#define TYST_EXPECT_STREQ(lhs, rhs) EXPECT_STREQ(lhs, rhs)
#define TYST_EXPECT_THROW(statement, exception_type) EXPECT_THROW(statement, exception_type)
#define TYST_EXPECT_NO_THROW(statement) EXPECT_NO_THROW(statement)
#define TYST_ASSERT_TRUE(condition) ASSERT_TRUE(condition)
#define TYST_ASSERT_FALSE(condition) ASSERT_FALSE(condition)
#define TYST_ASSERT_EQ(lhs, rhs) ASSERT_EQ(lhs, rhs)
#define TYST_ASSERT_NE(lhs, rhs) ASSERT_NE(lhs, rhs)
#define TYST_ASSERT_LT(lhs, rhs) ASSERT_LT(lhs, rhs)
#define TYST_ASSERT_LE(lhs, rhs) ASSERT_LE(lhs, rhs)
#define TYST_ASSERT_GT(lhs, rhs) ASSERT_GT(lhs, rhs)
#define TYST_ASSERT_GE(lhs, rhs) ASSERT_GE(lhs, rhs)
#define TYST_ASSERT_NEAR(lhs, rhs, abs_error) ASSERT_NEAR(lhs, rhs, abs_error)
#define TYST_ASSERT_DOUBLE_EQ(lhs, rhs) ASSERT_DOUBLE_EQ(lhs, rhs)
#define TYST_ASSERT_FLOAT_EQ(lhs, rhs) ASSERT_FLOAT_EQ(lhs, rhs)
#define TYST_ASSERT_STREQ(lhs, rhs) ASSERT_STREQ(lhs, rhs)
#define TYST_ASSERT_THROW(statement, exception_type) ASSERT_THROW(statement, exception_type)
#define TYST_ASSERT_NO_THROW(statement) ASSERT_NO_THROW(statement)
#define TYST_SKIP() GTEST_SKIP()
#define TYST_SUCCEED() SUCCEED()
#define TYST_FAIL() FAIL()
