#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace tyst {
namespace framework {

class Test;

namespace detail {

struct AssertionResult {
    bool passed;
    std::string message;
};

struct TestContext {
    std::string suite_name;
    std::string test_name;
    std::vector<std::string> failures;
    bool skipped = false;
    std::string skip_message;
};

struct TestRegistration {
    std::string suite_name;
    std::string test_name;
    std::function<std::unique_ptr<Test>()> factory;
};

class FatalAssertionFailure : public std::exception {
public:
    const char* what() const noexcept override {
        return "fatal assertion failure";
    }
};

class SkipTestFailure : public std::exception {
public:
    explicit SkipTestFailure(std::string message) : message_(std::move(message)) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

inline thread_local TestContext* current_test_context = nullptr;

inline std::vector<TestRegistration>& registry() {
    static std::vector<TestRegistration> tests;
    return tests;
}

inline bool register_test(
    std::string suite_name,
    std::string test_name,
    std::function<std::unique_ptr<Test>()> factory) {
    registry().push_back({std::move(suite_name), std::move(test_name), std::move(factory)});
    return true;
}

inline TestContext* current_context() {
    return current_test_context;
}

inline void push_failure(std::string message) {
    if (current_context() != nullptr) {
        current_context()->failures.push_back(std::move(message));
        return;
    }
    std::cerr << message << std::endl;
}

inline void push_skip(std::string message) {
    if (current_context() != nullptr) {
        current_context()->skipped = true;
        current_context()->skip_message = std::move(message);
        return;
    }
    throw SkipTestFailure(std::move(message));
}

template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
    : std::true_type {};

template <typename T>
std::string to_string_value(const T& value) {
    if constexpr (is_streamable<T>::value) {
        std::ostringstream stream;
        stream << std::boolalpha << value;
        return stream.str();
    } else {
        return "<non-streamable>";
    }
}

inline std::string to_string_value(const char* value) {
    if (value == nullptr) {
        return "<null>";
    }
    return std::string("\"") + value + "\"";
}

inline std::string to_string_value(char* value) {
    return to_string_value(static_cast<const char*>(value));
}

template <typename Left, typename Right>
AssertionResult make_binary_result(
    bool passed,
    const char* operation,
    const char* left_expr,
    const char* right_expr,
    const Left& left_value,
    const Right& right_value) {
    if (passed) {
        return {true, {}};
    }

    std::ostringstream stream;
    stream << "Expected " << left_expr << ' ' << operation << ' ' << right_expr
           << ", but got " << to_string_value(left_value)
           << " and " << to_string_value(right_value);
    return {false, stream.str()};
}

template <typename Left, typename Right>
AssertionResult compare_eq(const Left& left_value, const Right& right_value, const char* left_expr, const char* right_expr) {
    return make_binary_result(left_value == right_value, "==", left_expr, right_expr, left_value, right_value);
}

template <typename Left, typename Right>
AssertionResult compare_ne(const Left& left_value, const Right& right_value, const char* left_expr, const char* right_expr) {
    return make_binary_result(left_value != right_value, "!=", left_expr, right_expr, left_value, right_value);
}

template <typename Left, typename Right>
AssertionResult compare_lt(const Left& left_value, const Right& right_value, const char* left_expr, const char* right_expr) {
    return make_binary_result(left_value < right_value, "<", left_expr, right_expr, left_value, right_value);
}

template <typename Left, typename Right>
AssertionResult compare_le(const Left& left_value, const Right& right_value, const char* left_expr, const char* right_expr) {
    return make_binary_result(left_value <= right_value, "<=", left_expr, right_expr, left_value, right_value);
}

template <typename Left, typename Right>
AssertionResult compare_gt(const Left& left_value, const Right& right_value, const char* left_expr, const char* right_expr) {
    return make_binary_result(left_value > right_value, ">", left_expr, right_expr, left_value, right_value);
}

template <typename Left, typename Right>
AssertionResult compare_ge(const Left& left_value, const Right& right_value, const char* left_expr, const char* right_expr) {
    return make_binary_result(left_value >= right_value, ">=", left_expr, right_expr, left_value, right_value);
}

template <typename Left, typename Right, typename Error>
AssertionResult compare_near(
    const Left& left_value,
    const Right& right_value,
    const Error& error_value,
    const char* left_expr,
    const char* right_expr,
    const char* error_expr) {
    const long double difference = std::fabsl(static_cast<long double>(left_value) - static_cast<long double>(right_value));
    const long double tolerance = std::fabsl(static_cast<long double>(error_value));
    if (difference <= tolerance) {
        return {true, {}};
    }

    std::ostringstream stream;
    stream << "Expected |" << left_expr << " - " << right_expr << "| <= " << error_expr
           << ", but got |" << to_string_value(left_value) << " - " << to_string_value(right_value)
           << "| = " << difference;
    return {false, stream.str()};
}

template <typename Float>
AssertionResult compare_float_eq(const Float& left_value, const Float& right_value, const char* left_expr, const char* right_expr) {
    const auto scale = std::max<Float>({Float(1), std::fabs(left_value), std::fabs(right_value)});
    const auto tolerance = std::numeric_limits<Float>::epsilon() * scale * Float(4);
    return compare_near(left_value, right_value, tolerance, left_expr, right_expr, "scaled epsilon");
}

inline AssertionResult compare_streq(const char* left_value, const char* right_value, const char* left_expr, const char* right_expr) {
    const bool passed =
        (left_value == nullptr && right_value == nullptr) ||
        (left_value != nullptr && right_value != nullptr && std::strcmp(left_value, right_value) == 0);
    return make_binary_result(passed, "==", left_expr, right_expr, to_string_value(left_value), to_string_value(right_value));
}

inline AssertionResult compare_true(bool condition, const char* expression) {
    if (condition) {
        return {true, {}};
    }

    return {false, std::string("Expected true: ") + expression};
}

inline AssertionResult compare_false(bool condition, const char* expression) {
    if (!condition) {
        return {true, {}};
    }

    return {false, std::string("Expected false: ") + expression};
}

template <typename ExceptionType, typename Callable>
AssertionResult compare_throw(Callable&& callable, const char* statement, const char* exception_name) {
    try {
        std::forward<Callable>(callable)();
    } catch (const ExceptionType&) {
        return {true, {}};
    } catch (const std::exception& error) {
        return {false, std::string("Expected ") + exception_name + " from " + statement +
                    ", but caught " + error.what()};
    } catch (...) {
        return {false, std::string("Expected ") + exception_name + " from " + statement +
                    ", but caught a different exception type"};
    }

    return {false, std::string("Expected ") + exception_name + " from " + statement + ", but nothing was thrown"};
}

template <typename Callable>
AssertionResult compare_no_throw(Callable&& callable, const char* statement) {
    try {
        std::forward<Callable>(callable)();
        return {true, {}};
    } catch (const std::exception& error) {
        return {false, std::string("Expected no exception from ") + statement + ", but caught " + error.what()};
    } catch (...) {
        return {false, std::string("Expected no exception from ") + statement + ", but caught a different exception type"};
    }
}

class MessageProxy {
public:
    MessageProxy(bool passed, bool fatal, std::string message, const char* file_name, int line_number)
        : passed_(passed), fatal_(fatal), message_(std::move(message)), file_name_(file_name), line_number_(line_number) {}

    MessageProxy(const MessageProxy&) = delete;
    MessageProxy& operator=(const MessageProxy&) = delete;

    MessageProxy(MessageProxy&& other) noexcept
        : passed_(other.passed_), fatal_(other.fatal_), message_(std::move(other.message_)),
          extra_stream_(std::move(other.extra_stream_)), file_name_(other.file_name_), line_number_(other.line_number_),
          consumed_(other.consumed_) {
        other.consumed_ = true;
    }

    ~MessageProxy() noexcept(false) {
        if (consumed_ || passed_) {
            return;
        }

        std::ostringstream stream;
        stream << file_name_ << ':' << line_number_ << ": " << message_;
        if (!extra_stream_.str().empty()) {
            stream << " | " << extra_stream_.str();
        }

        push_failure(stream.str());
        if (fatal_ && std::uncaught_exceptions() == 0) {
            throw FatalAssertionFailure();
        }
    }

    template <typename Value>
    MessageProxy& operator<<(const Value& value) {
        if (!passed_) {
            extra_stream_ << value;
        }
        return *this;
    }

private:
    bool passed_;
    bool fatal_;
    std::string message_;
    std::ostringstream extra_stream_;
    const char* file_name_;
    int line_number_;
    bool consumed_ = false;
};

class SkipProxy {
public:
    SkipProxy(const char* file_name, int line_number)
        : file_name_(file_name), line_number_(line_number) {}

    SkipProxy(const SkipProxy&) = delete;
    SkipProxy& operator=(const SkipProxy&) = delete;

    SkipProxy(SkipProxy&& other) noexcept
        : stream_(std::move(other.stream_)), file_name_(other.file_name_), line_number_(other.line_number_), consumed_(other.consumed_) {
        other.consumed_ = true;
    }

    ~SkipProxy() noexcept(false) {
        if (consumed_) {
            return;
        }

        std::ostringstream message;
        message << file_name_ << ':' << line_number_ << ": skipped";
        if (!stream_.str().empty()) {
            message << " | " << stream_.str();
        }

        push_skip(message.str());
        if (std::uncaught_exceptions() == 0) {
            throw SkipTestFailure(message.str());
        }
    }

    template <typename Value>
    SkipProxy& operator<<(const Value& value) {
        stream_ << value;
        return *this;
    }

private:
    std::ostringstream stream_;
    const char* file_name_;
    int line_number_;
    bool consumed_ = false;
};

inline MessageProxy make_proxy(bool fatal, AssertionResult result, const char* file_name, int line_number) {
    return MessageProxy(result.passed, fatal, std::move(result.message), file_name, line_number);
}

inline MessageProxy make_fail_proxy(const char* file_name, int line_number) {
    return MessageProxy(false, true, "Failure invoked", file_name, line_number);
}

inline MessageProxy make_success_proxy(const char* file_name, int line_number) {
    return MessageProxy(true, false, {}, file_name, line_number);
}

inline SkipProxy make_skip_proxy(const char* file_name, int line_number) {
    return SkipProxy(file_name, line_number);
}

} // namespace detail

class Environment {};

class Test {
public:
    virtual ~Test() = default;

    void run() {
        bool setup_completed = false;
        try {
            SetUp();
            setup_completed = true;
            TestBody();
        } catch (...) {
            if (setup_completed) {
                run_teardown();
            }
            throw;
        }

        if (setup_completed) {
            run_teardown();
        }
    }

protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
    virtual void TestBody() = 0;

private:
    void run_teardown() {
        try {
            TearDown();
        } catch (const std::exception& error) {
            detail::push_failure(std::string("Unhandled exception in TearDown: ") + error.what());
        } catch (...) {
            detail::push_failure("Unhandled non-standard exception in TearDown");
        }
    }
};

inline void init(int* argc, char** argv) {
    (void)argc;
    (void)argv;
}

inline int run_all_tests() {
    std::size_t passed_count = 0;
    std::size_t failed_count = 0;
    std::size_t skipped_count = 0;

    for (const auto& registration : detail::registry()) {
        detail::TestContext context;
        context.suite_name = registration.suite_name;
        context.test_name = registration.test_name;

        std::cout << "[ RUN      ] " << registration.suite_name << '.' << registration.test_name << std::endl;
        detail::current_test_context = &context;

        try {
            auto test = registration.factory();
            test->run();
        } catch (const detail::SkipTestFailure& error) {
            context.skipped = true;
            if (context.skip_message.empty()) {
                context.skip_message = error.what();
            }
        } catch (const detail::FatalAssertionFailure&) {
        } catch (const std::exception& error) {
            detail::push_failure(std::string("Unhandled exception: ") + error.what());
        } catch (...) {
            detail::push_failure("Unhandled non-standard exception");
        }

        detail::current_test_context = nullptr;

        if (context.skipped) {
            ++skipped_count;
            std::cout << "[  SKIPPED ] " << registration.suite_name << '.' << registration.test_name;
            if (!context.skip_message.empty()) {
                std::cout << " - " << context.skip_message;
            }
            std::cout << std::endl;
            continue;
        }

        if (context.failures.empty()) {
            ++passed_count;
            std::cout << "[       OK ] " << registration.suite_name << '.' << registration.test_name << std::endl;
            continue;
        }

        ++failed_count;
        std::cout << "[  FAILED  ] " << registration.suite_name << '.' << registration.test_name << std::endl;
        for (const auto& failure : context.failures) {
            std::cout << "  " << failure << std::endl;
        }
    }

    std::cout << "[==========] " << detail::registry().size() << " tests ran. "
              << passed_count << " passed, " << failed_count << " failed, " << skipped_count << " skipped."
              << std::endl;

    return failed_count == 0 ? 0 : 1;
}

template <typename Callable>
void expect_no_throw(Callable&& callable) {
    auto result = detail::compare_no_throw([&]() { std::forward<Callable>(callable)(); }, "callable()");
    if (!result.passed) {
        detail::push_failure(result.message);
    }
}

template <typename Callable>
void assert_no_throw(Callable&& callable) {
    auto result = detail::compare_no_throw([&]() { std::forward<Callable>(callable)(); }, "callable()");
    if (!result.passed) {
        detail::push_failure(result.message);
        throw detail::FatalAssertionFailure();
    }
}

} // namespace framework
} // namespace tyst

#define TYST_INTERNAL_CONCAT_IMPL(lhs, rhs) lhs##rhs
#define TYST_INTERNAL_CONCAT(lhs, rhs) TYST_INTERNAL_CONCAT_IMPL(lhs, rhs)

#define TEST(test_suite_name, test_name) \
    class TYST_INTERNAL_CONCAT(test_suite_name##_, TYST_INTERNAL_CONCAT(test_name, _Test)) : public ::tyst::framework::Test { \
    public: \
        void TestBody() override; \
    private: \
        static const bool registered_; \
    }; \
    const bool TYST_INTERNAL_CONCAT(test_suite_name##_, TYST_INTERNAL_CONCAT(test_name, _Test))::registered_ = \
        ::tyst::framework::detail::register_test( \
            #test_suite_name, \
            #test_name, \
            []() -> std::unique_ptr<::tyst::framework::Test> { \
                return std::make_unique<TYST_INTERNAL_CONCAT(test_suite_name##_, TYST_INTERNAL_CONCAT(test_name, _Test))>(); \
            }); \
    void TYST_INTERNAL_CONCAT(test_suite_name##_, TYST_INTERNAL_CONCAT(test_name, _Test))::TestBody()

#define TEST_F(test_fixture, test_name) \
    class TYST_INTERNAL_CONCAT(test_fixture##_, TYST_INTERNAL_CONCAT(test_name, _Test)) : public test_fixture { \
    public: \
        void TestBody() override; \
    private: \
        static const bool registered_; \
    }; \
    const bool TYST_INTERNAL_CONCAT(test_fixture##_, TYST_INTERNAL_CONCAT(test_name, _Test))::registered_ = \
        ::tyst::framework::detail::register_test( \
            #test_fixture, \
            #test_name, \
            []() -> std::unique_ptr<::tyst::framework::Test> { \
                return std::make_unique<TYST_INTERNAL_CONCAT(test_fixture##_, TYST_INTERNAL_CONCAT(test_name, _Test))>(); \
            }); \
    void TYST_INTERNAL_CONCAT(test_fixture##_, TYST_INTERNAL_CONCAT(test_name, _Test))::TestBody()

#define EXPECT_TRUE(condition) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_true(static_cast<bool>(condition), #condition), __FILE__, __LINE__)
#define EXPECT_FALSE(condition) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_false(static_cast<bool>(condition), #condition), __FILE__, __LINE__)
#define EXPECT_EQ(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_eq((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_NE(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_ne((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_LT(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_lt((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_LE(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_le((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_GT(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_gt((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_GE(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_ge((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_NEAR(lhs, rhs, abs_error) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_near((lhs), (rhs), (abs_error), #lhs, #rhs, #abs_error), __FILE__, __LINE__)
#define EXPECT_DOUBLE_EQ(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_float_eq<double>((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_FLOAT_EQ(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_float_eq<float>((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_STREQ(lhs, rhs) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_streq((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define EXPECT_THROW(statement, exception_type) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_throw<exception_type>([&]() { statement; }, #statement, #exception_type), __FILE__, __LINE__)
#define EXPECT_NO_THROW(statement) ::tyst::framework::detail::make_proxy(false, ::tyst::framework::detail::compare_no_throw([&]() { statement; }, #statement), __FILE__, __LINE__)

#define ASSERT_TRUE(condition) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_true(static_cast<bool>(condition), #condition), __FILE__, __LINE__)
#define ASSERT_FALSE(condition) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_false(static_cast<bool>(condition), #condition), __FILE__, __LINE__)
#define ASSERT_EQ(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_eq((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_NE(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_ne((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_LT(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_lt((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_LE(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_le((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_GT(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_gt((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_GE(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_ge((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_NEAR(lhs, rhs, abs_error) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_near((lhs), (rhs), (abs_error), #lhs, #rhs, #abs_error), __FILE__, __LINE__)
#define ASSERT_DOUBLE_EQ(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_float_eq<double>((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_FLOAT_EQ(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_float_eq<float>((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_STREQ(lhs, rhs) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_streq((lhs), (rhs), #lhs, #rhs), __FILE__, __LINE__)
#define ASSERT_THROW(statement, exception_type) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_throw<exception_type>([&]() { statement; }, #statement, #exception_type), __FILE__, __LINE__)
#define ASSERT_NO_THROW(statement) ::tyst::framework::detail::make_proxy(true, ::tyst::framework::detail::compare_no_throw([&]() { statement; }, #statement), __FILE__, __LINE__)

#define SUCCEED() ::tyst::framework::detail::make_success_proxy(__FILE__, __LINE__)
#define FAIL() ::tyst::framework::detail::make_fail_proxy(__FILE__, __LINE__)
#define RUN_ALL_TESTS() ::tyst::framework::run_all_tests()

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
#define TYST_SKIP() ::tyst::framework::detail::make_skip_proxy(__FILE__, __LINE__)
#define TYST_SUCCEED() SUCCEED()
#define TYST_FAIL() FAIL()
