#include "json.h"
#include "test.h"
#include <iostream>
#include <cassert>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace json;

// ============================================
// JSON Pointer (RFC 6901)
// ============================================

TEST(resolve_root) {
    JsonValue j    = object({{"a", 1}});
    auto &    root = j.resolve("");
    ASSERT_TRUE(root.is<JsonObject>());
    ASSERT_DOUBLE_EQ(root.at("a").as<JsonNumber>(), 1.0);
}

TEST(resolve_simple_path) {
    JsonValue j = object({
            {
                    "a", object({
                            {
                                    "b", object({
                                            {"c", 42}
                                    })
                            }
                    })
            }
    });
    ASSERT_DOUBLE_EQ(j.resolve("/a/b/c").as<JsonNumber>(), 42.0);
}

TEST(resolve_array_index) {
    JsonValue j = array({10, 20, 30});
    ASSERT_DOUBLE_EQ(j.resolve("/1").as<JsonNumber>(), 20.0);
}

TEST(resolve_nested_array) {
    JsonValue j = object({
            {"data", array({1, 2, array({10, 20, 30})})}
    });
    ASSERT_DOUBLE_EQ(j.resolve("/data/2/1").as<JsonNumber>(), 20.0);
}

TEST(resolve_string_value) {
    JsonValue j = object({
            {
                    "user", object({
                            {"name", "Alice"},
                            {"age", 30}
                    })
            }
    });
    ASSERT_EQ(j.resolve("/user/name").as_string(), "Alice");
}

TEST(resolve_escaped_tilde) {
    JsonValue j = object({
            {"a~b", "tilde_value"}
    });
    ASSERT_EQ(j.resolve("/a~0b").as_string(), "tilde_value");
}

TEST(resolve_escaped_slash) {
    JsonValue j = object({
            {"a/b", "slash_value"}
    });
    ASSERT_EQ(j.resolve("/a~1b").as_string(), "slash_value");
}

TEST(resolve_escaped_both) {
    JsonValue j = object({
            {"a/~b", "both_value"}
    });
    ASSERT_EQ(j.resolve("/a~1~0b").as_string(), "both_value");
}

TEST(resolve_modify_value) {
    JsonValue j = object({
            {
                    "a", object({
                            {"b", 1}
                    })
            }
    });
    j.resolve("/a/b") = JsonValue(99);
    ASSERT_DOUBLE_EQ(j.resolve("/a/b").as<JsonNumber>(), 99.0);
}

TEST(resolve_const_access) {
    const JsonValue j = object({
            {"x", array({1, 2, 3})}
    });
    ASSERT_DOUBLE_EQ(j.resolve("/x/2").as<JsonNumber>(), 3.0);
}

TEST(resolve_key_not_found) {
    JsonValue j = object({{"a", 1}});
    ASSERT_THROW(j.resolve("/b"), std::out_of_range);
}

TEST(resolve_array_out_of_range) {
    JsonValue j = array({1, 2, 3});
    ASSERT_THROW(j.resolve("/5"), std::out_of_range);
}

TEST(resolve_invalid_array_index) {
    JsonValue j = object({{"a", array({1})}});
    ASSERT_THROW(j.resolve("/a/abc"), std::out_of_range);
}

TEST(resolve_traverse_into_scalar) {
    JsonValue j = object({{"a", 42}});
    ASSERT_THROW(j.resolve("/a/b"), std::runtime_error);
}

TEST(resolve_invalid_format) {
    JsonValue j = object({{"a", 1}});
    ASSERT_THROW(j.resolve("a/b"), std::runtime_error);
}

TEST(resolve_traverse_into_string) {
    JsonValue j = object({{"a", "hello"}});
    ASSERT_THROW(j.resolve("/a/b"), std::runtime_error);
}

TEST(resolve_traverse_into_bool) {
    JsonValue j = object({{"a", true}});
    ASSERT_THROW(j.resolve("/a/b"), std::runtime_error);
}

TEST(resolve_traverse_into_null) {
    JsonValue j = object({{"a", nullptr}});
    ASSERT_THROW(j.resolve("/a/b"), std::runtime_error);
}

TEST(resolve_complex_path) {
    JsonValue j = object({
            {"data", array({object({{"items", array({1, 2, 3})}})})}
    });
    ASSERT_DOUBLE_EQ(j.resolve("/data/0/items/2").as<JsonNumber>(), 3.0);
}

// ============================================
// 主测试运行器
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "Running JSON Pointer Tests\n";
    std::cout << "==========================\n\n";

    RUN_TEST(resolve_root);
    RUN_TEST(resolve_simple_path);
    RUN_TEST(resolve_array_index);
    RUN_TEST(resolve_nested_array);
    RUN_TEST(resolve_string_value);
    RUN_TEST(resolve_escaped_tilde);
    RUN_TEST(resolve_escaped_slash);
    RUN_TEST(resolve_escaped_both);
    RUN_TEST(resolve_modify_value);
    RUN_TEST(resolve_const_access);
    RUN_TEST(resolve_key_not_found);
    RUN_TEST(resolve_array_out_of_range);
    RUN_TEST(resolve_invalid_array_index);
    RUN_TEST(resolve_traverse_into_scalar);
    RUN_TEST(resolve_invalid_format);
    RUN_TEST(resolve_traverse_into_string);
    RUN_TEST(resolve_traverse_into_bool);
    RUN_TEST(resolve_traverse_into_null);
    RUN_TEST(resolve_complex_path);

    std::cout << "\n==========================\n";
    std::cout << "All JSON Pointer tests passed!\n";

    return 0;
}
