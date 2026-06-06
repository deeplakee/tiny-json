#include "json.h"
#include "test.h"
#include <iostream>
#include <cassert>
#include <string>
#include <limits>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace json;

// ============================================
// 基本类型序列化
// ============================================

TEST(serialize_null) {
    JsonValue j;
    ASSERT_EQ(j.serialize(), "null");
}

TEST(serialize_bool) {
    JsonValue j1(true);
    ASSERT_EQ(j1.serialize(), "true");

    JsonValue j2(false);
    ASSERT_EQ(j2.serialize(), "false");
}

TEST(serialize_number) {
    JsonValue j1(42);
    ASSERT_EQ(j1.serialize(), "42");

    JsonValue j2(3.14);
    ASSERT_TRUE(j2.serialize().find("3.14") != std::string::npos);

    JsonValue j3(-10);
    ASSERT_EQ(j3.serialize(), "-10");
}

TEST(serialize_string) {
    JsonValue j1("hello");
    ASSERT_EQ(j1.serialize(), "\"hello\"");

    JsonValue j2("say \"hello\"");
    ASSERT_EQ(j2.serialize(), "\"say \\\"hello\\\"\"");
}

TEST(serialize_array) {
    JsonValue   j      = array({1, "test", true});
    std::string result = j.serialize();
    ASSERT_TRUE(result.find("[1,\"test\",true]") != std::string::npos);

    JsonValue empty_arr = array({});
    ASSERT_EQ(empty_arr.serialize(), "[]");
}

TEST(serialize_object) {
    JsonValue j = object({
            {"name", "Alice"},
            {"age", 30}
    });
    std::string result = j.serialize();
    ASSERT_TRUE(result.find("\"name\":\"Alice\"") != std::string::npos);
    ASSERT_TRUE(result.find("\"age\":30") != std::string::npos);
}

// ============================================
// 格式化输出
// ============================================

TEST(serialize_with_indent) {
    JsonValue j = object({
            {"name", "Alice"},
            {
                    "address", object({
                            {"city", "Beijing"},
                            {"street", "Main St"}
                    })
            },
            {"scores", array({95, 88, 92})}
    });

    std::string result = j.serialize(2);
    ASSERT_TRUE(result.find("\n") != std::string::npos);
    ASSERT_TRUE(result.find("  ") != std::string::npos);
}

TEST(serialize_pretty_nested) {
    JsonValue j = object({
            {"name", "Alice"},
            {"scores", array({95, 88, 92})},
            {"active", true}
    });

    std::string pretty = j.serialize(4);
    ASSERT_TRUE(pretty.find("    ") != std::string::npos);
    ASSERT_TRUE(pretty.find('\n') != std::string::npos);
}

TEST(serialize_empty_pretty) {
    ASSERT_EQ(array({}).serialize(2), "[]");
    ASSERT_EQ(object({}).serialize(2), "{}");
}

// ============================================
// 数字格式化
// ============================================

TEST(serialize_nan_inf) {
    // NaN 和 Inf 不符合 JSON 规范，序列化为 null
    JsonValue j_nan(std::numeric_limits<double>::quiet_NaN());
    ASSERT_EQ(j_nan.serialize(), "null");

    JsonValue j_inf(std::numeric_limits<double>::infinity());
    ASSERT_EQ(j_inf.serialize(), "null");

    JsonValue j_neg_inf(-std::numeric_limits<double>::infinity());
    ASSERT_EQ(j_neg_inf.serialize(), "null");
}

TEST(serialize_integer_formatting) {
    // 整数应省略小数点
    ASSERT_EQ(JsonValue(0.0).serialize(), "0");
    ASSERT_EQ(JsonValue(42.0).serialize(), "42");
    ASSERT_EQ(JsonValue(-100.0).serialize(), "-100");

    // 非整数应保留小数点
    JsonValue j(3.14);
    ASSERT_TRUE(j.serialize().find('.') != std::string::npos);
}

TEST(serialize_decimal_formatting) {
    // 小数应保留必要位数，去除尾随零
    std::string result = JsonValue(3.14).serialize();
    ASSERT_EQ(result, "3.14");

    std::string result2 = JsonValue(1.50).serialize();
    ASSERT_EQ(result2, "1.5");
}

TEST(serialize_scientific_notation) {
    // 极大/极小数应使用科学计数法
    std::string result = JsonValue(1e20).serialize();
    ASSERT_TRUE(result.find('e') != std::string::npos || result.find('E') != std::string::npos);

    std::string result2 = JsonValue(1e-20).serialize();
    ASSERT_TRUE(result2.find('e') != std::string::npos || result2.find('E') != std::string::npos);
}

// ============================================
// 往返一致性
// ============================================

TEST(roundtrip_simple) {
    std::string original   = R"({"name":"Alice","age":30,"active":true,"score":95.5})";
    JsonValue   parsed     = parse(original);
    std::string serialized = parsed.serialize();
    JsonValue   reparsed   = parse(serialized);

    ASSERT_EQ(reparsed.at("name").as_string(), "Alice");
    ASSERT_DOUBLE_EQ(reparsed.at("age").as<JsonNumber>(), 30.0);
    ASSERT_TRUE(reparsed.at("active").as<JsonBool>());
    ASSERT_DOUBLE_EQ(reparsed.at("score").as<JsonNumber>(), 95.5);
}

TEST(roundtrip_nested) {
    std::string original   = R"({"a":{"b":[1,2,3]},"c":"hello"})";
    JsonValue   parsed     = parse(original);
    std::string serialized = parsed.serialize();
    JsonValue   reparsed   = parse(serialized);

    ASSERT_EQ(reparsed.at("a").at("b").at(1).as<JsonNumber>(), 2.0);
    ASSERT_EQ(reparsed.at("c").as_string(), "hello");
}

TEST(roundtrip_with_escapes) {
    std::string original   = R"({"msg":"say \"hello\"\n\tworld"})";
    JsonValue   parsed     = parse(original);
    std::string serialized = parsed.serialize();
    JsonValue   reparsed   = parse(serialized);

    ASSERT_EQ(reparsed.at("msg").as_string(), "say \"hello\"\n\tworld");
}

TEST(roundtrip_numbers) {
    std::string json       = R"({"a":3.14,"b":1e20,"c":0.001})";
    JsonValue   j          = parse(json);
    std::string serialized = j.serialize();
    JsonValue   j2         = parse(serialized);

    ASSERT_DOUBLE_EQ(j2.at("a").as<JsonNumber>(), 3.14);
    ASSERT_DOUBLE_EQ(j2.at("b").as<JsonNumber>(), 1e20);
    ASSERT_DOUBLE_EQ(j2.at("c").as<JsonNumber>(), 0.001);
}

TEST(roundtrip_utf8) {
    JsonValue   j          = JsonValue(std::u8string(u8"Hello 世界 🌍"));
    std::string serialized = j.serialize();
    JsonValue   reparsed   = parse(serialized);
    ASSERT_EQ(reparsed.as_string(), "Hello 世界 🌍");
}

// ============================================
// 主测试运行器
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "Running Serialize Tests\n";
    std::cout << "=======================\n\n";

    // 基本类型序列化
    RUN_TEST(serialize_null);
    RUN_TEST(serialize_bool);
    RUN_TEST(serialize_number);
    RUN_TEST(serialize_string);
    RUN_TEST(serialize_array);
    RUN_TEST(serialize_object);

    // 格式化输出
    RUN_TEST(serialize_with_indent);
    RUN_TEST(serialize_pretty_nested);
    RUN_TEST(serialize_empty_pretty);

    // 数字格式化
    RUN_TEST(serialize_nan_inf);
    RUN_TEST(serialize_integer_formatting);
    RUN_TEST(serialize_decimal_formatting);
    RUN_TEST(serialize_scientific_notation);

    // 往返一致性
    RUN_TEST(roundtrip_simple);
    RUN_TEST(roundtrip_nested);
    RUN_TEST(roundtrip_with_escapes);
    RUN_TEST(roundtrip_numbers);
    RUN_TEST(roundtrip_utf8);

    std::cout << "\n=======================\n";
    std::cout << "All serialize tests passed!\n";

    return 0;
}
