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
// JSON Merge Patch (RFC 7396)
// ============================================

TEST(merge_add_new_keys) {
    JsonValue target = object({{"a", 1}});
    JsonValue patch  = object({{"b", 2}});
    target.merge(patch);
    ASSERT_DOUBLE_EQ(target.at("a").as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(target.at("b").as<JsonNumber>(), 2.0);
}

TEST(merge_update_existing_key) {
    JsonValue target = object({{"a", 1}, {"b", 2}});
    JsonValue patch  = object({{"b", 99}});
    target.merge(patch);
    ASSERT_DOUBLE_EQ(target.at("a").as<JsonNumber>(), 1.0);
    ASSERT_DOUBLE_EQ(target.at("b").as<JsonNumber>(), 99.0);
}

TEST(merge_remove_key_with_null) {
    JsonValue target = object({{"a", 1}, {"b", 2}, {"c", 3}});
    JsonValue patch  = object({{"b", nullptr}});
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
    JsonValue patch  = JsonValue(42);
    target.merge(patch);
    ASSERT_TRUE(target.is<JsonNumber>());
    ASSERT_DOUBLE_EQ(target.as<JsonNumber>(), 42.0);
}

TEST(merge_patch_not_object) {
    JsonValue target = object({{"a", 1}});
    JsonValue patch  = "hello";
    target.merge(patch);
    ASSERT_TRUE(target.is<JsonString>());
    ASSERT_EQ(target.as_string(), "hello");
}

TEST(merge_target_not_object) {
    JsonValue target = 42;
    JsonValue patch  = object({{"a", 1}});
    target.merge(patch);
    ASSERT_TRUE(target.is<JsonObject>());
    ASSERT_DOUBLE_EQ(target.at("a").as<JsonNumber>(), 1.0);
}

TEST(merge_array_values) {
    JsonValue target = object({{"arr", array({1, 2, 3})}});
    JsonValue patch  = object({{"arr", array({10, 20})}});
    target.merge(patch);
    ASSERT_EQ(target.at("arr").size(), 2);
    ASSERT_DOUBLE_EQ(target.at("arr").at(0).as<JsonNumber>(), 10.0);
}

TEST(merge_deep_nested) {
    JsonValue target = object({
            {
                    "a", object({
                            {
                                    "b", object({
                                            {"c", 1}, {"d", 2}
                                    })
                            }
                    })
            }
    });
    JsonValue patch = object({
            {
                    "a", object({
                            {
                                    "b", object({
                                            {"d", nullptr}
                                    })
                            }
                    })
            }
    });
    target.merge(patch);
    ASSERT_DOUBLE_EQ(target.at("a").at("b").at("c").as<JsonNumber>(), 1.0);
    ASSERT_FALSE(target.at("a").at("b").contains("d"));
}

TEST(merge_empty_patch) {
    JsonValue target = object({{"a", 1}, {"b", 2}});
    JsonValue patch  = object({});
    target.merge(patch);
    ASSERT_EQ(target.size(), 2);
    ASSERT_DOUBLE_EQ(target.at("a").as<JsonNumber>(), 1.0);
}

// ============================================
// 主测试运行器
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "Running JSON Merge Patch Tests\n";
    std::cout << "==============================\n\n";

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

    std::cout << "\n==============================\n";
    std::cout << "All Merge Patch tests passed!\n";

    return 0;
}
