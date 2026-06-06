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
// 基本类型解析
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

// ============================================
// 结构解析
// ============================================

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

TEST(parse_deep_nesting) {
    std::string json = "{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":1}}}}}";
    JsonValue   j    = parse(json);
    ASSERT_EQ(j.at("a").at("b").at("c").at("d").at("e").as<JsonNumber>(), 1.0);
}

TEST(parse_empty_structures) {
    JsonValue empty_arr = parse("[]");
    ASSERT_EQ(empty_arr.size(), 0);

    JsonValue empty_obj = parse("{}");
    ASSERT_EQ(empty_obj.size(), 0);
}

// ============================================
// 特殊字符
// ============================================

TEST(parse_special_characters) {
    JsonValue   j      = parse("\"\\t\\n\\r\\b\\f\"");
    std::string result = j.as_string();
    ASSERT_EQ(result[0], '\t');
    ASSERT_EQ(result[1], '\n');
    ASSERT_EQ(result[2], '\r');
    ASSERT_EQ(result[3], '\b');
    ASSERT_EQ(result[4], '\f');
}

TEST(parse_large_numbers) {
    JsonValue j = parse("1e308");
    ASSERT_TRUE(j.as<JsonNumber>() > 0);

    JsonValue j2 = parse("-1e308");
    ASSERT_TRUE(j2.as<JsonNumber>() < 0);
}

// ============================================
// Unicode 转义序列
// ============================================

TEST(parse_unicode_escape_basic) {
    std::string json = R"("Hello")";
    JsonValue   j    = parse(json);
    ASSERT_EQ(j.as_string(), "Hello");
}

TEST(parse_unicode_escape_chinese) {
    std::string json = R"("你好")";
    JsonValue   j    = parse(json);
    ASSERT_EQ(j.as_string(), "你好");
}

TEST(parse_unicode_escape_japanese) {
    std::string json = R"("こんにちは")";
    JsonValue   j    = parse(json);
    ASSERT_EQ(j.as_string(), "こんにちは");
}

TEST(parse_unicode_escape_korean) {
    std::string json = R"("안녕")";
    JsonValue   j    = parse(json);
    ASSERT_EQ(j.as_string(), "안녕");
}

TEST(parse_unicode_escape_surrogate_pairs) {
    std::string json = R"("😀")";
    JsonValue   j    = parse(json);
    ASSERT_EQ(j.as_string(), "😀");
}

TEST(parse_unicode_escape_multiple_surrogates) {
    std::string json = R"("🌍 🎉")";
    JsonValue   j    = parse(json);
    ASSERT_EQ(j.as_string(), "🌍 🎉");
}

TEST(parse_unicode_escape_mixed_text) {
    std::string json = R"("Hello 世界 !")";
    JsonValue   j    = parse(json);
    ASSERT_EQ(j.as_string(), "Hello 世界 !");
}

TEST(parse_unicode_escape_all_bmp) {
    struct TestCase {
        std::string json_escape;
        std::string expected;
    };

    std::vector<TestCase> tests = {
            {"\\u00E9", "é"}, // 拉丁字母
            {"\\u03B1", "α"}, // 希腊字母
            {"\\u0416", "Ж"}, // 西里尔字母
            {"\\u05D0", "א"}, // 希伯来字母
            {"\\u0627", "ا"}, // 阿拉伯字母
            {"\\u0E01", "ก"}, // 泰文字母
            {"\\u3042", "あ"}, // 日文平假名
            {"\\u30A2", "ア"}, // 日文片假名
            {"\\uAC00", "가"}, // 韩文
    };

    for (const auto &test: tests) {
        std::string json = "\"" + test.json_escape + "\"";
        JsonValue   j    = parse(json);
        ASSERT_EQ(j.as_string(), test.expected);
    }
}

// ============================================
// UTF-8 直接输入
// ============================================

TEST(parse_utf8_chinese) {
    JsonValue   j1         = JsonValue(std::u8string(u8"你好世界"));
    std::string serialized = j1.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_TRUE(parsed.is<JsonString>());
    ASSERT_EQ(parsed.as_string(), "你好世界");
}

TEST(parse_utf8_japanese) {
    JsonValue   j1         = JsonValue(std::u8string(u8"こんにちは世界"));
    std::string serialized = j1.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.as_string(), "こんにちは世界");
}

TEST(parse_utf8_korean) {
    JsonValue   j1         = JsonValue(std::u8string(u8"안녕하세요 세계"));
    std::string serialized = j1.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.as_string(), "안녕하세요 세계");
}

TEST(parse_utf8_emoji) {
    JsonValue   j1         = JsonValue(std::u8string(u8"Hello 😀🌍🎉"));
    std::string serialized = j1.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.as_string(), "Hello 😀🌍🎉");
}

TEST(parse_utf8_mixed_languages) {
    std::u8string mixed = u8"English 中文 日本語 한국어 العربية";
    JsonValue     j1(mixed);
    std::string   serialized = j1.serialize();
    JsonValue     parsed     = parse(serialized);
    ASSERT_EQ(parsed.as_string(), util::to_std_string(mixed));
}

TEST(parse_utf8_special_symbols) {
    std::u8string symbols = u8"© ® ™ € £ ¥ § ¶ † ‡ • … ‰ ⁄";
    JsonValue     j1(symbols);
    std::string   serialized = j1.serialize();
    JsonValue     parsed     = parse(serialized);
    ASSERT_EQ(parsed.as_string(), util::to_std_string(symbols));
}

TEST(parse_utf8_4byte_chars) {
    JsonValue   j1         = JsonValue(std::u8string(u8"🎉🎊🎋🎌"));
    std::string serialized = j1.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.as_string(), "🎉🎊🎋🎌");
}

TEST(parse_utf8_long_multilingual) {
    std::u8string long_str;
    long_str += u8"Thank you ";
    long_str += u8"谢谢 ";
    long_str += u8"ありがとう ";
    long_str += u8"감사합니다 ";
    long_str += u8"شكرا ";
    long_str += u8"ধন্যবাদ ";
    long_str += u8"Спасибо ";
    long_str += u8"Gracias ";
    long_str += u8"Merci ";
    long_str += u8"Danke";

    JsonValue   j1         = JsonValue(long_str);
    std::string serialized = j1.serialize();
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.as_string(), util::to_std_string(long_str));
}

TEST(parse_utf8_complex_nested) {
    JsonValue j = object({
            {
                    "用户", object({
                            {"姓名", "李四"},
                            {
                                    "爱好", array({
                                            JsonValue(std::u8string(u8"编程 💻")),
                                            JsonValue(std::u8string(u8"阅读 📚")),
                                            JsonValue(std::u8string(u8"音乐 🎵"))
                                    })
                            },
                            {
                                    "地址", object({
                                            {"国家", "中国"},
                                            {"城市", "上海"}
                                    })
                            }
                    })
            }
    });

    std::string serialized = j.serialize(2);
    JsonValue   parsed     = parse(serialized);
    ASSERT_EQ(parsed.at(u8"用户").at(u8"姓名").as_string(), "李四");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"爱好").at(0).as_string(), "编程 💻");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"爱好").at(1).as_string(), "阅读 📚");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"爱好").at(2).as_string(), "音乐 🎵");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"地址").at(u8"国家").as_string(), "中国");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"地址").at(u8"城市").as_string(), "上海");
}

TEST(parse_utf8_valid_raw) {
    std::string json = R"("Hello 你好 こんにちは 🌍")";
    JsonValue   j    = parse(json);
    ASSERT_TRUE(j.is<JsonString>());
}

// ============================================
// UTF-8 验证（非法序列拒绝）
// ============================================

TEST(parse_utf8_invalid_continuation) {
    std::string json = "\"\x80\"";
    ASSERT_THROW(parse(json), std::runtime_error);
}

TEST(parse_utf8_invalid_overlong) {
    std::string json = "\"\xC0\x80\"";
    ASSERT_THROW(parse(json), std::runtime_error);
}

TEST(parse_utf8_invalid_surrogate) {
    std::string json = "\"\xED\xA0\x80\"";
    ASSERT_THROW(parse(json), std::runtime_error);
}

TEST(parse_utf8_invalid_surrogate_pair_incomplete) {
    std::string json = R"("\uD83D")";
    ASSERT_THROW(parse(json), std::runtime_error);
}

TEST(parse_utf8_invalid_low_surrogate) {
    std::string json = R"("\uD83D ")";
    ASSERT_THROW(parse(json), std::runtime_error);
}

// ============================================
// 数字解析边界
// ============================================

TEST(parse_number_negative_zero) {
    JsonValue j = parse("-0");
    ASSERT_DOUBLE_EQ(j.as<JsonNumber>(), 0.0);
}

TEST(parse_number_zero_decimal) {
    JsonValue j = parse("0.0");
    ASSERT_DOUBLE_EQ(j.as<JsonNumber>(), 0.0);
}

TEST(parse_number_leading_plus_invalid) {
    ASSERT_THROW(parse("+0"), std::runtime_error);
    ASSERT_THROW(parse("+1"), std::runtime_error);
}

TEST(parse_number_leading_zero_invalid) {
    ASSERT_THROW(parse("01"), std::runtime_error);
    ASSERT_THROW(parse("007"), std::runtime_error);
}

TEST(parse_number_trailing_dot_invalid) {
    ASSERT_THROW(parse("1."), std::runtime_error);
}

TEST(parse_number_exponent_no_digits_invalid) {
    ASSERT_THROW(parse("1e"), std::runtime_error);
    ASSERT_THROW(parse("1E"), std::runtime_error);
    ASSERT_THROW(parse("1e+"), std::runtime_error);
    ASSERT_THROW(parse("1e-"), std::runtime_error);
}

TEST(parse_nan_infinity_invalid) {
    ASSERT_THROW(parse("NaN"), std::runtime_error);
    ASSERT_THROW(parse("Infinity"), std::runtime_error);
    ASSERT_THROW(parse("-Infinity"), std::runtime_error);
}

// ============================================
// 字符串解析边界
// ============================================

TEST(parse_empty_string) {
    JsonValue j = parse("\"\"");
    ASSERT_TRUE(j.is<JsonString>());
    ASSERT_EQ(j.as_string(), "");
}

TEST(parse_string_escaped_forward_slash) {
    JsonValue j = parse("\"hello\\/world\"");
    ASSERT_EQ(j.as_string(), "hello/world");
}

TEST(parse_string_unicode_forward_slash) {
    JsonValue j = parse("\"\\u002F\"");
    ASSERT_EQ(j.as_string(), "/");
}

// ============================================
// 尾部数据检测
// ============================================

TEST(parse_trailing_data_after_value) {
    ASSERT_THROW(parse("1 2"), std::runtime_error);
    ASSERT_THROW(parse("null true"), std::runtime_error);
    ASSERT_THROW(parse("\"hello\" \"world\""), std::runtime_error);
    ASSERT_THROW(parse("[] {}"), std::runtime_error);
}

// ============================================
// 解析错误路径
// ============================================

TEST(parse_error_empty_input) {
    ASSERT_THROW(parse(""), std::runtime_error);
}

TEST(parse_error_invalid_token) {
    ASSERT_THROW(parse("invalid"), std::runtime_error);
}

TEST(parse_error_unclosed_brace) {
    ASSERT_THROW(parse("{"), std::runtime_error);
}

TEST(parse_error_unclosed_bracket) {
    ASSERT_THROW(parse("[1,"), std::runtime_error);
}

TEST(parse_error_unclosed_string) {
    ASSERT_THROW(parse("\"unclosed"), std::runtime_error);
}

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
    ASSERT_THROW(parse("[1,2,]"), std::runtime_error);
}

TEST(parse_error_trailing_comma_object) {
    ASSERT_THROW(parse("{\"a\":1,}"), std::runtime_error);
}

TEST(parse_error_invalid_unicode_escape) {
    ASSERT_THROW(parse("\"\\u00GG\""), std::runtime_error);
}

TEST(parse_error_number_with_trailing_chars) {
    ASSERT_THROW(parse("123abc"), std::runtime_error);
}

// ============================================
// 深度限制
// ============================================

TEST(parse_max_depth_default) {
    std::string json = R"({"a":{"b":{"c":1}}})";
    JsonValue   j    = parse(json);
    ASSERT_DOUBLE_EQ(j.resolve("/a/b/c").as<JsonNumber>(), 1.0);
}

TEST(parse_max_depth_custom) {
    std::string json = R"({"a":{"b":{"c":{"d":1}}}})";
    ASSERT_THROW(parse(json, 3), std::runtime_error);
}

TEST(parse_max_depth_exactly_at_limit) {
    std::string json = R"({"a":{"b":{"c":1}}})";
    JsonValue   j    = parse(json, 3);
    ASSERT_DOUBLE_EQ(j.resolve("/a/b/c").as<JsonNumber>(), 1.0);
}

TEST(parse_max_depth_array) {
    std::string json = R"([[[[1]]]])";
    ASSERT_THROW(parse(json, 3), std::runtime_error);
}

TEST(parse_max_depth_mixed) {
    std::string json = R"({"a":[[1]]})";
    ASSERT_THROW(parse(json, 2), std::runtime_error);
}

// ============================================
// 主测试运行器
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "Running Parse Tests\n";
    std::cout << "===================\n\n";

    // 基本类型解析
    RUN_TEST(parse_null);
    RUN_TEST(parse_bool);
    RUN_TEST(parse_number);
    RUN_TEST(parse_string);

    // 结构解析
    RUN_TEST(parse_empty_array);
    RUN_TEST(parse_simple_array);
    RUN_TEST(parse_mixed_array);
    RUN_TEST(parse_empty_object);
    RUN_TEST(parse_simple_object);
    RUN_TEST(parse_nested_structure);
    RUN_TEST(parse_deep_nesting);
    RUN_TEST(parse_empty_structures);

    // 特殊字符
    RUN_TEST(parse_special_characters);
    RUN_TEST(parse_large_numbers);

    // Unicode 转义序列
    RUN_TEST(parse_unicode_escape_basic);
    RUN_TEST(parse_unicode_escape_chinese);
    RUN_TEST(parse_unicode_escape_japanese);
    RUN_TEST(parse_unicode_escape_korean);
    RUN_TEST(parse_unicode_escape_surrogate_pairs);
    RUN_TEST(parse_unicode_escape_multiple_surrogates);
    RUN_TEST(parse_unicode_escape_mixed_text);
    RUN_TEST(parse_unicode_escape_all_bmp);

    // UTF-8 直接输入
    RUN_TEST(parse_utf8_chinese);
    RUN_TEST(parse_utf8_japanese);
    RUN_TEST(parse_utf8_korean);
    RUN_TEST(parse_utf8_emoji);
    RUN_TEST(parse_utf8_mixed_languages);
    RUN_TEST(parse_utf8_special_symbols);
    RUN_TEST(parse_utf8_4byte_chars);
    RUN_TEST(parse_utf8_long_multilingual);
    RUN_TEST(parse_utf8_complex_nested);
    RUN_TEST(parse_utf8_valid_raw);

    // UTF-8 验证
    RUN_TEST(parse_utf8_invalid_continuation);
    RUN_TEST(parse_utf8_invalid_overlong);
    RUN_TEST(parse_utf8_invalid_surrogate);
    RUN_TEST(parse_utf8_invalid_surrogate_pair_incomplete);
    RUN_TEST(parse_utf8_invalid_low_surrogate);

    // 数字解析边界
    RUN_TEST(parse_number_negative_zero);
    RUN_TEST(parse_number_zero_decimal);
    RUN_TEST(parse_number_leading_plus_invalid);
    RUN_TEST(parse_number_leading_zero_invalid);
    RUN_TEST(parse_number_trailing_dot_invalid);
    RUN_TEST(parse_number_exponent_no_digits_invalid);
    RUN_TEST(parse_nan_infinity_invalid);

    // 字符串解析边界
    RUN_TEST(parse_empty_string);
    RUN_TEST(parse_string_escaped_forward_slash);
    RUN_TEST(parse_string_unicode_forward_slash);

    // 尾部数据检测
    RUN_TEST(parse_trailing_data_after_value);

    // 解析错误路径
    RUN_TEST(parse_error_empty_input);
    RUN_TEST(parse_error_invalid_token);
    RUN_TEST(parse_error_unclosed_brace);
    RUN_TEST(parse_error_unclosed_bracket);
    RUN_TEST(parse_error_unclosed_string);
    RUN_TEST(parse_error_invalid_escape);
    RUN_TEST(parse_error_mismatched_brackets);
    RUN_TEST(parse_error_missing_colon);
    RUN_TEST(parse_error_trailing_comma_array);
    RUN_TEST(parse_error_trailing_comma_object);
    RUN_TEST(parse_error_invalid_unicode_escape);
    RUN_TEST(parse_error_number_with_trailing_chars);

    // 深度限制
    RUN_TEST(parse_max_depth_default);
    RUN_TEST(parse_max_depth_custom);
    RUN_TEST(parse_max_depth_exactly_at_limit);
    RUN_TEST(parse_max_depth_array);
    RUN_TEST(parse_max_depth_mixed);

    std::cout << "\n===================\n";
    std::cout << "All parse tests passed!\n";

    return 0;
}
