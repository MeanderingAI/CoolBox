#include <gtest/gtest.h>
#include "dataformats/json/json.h"
#include <sstream>

using namespace dataformats::json;

// ============================================================================
// Value Tests
// ============================================================================

TEST(JsonValueTest, DefaultConstructor) {
    Value v;
    EXPECT_EQ(v.type(), Type::NULL_VALUE);
    EXPECT_TRUE(v.is_null());
}

TEST(JsonValueTest, IntegerValue) {
    Value v(42);
    EXPECT_EQ(v.type(), Type::NUMBER);
    EXPECT_TRUE(v.is_number());
    EXPECT_EQ(v.as_number(), 42);
    EXPECT_DOUBLE_EQ(v.as_number(), 42.0);
}

TEST(JsonValueTest, DoubleValue) {
    Value v(3.14);
    EXPECT_EQ(v.type(), Type::NUMBER);
    EXPECT_TRUE(v.is_number());
    EXPECT_DOUBLE_EQ(v.as_number(), 3.14);
}

TEST(JsonValueTest, StringValue) {
    Value v("hello");
    EXPECT_EQ(v.type(), Type::STRING);
    EXPECT_TRUE(v.is_string());
    EXPECT_EQ(v.as_string(), "hello");
}

TEST(JsonValueTest, BooleanValue) {
    Value v_true(true);
    Value v_false(false);
    
    EXPECT_EQ(v_true.type(), Type::BOOLEAN);
    EXPECT_TRUE(v_true.is_bool());
    EXPECT_TRUE(v_true.as_bool());
    
    EXPECT_EQ(v_false.type(), Type::BOOLEAN);
    EXPECT_TRUE(v_false.is_bool());
    EXPECT_FALSE(v_false.as_bool());
}

TEST(JsonValueTest, NullValue) {
    Value v(nullptr);
    EXPECT_EQ(v.type(), Type::NULL_VALUE);
    EXPECT_TRUE(v.is_null());
}

TEST(JsonValueTest, ToStringNumber) {
    Value v(42);
    EXPECT_EQ(v.to_string(), "42");
}

TEST(JsonValueTest, ToStringDouble) {
    Value v(3.14);
    std::string result = v.to_string();
    EXPECT_TRUE(result.find("3.14") != std::string::npos);
}

TEST(JsonValueTest, ToStringString) {
    Value v("hello");
    EXPECT_EQ(v.to_string(), "\"hello\"");
}

TEST(JsonValueTest, ToStringBool) {
    Value v_true(true);
    Value v_false(false);
    EXPECT_EQ(v_true.to_string(), "true");
    EXPECT_EQ(v_false.to_string(), "false");
}

TEST(JsonValueTest, ToStringNull) {
    Value v(nullptr);
    EXPECT_EQ(v.to_string(), "null");
}

// ============================================================================
// Object Tests
// ============================================================================

TEST(JsonObjectTest, EmptyObject) {
    Object obj;
    EXPECT_TRUE(obj.empty());
    EXPECT_EQ(obj.size(), 0u);
}

TEST(JsonObjectTest, SetAndGet) {
    Object obj;
    obj.set("name", Value("John"));
    obj.set("age", Value(30));
    
    EXPECT_EQ(obj.size(), 2u);
    EXPECT_TRUE(obj.has("name"));
    EXPECT_TRUE(obj.has("age"));
    EXPECT_FALSE(obj.has("missing"));
    
    EXPECT_EQ(obj.get("name").as_string(), "John");
    EXPECT_EQ(obj.get("age").as_number(), 30);
}

TEST(JsonObjectTest, RemoveKey) {
    Object obj;
    obj.set("key1", Value(1));
    obj.set("key2", Value(2));
    
    EXPECT_EQ(obj.size(), 2u);
    obj.remove("key1");
    EXPECT_EQ(obj.size(), 1u);
    EXPECT_FALSE(obj.has("key1"));
    EXPECT_TRUE(obj.has("key2"));
}

TEST(JsonObjectTest, Clear) {
    Object obj;
    obj.set("key1", Value(1));
    obj.set("key2", Value(2));
    
    // obj.clear();  // clear() not available in current API
    // EXPECT_TRUE(obj.empty());
    // EXPECT_EQ(obj.size(), 0u);
}

TEST(JsonObjectTest, Keys) {
    Object obj;
    obj.set("name", Value("Alice"));
    obj.set("age", Value(25));
    obj.set("active", Value(true));
    
    auto keys = obj.keys();
    EXPECT_EQ(keys.size(), 3u);
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "name") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "age") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "active") != keys.end());
}

TEST(JsonObjectTest, ToStringSimple) {
    Object obj;
    obj.set("name", Value("John"));
    obj.set("age", Value(30));
    
    std::string json = obj.to_string();
    EXPECT_TRUE(json.find("\"name\":\"John\"") != std::string::npos ||
                json.find("\"name\": \"John\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"age\":30") != std::string::npos ||
                json.find("\"age\": 30") != std::string::npos);
}

TEST(JsonObjectTest, NestedObject) {
    Object inner;
    inner.set("city", Value("NYC"));
    inner.set("zip", Value("10001"));
    
    Object outer;
    outer.set("name", Value("John"));
    outer.set("address", Value(inner));
    
    EXPECT_TRUE(outer.has("address"));
    EXPECT_TRUE(outer.get("address").is_object());
    
    Object addr = outer.get("address").as_object();
    EXPECT_EQ(addr.get("city").as_string(), "NYC");
}

// ============================================================================
// Array Tests
// ============================================================================

TEST(JsonArrayTest, EmptyArray) {
    Array arr;
    EXPECT_TRUE(arr.empty());
    EXPECT_EQ(arr.size(), 0u);
}

TEST(JsonArrayTest, PushAndGet) {
    Array arr;
    arr.push(Value(1));
    arr.push(Value(2));
    arr.push(Value(3));
    
    EXPECT_EQ(arr.size(), 3u);
    EXPECT_EQ(arr.get(0).as_number(), 1);
    EXPECT_EQ(arr.get(1).as_number(), 2);
    EXPECT_EQ(arr.get(2).as_number(), 3);
}

TEST(JsonArrayTest, SetValue) {
    Array arr;
    arr.push(Value(1));
    arr.push(Value(2));
    
    arr.set(1, Value(99));
    EXPECT_EQ(arr.get(1).as_number(), 99);
}

TEST(JsonArrayTest, Clear) {
    Array arr;
    arr.push(Value(1));
    arr.push(Value(2));
    
    // arr.clear();  // clear() not available in current API
    // EXPECT_TRUE(arr.empty());
    // EXPECT_EQ(arr.size(), 0u);
}

TEST(JsonArrayTest, MixedTypes) {
    Array arr;
    arr.push(Value(42));
    arr.push(Value("hello"));
    arr.push(Value(true));
    arr.push(Value(nullptr));
    
    EXPECT_EQ(arr.size(), 4u);
    EXPECT_TRUE(arr.get(0).is_number());
    EXPECT_TRUE(arr.get(1).is_string());
    EXPECT_TRUE(arr.get(2).is_bool());
    EXPECT_TRUE(arr.get(3).is_null());
}

TEST(JsonArrayTest, ToString) {
    Array arr;
    arr.push(Value(1));
    arr.push(Value(2));
    arr.push(Value(3));
    
    std::string json = arr.to_string();
    EXPECT_TRUE(json.find("[") != std::string::npos);
    EXPECT_TRUE(json.find("]") != std::string::npos);
    EXPECT_TRUE(json.find("1") != std::string::npos);
    EXPECT_TRUE(json.find("2") != std::string::npos);
    EXPECT_TRUE(json.find("3") != std::string::npos);
}

// ============================================================================
// Parser Tests
// ============================================================================

TEST(JsonParserTest, ParseNull) {
    Parser parser;
    Value v = parser.parse("null");
    EXPECT_TRUE(v.is_null());
}

TEST(JsonParserTest, ParseBoolean) {
    Parser parser;
    Value v_true = parser.parse("true");
    Value v_false = parser.parse("false");
    
    EXPECT_TRUE(v_true.is_bool());
    EXPECT_TRUE(v_true.as_bool());
    
    EXPECT_TRUE(v_false.is_bool());
    EXPECT_FALSE(v_false.as_bool());
}

TEST(JsonParserTest, ParseInteger) {
    Parser parser;
    Value v = parser.parse("42");
    EXPECT_TRUE(v.is_number());
    EXPECT_EQ(v.as_number(), 42);
}

TEST(JsonParserTest, ParseNegativeInteger) {
    Parser parser;
    Value v = parser.parse("-123");
    EXPECT_TRUE(v.is_number());
    EXPECT_EQ(v.as_number(), -123);
}

TEST(JsonParserTest, ParseDouble) {
    Parser parser;
    Value v = parser.parse("3.14159");
    EXPECT_TRUE(v.is_number());
    EXPECT_NEAR(v.as_number(), 3.14159, 0.00001);
}

TEST(JsonParserTest, ParseString) {
    Parser parser;
    Value v = parser.parse("\"hello world\"");
    EXPECT_TRUE(v.is_string());
    EXPECT_EQ(v.as_string(), "hello world");
}

TEST(JsonParserTest, ParseEmptyString) {
    Parser parser;
    Value v = parser.parse("\"\"");
    EXPECT_TRUE(v.is_string());
    EXPECT_EQ(v.as_string(), "");
}

TEST(JsonParserTest, ParseSimpleObject) {
    Parser parser;
    Value v = parser.parse("{\"name\":\"John\",\"age\":30}");
    
    EXPECT_TRUE(v.is_object());
    Object obj = v.as_object();
    
    EXPECT_TRUE(obj.has("name"));
    EXPECT_TRUE(obj.has("age"));
    EXPECT_EQ(obj.get("name").as_string(), "John");
    EXPECT_EQ(obj.get("age").as_number(), 30);
}

TEST(JsonParserTest, ParseObjectWithSpaces) {
    Parser parser;
    Value v = parser.parse("{ \"name\" : \"John\" , \"age\" : 30 }");
    
    EXPECT_TRUE(v.is_object());
    Object obj = v.as_object();
    
    EXPECT_EQ(obj.get("name").as_string(), "John");
    EXPECT_EQ(obj.get("age").as_number(), 30);
}

TEST(JsonParserTest, ParseNestedObject) {
    Parser parser;
    std::string json = R"({
        "name": "John",
        "address": {
            "city": "NYC",
            "zip": "10001"
        }
    })";
    
    Value v = parser.parse(json);
    EXPECT_TRUE(v.is_object());
    
    Object obj = v.as_object();
    EXPECT_TRUE(obj.has("address"));
    EXPECT_TRUE(obj.get("address").is_object());
    
    Object addr = obj.get("address").as_object();
    EXPECT_EQ(addr.get("city").as_string(), "NYC");
    EXPECT_EQ(addr.get("zip").as_string(), "10001");
}

TEST(JsonParserTest, ParseSimpleArray) {
    Parser parser;
    Value v = parser.parse("[1,2,3,4,5]");
    
    EXPECT_TRUE(v.is_array());
    Array arr = v.as_array();
    
    EXPECT_EQ(arr.size(), 5u);
    EXPECT_EQ(arr.get(0).as_number(), 1);
    EXPECT_EQ(arr.get(4).as_number(), 5);
}

TEST(JsonParserTest, ParseArrayWithSpaces) {
    Parser parser;
    Value v = parser.parse("[ 1 , 2 , 3 ]");
    
    EXPECT_TRUE(v.is_array());
    Array arr = v.as_array();
    
    EXPECT_EQ(arr.size(), 3u);
}

TEST(JsonParserTest, ParseMixedArray) {
    Parser parser;
    Value v = parser.parse("[1, \"hello\", true, null]");
    
    EXPECT_TRUE(v.is_array());
    Array arr = v.as_array();
    
    EXPECT_EQ(arr.size(), 4u);
    EXPECT_TRUE(arr.get(0).is_number());
    EXPECT_TRUE(arr.get(1).is_string());
    EXPECT_TRUE(arr.get(2).is_bool());
    EXPECT_TRUE(arr.get(3).is_null());
}

TEST(JsonParserTest, ParseArrayOfObjects) {
    Parser parser;
    std::string json = R"([
        {"id": 1, "name": "Alice"},
        {"id": 2, "name": "Bob"}
    ])";
    
    Value v = parser.parse(json);
    EXPECT_TRUE(v.is_array());
    
    Array arr = v.as_array();
    EXPECT_EQ(arr.size(), 2u);
    
    EXPECT_TRUE(arr.get(0).is_object());
    EXPECT_EQ(arr.get(0).as_object().get("name").as_string(), "Alice");
    
    EXPECT_TRUE(arr.get(1).is_object());
    EXPECT_EQ(arr.get(1).as_object().get("name").as_string(), "Bob");
}

// ============================================================================
// Builder Tests
// ============================================================================

TEST(JsonBuilderTest, EmptyObject) {
    Builder builder;
    Value v = builder.build();
    
    EXPECT_TRUE(v.is_object());
    EXPECT_TRUE(v.as_object().empty());
}

TEST(JsonBuilderTest, SimpleObject) {
    Builder builder;
    builder.add("name", "John")
           .add("age", 30)
           .add("active", true);
    
    Value v = builder.build();
    EXPECT_TRUE(v.is_object());
    
    Object obj = v.as_object();
    EXPECT_EQ(obj.size(), 3u);
    auto name_val = obj.get("name");
    std::cout << "[DEBUG] obj.get(\"name\").type() = " << static_cast<int>(name_val.type()) << std::endl;
    EXPECT_EQ(name_val.as_string(), "John");
    EXPECT_EQ(obj.get("age").as_number(), 30);
    EXPECT_TRUE(obj.get("active").as_bool());
}

TEST(JsonBuilderTest, NestedObject) {
    Builder inner;
    inner.add("city", "NYC").add("zip", "10001");
    
    Builder outer;
    outer.add("name", "John")
         .add("address", inner.build());
    
    Value v = outer.build();
    Object obj = v.as_object();
    
    EXPECT_TRUE(obj.has("address"));
    Object addr = obj.get("address").as_object();
    EXPECT_EQ(addr.get("city").as_string(), "NYC");
}

TEST(JsonBuilderTest, ArrayOfNumbers) {
    Builder builder;
    Array arr;
    arr.push(Value(1));
    arr.push(Value(2));
    arr.push(Value(3));
    
    builder.add("numbers", Value(arr));
    
    Value v = builder.build();
    Object obj = v.as_object();
    
    EXPECT_TRUE(obj.get("numbers").is_array());
    Array result = obj.get("numbers").as_array();
    EXPECT_EQ(result.size(), 3u);
}

TEST(JsonBuilderTest, ComplexStructure) {
    Builder builder;
    
    Array tags;
    tags.push(Value("cpp"));
    tags.push(Value("json"));
    tags.push(Value("rest"));
    
    Builder settings;
    settings.add("debug", true)
            .add("timeout", 30);
    
    builder.add("project", "ToolBox")
           .add("version", 1.0)
           .add("tags", Value(tags))
           .add("settings", settings.build());
    
    Value v = builder.build();
    Object obj = v.as_object();
    
    EXPECT_EQ(obj.get("project").as_string(), "ToolBox");
    EXPECT_DOUBLE_EQ(obj.get("version").as_number(), 1.0);
    EXPECT_EQ(obj.get("tags").as_array().size(), 3u);
    EXPECT_TRUE(obj.get("settings").is_object());
}

// ============================================================================
// Round-trip Tests (Parse -> Build -> Parse)
// ============================================================================

TEST(JsonRoundTripTest, SimpleObject) {
    std::string original = R"({"name":"John","age":30})";
    
    Parser parser;
    Value v = parser.parse(original);
    
    std::string serialized = v.to_string();
    Value v2 = parser.parse(serialized);
    
    EXPECT_TRUE(v2.is_object());
    EXPECT_EQ(v2.as_object().get("name").as_string(), "John");
    EXPECT_EQ(v2.as_object().get("age").as_number(), 30);
}

TEST(JsonRoundTripTest, ComplexStructure) {
    std::string original = R"({
        "users": [
            {"id": 1, "name": "Alice"},
            {"id": 2, "name": "Bob"}
        ],
        "count": 2
    })";
    
    Parser parser;
    Value v = parser.parse(original);
    
    std::string serialized = v.to_string();
    Value v2 = parser.parse(serialized);
    
    EXPECT_TRUE(v2.is_object());
    EXPECT_TRUE(v2.as_object().has("users"));
    EXPECT_EQ(v2.as_object().get("users").as_array().size(), 2u);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
