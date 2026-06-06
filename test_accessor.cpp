#include "json.h"
#include "test.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace json;

// ============================================
// 类型构造
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
    JsonValue j   = arr;
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
// 数组访问与修改
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

TEST(array_size_limit_normal) {
    JsonValue j;
    j[100] = 42;
    ASSERT_EQ(j.size(), 101);
    ASSERT_DOUBLE_EQ(j.at(100).as<JsonNumber>(), 42.0);
}

TEST(array_size_limit_exceeded) {
    JsonValue j;
    ASSERT_THROW(j[1000000] = 1, std::out_of_range);
}

// ============================================
// 对象访问与修改
// ============================================

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
    JsonValue j; // null
    j[0] = 1;
    ASSERT_EQ(j.type(), JsonValueType::Array);
    ASSERT_EQ(j.size(), 1);

    JsonValue j2;
    j2["key"] = "value";
    ASSERT_EQ(j2.type(), JsonValueType::Object);
    ASSERT_EQ(j2.size(), 1);
}

// ============================================
// emplace_back
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
    JsonValue j;
    j.emplace_back(1);
    ASSERT_EQ(j.type(), JsonValueType::Array);
    ASSERT_EQ(j.size(), 1);
}

TEST(emplace_back_type_error) {
    JsonValue j = true;
    ASSERT_THROW(j.emplace_back(1), std::runtime_error);
}

// ============================================
// 类型检查
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
    ASSERT_THROW(j.size(), std::runtime_error);

    JsonValue j_arr = array({});
    ASSERT_THROW(j_arr.at("key"), std::runtime_error);

    JsonValue j_obj = object({});
    ASSERT_THROW(j_obj.at(0), std::runtime_error);
}

TEST(type_name) {
    ASSERT_EQ(JsonValue().type_name(), "null");
    ASSERT_EQ(JsonValue(true).type_name(), "bool");
    ASSERT_EQ(JsonValue(3.14).type_name(), "number");
    ASSERT_EQ(JsonValue("hello").type_name(), "string");
    ASSERT_EQ(array({}).type_name(), "array");
    ASSERT_EQ(object({}).type_name(), "object");
}

// ============================================
// const 访问器
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
// 移动语义
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
    JsonValue  j1        = object({{"key", "value"}});
    JsonObject moved_obj = std::move(j1.as<JsonObject>());
    JsonValue  j2(moved_obj);
    ASSERT_EQ(j2.size(), 1);
    ASSERT_EQ(j2.at("key").as_string(), "value");
}

// ============================================
// for_each 遍历
// ============================================

TEST(foreach_array) {
    JsonValue           j = array({1, 2, 3});
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

TEST(foreach_const_and_mut) {
    JsonValue j = array({1, 2, 3});

    // 非 const 版本：修改元素
    j.for_each_array([](JsonValue &elem) {
        elem = JsonValue(elem.as<JsonNumber>() * 10);
    });

    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 10.0);
    ASSERT_DOUBLE_EQ(j.at(1).as<JsonNumber>(), 20.0);
    ASSERT_DOUBLE_EQ(j.at(2).as<JsonNumber>(), 30.0);

    // const 版本：只读
    const JsonValue &   const_j = j;
    std::vector<double> values;

    const_j.for_each_array([&](const JsonValue &elem) {
        values.push_back(elem.as<JsonNumber>());
    });

    ASSERT_DOUBLE_EQ(values[0], 10.0);
    ASSERT_DOUBLE_EQ(values[1], 20.0);
    ASSERT_DOUBLE_EQ(values[2], 30.0);
}

// ============================================
// for_each with UTF-8 data
// ============================================

TEST(foreach_array_utf8) {
    const JsonValue j = array({
            JsonValue(std::u8string(u8"苹果 🍎")),
            JsonValue(std::u8string(u8"香蕉 🍌")),
            JsonValue(std::u8string(u8"橙子 🍊")),
            JsonValue(std::u8string(u8"葡萄 🍇"))
    });

    std::vector<std::string> fruits;

    j.for_each_array([&](const JsonValue &elem) {
        fruits.push_back(elem.as_string());
    });

    ASSERT_EQ(fruits.size(), 4);
    ASSERT_EQ(fruits[0], "苹果 🍎");
    ASSERT_EQ(fruits[1], "香蕉 🍌");
    ASSERT_EQ(fruits[2], "橙子 🍊");
    ASSERT_EQ(fruits[3], "葡萄 🍇");
}

TEST(foreach_object_utf8) {
    const JsonValue j = object({
            {"フルーツ", "りんご"},
            {"やさい", "にんじん"},
            {"にく", "ぎゅうにく"}
    });

    std::vector<std::pair<std::string, std::string> > items;

    j.for_each_object([&](const JsonString &key, const JsonValue &val) {
        items.emplace_back(util::to_std_string(key), val.as_string());
    });

    ASSERT_EQ(items.size(), 3);
    bool found = false;
    for (const auto &[key, value]: items) {
        if (key == "フルーツ" && value == "りんご") {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
}

TEST(foreach_modify_utf8_array) {
    JsonValue j = array({
            JsonValue(std::u8string(u8"hello")),
            JsonValue(std::u8string(u8"world")),
            JsonValue(std::u8string(u8"你好"))
    });

    j.for_each_array([](JsonValue &elem) {
        if (elem.is<JsonString>()) {
            std::string s = elem.as_string();
            std::ranges::transform(s, s.begin(),
                                   [](unsigned char c) { return std::toupper(c); });
            elem = JsonValue(s);
        }
    });

    ASSERT_EQ(j.at(0).as_string(), "HELLO");
    ASSERT_EQ(j.at(1).as_string(), "WORLD");
    ASSERT_EQ(j.at(2).as_string(), "你好");
}

// ============================================
// 迭代器（range-based for）
// ============================================

TEST(iterator_array_range_for) {
    JsonValue j   = array({1, 2, 3, 4, 5});
    double    sum = 0;
    for (auto &elem: j.as_array()) {
        sum += elem.as<JsonNumber>();
    }
    ASSERT_DOUBLE_EQ(sum, 15.0);
}

TEST(iterator_array_const) {
    const JsonValue     j = array({10, 20, 30});
    std::vector<double> values;
    for (const auto &elem: j.as_array()) {
        values.push_back(elem.as<JsonNumber>());
    }
    ASSERT_EQ(values.size(), 3);
    ASSERT_DOUBLE_EQ(values[0], 10.0);
    ASSERT_DOUBLE_EQ(values[1], 20.0);
    ASSERT_DOUBLE_EQ(values[2], 30.0);
}

TEST(iterator_array_modify) {
    JsonValue j = array({1, 2, 3});
    for (auto &elem: j.as_array()) {
        elem = JsonValue(elem.as<JsonNumber>() * 10);
    }
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 10.0);
    ASSERT_DOUBLE_EQ(j.at(1).as<JsonNumber>(), 20.0);
    ASSERT_DOUBLE_EQ(j.at(2).as<JsonNumber>(), 30.0);
}

TEST(iterator_object_range_for) {
    JsonValue j   = object({{"a", 1}, {"b", 2}, {"c", 3}});
    double    sum = 0;
    for (auto &[key, value]: j.as_object()) {
        sum += value.as<JsonNumber>();
    }
    ASSERT_DOUBLE_EQ(sum, 6.0);
}

TEST(iterator_object_const) {
    const JsonValue j     = object({{"x", 10}, {"y", 20}});
    int             count = 0;
    for (const auto &[key, value]: j.as_object()) {
        (void) key;
        count++;
    }
    ASSERT_EQ(count, 2);
}

TEST(iterator_object_modify_values) {
    JsonValue j = object({{"a", 1}, {"b", 2}});
    for (auto &[key, value]: j.as_object()) {
        value = JsonValue(value.as<JsonNumber>() * 100);
    }
    ASSERT_DOUBLE_EQ(j.at("a").as<JsonNumber>(), 100.0);
    ASSERT_DOUBLE_EQ(j.at("b").as<JsonNumber>(), 200.0);
}

TEST(iterator_type_mismatch_throws) {
    // null
    JsonValue j_null;
    ASSERT_THROW(j_null.as_array(), std::runtime_error);
    ASSERT_THROW(j_null.as_object(), std::runtime_error);

    // bool
    JsonValue j_bool(true);
    ASSERT_THROW(j_bool.as_array(), std::runtime_error);
    ASSERT_THROW(j_bool.as_object(), std::runtime_error);

    // number
    JsonValue j_num(42);
    ASSERT_THROW(j_num.as_array(), std::runtime_error);
    ASSERT_THROW(j_num.as_object(), std::runtime_error);

    // string
    JsonValue j_str("hello");
    ASSERT_THROW(j_str.as_array(), std::runtime_error);
    ASSERT_THROW(j_str.as_object(), std::runtime_error);
}

TEST(iterator_stl_algorithm) {
    JsonValue j   = array({3, 1, 4, 1, 5, 9, 2, 6});
    auto &    arr = j.as_array();
    auto      it  = std::min_element(arr.begin(), arr.end(),
                               [](const JsonValue &a, const JsonValue &b) {
                                   return a.as<JsonNumber>() < b.as<JsonNumber>();
                               });
    ASSERT_DOUBLE_EQ(it->as<JsonNumber>(), 1.0);
}

TEST(iterator_stl_count_if) {
    JsonValue j          = array({1, 2, 3, 4, 5, 6});
    auto &    arr        = j.as_array();
    auto      even_count = std::count_if(arr.begin(), arr.end(),
                                    [](const JsonValue &v) {
                                        return static_cast<int>(v.as<JsonNumber>()) % 2 == 0;
                                    });
    ASSERT_EQ(even_count, 3);
}

// ============================================
// UTF-8 对象键名
// ============================================

TEST(object_keys_chinese) {
    JsonValue j = object({
            {"姓名", "张三"},
            {"年龄", 25},
            {"城市", "北京"}
    });

    std::string serialized = j.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.at(u8"姓名").as_string(), "张三");
    ASSERT_DOUBLE_EQ(parsed.at(u8"年龄").as<JsonNumber>(), 25.0);
    ASSERT_EQ(parsed.at(u8"城市").as_string(), "北京");
}

TEST(object_keys_mixed_languages) {
    JsonValue j = object({
            {"name", "Alice"},
            {"名前", "アリス"},
            {"이름", "앨리스"},
            {"姓名", "爱丽丝"},
            {"الاسم", "أليس"}
    });

    std::string serialized = j.serialize();
    JsonValue   parsed     = parse(serialized);

    ASSERT_EQ(parsed.at(u8"name").as_string(), "Alice");
    ASSERT_EQ(parsed.at(u8"名前").as_string(), "アリス");
    ASSERT_EQ(parsed.at(u8"이름").as_string(), "앨리스");
    ASSERT_EQ(parsed.at(u8"姓名").as_string(), "爱丽丝");
    ASSERT_EQ(parsed.at(u8"الاسم").as_string(), "أليس");
}

TEST(object_keys_emoji) {
    JsonValue j = object({
            {"😀", "happy"},
            {"🌟", "star"},
            {"🎵", "music"}
    });

    std::string serialized = j.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.at(u8"😀").as_string(), "happy");
    ASSERT_EQ(parsed.at(u8"🌟").as_string(), "star");
    ASSERT_EQ(parsed.at(u8"🎵").as_string(), "music");
}

// ============================================
// UTF-8 size
// ============================================

TEST(utf8_string_size) {
    JsonValue j(std::u8string(u8"Hello世界"));
    ASSERT_EQ(j.size(), 7); // 5 ASCII + 2 中文字符

    JsonValue j2(std::u8string(u8"🍎🍊🍋"));
    ASSERT_TRUE(j2.size() > 0);
}

TEST(utf8_array_multilingual) {
    JsonValue j = array({
            JsonValue(std::u8string(u8"Hello")),
            JsonValue(std::u8string(u8"你好")),
            JsonValue(std::u8string(u8"こんにちは")),
            JsonValue(std::u8string(u8"안녕하세요")),
            JsonValue(std::u8string(u8"مرحبا")),
            JsonValue(std::u8string(u8"😀🌍"))
    });

    std::string serialized = j.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.size(), 6);
    ASSERT_EQ(parsed.at(0).as_string(), "Hello");
    ASSERT_EQ(parsed.at(1).as_string(), "你好");
    ASSERT_EQ(parsed.at(2).as_string(), "こんにちは");
    ASSERT_EQ(parsed.at(3).as_string(), "안녕하세요");
    ASSERT_EQ(parsed.at(4).as_string(), "مرحبا");
    ASSERT_EQ(parsed.at(5).as_string(), "😀🌍");
}

// ============================================
// copy/move 语义
// ============================================

TEST(copy_construct_independent) {
    JsonValue original = object({{"a", 1}, {"b", array({1, 2, 3})}});
    JsonValue copy     = original;

    // 修改 copy 不影响 original
    copy["a"]    = 99;
    copy["b"][0] = 100;

    ASSERT_DOUBLE_EQ(original.at("a").as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(original.at("b").at(0).as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(copy.at("a").as<JsonNumber>(), 99.0);
    ASSERT_DOUBLE_EQ(copy.at("b").at(0).as<JsonNumber>(), 100.0);
}

TEST(copy_assign_independent) {
    JsonValue original = array({10, 20, 30});
    JsonValue copy;
    copy = original;

    copy[0] = 999;
    ASSERT_DOUBLE_EQ(original.at(0).as<JsonNumber>(), 10.0);
    ASSERT_DOUBLE_EQ(copy.at(0).as<JsonNumber>(), 999.0);
}

TEST(move_construct) {
    JsonValue src = object({{"key", "value"}});
    JsonValue dst = std::move(src);

    ASSERT_EQ(dst.at("key").as_string(), "value");
    ASSERT_TRUE(dst.is<JsonObject>());
}

TEST(move_assign) {
    JsonValue src = array({1, 2, 3});
    JsonValue dst;
    dst = std::move(src);

    ASSERT_EQ(dst.size(), 3);
    ASSERT_DOUBLE_EQ(dst.at(0).as<JsonNumber>(), 1.0);
}

TEST(copy_construct_string) {
    JsonString s = u8"hello world";
    JsonValue  j1(s);
    JsonValue  j2(j1);

    ASSERT_EQ(j2.as_string(), "hello world");
    // 修改 j2 不影响 j1
    j2 = JsonValue("changed");
    ASSERT_EQ(j1.as_string(), "hello world");
}

TEST(move_construct_string) {
    JsonString s = u8"moved string";
    JsonValue  j1(std::move(s));
    ASSERT_EQ(j1.as_string(), "moved string");
}

TEST(move_construct_array) {
    JsonArray arr = {1, 2, 3};
    JsonValue j(std::move(arr));
    ASSERT_EQ(j.size(), 3);
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 1.0);
}

TEST(move_construct_object) {
    JsonObject obj;
    obj[u8"key"] = JsonValue("value");
    JsonValue j(std::move(obj));
    ASSERT_EQ(j.size(), 1);
    ASSERT_EQ(j.at("key").as_string(), "value");
}

// ============================================
// push_back 边界情况
// ============================================

TEST(push_back_from_null) {
    JsonValue j;
    ASSERT_TRUE(j.is<JsonNull>());

    j.push_back(JsonValue(42));
    ASSERT_TRUE(j.is<JsonArray>());
    ASSERT_EQ(j.size(), 1);
    ASSERT_DOUBLE_EQ(j.at(0).as<JsonNumber>(), 42.0);
}

TEST(push_back_type_error) {
    JsonValue j(42);
    ASSERT_THROW(j.push_back(JsonValue(1)), std::runtime_error);

    JsonValue j2("hello");
    ASSERT_THROW(j2.push_back(JsonValue(1)), std::runtime_error);

    JsonValue j3(true);
    ASSERT_THROW(j3.push_back(JsonValue(1)), std::runtime_error);
}

TEST(push_back_with_move) {
    JsonValue j = array({});
    JsonValue val("hello");
    j.push_back(std::move(val));
    ASSERT_EQ(j.size(), 1);
    ASSERT_EQ(j.at(0).as_string(), "hello");
}

// ============================================
// insert 边界情况
// ============================================

TEST(insert_from_null) {
    JsonValue j;
    j.insert("key", JsonValue(42));
    ASSERT_TRUE(j.is<JsonObject>());
    ASSERT_EQ(j.size(), 1);
    ASSERT_DOUBLE_EQ(j.at("key").as<JsonNumber>(), 42.0);
}

TEST(insert_type_error) {
    JsonValue j(42);
    ASSERT_THROW(j.insert("key", JsonValue(1)), std::runtime_error);

    JsonValue j2 = array({});
    ASSERT_THROW(j2.insert("key", JsonValue(1)), std::runtime_error);

    JsonValue j3(true);
    ASSERT_THROW(j3.insert("key", JsonValue(1)), std::runtime_error);
}

// ============================================
// contains 边界情况
// ============================================

TEST(contains_on_non_object_throws) {
    JsonValue j_null;
    ASSERT_THROW(j_null.contains("key"), std::runtime_error);

    JsonValue j_num(42);
    ASSERT_THROW(j_num.contains("key"), std::runtime_error);

    JsonValue j_str("hello");
    ASSERT_THROW(j_str.contains("key"), std::runtime_error);

    JsonValue j_arr = array({});
    ASSERT_THROW(j_arr.contains("key"), std::runtime_error);

    JsonValue j_bool(true);
    ASSERT_THROW(j_bool.contains("key"), std::runtime_error);
}

// ============================================
// as<T>() const 错误路径
// ============================================

TEST(const_as_type_mismatch_throws) {
    const JsonValue j(42);
    ASSERT_THROW(j.as<JsonString>(), std::runtime_error);
    ASSERT_THROW(j.as<JsonBool>(), std::runtime_error);
    ASSERT_THROW(j.as<JsonNull>(), std::runtime_error);
    ASSERT_THROW(j.as<JsonArray>(), std::runtime_error);
    ASSERT_THROW(j.as<JsonObject>(), std::runtime_error);
}

TEST(const_as_string_type_mismatch) {
    const JsonValue j_null;
    ASSERT_THROW(j_null.as_string(), std::runtime_error);

    const JsonValue j_bool(true);
    ASSERT_THROW(j_bool.as_string(), std::runtime_error);

    const JsonValue j_arr = array({});
    ASSERT_THROW(j_arr.as_string(), std::runtime_error);

    const JsonValue j_obj = object({});
    ASSERT_THROW(j_obj.as_string(), std::runtime_error);
}

// ============================================
// 数组自动填充验证
// ============================================

TEST(array_auto_fill_null) {
    JsonValue j;
    j[5] = 99;

    ASSERT_EQ(j.size(), 6);
    ASSERT_TRUE(j.at(0).is<JsonNull>());
    ASSERT_TRUE(j.at(1).is<JsonNull>());
    ASSERT_TRUE(j.at(2).is<JsonNull>());
    ASSERT_TRUE(j.at(3).is<JsonNull>());
    ASSERT_TRUE(j.at(4).is<JsonNull>());
    ASSERT_DOUBLE_EQ(j.at(5).as<JsonNumber>(), 99.0);
}

// ============================================
// size() 边界情况
// ============================================

TEST(size_empty_string) {
    JsonValue j(std::u8string(u8""));
    ASSERT_EQ(j.size(), 0);
}

TEST(size_ascii_string) {
    JsonValue j(std::u8string(u8"hello"));
    ASSERT_EQ(j.size(), 5);
}

TEST(size_emoji_only) {
    // 每个 emoji 是 1 个字符（尽管占 4 字节）
    JsonValue j(std::u8string(u8"🍎🍊🍋"));
    ASSERT_EQ(j.size(), 3);
}

TEST(size_bool_throws) {
    JsonValue j(true);
    ASSERT_THROW(j.size(), std::runtime_error);
}

// ============================================
// u8string 参数重载
// ============================================

TEST(parse_u8string_overload) {
    std::u8string json = u8"{\"name\": \"Alice\"}";
    JsonValue     j    = parse(json);
    ASSERT_EQ(j.at("name").as_string(), "Alice");
}

TEST(resolve_u8string_overload) {
    JsonValue     j   = object({{"a", array({1, 2, 3})}});
    std::u8string ptr = u8"/a/1";
    ASSERT_DOUBLE_EQ(j.resolve(ptr).as<JsonNumber>(), 2.0);
}

// ============================================
// 主测试运行器
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "Running Accessor Tests\n";
    std::cout << "======================\n\n";

    // 类型构造
    RUN_TEST(create_null);
    RUN_TEST(create_bool);
    RUN_TEST(create_number);
    RUN_TEST(create_string);
    RUN_TEST(create_array);
    RUN_TEST(create_object);

    // 数组访问与修改
    RUN_TEST(array_access);
    RUN_TEST(array_modification);
    RUN_TEST(array_size_limit_normal);
    RUN_TEST(array_size_limit_exceeded);

    // 对象访问与修改
    RUN_TEST(object_access);
    RUN_TEST(object_modification);
    RUN_TEST(auto_convert_when_modifying);

    // emplace_back
    RUN_TEST(emplace_back_basic);
    RUN_TEST(emplace_back_from_null);
    RUN_TEST(emplace_back_type_error);

    // 类型检查
    RUN_TEST(type_checking);
    RUN_TEST(type_mismatch_errors);
    RUN_TEST(type_name);

    // const 访问器
    RUN_TEST(const_array_access);
    RUN_TEST(const_object_access);

    // 移动语义
    RUN_TEST(move_string);
    RUN_TEST(move_array);
    RUN_TEST(move_object);

    // for_each
    RUN_TEST(foreach_array);
    RUN_TEST(foreach_object);
    RUN_TEST(foreach_const_and_mut);
    RUN_TEST(foreach_array_utf8);
    RUN_TEST(foreach_object_utf8);
    RUN_TEST(foreach_modify_utf8_array);

    // 迭代器
    RUN_TEST(iterator_array_range_for);
    RUN_TEST(iterator_array_const);
    RUN_TEST(iterator_array_modify);
    RUN_TEST(iterator_object_range_for);
    RUN_TEST(iterator_object_const);
    RUN_TEST(iterator_object_modify_values);
    RUN_TEST(iterator_type_mismatch_throws);
    RUN_TEST(iterator_stl_algorithm);
    RUN_TEST(iterator_stl_count_if);

    // UTF-8 对象键名
    RUN_TEST(object_keys_chinese);
    RUN_TEST(object_keys_mixed_languages);
    RUN_TEST(object_keys_emoji);

    // UTF-8 size
    RUN_TEST(utf8_string_size);
    RUN_TEST(utf8_array_multilingual);

    // copy/move 语义
    RUN_TEST(copy_construct_independent);
    RUN_TEST(copy_assign_independent);
    RUN_TEST(move_construct);
    RUN_TEST(move_assign);
    RUN_TEST(copy_construct_string);
    RUN_TEST(move_construct_string);
    RUN_TEST(move_construct_array);
    RUN_TEST(move_construct_object);

    // push_back 边界
    RUN_TEST(push_back_from_null);
    RUN_TEST(push_back_type_error);
    RUN_TEST(push_back_with_move);

    // insert 边界
    RUN_TEST(insert_from_null);
    RUN_TEST(insert_type_error);

    // contains 边界
    RUN_TEST(contains_on_non_object_throws);

    // const as<T>() 错误路径
    RUN_TEST(const_as_type_mismatch_throws);
    RUN_TEST(const_as_string_type_mismatch);

    // 数组自动填充
    RUN_TEST(array_auto_fill_null);

    // size() 边界
    RUN_TEST(size_empty_string);
    RUN_TEST(size_ascii_string);
    RUN_TEST(size_emoji_only);
    RUN_TEST(size_bool_throws);

    // u8string 重载
    RUN_TEST(parse_u8string_overload);
    RUN_TEST(resolve_u8string_overload);

    std::cout << "\n======================\n";
    std::cout << "All accessor tests passed!\n";

    return 0;
}
