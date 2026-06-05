#include "json.h"
#include "test.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <limits>
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
    ASSERT_EQ(j1.as<JsonBool>(), true);

    JsonValue j2 = false;
    ASSERT_EQ(j2.as<JsonBool>(), false);
}

TEST(create_number) {
    JsonValue j1 = 42.5;
    ASSERT_EQ(j1.type(), JsonValueType::Number);
    ASSERT_TRUE(j1.is<JsonNumber>());
    ASSERT_DOUBLE_EQ(j1.as<JsonNumber>(), 42.5);

    JsonValue j2 = 100;
    ASSERT_DOUBLE_EQ(j2.as<JsonNumber>(), 100.0);

    JsonValue j3 = -3.14;
    ASSERT_DOUBLE_EQ(j3.as<JsonNumber>(), -3.14);
}

TEST(create_string) {
    JsonValue j1 = "hello";
    ASSERT_EQ(j1.type(), JsonValueType::String);
    ASSERT_TRUE(j1.is<JsonString>());
    ASSERT_EQ(j1.as_string(), "hello");

    JsonValue j2(std::string("world"));
    ASSERT_EQ(j2.as_string(), "world");

    JsonValue j3 = u8"这是";
    ASSERT_EQ(j3.type(), JsonValueType::String);
    ASSERT_TRUE(j3.is<JsonString>());
    ASSERT_EQ(j3.as<JsonString>(), u8"这是");

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
    ASSERT_TRUE(j1.as<JsonBool>());

    JsonValue j2 = parse("false");
    ASSERT_FALSE(j2.as<JsonBool>());
}

TEST(parse_number) {
    JsonValue j1 = parse("42");
    ASSERT_DOUBLE_EQ(j1.as<JsonNumber>(), 42.0);

    JsonValue j2 = parse("-3.14");
    ASSERT_DOUBLE_EQ(j2.as<JsonNumber>(), -3.14);

    JsonValue j3 = parse("1.5e2");
    ASSERT_DOUBLE_EQ(j3.as<JsonNumber>(), 150.0);

    JsonValue j4 = parse("0");
    ASSERT_DOUBLE_EQ(j4.as<JsonNumber>(), 0.0);
}

TEST(parse_string) {
    JsonValue j1 = parse("\"hello\"");
    ASSERT_EQ(j1.as_string(), "hello");

    JsonValue j2 = parse("\"say \\\"hello\\\"\"");
    ASSERT_EQ(j2.as_string(), "say \"hello\"");

    JsonValue j3 = parse("\"line1\\nline2\"");
    ASSERT_EQ(j3.as_string(), "line1\nline2");
}

TEST(parse_empty_array) {
    JsonValue j = parse("[]");
    ASSERT_EQ(j.type(), JsonValueType::Array);
    ASSERT_EQ(j.size(), 0);
}

TEST(parse_simple_array) {
    JsonValue j = parse("[1, 2, 3]");
    ASSERT_EQ(j.size(), 3);
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(j.at(1).as<JsonNumber>(), 2.0);
    ASSERT_DOUBLE_EQ(j.at(2).as<JsonNumber>(), 3.0);
}

TEST(parse_mixed_array) {
    JsonValue j = parse("[1, \"two\", true, null]");
    ASSERT_EQ(j.size(), 4);
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 1.0);
    ASSERT_EQ(j.at(1).as_string(), "two");
    ASSERT_TRUE(j.at(2).as<JsonBool>());
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
    ASSERT_EQ(j.at("name").as_string(), "Alice");
    ASSERT_DOUBLE_EQ(j.at("age").as<JsonNumber>(), 30.0);
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
    ASSERT_EQ(j.at("name").as_string(), "Alice");
    ASSERT_EQ(j.at("address").at("city").as_string(), "Beijing");
    ASSERT_DOUBLE_EQ(j.at("scores").at(1).as<JsonNumber>(), 88.0);
}

TEST(parse_unicode) {
    JsonValue j = parse("\"\\u0048\\u0065\\u006c\\u006c\\u006f\"");
    ASSERT_EQ(j.as_string(), "Hello");
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
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(j.at(2).as<JsonNumber>(), 3.0);

    ASSERT_THROW(j.at(3), std::out_of_range);
    ASSERT_THROW(j.at("key"), std::runtime_error);
}

TEST(array_modification) {
    JsonValue j = array({1, 2});

    j[2] = 3;
    ASSERT_EQ(j.size(), 3);
    ASSERT_DOUBLE_EQ(j.at(2).as<JsonNumber>(), 3.0);

    j.push_back(JsonValue(4));
    ASSERT_EQ(j.size(), 4);

    j[0] = 10;
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 10.0);
}

TEST(object_access) {
    JsonValue j = parse("{\"name\": \"Alice\", \"age\": 30}");

    ASSERT_EQ(j.at("name").as_string(), "Alice");
    ASSERT_DOUBLE_EQ(j.at("age").as<JsonNumber>(), 30.0);
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
    ASSERT_DOUBLE_EQ(j.at("age").as<JsonNumber>(), 30.0);

    j["name"] = "Bob";
    ASSERT_EQ(j.at("name").as_string(), "Bob");

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
    ASSERT_THROW(j.as<JsonString>(), std::runtime_error);
    ASSERT_THROW(j.as_string(), std::runtime_error);
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
    ASSERT_TRUE(j.as<JsonNumber>() > 0);

    JsonValue j2 = parse("-1e308");
    ASSERT_TRUE(j2.as<JsonNumber>() < 0);
}

TEST(special_characters_in_string) {
    JsonValue j = parse("\"\\t\\n\\r\\b\\f\"");
    std::string result = j.as_string();
    ASSERT_EQ(result[0], '\t');
    ASSERT_EQ(result[1], '\n');
    ASSERT_EQ(result[2], '\r');
    ASSERT_EQ(result[3], '\b');
    ASSERT_EQ(result[4], '\f');
}

TEST(deep_nesting) {
    std::string json = "{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":1}}}}}";
    JsonValue j = parse(json);
    ASSERT_EQ(j.at("a").at("b").at("c").at("d").at("e").as<JsonNumber>(), 1.0);
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
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 1.0);
}

TEST(helper_object_function) {
    JsonValue j = object({
        {"name", "Alice"},
        {"age", 30}
    });
    ASSERT_EQ(j.type(), JsonValueType::Object);
    ASSERT_EQ(j.at("name").as_string(), "Alice");
    ASSERT_DOUBLE_EQ(j.at("age").as<JsonNumber>(), 30.0);
}

// ============================================
// 测试for_each功能
// ============================================

TEST(foreach_array) {
    JsonValue j = array({1, 2, 3});
    std::vector<double> values;

    j.for_each_array([&](const JsonValue &elem) {
        values.push_back(elem.as<JsonNumber>());
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
            ASSERT_EQ(value.as_string(), "Alice");
        } else if (util::to_std_string(key) == "age") {
            ASSERT_DOUBLE_EQ(value.as<JsonNumber>(), 30.0);
        }
    });

    ASSERT_EQ(count, 2);
}

TEST(foreach_with_two_version) {
    JsonValue j = array({1, 2, 3});

    // 非const引用调用非const版本
    j.for_each_array([](JsonValue &elem) {
        elem = JsonValue(elem.as<JsonNumber>() * 10);
    });

    // 验证修改成功
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 10.0);
    ASSERT_DOUBLE_EQ(j.at(1).as<JsonNumber>(), 20.0);
    ASSERT_DOUBLE_EQ(j.at(2).as<JsonNumber>(), 30.0);

    // 通过const引用强制调用const版本
    const JsonValue &const_j = j;
    std::vector<double> values;

    const_j.for_each_array([&](const JsonValue &elem) {
        // elem是const引用，不能修改
        values.push_back(elem.as<JsonNumber>());
    });

    ASSERT_DOUBLE_EQ(values[0], 10.0);
    ASSERT_DOUBLE_EQ(values[1], 20.0);
    ASSERT_DOUBLE_EQ(values[2], 30.0);
}

// ============================================
// 数组大小限制测试
// ============================================

TEST(array_size_limit_normal) {
    // 正常范围内应该可以正常使用
    JsonValue j;
    j[100] = 42;
    ASSERT_EQ(j.size(), 101);
    ASSERT_DOUBLE_EQ(j.at(100).as<JsonNumber>(), 42.0);
}

TEST(array_size_limit_exceeded) {
    // 超过限制应该抛异常
    JsonValue j;
    ASSERT_THROW(j[1000000] = 1, std::out_of_range);
}


// ============================================
// 迭代器测试
// ============================================

TEST(iterator_array_range_for) {
    JsonValue j = array({1, 2, 3, 4, 5});
    double sum = 0;
    for (auto &elem : j.as_array()) {
        sum += elem.as<JsonNumber>();
    }
    ASSERT_DOUBLE_EQ(sum, 15.0);
}

TEST(iterator_array_const) {
    const JsonValue j = array({10, 20, 30});
    std::vector<double> values;
    for (const auto &elem : j.as_array()) {
        values.push_back(elem.as<JsonNumber>());
    }
    ASSERT_EQ(values.size(), 3);
    ASSERT_DOUBLE_EQ(values[0], 10.0);
    ASSERT_DOUBLE_EQ(values[1], 20.0);
    ASSERT_DOUBLE_EQ(values[2], 30.0);
}

TEST(iterator_array_modify) {
    JsonValue j = array({1, 2, 3});
    for (auto &elem : j.as_array()) {
        elem = JsonValue(elem.as<JsonNumber>() * 10);
    }
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 10.0);
    ASSERT_DOUBLE_EQ(j.at(1).as<JsonNumber>(), 20.0);
    ASSERT_DOUBLE_EQ(j.at(2).as<JsonNumber>(), 30.0);
}

TEST(iterator_object_range_for) {
    JsonValue j = object({{"a", 1}, {"b", 2}, {"c", 3}});
    double sum = 0;
    for (auto &[key, value] : j.as_object()) {
        sum += value.as<JsonNumber>();
    }
    ASSERT_DOUBLE_EQ(sum, 6.0);
}

TEST(iterator_object_const) {
    const JsonValue j = object({{"x", 10}, {"y", 20}});
    int count = 0;
    for (const auto &[key, value] : j.as_object()) {
        (void)key;
        count++;
    }
    ASSERT_EQ(count, 2);
}

TEST(iterator_object_modify_values) {
    JsonValue j = object({{"a", 1}, {"b", 2}});
    for (auto &[key, value] : j.as_object()) {
        value = JsonValue(value.as<JsonNumber>() * 100);
    }
    ASSERT_DOUBLE_EQ(j.at("a").as<JsonNumber>(), 100.0);
    ASSERT_DOUBLE_EQ(j.at("b").as<JsonNumber>(), 200.0);
}

TEST(iterator_null_array_throws) {
    JsonValue j;
    ASSERT_THROW(j.as_array(), std::runtime_error);
}

TEST(iterator_null_object_throws) {
    JsonValue j;
    ASSERT_THROW(j.as_object(), std::runtime_error);
}

TEST(iterator_bool_array_throws) {
    JsonValue j(true);
    ASSERT_THROW(j.as_array(), std::runtime_error);
}

TEST(iterator_number_object_throws) {
    JsonValue j(42);
    ASSERT_THROW(j.as_object(), std::runtime_error);
}

TEST(iterator_string_array_throws) {
    JsonValue j("hello");
    ASSERT_THROW(j.as_array(), std::runtime_error);
}

TEST(iterator_stl_algorithm) {
    JsonValue j = array({3, 1, 4, 1, 5, 9, 2, 6});
    auto &arr = j.as_array();
    auto it = std::min_element(arr.begin(), arr.end(),
        [](const JsonValue &a, const JsonValue &b) {
            return a.as<JsonNumber>() < b.as<JsonNumber>();
        });
    ASSERT_DOUBLE_EQ(it->as<JsonNumber>(), 1.0);
}

TEST(iterator_stl_count_if) {
    JsonValue j = array({1, 2, 3, 4, 5, 6});
    auto &arr = j.as_array();
    auto even_count = std::count_if(arr.begin(), arr.end(),
        [](const JsonValue &v) {
            return static_cast<int>(v.as<JsonNumber>()) % 2 == 0;
        });
    ASSERT_EQ(even_count, 3);
}


// ============================================
// emplace_back 测试
// ============================================

TEST(emplace_back_basic) {
    JsonValue j = array({});
    j.emplace_back(42);
    j.emplace_back("hello");
    j.emplace_back(true);
    ASSERT_EQ(j.size(), 3);
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 42.0);
    ASSERT_EQ(j.at(1).as_string(), "hello");
    ASSERT_TRUE(j.at(2).as<JsonBool>());
}

TEST(emplace_back_from_null) {
    JsonValue j; // null
    j.emplace_back(1);
    ASSERT_EQ(j.type(), JsonValueType::Array);
    ASSERT_EQ(j.size(), 1);
}

TEST(emplace_back_type_error) {
    JsonValue j = true;
    ASSERT_THROW(j.emplace_back(1), std::runtime_error);
}

// ============================================
// JSON Pointer (RFC 6901) 测试
// ============================================

TEST(json_pointer_root) {
    JsonValue j = object({{"a", 1}});
    // 空指针引用根值自身
    auto &root = j.resolve("");
    ASSERT_TRUE(root.is<JsonObject>());
    ASSERT_DOUBLE_EQ(root.at("a").as<JsonNumber>(), 1.0);
}

TEST(json_pointer_simple_path) {
    JsonValue j = object({
        {"a", object({
            {"b", object({
                {"c", 42}
            })}
        })}
    });
    ASSERT_DOUBLE_EQ(j.resolve("/a/b/c").as<JsonNumber>(), 42.0);
}

TEST(json_pointer_array_index) {
    JsonValue j = array({10, 20, 30});
    ASSERT_DOUBLE_EQ(j.resolve("/1").as<JsonNumber>(), 20.0);
}

TEST(json_pointer_nested_array) {
    JsonValue j = object({
        {"data", array({1, 2, array({10, 20, 30})})}
    });
    ASSERT_DOUBLE_EQ(j.resolve("/data/2/1").as<JsonNumber>(), 20.0);
}

TEST(json_pointer_string_value) {
    JsonValue j = object({
        {"user", object({
            {"name", "Alice"},
            {"age", 30}
        })}
    });
    ASSERT_EQ(j.resolve("/user/name").as_string(), "Alice");
}

TEST(json_pointer_escaped_tilde) {
    // ~0 代表 ~
    JsonValue j = object({
        {"a~b", "tilde_value"}
    });
    ASSERT_EQ(j.resolve("/a~0b").as_string(), "tilde_value");
}

TEST(json_pointer_escaped_slash) {
    // ~1 代表 /
    JsonValue j = object({
        {"a/b", "slash_value"}
    });
    ASSERT_EQ(j.resolve("/a~1b").as_string(), "slash_value");
}

TEST(json_pointer_escaped_both) {
    // 同时包含 ~0 和 ~1
    JsonValue j = object({
        {"a/~b", "both_value"}
    });
    ASSERT_EQ(j.resolve("/a~1~0b").as_string(), "both_value");
}

TEST(json_pointer_modify_value) {
    JsonValue j = object({
        {"a", object({
            {"b", 1}
        })}
    });
    j.resolve("/a/b") = JsonValue(99);
    ASSERT_DOUBLE_EQ(j.resolve("/a/b").as<JsonNumber>(), 99.0);
}

TEST(json_pointer_const_access) {
    const JsonValue j = object({
        {"x", array({1, 2, 3})}
    });
    ASSERT_DOUBLE_EQ(j.resolve("/x/2").as<JsonNumber>(), 3.0);
}

TEST(json_pointer_key_not_found) {
    JsonValue j = object({{"a", 1}});
    ASSERT_THROW(j.resolve("/b"), std::out_of_range);
}

TEST(json_pointer_array_out_of_range) {
    JsonValue j = array({1, 2, 3});
    ASSERT_THROW(j.resolve("/5"), std::out_of_range);
}

TEST(json_pointer_invalid_array_index) {
    JsonValue j = object({{"a", array({1})}});
    ASSERT_THROW(j.resolve("/a/abc"), std::out_of_range);
}

TEST(json_pointer_traverse_into_scalar) {
    JsonValue j = object({{"a", 42}});
    ASSERT_THROW(j.resolve("/a/b"), std::runtime_error);
}

TEST(json_pointer_invalid_format) {
    JsonValue j = object({{"a", 1}});
    ASSERT_THROW(j.resolve("a/b"), std::runtime_error);
}

// ============================================
// JSON Merge Patch (RFC 7396) 测试
// ============================================

TEST(merge_add_new_keys) {
    JsonValue target = object({{"a", 1}});
    JsonValue patch = object({{"b", 2}});
    target.merge(patch);
    ASSERT_DOUBLE_EQ(target.at("a").as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(target.at("b").as<JsonNumber>(), 2.0);
}

TEST(merge_update_existing_key) {
    JsonValue target = object({{"a", 1}, {"b", 2}});
    JsonValue patch = object({{"b", 99}});
    target.merge(patch);
    ASSERT_DOUBLE_EQ(target.at("a").as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(target.at("b").as<JsonNumber>(), 99.0);
}

TEST(merge_remove_key_with_null) {
    JsonValue target = object({{"a", 1}, {"b", 2}, {"c", 3}});
    JsonValue patch = object({{"b", nullptr}});
    target.merge(patch);
    ASSERT_TRUE(target.contains("a"));
    ASSERT_FALSE(target.contains("b"));
    ASSERT_TRUE(target.contains("c"));
}

TEST(merge_nested_object) {
    JsonValue target = object({
        {"a", object({{"x", 1}, {"y", 2}})}
    });
    JsonValue patch = object({
        {"a", object({{"y", 99}, {"z", 3}})}
    });
    target.merge(patch);
    ASSERT_DOUBLE_EQ(target.at("a").at("x").as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(target.at("a").at("y").as<JsonNumber>(), 99.0);
    ASSERT_DOUBLE_EQ(target.at("a").at("z").as<JsonNumber>(), 3.0);
}

TEST(merge_replace_non_object) {
    JsonValue target = object({{"a", 1}});
    JsonValue patch = JsonValue(42);
    target.merge(patch);
    ASSERT_TRUE(target.is<JsonNumber>());
    ASSERT_DOUBLE_EQ(target.as<JsonNumber>(), 42.0);
}

TEST(merge_patch_not_object) {
    JsonValue target = object({{"a", 1}});
    JsonValue patch = "hello";
    target.merge(patch);
    ASSERT_TRUE(target.is<JsonString>());
    ASSERT_EQ(target.as_string(), "hello");
}

TEST(merge_target_not_object) {
    JsonValue target = 42;
    JsonValue patch = object({{"a", 1}});
    target.merge(patch);
    ASSERT_TRUE(target.is<JsonObject>());
    ASSERT_DOUBLE_EQ(target.at("a").as<JsonNumber>(), 1.0);
}

TEST(merge_array_values) {
    JsonValue target = object({{"arr", array({1, 2, 3})}});
    JsonValue patch = object({{"arr", array({10, 20})}});
    target.merge(patch);
    ASSERT_EQ(target.at("arr").size(), 2);
    ASSERT_DOUBLE_EQ(target.at("arr").at(0).as<JsonNumber>(), 10.0);
}

TEST(merge_deep_nested) {
    JsonValue target = object({
        {"a", object({
            {"b", object({
                {"c", 1}, {"d", 2}
            })}
        })}
    });
    JsonValue patch = object({
        {"a", object({
            {"b", object({
                {"d", nullptr}
            })}
        })}
    });
    target.merge(patch);
    ASSERT_DOUBLE_EQ(target.at("a").at("b").at("c").as<JsonNumber>(), 1.0);
    ASSERT_FALSE(target.at("a").at("b").contains("d"));
}

TEST(merge_empty_patch) {
    JsonValue target = object({{"a", 1}, {"b", 2}});
    JsonValue patch = object({});
    target.merge(patch);
    ASSERT_EQ(target.size(), 2);
    ASSERT_DOUBLE_EQ(target.at("a").as<JsonNumber>(), 1.0);
}

// ============================================
// 解析器深度限制测试
// ============================================

TEST(parse_max_depth_default) {
    // 默认深度 256，正常嵌套应该成功
    std::string json = R"({"a":{"b":{"c":1}}})";
    JsonValue j = parse(json);
    ASSERT_DOUBLE_EQ(j.resolve("/a/b/c").as<JsonNumber>(), 1.0);
}

TEST(parse_max_depth_custom) {
    // 自定义深度限制为 3
    std::string json = R"({"a":{"b":{"c":{"d":1}}}})";
    ASSERT_THROW(parse(json, 3), std::runtime_error);
}

TEST(parse_max_depth_exactly_at_limit) {
    // 刚好在深度限制内应该成功
    std::string json = R"({"a":{"b":{"c":1}}})";
    JsonValue j = parse(json, 3);
    ASSERT_DOUBLE_EQ(j.resolve("/a/b/c").as<JsonNumber>(), 1.0);
}

TEST(parse_max_depth_array) {
    // 数组也受深度限制
    std::string json = R"([[[[1]]]])";
    ASSERT_THROW(parse(json, 3), std::runtime_error);
}

TEST(parse_max_depth_mixed) {
    // 混合嵌套
    std::string json = R"({"a":[[1]]})";
    ASSERT_THROW(parse(json, 2), std::runtime_error);
}

// ============================================
// 数字序列化格式测试
// ============================================

TEST(serialize_number_nan) {
    // NaN 不符合 JSON 规范，序列化为 null
    JsonValue j(std::numeric_limits<double>::quiet_NaN());
    ASSERT_EQ(j.serialize(), "null");
}

TEST(serialize_number_inf) {
    JsonValue j(std::numeric_limits<double>::infinity());
    ASSERT_EQ(j.serialize(), "null");

    JsonValue j2(-std::numeric_limits<double>::infinity());
    ASSERT_EQ(j2.serialize(), "null");
}

TEST(serialize_number_integer) {
    // 整数应省略小数点
    ASSERT_EQ(JsonValue(0.0).serialize(), "0");
    ASSERT_EQ(JsonValue(42.0).serialize(), "42");
    ASSERT_EQ(JsonValue(-100.0).serialize(), "-100");
}

TEST(serialize_number_decimal) {
    // 小数应保留必要位数，去除尾随零
    std::string result = JsonValue(3.14).serialize();
    ASSERT_EQ(result, "3.14");

    std::string result2 = JsonValue(1.50).serialize();
    ASSERT_EQ(result2, "1.5");
}

TEST(serialize_number_scientific) {
    // 极大/极小数应使用科学计数法
    std::string result = JsonValue(1e20).serialize();
    ASSERT_TRUE(result.find('e') != std::string::npos || result.find('E') != std::string::npos);

    std::string result2 = JsonValue(1e-20).serialize();
    ASSERT_TRUE(result2.find('e') != std::string::npos || result2.find('E') != std::string::npos);
}

TEST(serialize_number_roundtrip) {
    // 数字往返一致性
    std::string json = R"({"a":3.14,"b":1e20,"c":0.001})";
    JsonValue j = parse(json);
    std::string serialized = j.serialize();
    JsonValue j2 = parse(serialized);

    ASSERT_DOUBLE_EQ(j2.at("a").as<JsonNumber>(), 3.14);
    ASSERT_DOUBLE_EQ(j2.at("b").as<JsonNumber>(), 1e20);
    ASSERT_DOUBLE_EQ(j2.at("c").as<JsonNumber>(), 0.001);
}

// ============================================
// typeName 测试
// ============================================

TEST(type_name_returns_correct_strings) {
    ASSERT_EQ(JsonValue().typeName(), "null");
    ASSERT_EQ(JsonValue(true).typeName(), "bool");
    ASSERT_EQ(JsonValue(3.14).typeName(), "number");
    ASSERT_EQ(JsonValue("hello").typeName(), "string");
    ASSERT_EQ(array({}).typeName(), "array");
    ASSERT_EQ(object({}).typeName(), "object");
}

// ============================================
// 移动语义测试
// ============================================

TEST(move_string) {
    JsonValue j1("hello world");
    JsonValue j2(std::move(j1.as<JsonString>()));
    ASSERT_EQ(j2.as_string(), "hello world");
}

TEST(move_array) {
    JsonArray arr = {1, 2, 3};
    JsonValue j1(arr);
    JsonArray moved_arr = std::move(j1.as<JsonArray>());
    JsonValue j2(moved_arr);
    ASSERT_EQ(j2.size(), 3);
    ASSERT_DOUBLE_EQ(j2.at(0).as<JsonNumber>(), 1.0);
}

TEST(move_object) {
    JsonValue j1 = object({{"key", "value"}});
    JsonObject moved_obj = std::move(j1.as<JsonObject>());
    JsonValue j2(moved_obj);
    ASSERT_EQ(j2.size(), 1);
    ASSERT_EQ(j2.at("key").as_string(), "value");
}

// ============================================
// const 访问器测试
// ============================================

TEST(const_array_access) {
    const JsonValue j = array({10, 20, 30});
    ASSERT_EQ(j.size(), 3);
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 10.0);
    ASSERT_DOUBLE_EQ(j.at(2).as<JsonNumber>(), 30.0);
    ASSERT_TRUE(j.is<JsonArray>());
    ASSERT_THROW(j.at(3), std::out_of_range);
}

TEST(const_object_access) {
    const JsonValue j = object({{"name", "Alice"}, {"age", 30}});
    ASSERT_EQ(j.size(), 2);
    ASSERT_EQ(j.at("name").as_string(), "Alice");
    ASSERT_TRUE(j.contains("name"));
    ASSERT_FALSE(j.contains("missing"));
    ASSERT_THROW(j.at("missing"), std::out_of_range);
}

// ============================================
// 更多解析器错误路径测试
// ============================================

TEST(parse_error_invalid_escape) {
    ASSERT_THROW(parse("\"\\x\""), std::runtime_error);
}

TEST(parse_error_mismatched_brackets) {
    ASSERT_THROW(parse("{]"), std::runtime_error);
    ASSERT_THROW(parse("[}"), std::runtime_error);
}

TEST(parse_error_missing_colon) {
    ASSERT_THROW(parse("{\"a\" 1}"), std::runtime_error);
}

TEST(parse_error_trailing_comma_array) {
    // 标准 JSON 不允许尾逗号
    ASSERT_THROW(parse("[1,2,]"), std::runtime_error);
}

TEST(parse_error_trailing_comma_object) {
    ASSERT_THROW(parse("{\"a\":1,}"), std::runtime_error);
}

TEST(parse_error_invalid_unicode_escape) {
    ASSERT_THROW(parse("\"\\u00GG\""), std::runtime_error);
}

TEST(parse_error_number_with_trailing_chars) {
    // "123abc" 不是合法的 JSON（顶级值必须是完整 token）
    ASSERT_THROW(parse("123abc"), std::runtime_error);
}

// ============================================
// UTF-8 验证测试
// ============================================

TEST(parse_valid_utf8) {
    // 合法的 UTF-8 应该能正常解析
    std::string json = R"("Hello 你好 こんにちは 🌍")";
    JsonValue j = parse(json);
    ASSERT_TRUE(j.is<JsonString>());
}

TEST(parse_invalid_utf8_continuation) {
    // 非法的后续字节：0x80 单独出现不是合法的 UTF-8
    std::string json = "\"\x80\"";
    ASSERT_THROW(parse(json), std::runtime_error);
}

TEST(parse_invalid_utf8_overlong) {
    // 过长编码：0xC0 0x80 是 null 的过长表示
    std::string json = "\"\xC0\x80\"";
    ASSERT_THROW(parse(json), std::runtime_error);
}

TEST(parse_invalid_utf8_surrogate) {
    // 代理对不应该出现在 UTF-8 中
    // 0xED 0xA0 0x80 是 U+D800 的 UTF-8 编码
    std::string json = "\"\xED\xA0\x80\"";
    ASSERT_THROW(parse(json), std::runtime_error);
}

// ============================================
// 数字格式化边界测试
// ============================================

TEST(number_nan_inf) {
    // NaN 和 Inf 不符合 JSON 规范，序列化为 null
    JsonValue j_nan(std::numeric_limits<double>::quiet_NaN());
    ASSERT_EQ(j_nan.serialize(), "null");

    JsonValue j_inf(std::numeric_limits<double>::infinity());
    ASSERT_EQ(j_inf.serialize(), "null");

    JsonValue j_neg_inf(-std::numeric_limits<double>::infinity());
    ASSERT_EQ(j_neg_inf.serialize(), "null");
}

TEST(number_integer_formatting) {
    // 整数应省略小数点
    ASSERT_EQ(JsonValue(0.0).serialize(), "0");
    ASSERT_EQ(JsonValue(42.0).serialize(), "42");
    ASSERT_EQ(JsonValue(-100.0).serialize(), "-100");

    // 非整数应保留小数点
    JsonValue j(3.14);
    ASSERT_TRUE(j.serialize().find('.') != std::string::npos);
}

TEST(number_int_constructor) {
    // int 构造应转为 double
    JsonValue j(42);
    ASSERT_TRUE(j.is<JsonNumber>());
    ASSERT_DOUBLE_EQ(j.as<JsonNumber>(), 42.0);
    ASSERT_EQ(j.serialize(), "42");
}

// ============================================
// 往返一致性测试
// ============================================

TEST(round_trip_simple) {
    std::string original = R"({"name":"Alice","age":30,"active":true,"score":95.5})";
    JsonValue parsed = parse(original);
    std::string serialized = parsed.serialize();
    JsonValue reparsed = parse(serialized);

    ASSERT_EQ(reparsed.at("name").as_string(), "Alice");
    ASSERT_DOUBLE_EQ(reparsed.at("age").as<JsonNumber>(), 30.0);
    ASSERT_TRUE(reparsed.at("active").as<JsonBool>());
    ASSERT_DOUBLE_EQ(reparsed.at("score").as<JsonNumber>(), 95.5);
}

TEST(round_trip_nested) {
    std::string original = R"({"a":{"b":[1,2,3]},"c":"hello"})";
    JsonValue parsed = parse(original);
    std::string serialized = parsed.serialize();
    JsonValue reparsed = parse(serialized);

    ASSERT_EQ(reparsed.at("a").at("b").at(1).as<JsonNumber>(), 2.0);
    ASSERT_EQ(reparsed.at("c").as_string(), "hello");
}

TEST(round_trip_with_escapes) {
    std::string original = R"({"msg":"say \"hello\"\n\tworld"})";
    JsonValue parsed = parse(original);
    std::string serialized = parsed.serialize();
    JsonValue reparsed = parse(serialized);

    ASSERT_EQ(reparsed.at("msg").as_string(), "say \"hello\"\n\tworld");
}

// ============================================
// serialize 格式化测试补充
// ============================================

TEST(serialize_pretty_nested) {
    JsonValue j = object({
        {"name", "Alice"},
        {"scores", array({95, 88, 92})},
        {"active", true}
    });

    std::string pretty = j.serialize(4);
    // 应包含缩进
    ASSERT_TRUE(pretty.find("    ") != std::string::npos);
    // 应包含换行
    ASSERT_TRUE(pretty.find('\n') != std::string::npos);
}

TEST(serialize_empty_pretty) {
    ASSERT_EQ(array({}).serialize(2), "[]");
    ASSERT_EQ(object({}).serialize(2), "{}");
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

    // 数组大小限制测试
    RUN_TEST(array_size_limit_normal);
    RUN_TEST(array_size_limit_exceeded);

    // 迭代器测试
    RUN_TEST(iterator_array_range_for);
    RUN_TEST(iterator_array_const);
    RUN_TEST(iterator_array_modify);
    RUN_TEST(iterator_object_range_for);
    RUN_TEST(iterator_object_const);
    RUN_TEST(iterator_object_modify_values);
    RUN_TEST(iterator_null_array_throws);
    RUN_TEST(iterator_null_object_throws);
    RUN_TEST(iterator_bool_array_throws);
    RUN_TEST(iterator_number_object_throws);
    RUN_TEST(iterator_string_array_throws);
    RUN_TEST(iterator_stl_algorithm);
    RUN_TEST(iterator_stl_count_if);

    // emplace_back 测试
    // JSON Pointer 测试
    RUN_TEST(json_pointer_root);
    RUN_TEST(json_pointer_simple_path);
    RUN_TEST(json_pointer_array_index);
    RUN_TEST(json_pointer_nested_array);
    RUN_TEST(json_pointer_string_value);
    RUN_TEST(json_pointer_escaped_tilde);
    RUN_TEST(json_pointer_escaped_slash);
    RUN_TEST(json_pointer_escaped_both);
    RUN_TEST(json_pointer_modify_value);
    RUN_TEST(json_pointer_const_access);
    RUN_TEST(json_pointer_key_not_found);
    RUN_TEST(json_pointer_array_out_of_range);
    RUN_TEST(json_pointer_invalid_array_index);
    RUN_TEST(json_pointer_traverse_into_scalar);
    RUN_TEST(json_pointer_invalid_format);

    // JSON Merge Patch 测试
    RUN_TEST(merge_add_new_keys);
    RUN_TEST(merge_update_existing_key);
    RUN_TEST(merge_remove_key_with_null);
    RUN_TEST(merge_nested_object);
    RUN_TEST(merge_replace_non_object);
    RUN_TEST(merge_patch_not_object);
    RUN_TEST(merge_target_not_object);
    RUN_TEST(merge_array_values);
    RUN_TEST(merge_deep_nested);
    RUN_TEST(merge_empty_patch);

    // 解析器深度限制测试
    RUN_TEST(parse_max_depth_default);
    RUN_TEST(parse_max_depth_custom);
    RUN_TEST(parse_max_depth_exactly_at_limit);
    RUN_TEST(parse_max_depth_array);
    RUN_TEST(parse_max_depth_mixed);

    // 数字序列化格式测试
    RUN_TEST(serialize_number_nan);
    RUN_TEST(serialize_number_inf);
    RUN_TEST(serialize_number_integer);
    RUN_TEST(serialize_number_decimal);
    RUN_TEST(serialize_number_scientific);
    RUN_TEST(serialize_number_roundtrip);

    RUN_TEST(emplace_back_basic);
    RUN_TEST(emplace_back_from_null);
    RUN_TEST(emplace_back_type_error);

    // typeName 测试
    RUN_TEST(type_name_returns_correct_strings);

    // 移动语义测试
    RUN_TEST(move_string);
    RUN_TEST(move_array);
    RUN_TEST(move_object);

    // const 访问器测试
    RUN_TEST(const_array_access);
    RUN_TEST(const_object_access);

    // 更多解析器错误路径测试
    RUN_TEST(parse_error_invalid_escape);
    RUN_TEST(parse_error_mismatched_brackets);
    RUN_TEST(parse_error_missing_colon);
    RUN_TEST(parse_error_trailing_comma_array);
    RUN_TEST(parse_error_trailing_comma_object);
    RUN_TEST(parse_error_invalid_unicode_escape);
    RUN_TEST(parse_error_number_with_trailing_chars);

    // UTF-8 验证测试
    RUN_TEST(parse_valid_utf8);
    RUN_TEST(parse_invalid_utf8_continuation);
    RUN_TEST(parse_invalid_utf8_overlong);
    RUN_TEST(parse_invalid_utf8_surrogate);

    // 数字格式化边界测试
    RUN_TEST(number_nan_inf);
    RUN_TEST(number_integer_formatting);
    RUN_TEST(number_int_constructor);

    // 往返一致性测试
    RUN_TEST(round_trip_simple);
    RUN_TEST(round_trip_nested);
    RUN_TEST(round_trip_with_escapes);

    // serialize 格式化补充测试
    RUN_TEST(serialize_pretty_nested);
    RUN_TEST(serialize_empty_pretty);

    std::cout << "\n==========================\n";
    std::cout << "All tests passed successfully!\n";

    return 0;
}
