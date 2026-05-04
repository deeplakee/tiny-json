#include "json.h"
#include "test.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace json;

// ============================================
// 测试基本类型
// ============================================

TEST(create_null) {
    JsonValue j;
    ASSERT_EQ(j.type(), JsonValueType::Null);
    ASSERT_TRUE(j.is<JsonNull>());

    JsonValue j2 = nullptr;
    ASSERT_EQ(j2.type(), JsonValueType::Null);
}

TEST(create_bool) {
    JsonValue j1 = true;
    ASSERT_EQ(j1.type(), JsonValueType::Bool);
    ASSERT_TRUE(j1.is<JsonBool>());
    ASSERT_EQ(j1.get<JsonBool>(), true);

    JsonValue j2 = false;
    ASSERT_EQ(j2.get<JsonBool>(), false);
}

TEST(create_number) {
    JsonValue j1 = 42.5;
    ASSERT_EQ(j1.type(), JsonValueType::Number);
    ASSERT_TRUE(j1.is<JsonNumber>());
    ASSERT_DOUBLE_EQ(j1.get<JsonNumber>(), 42.5);

    JsonValue j2 = 100;
    ASSERT_DOUBLE_EQ(j2.get<JsonNumber>(), 100.0);

    JsonValue j3 = -3.14;
    ASSERT_DOUBLE_EQ(j3.get<JsonNumber>(), -3.14);
}

TEST(create_string) {
    JsonValue j1 = "hello";
    ASSERT_EQ(j1.type(), JsonValueType::String);
    ASSERT_TRUE(j1.is<JsonString>());
    ASSERT_EQ(j1.get_string(), "hello");

    JsonValue j2(std::string("world"));
    ASSERT_EQ(j2.get_string(), "world");

    JsonValue j3 = u8"这是";
    ASSERT_EQ(j3.type(), JsonValueType::String);
    ASSERT_TRUE(j3.is<JsonString>());
    ASSERT_EQ(j3.get<JsonString>(), u8"这是");

    JsonValue j4(std::u8string(u8"测试"));
    ASSERT_TRUE(j4.is<JsonString>());
}

TEST(create_array) {
    JsonArray arr = {1, "test", true};
    JsonValue j = arr;
    ASSERT_EQ(j.type(), JsonValueType::Array);
    ASSERT_EQ(j.size(), 3);

    JsonValue j2(array({1, 2, 3}));
    ASSERT_EQ(j2.size(), 3);
}

TEST(create_object) {
    JsonObject obj;
    obj[util::to_u8string("key")] = JsonValue("value");
    JsonValue j(obj);
    ASSERT_EQ(j.type(), JsonValueType::Object);
    ASSERT_EQ(j.size(), 1);

    JsonValue j2(object({
        {"name", "Alice"},
        {"age", 30}
    }));
    ASSERT_EQ(j2.size(), 2);
}

// ============================================
// 测试序列化
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
    JsonValue j = array({1, "test", true});
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

// ============================================
// 测试解析
// ============================================

TEST(parse_null) {
    JsonValue j = parse("null");
    ASSERT_EQ(j.type(), JsonValueType::Null);

    JsonValue j2 = parse(std::string("null"));
    ASSERT_EQ(j2.type(), JsonValueType::Null);
}

TEST(parse_bool) {
    JsonValue j1 = parse("true");
    ASSERT_TRUE(j1.get<JsonBool>());

    JsonValue j2 = parse("false");
    ASSERT_FALSE(j2.get<JsonBool>());
}

TEST(parse_number) {
    JsonValue j1 = parse("42");
    ASSERT_DOUBLE_EQ(j1.get<JsonNumber>(), 42.0);

    JsonValue j2 = parse("-3.14");
    ASSERT_DOUBLE_EQ(j2.get<JsonNumber>(), -3.14);

    JsonValue j3 = parse("1.5e2");
    ASSERT_DOUBLE_EQ(j3.get<JsonNumber>(), 150.0);

    JsonValue j4 = parse("0");
    ASSERT_DOUBLE_EQ(j4.get<JsonNumber>(), 0.0);
}

TEST(parse_string) {
    JsonValue j1 = parse("\"hello\"");
    ASSERT_EQ(j1.get_string(), "hello");

    JsonValue j2 = parse("\"say \\\"hello\\\"\"");
    ASSERT_EQ(j2.get_string(), "say \"hello\"");

    JsonValue j3 = parse("\"line1\\nline2\"");
    ASSERT_EQ(j3.get_string(), "line1\nline2");
}

TEST(parse_empty_array) {
    JsonValue j = parse("[]");
    ASSERT_EQ(j.type(), JsonValueType::Array);
    ASSERT_EQ(j.size(), 0);
}

TEST(parse_simple_array) {
    JsonValue j = parse("[1, 2, 3]");
    ASSERT_EQ(j.size(), 3);
    ASSERT_DOUBLE_EQ(j.at(0).get<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(j.at(1).get<JsonNumber>(), 2.0);
    ASSERT_DOUBLE_EQ(j.at(2).get<JsonNumber>(), 3.0);
}

TEST(parse_mixed_array) {
    JsonValue j = parse("[1, \"two\", true, null]");
    ASSERT_EQ(j.size(), 4);
    ASSERT_DOUBLE_EQ(j.at(0).get<JsonNumber>(), 1.0);
    ASSERT_EQ(j.at(1).get_string(), "two");
    ASSERT_TRUE(j.at(2).get<JsonBool>());
    ASSERT_TRUE(j.at(3).is<JsonNull>());
}

TEST(parse_empty_object) {
    JsonValue j = parse("{}");
    ASSERT_EQ(j.type(), JsonValueType::Object);
    ASSERT_EQ(j.size(), 0);
}

TEST(parse_simple_object) {
    JsonValue j = parse("{\"name\": \"Alice\", \"age\": 30}");
    ASSERT_EQ(j.size(), 2);
    ASSERT_EQ(j.at("name").get_string(), "Alice");
    ASSERT_DOUBLE_EQ(j.at("age").get<JsonNumber>(), 30.0);
}

TEST(parse_nested_structure) {
    std::string json = R"({
        "name": "Alice",
        "address": {
            "city": "Beijing",
            "zip": "100000"
        },
        "scores": [95, 88, 92]
    })";

    JsonValue j = parse(json);
    ASSERT_EQ(j.at("name").get_string(), "Alice");
    ASSERT_EQ(j.at("address").at("city").get_string(), "Beijing");
    ASSERT_DOUBLE_EQ(j.at("scores").at(1).get<JsonNumber>(), 88.0);
}

TEST(parse_unicode) {
    JsonValue j = parse("\"\\u0048\\u0065\\u006c\\u006c\\u006f\"");
    ASSERT_EQ(j.get_string(), "Hello");
}

TEST(parse_errors) {
    ASSERT_THROW(parse(""), std::runtime_error);
    ASSERT_THROW(parse("invalid"), std::runtime_error);
    ASSERT_THROW(parse("{" ), std::runtime_error);
    ASSERT_THROW(parse("[1,"), std::runtime_error);
    ASSERT_THROW(parse("\"unclosed"), std::runtime_error);
}

// ============================================
// 测试访问和修改
// ============================================

TEST(array_access) {
    JsonValue j = parse("[1, 2, 3]");
    ASSERT_DOUBLE_EQ(j.at(0).get<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(j.at(2).get<JsonNumber>(), 3.0);

    ASSERT_THROW(j.at(3), std::out_of_range);
    ASSERT_THROW(j.at("key"), std::runtime_error);
}

TEST(array_modification) {
    JsonValue j = array({1, 2});

    j[2] = 3;
    ASSERT_EQ(j.size(), 3);
    ASSERT_DOUBLE_EQ(j.at(2).get<JsonNumber>(), 3.0);

    j.push_back(JsonValue(4));
    ASSERT_EQ(j.size(), 4);

    j[0] = 10;
    ASSERT_DOUBLE_EQ(j.at(0).get<JsonNumber>(), 10.0);
}

TEST(object_access) {
    JsonValue j = parse("{\"name\": \"Alice\", \"age\": 30}");

    ASSERT_EQ(j.at("name").get_string(), "Alice");
    ASSERT_DOUBLE_EQ(j.at("age").get<JsonNumber>(), 30.0);
    ASSERT_TRUE(j.contains("name"));
    ASSERT_FALSE(j.contains("city"));

    ASSERT_THROW(j.at("invalid"), std::out_of_range);
    ASSERT_THROW(j.at(0), std::runtime_error);
}

TEST(object_modification) {
    JsonValue j = object({
        {"name", "Alice"}
    });

    j["age"] = 30;
    ASSERT_EQ(j.size(), 2);
    ASSERT_DOUBLE_EQ(j.at("age").get<JsonNumber>(), 30.0);

    j["name"] = "Bob";
    ASSERT_EQ(j.at("name").get_string(), "Bob");

    j.insert("city", JsonValue("Beijing"));
    ASSERT_TRUE(j.contains("city"));
}

TEST(auto_convert_when_modifying) {
    JsonValue j; // null initially

    // Access as array should auto-convert
    j[0] = 1;
    ASSERT_EQ(j.type(), JsonValueType::Array);
    ASSERT_EQ(j.size(), 1);

    JsonValue j2;
    j2["key"] = "value";
    ASSERT_EQ(j2.type(), JsonValueType::Object);
    ASSERT_EQ(j2.size(), 1);
}

// ============================================
// 测试类型检查和转换
// ============================================

TEST(type_checking) {
    JsonValue j_null;
    ASSERT_TRUE(j_null.is<JsonNull>());
    ASSERT_FALSE(j_null.is<JsonBool>());
    ASSERT_FALSE(j_null.is<JsonNumber>());

    JsonValue j_bool(true);
    ASSERT_TRUE(j_bool.is<JsonBool>());

    JsonValue j_num(42.0);
    ASSERT_TRUE(j_num.is<JsonNumber>());

    JsonValue j_str("test");
    ASSERT_TRUE(j_str.is<JsonString>());

    JsonValue j_arr = array({});
    ASSERT_TRUE(j_arr.is<JsonArray>());

    JsonValue j_obj = object({});
    ASSERT_TRUE(j_obj.is<JsonObject>());
}

TEST(type_mismatch_errors) {
    JsonValue j(42);
    ASSERT_THROW(j.get<JsonString>(), std::runtime_error);
    ASSERT_THROW(j.get_string(), std::runtime_error);
    ASSERT_THROW(j.size(), std::runtime_error); // number has no size

    JsonValue j_arr = array({});
    ASSERT_THROW(j_arr.at("key"), std::runtime_error);

    JsonValue j_obj = object({});
    ASSERT_THROW(j_obj.at(0), std::runtime_error);
}

// ============================================
// 测试边界情况
// ============================================

TEST(large_numbers) {
    JsonValue j = parse("1e308");
    ASSERT_TRUE(j.get<JsonNumber>() > 0);

    JsonValue j2 = parse("-1e308");
    ASSERT_TRUE(j2.get<JsonNumber>() < 0);
}

TEST(special_characters_in_string) {
    JsonValue j = parse("\"\\t\\n\\r\\b\\f\"");
    std::string result = j.get_string();
    ASSERT_EQ(result[0], '\t');
    ASSERT_EQ(result[1], '\n');
    ASSERT_EQ(result[2], '\r');
    ASSERT_EQ(result[3], '\b');
    ASSERT_EQ(result[4], '\f');
}

TEST(deep_nesting) {
    std::string json = "{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":1}}}}}";
    JsonValue j = parse(json);
    ASSERT_EQ(j.at("a").at("b").at("c").at("d").at("e").get<JsonNumber>(), 1.0);
}

TEST(empty_structures) {
    JsonValue empty_arr = parse("[]");
    ASSERT_EQ(empty_arr.size(), 0);

    JsonValue empty_obj = parse("{}");
    ASSERT_EQ(empty_obj.size(), 0);

    // Serialize empty structures
    ASSERT_EQ(empty_arr.serialize(), "[]");
    ASSERT_EQ(empty_obj.serialize(), "{}");
}

// ============================================
// 测试辅助函数
// ============================================

TEST(helper_array_function) {
    JsonValue j = array({1, "two", true});
    ASSERT_EQ(j.type(), JsonValueType::Array);
    ASSERT_EQ(j.size(), 3);
    ASSERT_DOUBLE_EQ(j.at(0).get<JsonNumber>(), 1.0);
}

TEST(helper_object_function) {
    JsonValue j = object({
        {"name", "Alice"},
        {"age", 30}
    });
    ASSERT_EQ(j.type(), JsonValueType::Object);
    ASSERT_EQ(j.at("name").get_string(), "Alice");
    ASSERT_DOUBLE_EQ(j.at("age").get<JsonNumber>(), 30.0);
}

// ============================================
// 测试for_each功能
// ============================================

TEST(foreach_array) {
    JsonValue j = array({1, 2, 3});
    std::vector<double> values;

    j.for_each_array([&](const JsonValue &elem) {
        values.push_back(elem.get<JsonNumber>());
    });

    ASSERT_EQ(values.size(), 3);
    ASSERT_DOUBLE_EQ(values[0], 1.0);
    ASSERT_DOUBLE_EQ(values[1], 2.0);
    ASSERT_DOUBLE_EQ(values[2], 3.0);
}

TEST(foreach_object) {
    JsonValue j = object({
        {"name", "Alice"},
        {"age", 30}
    });

    int count = 0;
    j.for_each_object([&](const JsonString &key, const JsonValue &value) {
        count++;
        if (util::to_std_string(key) == "name") {
            ASSERT_EQ(value.get_string(), "Alice");
        } else if (util::to_std_string(key) == "age") {
            ASSERT_DOUBLE_EQ(value.get<JsonNumber>(), 30.0);
        }
    });

    ASSERT_EQ(count, 2);
}

TEST(foreach_with_two_version) {
    JsonValue j = array({1, 2, 3});

    // 非const引用调用非const版本
    j.for_each_array([](JsonValue &elem) {
        elem = JsonValue(elem.get<JsonNumber>() * 10);
    });

    // 验证修改成功
    ASSERT_DOUBLE_EQ(j.at(0).get<JsonNumber>(), 10.0);
    ASSERT_DOUBLE_EQ(j.at(1).get<JsonNumber>(), 20.0);
    ASSERT_DOUBLE_EQ(j.at(2).get<JsonNumber>(), 30.0);

    // 通过const引用强制调用const版本
    const JsonValue &const_j = j;
    std::vector<double> values;

    const_j.for_each_array([&](const JsonValue &elem) {
        // elem是const引用，不能修改
        values.push_back(elem.get<JsonNumber>());
    });

    ASSERT_DOUBLE_EQ(values[0], 10.0);
    ASSERT_DOUBLE_EQ(values[1], 20.0);
    ASSERT_DOUBLE_EQ(values[2], 30.0);
}


// ============================================
// 主测试运行器
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "Running JSON Library Tests\n";
    std::cout << "==========================\n\n";

    // 创建测试
    RUN_TEST(create_null);
    RUN_TEST(create_bool);
    RUN_TEST(create_number);
    RUN_TEST(create_string);
    RUN_TEST(create_array);
    RUN_TEST(create_object);

    // 序列化测试
    RUN_TEST(serialize_null);
    RUN_TEST(serialize_bool);
    RUN_TEST(serialize_number);
    RUN_TEST(serialize_string);
    RUN_TEST(serialize_array);
    RUN_TEST(serialize_object);
    RUN_TEST(serialize_with_indent);

    // 解析测试
    RUN_TEST(parse_null);
    RUN_TEST(parse_bool);
    RUN_TEST(parse_number);
    RUN_TEST(parse_string);
    RUN_TEST(parse_empty_array);
    RUN_TEST(parse_simple_array);
    RUN_TEST(parse_mixed_array);
    RUN_TEST(parse_empty_object);
    RUN_TEST(parse_simple_object);
    RUN_TEST(parse_nested_structure);
    RUN_TEST(parse_unicode);
    RUN_TEST(parse_errors);

    // 访问和修改测试
    RUN_TEST(array_access);
    RUN_TEST(array_modification);
    RUN_TEST(object_access);
    RUN_TEST(object_modification);
    RUN_TEST(auto_convert_when_modifying);

    // 类型检查测试
    RUN_TEST(type_checking);
    RUN_TEST(type_mismatch_errors);

    // utf-8

    // 边界情况测试
    RUN_TEST(large_numbers);
    RUN_TEST(special_characters_in_string);
    RUN_TEST(deep_nesting);
    RUN_TEST(empty_structures);

    // 辅助函数测试
    RUN_TEST(helper_array_function);
    RUN_TEST(helper_object_function);

    // For each 测试
    RUN_TEST(foreach_array);
    RUN_TEST(foreach_object);
    RUN_TEST(foreach_with_two_version);

    std::cout << "\n==========================\n";
    std::cout << "All tests passed successfully!\n";

#ifdef _WIN32
    system("pause");
#endif
    return 0;
}
