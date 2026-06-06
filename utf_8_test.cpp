#include "json.h"
#include "test.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace json;

// ============================================
// UTF-8 编码测试 - 基本多语言字符
// ============================================

TEST(utf8_chinese_characters) {
    // 测试中文字符的序列化和解析
    JsonValue   j1 = JsonValue(std::u8string(u8"你好世界"));
    std::string serialized = j1.serialize();

    std::cout << "中文字符串序列化: " << serialized << std::endl;

    // 解析回来
    JsonValue parsed = parse(serialized);
    ASSERT_TRUE(parsed.is<JsonString>());
    ASSERT_EQ(parsed.as_string(), "你好世界");
}

TEST(utf8_japanese_characters) {
    // 测试日文字符
    JsonValue   j1 = JsonValue(std::u8string(u8"こんにちは世界"));
    std::string serialized = j1.serialize();

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.as_string(), "こんにちは世界");
}

TEST(utf8_korean_characters) {
    // 测试韩文字符
    JsonValue   j1 = JsonValue(std::u8string(u8"안녕하세요 세계"));
    std::string serialized = j1.serialize();

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.as_string(), "안녕하세요 세계");
}

TEST(utf8_emoji_characters) {
    // 测试emoji字符
    JsonValue   j1 = JsonValue(std::u8string(u8"Hello 😀🌍🎉"));
    std::string serialized = j1.serialize();

    std::cout << "Emoji字符串序列化: " << serialized << std::endl;

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.as_string(), "Hello 😀🌍🎉");
}

TEST(utf8_mixed_languages) {
    // 测试混合语言
    std::u8string mixed = u8"English 中文 日本語 한국어 العربية";
    JsonValue     j1(mixed);
    std::string   serialized = j1.serialize();

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.as_string(), util::to_std_string(mixed));
}

TEST(utf8_special_symbols) {
    // 测试特殊符号
    std::u8string symbols = u8"© ® ™ € £ ¥ § ¶ † ‡ • … ‰ ⁄";
    JsonValue     j1(symbols);
    std::string   serialized = j1.serialize();

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.as_string(), util::to_std_string(symbols));
}

// ============================================
// Unicode 转义序列测试
// ============================================

TEST(unicode_escape_basic) {
    // 测试基本Unicode转义 \uXXXX
    std::string json = R"("\u0048\u0065\u006c\u006c\u006f")"; // "Hello"
    JsonValue   j = parse(json);
    ASSERT_EQ(j.as_string(), "Hello");
}

TEST(unicode_escape_chinese) {
    // 测试中文Unicode转义
    // 你好 = \u4F60\u597D
    std::string json = R"("\u4F60\u597D")";
    JsonValue   j = parse(json);
    ASSERT_EQ(j.as_string(), "你好");
}

TEST(unicode_escape_japanese) {
    // 测试日文Unicode转义
    // こんにちは = \u3053\u3093\u306B\u3061\u306F
    std::string json = R"("\u3053\u3093\u306B\u3061\u306F")";
    JsonValue   j = parse(json);
    ASSERT_EQ(j.as_string(), "こんにちは");
}

TEST(unicode_escape_korean) {
    // 测试韩文Unicode转义
    // 안녕 = \uC548\uB155
    std::string json = R"("\uC548\uB155")";
    JsonValue   j = parse(json);
    ASSERT_EQ(j.as_string(), "안녕");
}

TEST(unicode_escape_surrogate_pairs) {
    // 测试代理对（Surrogate Pairs）
    // 😀 = U+1F600 = \uD83D\uDE00
    std::string json = R"("\uD83D\uDE00")";
    JsonValue   j = parse(json);
    ASSERT_EQ(j.as_string(), "😀");
}

TEST(unicode_escape_multiple_surrogates) {
    // 测试多个emoji的代理对
    // 🌍 = \uD83C\uDF0D, 🎉 = \uD83C\uDF89
    std::string json = R"("\uD83C\uDF0D \uD83C\uDF89")";
    JsonValue   j = parse(json);
    ASSERT_EQ(j.as_string(), "🌍 🎉");
}

TEST(unicode_escape_mixed_text) {
    // 测试混合普通文本和Unicode转义
    std::string json = R"("Hello \u4E16\u754C !")"; // Hello 世界 !
    JsonValue   j = parse(json);
    ASSERT_EQ(j.as_string(), "Hello 世界 !");
}

TEST(unicode_escape_all_bmp) {
    // 测试BMP（基本多文种平面）范围内的各种字符
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
        JsonValue   j = parse(json);
        ASSERT_EQ(j.as_string(), test.expected);
    }
}

// ============================================
// JSON对象中的UTF-8键名测试
// ============================================

TEST(utf8_object_keys_chinese) {
    // 使用中文字段名构建JSON对象
    JsonValue j = object({
            {"姓名", "张三"},
            {"年龄", 25},
            {"城市", "北京"}
    });

    std::string serialized = j.serialize();
    std::cout << "中文字段名对象: " << serialized << std::endl;

    // 解析并验证
    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.at(u8"姓名").as_string(), "张三");
    ASSERT_DOUBLE_EQ(parsed.at(u8"年龄").as<JsonNumber>(), 25.0);
    ASSERT_EQ(parsed.at(u8"城市").as_string(), "北京");
}

TEST(utf8_object_keys_mixed) {
    // 混合语言字段名
    JsonValue j = object({
            {"name", "Alice"},
            {"名前", "アリス"},
            {"이름", "앨리스"},
            {"姓名", "爱丽丝"},
            {"الاسم", "أليس"}
    });

    std::string serialized = j.serialize();
    JsonValue   parsed = parse(serialized);

    ASSERT_EQ(parsed.at(u8"name").as_string(), "Alice");
    ASSERT_EQ(parsed.at(u8"名前").as_string(), "アリス");
    ASSERT_EQ(parsed.at(u8"이름").as_string(), "앨리스");
    ASSERT_EQ(parsed.at(u8"姓名").as_string(), "爱丽丝");
    ASSERT_EQ(parsed.at(u8"الاسم").as_string(), "أليس");
}

TEST(utf8_object_keys_emoji) {
    // Emoji字段名（虽然不推荐但JSON标准允许）
    JsonValue j = object({
            {"😀", "happy"},
            {"🌟", "star"},
            {"🎵", "music"}
    });

    std::string serialized = j.serialize();
    std::cout << "Emoji字段名对象: " << serialized << std::endl;

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.at(u8"😀").as_string(), "happy");
    ASSERT_EQ(parsed.at(u8"🌟").as_string(), "star");
    ASSERT_EQ(parsed.at(u8"🎵").as_string(), "music");
}

// ============================================
// UTF-8数组中的Unicode测试
// ============================================

TEST(utf8_array_multilingual) {
    // 创建包含多语言字符串的数组
    JsonValue j = array({
            JsonValue(std::u8string(u8"Hello")),
            JsonValue(std::u8string(u8"你好")),
            JsonValue(std::u8string(u8"こんにちは")),
            JsonValue(std::u8string(u8"안녕하세요")),
            JsonValue(std::u8string(u8"مرحبا")),
            JsonValue(std::u8string(u8"😀🌍"))
    });

    std::string serialized = j.serialize();
    std::cout << "多语言数组: " << serialized << std::endl;

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.size(), 6);
    ASSERT_EQ(parsed.at(0).as_string(), "Hello");
    ASSERT_EQ(parsed.at(1).as_string(), "你好");
    ASSERT_EQ(parsed.at(2).as_string(), "こんにちは");
    ASSERT_EQ(parsed.at(3).as_string(), "안녕하세요");
    ASSERT_EQ(parsed.at(4).as_string(), "مرحبا");
    ASSERT_EQ(parsed.at(5).as_string(), "😀🌍");
}

// ============================================
// 复杂嵌套结构中的UTF-8测试
// ============================================

TEST(utf8_complex_nested_structure) {
    // 创建包含UTF-8的复杂嵌套结构
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
    std::cout << "复杂UTF-8嵌套结构:\n" << serialized << std::endl;

    // 解析并验证
    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.at(u8"用户").at(u8"姓名").as_string(), "李四");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"爱好").at(0).as_string(), "编程 💻");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"爱好").at(1).as_string(), "阅读 📚");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"爱好").at(2).as_string(), "音乐 🎵");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"地址").at(u8"国家").as_string(), "中国");
    ASSERT_EQ(parsed.at(u8"用户").at(u8"地址").at(u8"城市").as_string(), "上海");
}

// ============================================
// for_each 遍历 UTF-8 数据测试
// ============================================

TEST(utf8_foreach_array_with_unicode) {
    const JsonValue j = array({
            JsonValue(std::u8string(u8"苹果 🍎")),
            JsonValue(std::u8string(u8"香蕉 🍌")),
            JsonValue(std::u8string(u8"橙子 🍊")),
            JsonValue(std::u8string(u8"葡萄 🍇"))
    });

    std::vector<std::string> fruits;

    // 使用const版本遍历
    j.for_each_array([&](const JsonValue &elem) {
        fruits.push_back(elem.as_string());
    });

    ASSERT_EQ(fruits.size(), 4);
    ASSERT_EQ(fruits[0], "苹果 🍎");
    ASSERT_EQ(fruits[1], "香蕉 🍌");
    ASSERT_EQ(fruits[2], "橙子 🍊");
    ASSERT_EQ(fruits[3], "葡萄 🍇");
}

TEST(utf8_foreach_object_with_unicode) {
    const JsonValue j = object({
            {"フルーツ", "りんご"},
            {"やさい", "にんじん"},
            {"にく", "ぎゅうにく"}
    });

    std::vector<std::pair<std::string, std::string> > items;

    // 使用const版本遍历对象
    j.for_each_object([&](const JsonString &key, const JsonValue &val) {
        items.emplace_back(util::to_std_string(key), val.as_string());
    });

    ASSERT_EQ(items.size(), 3);
    // 检查是否包含特定的键值对
    bool found = false;
    for (const auto &[key, value]: items) {
        if (key == "フルーツ" && value == "りんご") {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
}

TEST(utf8_modify_array_with_foreach) {
    // 使用非const版本修改包含Unicode的数组
    JsonValue j = array({
            JsonValue(std::u8string(u8"hello")),
            JsonValue(std::u8string(u8"world")),
            JsonValue(std::u8string(u8"你好"))
    });

    // 转换为大写
    j.for_each_array([](JsonValue &elem) {
        if (elem.is<JsonString>()) {
            std::string s = elem.as_string();
            // 这里不会改变中文字符，但会尝试转换ASCII部分
            std::ranges::transform(s, s.begin(),
                                   [](unsigned char c) { return std::toupper(c); });
            elem = JsonValue(s);
        }
    });

    ASSERT_EQ(j.at(0).as_string(), "HELLO");
    ASSERT_EQ(j.at(1).as_string(), "WORLD");
    // 中文字符不应被改变（没有大写形式）
    ASSERT_EQ(j.at(2).as_string(), "你好");
}

// ============================================
// 边界情况和错误处理测试
// ============================================

TEST(utf8_invalid_surrogate_pair) {
    // 测试无效的代理对
    // 只有高代理项没有低代理项
    std::string json = R"("\uD83D")"; // 不完整的代理对
    ASSERT_THROW(parse(json), std::runtime_error);
}

TEST(utf8_invalid_low_surrogate) {
    // 测试低代理项范围错误
    std::string json = R"("\uD83D\u0000")"; // 无效的低代理项
    ASSERT_THROW(parse(json), std::runtime_error);
}

TEST(utf8_valid_4byte_utf8) {
    // 测试直接写入4字节UTF-8字符（不需要转义）
    JsonValue   j(std::u8string(u8"🎉🎊🎋🎌"));
    std::string serialized = j.serialize();

    std::cout << "4字节UTF-8序列化: " << serialized << std::endl;

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.as_string(), "🎉🎊🎋🎌");
}

TEST(utf8_long_multilingual_string) {
    // 测试长多语言字符串
    std::u8string long_str;
    // 添加各种语言的"谢谢"
    long_str += u8"Thank you "; // 英语
    long_str += u8"谢谢 ";        // 中文
    long_str += u8"ありがとう ";     // 日语
    long_str += u8"감사합니다 ";     // 韩语
    long_str += u8"شكرا ";      // 阿拉伯语
    long_str += u8"ধন্যবাদ ";   // 孟加拉语
    long_str += u8"Спасибо ";   // 俄语
    long_str += u8"Gracias ";   // 西班牙语
    long_str += u8"Merci ";     // 法语
    long_str += u8"Danke";      // 德语

    JsonValue   j(long_str);
    std::string serialized = j.serialize();

    std::cout << "多语言长字符串长度: " << serialized.length() << std::endl;

    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.as_string(), util::to_std_string(long_str));
}

// ============================================
// 序列化选项与UTF-8测试
// ============================================

TEST(utf8_serialize_with_indent) {
    // 带缩进的UTF-8 JSON序列化
    JsonValue j = object({
            {
                    "数据", object({
                            {"メッセージ", std::u8string(u8"こんにちは世界")},
                            {"数字", array({1, 2, 3, 4, 5})},
                            {"フラグ", true}
                    })
            }
    });

    std::string serialized = j.serialize(2);
    std::cout << "带缩进的UTF-8 JSON:\n" << serialized << std::endl;

    // 验证解析回来
    JsonValue parsed = parse(serialized);
    ASSERT_EQ(parsed.at(u8"数据").at(u8"メッセージ").as_string(), "こんにちは世界");
}

// ============================================
// 性能相关测试（基本大小检查）
// ============================================

TEST(utf8_string_size) {
    // UTF-8字符串的size()应该返回正确的字符数
    JsonValue j(std::u8string(u8"Hello世界"));
    ASSERT_EQ(j.size(), 7); // 5个ASCII + 2个中文字符 = 7

    JsonValue j2(std::u8string(u8"🍎🍊🍋"));
    ASSERT_TRUE(j2.size() > 0); // emoji占多个字节，但应该正确计算
}

// ============================================
// 主测试运行器
// ============================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "Running UTF-8 and Unicode Tests\n";
    std::cout << "================================\n\n";

    std::cout << "=== UTF-8 基本字符测试 ===\n";
    RUN_TEST(utf8_chinese_characters);
    RUN_TEST(utf8_japanese_characters);
    RUN_TEST(utf8_korean_characters);
    RUN_TEST(utf8_emoji_characters);
    RUN_TEST(utf8_mixed_languages);
    RUN_TEST(utf8_special_symbols);

    std::cout << "\n=== Unicode 转义序列测试 ===\n";
    RUN_TEST(unicode_escape_basic);
    RUN_TEST(unicode_escape_chinese);
    RUN_TEST(unicode_escape_japanese);
    RUN_TEST(unicode_escape_korean);
    RUN_TEST(unicode_escape_surrogate_pairs);
    RUN_TEST(unicode_escape_multiple_surrogates);
    RUN_TEST(unicode_escape_mixed_text);
    RUN_TEST(unicode_escape_all_bmp);

    std::cout << "\n=== UTF-8 对象键名测试 ===\n";
    RUN_TEST(utf8_object_keys_chinese);
    RUN_TEST(utf8_object_keys_mixed);
    RUN_TEST(utf8_object_keys_emoji);

    std::cout << "\n=== UTF-8 数组测试 ===\n";
    RUN_TEST(utf8_array_multilingual);

    std::cout << "\n=== 复杂嵌套结构测试 ===\n";
    RUN_TEST(utf8_complex_nested_structure);

    std::cout << "\n=== for_each 遍历UTF-8数据测试 ===\n";
    RUN_TEST(utf8_foreach_array_with_unicode);
    RUN_TEST(utf8_foreach_object_with_unicode);
    RUN_TEST(utf8_modify_array_with_foreach);

    std::cout << "\n=== 边界情况和错误处理 ===\n";
    RUN_TEST(utf8_invalid_surrogate_pair);
    RUN_TEST(utf8_invalid_low_surrogate);
    RUN_TEST(utf8_valid_4byte_utf8);
    RUN_TEST(utf8_long_multilingual_string);

    std::cout << "\n=== 序列化选项测试 ===\n";
    RUN_TEST(utf8_serialize_with_indent);

    std::cout << "\n=== 基本性能检查 ===\n";
    RUN_TEST(utf8_string_size);

    std::cout << "\n================================\n";
    std::cout << "All UTF-8/Unicode tests passed successfully!\n";

    return 0;
}
