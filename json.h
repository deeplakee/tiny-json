/**
 * @file json.h
 * @brief tiny-json — 轻量级现代 C++20 JSON 库
 *
 * 特性：
 *   - 单头文件，零依赖
 *   - 基于 std::variant 的类型安全设计
 *   - 原生 UTF-8 支持（内部使用 std::u8string）
 *   - JSON Pointer (RFC 6901) 和 JSON Merge Patch (RFC 7396) 支持
 *   - 解析深度限制防止栈溢出
 *   - 数字序列化符合 JSON 规范（snprintf + %g）
 *
 * 用法：
 *   #include "json.h"
 *   using namespace json;
 *   auto j = parse(R"({"key": "value"})");
 *   std::cout << j["key"].as_string() << std::endl;
 */

#ifndef TINY_JSON_JSON_H
#define TINY_JSON_JSON_H

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <variant>
#include <type_traits>
#include <ranges>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <cstdint>
#include <cctype>
#include <cmath>
#include <limits>
#include <string_view>
#include <algorithm>
#include <functional>

namespace json {

    // 前向声明
    class JsonValue;

    /**
     * @brief JSON 值类型枚举
     *
     * 与 JsonVariant 的索引一一对应，用于类型检查和序列化
     */
    enum class JsonValueType {
        Null,    ///< null 值
        Bool,    ///< 布尔值 (true/false)
        Number,  ///< 数值（内部使用 double）
        String,  ///< 字符串（内部使用 std::u8string）
        Array,   ///< 数组（内部使用 std::vector<JsonValue>）
        Object   ///< 对象（内部使用 std::unordered_map<std::u8string, JsonValue>）
    };

    // JSON 类型别名
    using JsonNull = std::nullptr_t;                           ///< JSON null 类型
    using JsonBool = bool;                                     ///< JSON 布尔类型
    using JsonNumber = double;                                 ///< JSON 数值类型
    using JsonString = std::u8string;                          ///< JSON 字符串类型（UTF-8）
    using JsonArray = std::vector<JsonValue>;                  ///< JSON 数组类型
    using JsonObject = std::unordered_map<std::u8string, JsonValue>; ///< JSON 对象类型

    /**
     * @brief JSON 值的 variant 类型
     *
     * 按 JsonValueType 枚举顺序存储六种 JSON 类型
     */
    using JsonVariant = std::variant<
        JsonNull,
        JsonBool,
        JsonNumber,
        JsonString,
        JsonArray,
        JsonObject
    >;

    /**
     * @brief 概念：类型 T 是 JSON 类型之一
     *
     * 用于约束模板参数，确保只能访问 variant 中实际存在的类型
     */
    template<typename T>
    concept JsonTy = requires
    {
        []<std::size_t... Is>(std::index_sequence<Is...>) -> bool {
            return (std::same_as<T, std::variant_alternative_t<Is, JsonVariant> > || ...);
        }(std::make_index_sequence<std::variant_size_v<JsonVariant> >{});
    };

    /**
     * @brief 概念：整数类型（不包括 bool）
     *
     * 用于数组索引操作，允许 int/size_t 等整数类型作为下标
     */
    template<typename T>
    concept IntegerLike = std::integral<T> && !std::same_as<T, bool>;

    /**
     * @brief 概念：UTF-8 字符串类型
     *
     * 包括 std::u8string、const char8_t*、char8_t*
     */
    template<typename T>
    concept U8Stringlike = std::same_as<std::decay_t<T>, std::u8string> ||
                           std::same_as<std::decay_t<T>, const char8_t *> ||
                           std::same_as<std::decay_t<T>, char8_t *>;

    /**
     * @brief 概念：普通字符串类型
     *
     * 包括 std::string、const char*、char*
     */
    template<typename T>
    concept NormalStringLike = std::same_as<std::decay_t<T>, std::string> ||
                               std::same_as<std::decay_t<T>, const char *> ||
                               std::same_as<std::decay_t<T>, char *>;

    /**
     * @brief 概念：所有字符串类型（UTF-8 + 普通字符串）
     *
     * 统一处理两种字符串类型，内部自动转换为 std::u8string
     */
    template<typename T>
    concept StringLike = NormalStringLike<T> || U8Stringlike<T>;

    /**
     * @brief 获取 JsonVariant 的类型名称字符串
     * @param v JSON variant 值
     * @return 类型名称（如 "null"、"bool"、"number" 等）
     */
    inline std::string jsonTypeName(const JsonVariant &v) {
        switch (static_cast<JsonValueType>(v.index())) {
            case JsonValueType::Null: return "null";
            case JsonValueType::Bool: return "bool";
            case JsonValueType::Number: return "number";
            case JsonValueType::String: return "string";
            case JsonValueType::Array: return "array";
            case JsonValueType::Object: return "object";
            default: return "unknown";
        }
    }

    /**
     * @brief 工具函数命名空间
     *
     * 包含字符串转换、UTF-8 处理、字符分类等内部辅助函数
     */
    namespace util {

        /**
         * @brief 将范围内的元素用分隔符连接成字符串
         *
         * 用于序列化数组和对象，将每个元素通过 func 转换后用 delimiter 连接
         *
         * @tparam Range 范围类型
         * @tparam Func 转换函数类型
         * @param range 输入范围
         * @param delimiter 分隔符
         * @param func 元素转换函数
         * @return 连接后的字符串
         */
        template<std::ranges::input_range Range, typename Func>
        std::string join(const Range &range, const std::string_view delimiter, Func &&func) {
            std::string result;
            result.reserve(32);

            bool first = true;
            for (const auto &elem: range) {
                if (!first) {
                    result += delimiter;
                }
                result += func(elem);
                first = false;
            }

            return result;
        }

        /**
         * @brief 检查 double 值是否可以安全转换为 int
         *
         * 排除 NaN、Inf、超出 int 范围、非整数值的情况
         * 用于序列化时决定是否省略小数点
         *
         * @param value 要检查的 double 值
         * @return 如果可以安全转换为 int 返回 true
         */
        inline bool isSafeToConvertToInt(double value) {
            if (std::isnan(value) || std::isinf(value)) {
                return false;
            }

            if (value < static_cast<double>(std::numeric_limits<int>::min()) ||
                value > static_cast<double>(std::numeric_limits<int>::max())) {
                return false;
            }

            double int_part;
            if (std::modf(value, &int_part) != 0.0) {
                return false;
            }

            return true;
        }

        /**
         * @brief 将 Unicode 码点转换为 UTF-8 字节序列
         *
         * 支持 U+0000 到 U+10FFFF 的完整 Unicode 范围
         * 用于解析 \uXXXX 转义序列
         *
         * @param codepoint Unicode 码点
         * @return UTF-8 字节序列（作为 std::string）
         */
        inline std::string utf16_to_utf8(uint32_t codepoint) {
            std::string result;

            if (codepoint <= 0x7F) {
                // 1 字节：0xxxxxxx
                result += static_cast<char>(codepoint);
            } else if (codepoint <= 0x7FF) {
                // 2 字节：110xxxxx 10xxxxxx
                result += static_cast<char>(0xC0 | (codepoint >> 6));
                result += static_cast<char>(0x80 | (codepoint & 0x3F));
            } else if (codepoint <= 0xFFFF) {
                // 3 字节：1110xxxx 10xxxxxx 10xxxxxx
                result += static_cast<char>(0xE0 | (codepoint >> 12));
                result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (codepoint & 0x3F));
            } else {
                // 4 字节：11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                result += static_cast<char>(0xF0 | (codepoint >> 18));
                result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (codepoint & 0x3F));
            }

            return result;
        }

        /**
         * @brief 统计 UTF-8 字符串中的字符数（非字节数）
         *
         * 根据 UTF-8 首字节判断字符长度，正确处理多字节字符
         *
         * @param str UTF-8 字符串
         * @return 字符数
         */
        inline size_t utf8_char_count(const std::u8string &str) {
            size_t count = 0;
            for (size_t i = 0; i < str.size();) {
                const char8_t c = str[i];
                if ((c & 0xF8) == 0xF0) i += 4;      // 4字节字符
                else if ((c & 0xF0) == 0xE0) i += 3;  // 3字节字符
                else if ((c & 0xE0) == 0xC0) i += 2;  // 2字节字符
                else i += 1;                            // 1字节字符或无效字节
                count++;
            }
            return count;
        }

        /**
         * @brief 字符串转换的内部实现
         *
         * 使用 reinterpret_cast 在 std::string 和 std::u8string 之间转换
         * 两者内存布局相同，只是类型不同
         */
        namespace string_cov_detail {
            /// std::string -> std::u8string（拷贝）
            inline std::u8string to_u8string_impl(const std::string &s) {
                return {reinterpret_cast<const char8_t *>(s.data()), s.size()};
            }

            /// const char* -> std::u8string
            inline std::u8string to_u8string_impl(const char *s) {
                return {reinterpret_cast<const char8_t *>(s)};
            }

            /// std::string&& -> std::u8string（移动语义）
            inline std::u8string to_u8string_impl(std::string &&s) {
                return {reinterpret_cast<const char8_t *>(s.data()), s.size()};
            }

            /// UTF-8 类型直接透传
            template<U8Stringlike T>
            std::u8string to_u8string_impl(T &&s) {
                return std::u8string(std::forward<T>(s));
            }

            /// std::u8string -> std::string（拷贝）
            inline std::string to_std_string_impl(const std::u8string &s) {
                return {reinterpret_cast<const char *>(s.data()), s.size()};
            }

            /// const char8_t* -> std::string
            inline std::string to_std_string_impl(const char8_t *s) {
                return {reinterpret_cast<const char *>(s)};
            }

            /// std::u8string&& -> std::string（移动语义）
            inline std::string to_std_string_impl(std::u8string &&s) {
                return {reinterpret_cast<const char *>(s.data()), s.size()};
            }

            /// 普通字符串类型直接透传
            template<NormalStringLike T>
            std::string to_std_string_impl(T &&s) {
                return std::string(std::forward<T>(s));
            }
        }

        /**
         * @brief 将任意字符串类型转换为 std::u8string
         *
         * 支持 std::string、const char*、std::u8string、const char8_t* 等类型
         * 对于普通字符串，通过 reinterpret_cast 转换（内存布局相同）
         */
        template<StringLike T>
        std::u8string to_u8string(T &&s) {
            return string_cov_detail::to_u8string_impl(std::forward<T>(s));
        }

        /**
         * @brief 将任意字符串类型转换为 std::string
         *
         * 用于对外 API 返回普通字符串
         */
        template<StringLike T>
        std::string to_std_string(T &&s) {
            return string_cov_detail::to_std_string_impl(std::forward<T>(s));
        }

        /// 判断字符是否为空白字符（空格、制表符、换行符、回车符）
        inline bool isspace(char8_t c) {
            return c == u8' ' || c == u8'\t' || c == u8'\n' || c == u8'\r';
        }

        /// 判断字符是否为数字（0-9）
        inline bool isdigit(char8_t c) {
            return c >= u8'0' && c <= u8'9';
        }

        /// 判断字符是否为十六进制数字（0-9、a-f、A-F）
        inline bool isxdigit(char8_t c) {
            return isdigit(c) ||
                   (c >= u8'a' && c <= u8'f') ||
                   (c >= u8'A' && c <= u8'F');
        }

        /**
         * @brief 验证单个 UTF-8 序列是否合法
         *
         * 检查内容：
         *   - 合法的起始字节
         *   - 正确的后续字节数
         *   - 过长编码（如 0xC0 0x80）
         *   - 代理对（0xD800-0xDFFF）
         *   - 超出 Unicode 范围（> 0x10FFFF）
         *
         * @param data 指向 UTF-8 序列起始位置的指针
         * @param remaining 剩余可用字节数
         * @return 序列的字节数，如果非法返回 0
         */
        inline size_t validate_utf8_sequence(const char8_t *data, size_t remaining) {
            const auto c = static_cast<uint8_t>(data[0]);

            if (c <= 0x7F) {
                return 1; // ASCII
            }

            size_t len = 0;
            uint32_t codepoint = 0;

            if ((c & 0xE0) == 0xC0) {
                len = 2;
                codepoint = c & 0x1F;
            } else if ((c & 0xF0) == 0xE0) {
                len = 3;
                codepoint = c & 0x0F;
            } else if ((c & 0xF8) == 0xF0) {
                len = 4;
                codepoint = c & 0x07;
            } else {
                return 0; // 非法的起始字节
            }

            if (remaining < len) return 0; // 不够长

            for (size_t i = 1; i < len; ++i) {
                const auto b = static_cast<uint8_t>(data[i]);
                if ((b & 0xC0) != 0x80) return 0; // 非法的后续字节
                codepoint = (codepoint << 6) | (b & 0x3F);
            }

            // 检查过长编码
            if (len == 2 && codepoint < 0x80) return 0;
            if (len == 3 && codepoint < 0x800) return 0;
            if (len == 4 && codepoint < 0x10000) return 0;

            // 检查代理对
            if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return 0;

            // 检查超出 Unicode 范围
            if (codepoint > 0x10FFFF) return 0;

            return len;
        }
    }

    /**
     * @brief JSON 值类 — 核心 API
     *
     * 包装 JsonVariant，提供类型安全的访问和操作接口
     * 支持从多种类型构造，自动处理类型转换
     *
     * 使用示例：
     *   JsonValue j;                // null
     *   JsonValue j = 42;           // number
     *   JsonValue j = "hello";      // string
     *   JsonValue j = array({1,2}); // array
     *   JsonValue j = object({});   // object
     */
    class JsonValue {
    public:
        /// 数组最大元素数限制，防止无限扩容
        static constexpr size_t max_array_size = 1000000;

        // ========== 构造函数 ==========

        /// 默认构造：null 值
        JsonValue() : _value{nullptr} {
        }

        /// 从 null 构造
        JsonValue(JsonNull) : _value{nullptr} {
        }

        /// 从 bool 构造
        JsonValue(JsonBool b) : _value{b} {
        }

        /// 从 double 构造
        JsonValue(JsonNumber n) : _value{n} {
        }

        /// 从 int 构造（自动转换为 double）
        JsonValue(const int n) : _value{static_cast<JsonNumber>(n)} {
        }

        /// 从 C 字符串构造（自动转换为 u8string）
        JsonValue(const char *s) : _value{util::to_u8string(s)} {
        }

        /// 从 char8_t 字符串构造
        JsonValue(const char8_t *s) : _value{s} {
        }

        /// 从 std::string 构造（自动转换为 u8string）
        JsonValue(const std::string &s) : _value{util::to_u8string(s)} {
        }

        /// 从 std::string 移动构造
        JsonValue(std::string &&s) : _value{util::to_u8string(s)} {
        }

        /// 从 u8string 拷贝构造
        JsonValue(const JsonString &s) : _value{s} {
        }

        /// 从 u8string 移动构造
        JsonValue(JsonString &&s) noexcept : _value{std::move(s)} {
        }

        /// 从数组拷贝构造
        JsonValue(const JsonArray &arr) : _value{arr} {
        }

        /// 从数组移动构造
        JsonValue(JsonArray &&arr) noexcept : _value{std::move(arr)} {
        }

        /// 从对象拷贝构造
        JsonValue(const JsonObject &obj) : _value{obj} {
        }

        /// 从对象移动构造
        JsonValue(JsonObject &&obj) noexcept : _value{std::move(obj)} {
        }

        // ========== 类型检查 ==========

        /**
         * @brief 获取当前值的类型
         * @return JsonValueType 枚举值
         */
        JsonValueType type() const {
            return static_cast<JsonValueType>(_value.index());
        }

        /**
         * @brief 获取当前值的类型名称
         * @return 类型名称字符串（如 "null"、"bool" 等）
         */
        std::string typeName() const {
            return jsonTypeName(_value);
        }

        /**
         * @brief 检查当前值是否为指定类型
         * @tparam T JSON 类型（如 JsonNull、JsonBool、JsonNumber 等）
         * @return 如果是该类型返回 true
         */
        template<JsonTy T>
        constexpr bool is() const {
            return std::holds_alternative<T>(_value);
        }

        /**
         * @brief 获取指定类型的引用（类型不匹配时抛异常）
         * @tparam T JSON 类型
         * @return 对应类型的引用
         * @throw std::runtime_error 类型不匹配时
         */
        template<JsonTy T>
        T &as() {
            if (!is<T>()) {
                std::ostringstream oss;
                oss << "JsonValue type mismatch: requested '"
                        << typeid(T).name()
                        << "' but actual type is '"
                        << typeName() << "'";
                throw std::runtime_error(oss.str());
            }
            return std::get<T>(_value);
        }

        /// @brief as() 的 const 版本
        template<JsonTy T>
        const T &as() const {
            if (!is<T>()) {
                std::ostringstream oss;
                oss << "JsonValue type mismatch: requested '"
                        << typeid(T).name()
                        << "' but actual type is '"
                        << typeName() << "'";
                throw std::runtime_error(oss.str());
            }
            return std::get<T>(_value);
        }

        /**
         * @brief 获取字符串值为 std::string（方便对外 API）
         * @return std::string 类型的字符串值
         * @throw std::runtime_error 当前值不是字符串时
         */
        std::string as_string() const {
            if (!is<JsonString>()) {
                throw std::runtime_error("JsonValue type mismatch: not a string");
            }
            return util::to_std_string(std::get<JsonString>(_value));
        }

        /**
         * @brief 获取容器大小
         *
         * - JsonString: 返回字符数（非字节数）
         * - JsonArray: 返回元素数
         * - JsonObject: 返回键值对数
         *
         * @return 大小值
         * @throw std::runtime_error 类型不支持 size() 时
         */
        size_t size() const {
            if (is<JsonString>()) return util::utf8_char_count(std::get<JsonString>(_value));
            if (is<JsonArray>()) return std::get<JsonArray>(_value).size();
            if (is<JsonObject>()) return std::get<JsonObject>(_value).size();
            throw std::runtime_error("JsonValue type mismatch: Type does not support size()");
        }

        // ========== 数组操作 ==========

        /**
         * @brief 访问数组元素（带边界检查）
         * @param index 数组索引
         * @return 对应位置的 JsonValue 引用
         * @throw std::runtime_error 当前值不是数组时
         * @throw std::out_of_range 索引越界时
         */
        JsonValue &at(const size_t index) {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            auto &arr = std::get<JsonArray>(_value);
            if (index >= arr.size()) {
                throw std::out_of_range("JsonArray index out of range");
            }
            return arr[index];
        }

        /// @brief at() 的 const 版本
        const JsonValue &at(const size_t index) const {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            auto &arr = std::get<JsonArray>(_value);
            if (index >= arr.size()) {
                throw std::out_of_range("JsonArray index out of range");
            }
            return arr[index];
        }

        /**
         * @brief 数组下标操作符（自动扩容）
         *
         * null 值会自动转换为数组
         * 超出当前大小时自动扩容（受 max_array_size 限制）
         *
         * @tparam T 整数类型
         * @param index 数组索引
         * @return 对应位置的 JsonValue 引用
         * @throw std::runtime_error 当前值不是数组/null 时
         * @throw std::out_of_range 索引超过 max_array_size 时
         */
        template<IntegerLike T>
        JsonValue &operator[](const T index) {
            if (is<JsonNull>()) {
                _value = JsonArray();
            } else if (!is<JsonArray>()) {
                throw std::runtime_error("Not a JsonArray value");
            }

            auto &arr = std::get<JsonArray>(_value);
            const auto idx = static_cast<size_t>(index);
            if (idx >= arr.size()) {
                if (idx >= max_array_size) {
                    throw std::out_of_range("JsonArray index exceeds maximum allowed size");
                }
                arr.resize(idx + 1, JsonValue(nullptr));
            }
            return arr[idx];
        }

        /**
         * @brief 向数组末尾添加元素
         *
         * null 值会自动转换为数组
         *
         * @tparam T JsonValue 类型
         * @param value 要添加的值
         * @throw std::runtime_error 当前值不是数组/null 时
         */
        template<typename T>
            requires std::same_as<std::decay_t<T>, JsonValue>
        void push_back(T &&value) {
            if (is<JsonNull>()) {
                _value = JsonArray();
            } else if (!is<JsonArray>()) {
                throw std::runtime_error("Cannot push_back to non-JsonArray value");
            }

            std::get<JsonArray>(_value).push_back(std::forward<T>(value));
        }

        /**
         * @brief 原地构造并添加元素到数组末尾
         *
         * 参数直接传递给 JsonValue 的构造函数，避免临时对象
         *
         * @tparam Args 构造参数类型
         * @param args 构造参数
         * @throw std::runtime_error 当前值不是数组/null 时
         */
        template<typename... Args>
        void emplace_back(Args &&... args) {
            if (is<JsonNull>()) {
                _value = JsonArray();
            } else if (!is<JsonArray>()) {
                throw std::runtime_error("Cannot emplace_back to non-JsonArray value");
            }

            std::get<JsonArray>(_value).emplace_back(std::forward<Args>(args)...);
        }

        // ========== 对象操作 ==========

        /**
         * @brief 向对象插入键值对
         *
         * null 值会自动转换为对象
         * 如果键已存在，覆盖其值
         *
         * @tparam T 字符串类型
         * @param key 键名
         * @param value 值
         * @throw std::runtime_error 当前值不是对象/null 时
         */
        template<StringLike T>
        void insert(T &&key, const JsonValue &value) {
            if (is<JsonNull>()) {
                _value = JsonObject();
            } else if (!is<JsonObject>()) {
                throw std::runtime_error("Cannot insert to non-JsonObject value");
            }
            auto u8key = util::to_u8string(std::forward<T>(key));
            std::get<JsonObject>(_value)[std::move(u8key)] = value;
        }

        /**
         * @brief 访问对象成员（带边界检查）
         * @tparam T 字符串类型
         * @param key 键名
         * @return 对应键的 JsonValue 引用
         * @throw std::runtime_error 当前值不是对象时
         * @throw std::out_of_range 键不存在时
         */
        template<StringLike T>
        JsonValue &at(T &&key) {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            auto u8key = util::to_u8string(std::forward<T>(key));
            auto &obj = std::get<JsonObject>(_value);
            if (!obj.contains(u8key)) throw std::out_of_range("JsonObject key not found");
            return obj[u8key];
        }

        /// @brief at() 的 const 版本
        template<StringLike T>
        const JsonValue &at(T &&key) const {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            auto u8key = util::to_u8string(std::forward<T>(key));
            auto &obj = std::get<JsonObject>(_value);
            if (!obj.contains(u8key)) throw std::out_of_range("JsonObject key not found");
            return obj.at(u8key);
        }

        /**
         * @brief 对象下标操作符（自动创建）
         *
         * null 值会自动转换为对象
         * 如果键不存在，自动创建 null 值
         *
         * @tparam T 字符串类型
         * @param key 键名
         * @return 对应键的 JsonValue 引用
         * @throw std::runtime_error 当前值不是对象/null 时
         */
        template<StringLike T>
        JsonValue &operator[](T &&key) {
            if (is<JsonNull>()) {
                _value = JsonObject();
            } else if (!is<JsonObject>()) {
                throw std::runtime_error("Not a JsonObject value");
            }
            auto u8key = util::to_u8string(std::forward<T>(key));
            return std::get<JsonObject>(_value)[std::move(u8key)];
        }

        /**
         * @brief 检查对象是否包含指定键
         * @tparam T 字符串类型
         * @param key 键名
         * @return 如果包含返回 true
         * @throw std::runtime_error 当前值不是对象时
         */
        template<StringLike T>
        bool contains(T &&key) const {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            auto u8key = util::to_u8string(std::forward<T>(key));
            return std::get<JsonObject>(_value).contains(u8key);
        }

        // ========== JSON Pointer (RFC 6901) ==========

        /**
         * @brief 通过 JSON Pointer 路径访问值
         *
         * 用法：j.resolve("/a/b/0") 等价于 j["a"]["b"][0]
         * 转义规则：~0 代表 ~，~1 代表 /
         * 空指针 "" 引用根值自身
         *
         * @tparam T 字符串类型
         * @param pointer JSON Pointer 路径
         * @return 路径指向的 JsonValue 引用
         * @throw std::runtime_error 路径格式错误时
         * @throw std::out_of_range 路径不存在时
         */
        template<StringLike T>
        JsonValue &resolve(T &&pointer) {
            auto ptr = util::to_u8string(std::forward<T>(pointer));
            return resolve_impl(ptr);
        }

        /// @brief resolve() 的 const 版本
        template<StringLike T>
        const JsonValue &resolve(T &&pointer) const {
            auto ptr = util::to_u8string(std::forward<T>(pointer));
            return resolve_impl(ptr);
        }

        // ========== JSON Merge Patch (RFC 7396) ==========

        /**
         * @brief 将 patch 合并到当前值（原地修改）
         *
         * 合并规则：
         *   - patch 不是 object → 直接替换当前值
         *   - patch 是 object → 遍历 patch 的每个键：
         *     - 值为 null → 从当前 object 删除该键
         *     - 值是 object 且当前键也是 object → 递归合并
         *     - 否则 → 直接覆盖
         *
         * @param patch 合并补丁
         */
        void merge(const JsonValue &patch) {
            if (!patch.is<JsonObject>()) {
                _value = patch._value;
                return;
            }

            // 当前值不是 object 时，先转为空 object
            if (!is<JsonObject>()) {
                _value = JsonObject();
            }

            auto &target = std::get<JsonObject>(_value);
            const auto &patch_obj = std::get<JsonObject>(patch._value);

            for (const auto &[key, value] : patch_obj) {
                if (value.is<JsonNull>()) {
                    // null 值表示删除该键
                    target.erase(key);
                } else if (value.is<JsonObject>() && target.contains(key) && target[key].is<JsonObject>()) {
                    // 两边都是 object，递归合并
                    target[key].merge(value);
                } else {
                    // 其他情况直接覆盖
                    target[key] = value;
                }
            }
        }

        // ========== 序列化 ==========

        /**
         * @brief 将值序列化为 JSON 字符串
         *
         * @param indent 缩进空格数（0 或负数表示紧凑格式）
         * @return JSON 字符串
         */
        std::string serialize(const int indent = 0) const {
            if (indent <= 0) {
                // 紧凑格式
                switch (type()) {
                    case JsonValueType::Null:
                        return "null";
                    case JsonValueType::Bool:
                        return std::get<JsonBool>(_value) ? "true" : "false";
                    case JsonValueType::Number: {
                        const auto num = std::get<JsonNumber>(_value);
                        // 整数省略小数点
                        if (util::isSafeToConvertToInt(num)) return std::to_string(static_cast<int>(num));
                        return serialize_number(num);
                    }
                    case JsonValueType::String:
                        return serialize_string(std::get<JsonString>(_value));
                    case JsonValueType::Array: {
                        const auto &arr = std::get<JsonArray>(_value);
                        const auto result = util::join(arr, ",", [](const JsonValue &v) { return v.serialize(); });
                        return "[" + result + "]";
                    }
                    case JsonValueType::Object: {
                        const auto &obj = std::get<JsonObject>(_value);
                        const auto result = util::join(obj, ",", [](const auto &kv) {
                            const auto &[key, value] = kv;
                            return serialize_string(key) + ":" + value.serialize();
                        });
                        return "{" + result + "}";
                    }
                }
            }
            // 格式化输出
            return serialize_with_indent(indent, 0);
        }

        // ========== 遍历 ==========

        /**
         * @brief 遍历数组（非 const 版本）
         * @tparam Func 回调函数类型
         * @param func 回调函数，参数为 JsonValue&
         * @throw std::runtime_error 当前值不是数组时
         */
        template<typename Func>
        void for_each_array(Func &&func) {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            for (auto &arr = std::get<JsonArray>(_value); auto &elem: arr) {
                func(elem);
            }
        }

        /// @brief for_each_array() 的 const 版本
        template<typename Func>
        void for_each_array(Func &&func) const {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            for (const auto &arr = std::get<JsonArray>(_value); const auto &elem: arr) {
                func(elem);
            }
        }

        /**
         * @brief 遍历对象（非 const 版本）
         * @tparam Func 回调函数类型
         * @param func 回调函数，参数为 (const std::u8string& key, JsonValue& value)
         * @throw std::runtime_error 当前值不是对象时
         */
        template<typename Func>
        void for_each_object(Func &&func) {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            for (auto &obj = std::get<JsonObject>(_value); auto &[key, value]: obj) {
                func(key, value);
            }
        }

        /// @brief for_each_object() 的 const 版本
        template<typename Func>
        void for_each_object(Func &&func) const {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            for (const auto &obj = std::get<JsonObject>(_value); const auto &[key, value]: obj) {
                func(key, value);
            }
        }

        // ========== 底层容器访问（迭代器支持） ==========

        /**
         * @brief 获取底层数组引用，兼容 range-based for
         *
         * 用法：for (auto& v : j.as_array()) { ... }
         *
         * @return JsonArray 引用
         * @throw std::runtime_error 当前值不是数组时
         */
        JsonArray &as_array() {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            return std::get<JsonArray>(_value);
        }

        /// @brief as_array() 的 const 版本
        const JsonArray &as_array() const {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            return std::get<JsonArray>(_value);
        }

        /**
         * @brief 获取底层对象引用，兼容 range-based for
         *
         * 用法：for (auto& [key, val] : j.as_object()) { ... }
         *
         * @return JsonObject 引用
         * @throw std::runtime_error 当前值不是对象时
         */
        JsonObject &as_object() {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            return std::get<JsonObject>(_value);
        }

        /// @brief as_object() 的 const 版本
        const JsonObject &as_object() const {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            return std::get<JsonObject>(_value);
        }

    private:
        /// 底层存储：使用 variant 存储六种 JSON 类型
        JsonVariant _value;

        // ========== 私有辅助方法 ==========

        /**
         * @brief 数字序列化辅助函数
         *
         * 使用 snprintf + %.15g 格式化，符合 JSON 规范
         * - NaN/Inf 序列化为 "null"
         * - 自动去除尾随零
         *
         * @param num 要序列化的数字
         * @return JSON 格式的数字字符串
         */
        static std::string serialize_number(double num) {
            if (std::isnan(num) || std::isinf(num)) {
                return "null";
            }
            char buf[32];
            // %.15g 在精度和可读性之间取得平衡
            snprintf(buf, sizeof(buf), "%.15g", num);
            // 如果结果包含小数点但没有指数符号，去除尾随零
            std::string result(buf);
            auto dot_pos = result.find('.');
            if (dot_pos != std::string::npos) {
                auto e_pos = result.find('e');
                if (e_pos == std::string::npos) e_pos = result.find('E');
                if (e_pos == std::string::npos) {
                    // 去除小数部分的尾随零
                    size_t last_nonzero = result.find_last_not_of('0');
                    if (last_nonzero == dot_pos) {
                        result.erase(dot_pos); // 小数点后全是零，去除小数点
                    } else {
                        result.erase(last_nonzero + 1);
                    }
                }
            }
            return result;
        }

        // JSON Pointer 解析的内部实现
        // 将 "~1" 解码为 "/"，"~0" 解码为 "~"
        static std::u8string unescape_pointer_token(const std::u8string &token) {
            std::u8string result;
            result.reserve(token.size());
            for (size_t i = 0; i < token.size(); ++i) {
                if (token[i] == u8'~' && i + 1 < token.size()) {
                    if (token[i + 1] == u8'0') {
                        result += u8'~';
                        ++i;
                    } else if (token[i + 1] == u8'1') {
                        result += u8'/';
                        ++i;
                    } else {
                        result += token[i];
                    }
                } else {
                    result += token[i];
                }
            }
            return result;
        }

        // 判断字符串是否为非负整数（用于数组索引判断）
        static bool is_array_index(const std::u8string &token) {
            if (token.empty()) return false;
            for (auto c : token) {
                if (!util::isdigit(c)) return false;
            }
            return true;
        }

        JsonValue &resolve_impl(const std::u8string &pointer) {
            if (pointer.empty()) return *this;
            if (pointer[0] != u8'/') {
                throw std::runtime_error("JSON Pointer must start with '/' or be empty");
            }

            // 按 '/' 分割并逐级解析
            JsonValue *current = this;
            size_t pos = 1;
            while (pos <= pointer.size()) {
                size_t next = pointer.find(u8'/', pos);
                if (next == std::u8string::npos) next = pointer.size();

                std::u8string token = unescape_pointer_token(pointer.substr(pos, next - pos));

                if (current->is<JsonObject>()) {
                    auto &obj = std::get<JsonObject>(current->_value);
                    if (!obj.contains(token)) {
                        throw std::out_of_range("JSON Pointer: key not found");
                    }
                    current = &obj[token];
                } else if (current->is<JsonArray>()) {
                    if (!is_array_index(token)) {
                        throw std::out_of_range("JSON Pointer: invalid array index");
                    }
                    size_t idx = 0;
                    for (auto c : token) {
                        idx = idx * 10 + (c - u8'0');
                    }
                    auto &arr = std::get<JsonArray>(current->_value);
                    if (idx >= arr.size()) {
                        throw std::out_of_range("JSON Pointer: array index out of range");
                    }
                    current = &arr[idx];
                } else {
                    throw std::runtime_error("JSON Pointer: cannot traverse into non-container value");
                }

                pos = next + 1;
            }
            return *current;
        }

        const JsonValue &resolve_impl(const std::u8string &pointer) const {
            if (pointer.empty()) return *this;
            if (pointer[0] != u8'/') {
                throw std::runtime_error("JSON Pointer must start with '/' or be empty");
            }

            const JsonValue *current = this;
            size_t pos = 1;
            while (pos <= pointer.size()) {
                size_t next = pointer.find(u8'/', pos);
                if (next == std::u8string::npos) next = pointer.size();

                std::u8string token = unescape_pointer_token(pointer.substr(pos, next - pos));

                if (current->is<JsonObject>()) {
                    const auto &obj = std::get<JsonObject>(current->_value);
                    if (!obj.contains(token)) {
                        throw std::out_of_range("JSON Pointer: key not found");
                    }
                    current = &obj.at(token);
                } else if (current->is<JsonArray>()) {
                    if (!is_array_index(token)) {
                        throw std::out_of_range("JSON Pointer: invalid array index");
                    }
                    size_t idx = 0;
                    for (auto c : token) {
                        idx = idx * 10 + (c - u8'0');
                    }
                    const auto &arr = std::get<JsonArray>(current->_value);
                    if (idx >= arr.size()) {
                        throw std::out_of_range("JSON Pointer: array index out of range");
                    }
                    current = &arr[idx];
                } else {
                    throw std::runtime_error("JSON Pointer: cannot traverse into non-container value");
                }

                pos = next + 1;
            }
            return *current;
        }

        /**
         * @brief 字符串序列化（添加引号和转义）
         *
         * 转义规则：
         *   - 特殊字符：\"、\\、\b、\f、\n、\r、\t
         *   - 控制字符（< 0x20）：\u00XX
         *
         * @param str 要序列化的字符串
         * @return 带引号的 JSON 字符串
         */
        static std::string serialize_string(const JsonString &str) {
            std::string result;
            result.reserve(str.size() * 2 + 2);

            result.push_back('"');

            for (char8_t c8: str) {
                switch (auto c = static_cast<unsigned char>(c8)) {
                    case '"': result += "\\\"";
                        break;
                    case '\\': result += "\\\\";
                        break;
                    case '\b': result += "\\b";
                        break;
                    case '\f': result += "\\f";
                        break;
                    case '\n': result += "\\n";
                        break;
                    case '\r': result += "\\r";
                        break;
                    case '\t': result += "\\t";
                        break;
                    default:
                        if (c < 0x20) {
                            // 控制字符使用 \u00XX 格式
                            constexpr char hex[] = "0123456789abcdef";
                            result += "\\u00";
                            result += hex[c >> 4];
                            result += hex[c & 0xF];
                        } else {
                            result += static_cast<char>(c);
                        }
                }
            }

            result.push_back('"');
            return result;
        }

        /**
         * @brief 带缩进的格式化序列化
         *
         * @param indent 每级缩进的空格数
         * @param current_indent 当前缩进级别
         * @return 格式化的 JSON 字符串
         */
        std::string serialize_with_indent(int indent, int current_indent) const {
            const std::string indent_str(current_indent, ' ');
            const std::string next_indent_str(current_indent + indent, ' ');

            switch (type()) {
                case JsonValueType::Null:
                    return "null";
                case JsonValueType::Bool:
                    return std::get<JsonBool>(_value) ? "true" : "false";
                case JsonValueType::Number: {
                    const auto num = std::get<JsonNumber>(_value);
                    if (util::isSafeToConvertToInt(num)) return std::to_string(static_cast<int>(num));
                    return serialize_number(num);
                }
                case JsonValueType::String:
                    return serialize_string(std::get<JsonString>(_value));
                case JsonValueType::Array: {
                    const auto &arr = std::get<JsonArray>(_value);
                    if (arr.empty()) return "[]";
                    std::string result = util::join(arr, ",\n", [&](const JsonValue &elem) {
                        return next_indent_str + elem.serialize_with_indent(indent, current_indent + indent);
                    });
                    result = "[\n" + result + "\n" + indent_str + "]";
                    return result;
                }
                case JsonValueType::Object: {
                    const auto &obj = std::get<JsonObject>(_value);
                    if (obj.empty()) return "{}";
                    std::string result = util::join(obj, ",\n", [&](const auto &kv) {
                        const auto &[key, value] = kv;
                        return next_indent_str + serialize_string(key) + ": " +
                               value.serialize_with_indent(indent, current_indent + indent);
                    });
                    result = "{\n" + result + "\n" + indent_str + "}";
                    return result;
                }
                default:
                    throw std::runtime_error("Unknown JSON type");
            }
        }
    };

    // ========== 工厂函数 ==========

    /**
     * @brief 创建 JSON 数组
     *
     * 用法：auto j = array({1, "hello", true});
     *
     * @param elements 初始化列表
     * @return 包含数组的 JsonValue
     */
    inline JsonValue array(const std::initializer_list<JsonValue> elements) {
        return {JsonArray(elements)};
    }

    /**
     * @brief 创建 JSON 对象
     *
     * 用法：auto j = object({{"key", "value"}, {"num", 42}});
     *
     * @param elements 键值对初始化列表
     * @return 包含对象的 JsonValue
     */
    inline JsonValue object(const std::initializer_list<std::pair<const std::string, JsonValue> > elements) {
        JsonObject obj;
        for (const auto &[key, value]: elements) {
            obj[util::to_u8string(key)] = value;
        }
        return {obj};
    }

    // ========== JSON 解析器 ==========

    /**
     * @brief JSON 解析器（递归下降）
     *
     * 支持完整的 JSON 语法，包括：
     *   - 所有 JSON 数据类型
     *   - Unicode 转义序列（\uXXXX）
     *   - 代理对（surrogate pairs）
     *   - UTF-8 验证
     *   - 深度限制防止栈溢出
     */
    class JsonParser {
    public:
        /// 默认最大嵌套深度
        static constexpr size_t default_max_depth = 256;

        /**
         * @brief 解析 JSON 字符串
         * @param json_str JSON 字符串（u8string）
         * @param max_depth 最大嵌套深度
         * @return 解析后的 JsonValue
         * @throw std::runtime_error 解析错误时
         */
        static JsonValue parse(const std::u8string &json_str, size_t max_depth = default_max_depth) {
            JsonParser parser(json_str, max_depth);
            JsonValue result = parser.parse_value();
            parser.skip_whitespace();
            if (parser._pos < parser._str.size()) {
                throw std::runtime_error("JSON String Parsing Error: Unexpected characters after JSON value");
            }
            return result;
        }

        /// @brief parse() 的 std::string 重载
        static JsonValue parse(const std::string &json_str, size_t max_depth = default_max_depth) {
            return parse(util::to_u8string(json_str), max_depth);
        }

    private:
        std::u8string _str;   ///< 输入字符串
        size_t _pos;          ///< 当前解析位置
        size_t _max_depth;    ///< 最大嵌套深度
        size_t _depth;        ///< 当前嵌套深度

        /// 构造函数
        explicit JsonParser(std::u8string json_str, size_t max_depth)
            : _str(std::move(json_str)), _pos(0), _max_depth(max_depth), _depth(0) {
        }

        /// 跳过空白字符
        void skip_whitespace() {
            while (_pos < _str.size() && util::isspace(_str[_pos])) {
                _pos++;
            }
        }

        /// 查看当前字符（不移动位置）
        [[nodiscard]] char8_t peek() const {
            if (_pos >= _str.size()) {
                throw std::runtime_error("JSON String Parsing Error: Unexpected end of JSON String");
            }
            return _str[_pos];
        }

        /// 读取当前字符并移动位置
        char8_t next() {
            if (_pos >= _str.size()) {
                throw std::runtime_error("JSON String Parsing Error: Unexpected end of JSON String");
            }
            return _str[_pos++];
        }

        /**
         * @brief 解析一个 JSON 值
         *
         * 根据当前字符判断类型，分发到对应的解析函数
         * 遇到数组或对象时检查深度限制
         */
        JsonValue parse_value() {
            skip_whitespace();
            if (_pos >= _str.size()) {
                throw std::runtime_error("JSON String Parsing Error: Unexpected end of JSON String");
            }

            const char8_t c = peek();
            // 深度检查：遇到数组或对象时检查是否超过最大深度
            if ((c == u8'[' || c == u8'{') && _depth >= _max_depth) {
                throw std::runtime_error("JSON String Parsing Error: Maximum nesting depth exceeded");
            }

            switch (c) {
                case u8'n': return parse_null();
                case u8't':
                case u8'f': return parse_bool();
                case u8'"': return parse_string();
                case u8'[': return parse_array();
                case u8'{': return parse_object();
                default:
                    if (c == u8'-' || util::isdigit(c)) {
                        return parse_number();
                    }
                    throw std::runtime_error(std::string("JSON String Parsing Error: Unexpected character: ") +
                                             static_cast<char>(c));
            }
        }

        /// 解析 null 值
        JsonValue parse_null() {
            if (_str.substr(_pos, 4) == u8"null") {
                _pos += 4;
                return JsonValue{nullptr};
            }
            throw std::runtime_error("JSON String Parsing Error: Invalid null value");
        }

        /// 解析布尔值（true/false）
        JsonValue parse_bool() {
            if (_str.substr(_pos, 4) == u8"true") {
                _pos += 4;
                return JsonValue{true};
            }
            if (_str.substr(_pos, 5) == u8"false") {
                _pos += 5;
                return JsonValue{false};
            }
            throw std::runtime_error("JSON String Parsing Error: Invalid boolean value");
        }

        /**
         * @brief 解析数字
         *
         * 支持格式：整数、小数、科学计数法（e/E）
         */
        JsonValue parse_number() {
            const size_t start = _pos;

            // 符号
            if (peek() == u8'-') {
                next();
            }

            // 整数部分
            if (peek() == u8'0') {
                next();
            } else if (util::isdigit(peek())) {
                while (_pos < _str.size() && util::isdigit(_str[_pos])) {
                    next();
                }
            } else {
                throw std::runtime_error("JSON String Parsing Error: Invalid number format");
            }

            // 小数部分
            if (_pos < _str.size() && peek() == u8'.') {
                next();
                if (_pos >= _str.size() || !util::isdigit(peek())) {
                    throw std::runtime_error("JSON String Parsing Error: Invalid number format");
                }
                while (_pos < _str.size() && util::isdigit(_str[_pos])) {
                    next();
                }
            }

            // 指数部分
            if (_pos < _str.size() && (peek() == u8'e' || peek() == u8'E')) {
                next();
                if (_pos < _str.size() && (peek() == u8'+' || peek() == u8'-')) {
                    next();
                }
                if (_pos >= _str.size() || !util::isdigit(peek())) {
                    throw std::runtime_error("JSON String Parsing Error: Invalid number format");
                }
                while (_pos < _str.size() && util::isdigit(_str[_pos])) {
                    next();
                }
            }

            // 转换为 std::string 以便使用 stod
            std::string num_str;
            num_str.reserve(_pos - start);
            for (size_t i = start; i < _pos; ++i) {
                num_str.push_back(static_cast<char>(_str[i]));
            }
            return JsonValue{std::stod(num_str)};
        }

        /**
         * @brief 解析字符串
         *
         * 支持：
         *   - 标准转义序列（\"、\\、\/、\b、\f、\n、\r、\t）
         *   - Unicode 转义（\uXXXX）
         *   - 代理对（surrogate pairs）
         *   - UTF-8 验证
         */
        JsonValue parse_string() {
            next(); // 跳过开头的引号
            std::u8string raw_result;

            while (_pos < _str.size()) {
                char8_t c = next();
                if (c == u8'"') {
                    // 遇到结束引号
                    return JsonValue{std::move(raw_result)};
                }
                if (c == u8'\\') {
                    // 转义序列
                    if (_pos >= _str.size()) {
                        throw std::runtime_error("JSON String Parsing Error: Invalid string escape");
                    }
                    c = next();
                    switch (c) {
                        case u8'"': raw_result += u8'"';
                            break;
                        case u8'\\': raw_result += u8'\\';
                            break;
                        case u8'/': raw_result += u8'/';
                            break;
                        case u8'b': raw_result += u8'\b';
                            break;
                        case u8'f': raw_result += u8'\f';
                            break;
                        case u8'n': raw_result += u8'\n';
                            break;
                        case u8'r': raw_result += u8'\r';
                            break;
                        case u8't': raw_result += u8'\t';
                            break;
                        case u8'u': {
                            // Unicode 转义：\uXXXX
                            if (_pos + 4 > _str.size()) {
                                throw std::runtime_error("JSON String Parsing Error: Invalid unicode escape");
                            }

                            // 验证4个字符都是合法的十六进制数字
                            for (size_t i = 0; i < 4; ++i) {
                                if (!util::isxdigit(_str[_pos + i])) {
                                    throw std::runtime_error("JSON String Parsing Error: Invalid unicode escape");
                                }
                            }

                            // 将 u8string 的十六进制部分转换为普通字符串以便使用 stoul
                            std::string hex_str;
                            hex_str.reserve(4);
                            for (size_t i = 0; i < 4; ++i) {
                                hex_str.push_back(static_cast<char>(_str[_pos + i]));
                            }

                            uint32_t code = std::stoul(hex_str, nullptr, 16);
                            _pos += 4;

                            if (code >= 0xD800 && code <= 0xDBFF) {
                                if (_pos + 6 > _str.size() || _str[_pos] != u8'\\' || _str[_pos + 1] != u8'u') {
                                    throw std::runtime_error("JSON String Parsing Error: Invalid surrogate pair");
                                }

                                _pos += 2;

                                // 验证低代理项的4个十六进制字符
                                for (size_t i = 0; i < 4; ++i) {
                                    if (!util::isxdigit(_str[_pos + i])) {
                                        throw std::runtime_error("JSON String Parsing Error: Invalid unicode escape");
                                    }
                                }

                                hex_str.clear();
                                for (size_t i = 0; i < 4; ++i) {
                                    hex_str.push_back(static_cast<char>(_str[_pos + i]));
                                }

                                const uint32_t low = std::stoul(hex_str, nullptr, 16);
                                _pos += 4;

                                if (low < 0xDC00 || low > 0xDFFF) {
                                    throw std::runtime_error("JSON String Parsing Error: Invalid low surrogate");
                                }

                                code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                            }

                            // 将 utf16_to_utf8 的结果追加到 u8string
                            const std::string utf8_bytes = util::utf16_to_utf8(code);
                            for (char ch: utf8_bytes) {
                                raw_result += static_cast<char8_t>(ch);
                            }
                            break;
                        }
                        default:
                            throw std::runtime_error(
                                std::string("JSON String Parsing Error: Invalid escape character: \\") +
                                static_cast<char>(c));
                    }
                } else {
                    // 普通字符：验证 UTF-8 序列
                    size_t remaining = _str.size() - _pos + 1; // +1 因为已经 next() 了
                    size_t seq_len = util::validate_utf8_sequence(&_str[_pos - 1], remaining);
                    if (seq_len == 0) {
                        throw std::runtime_error("JSON String Parsing Error: Invalid UTF-8 sequence");
                    }
                    // 添加当前字节（已读取）
                    raw_result += c;
                    // 添加后续字节（如果有多字节序列）
                    for (size_t i = 1; i < seq_len; ++i) {
                        raw_result += next();
                    }
                }
            }
            throw std::runtime_error("JSON String Parsing Error: Unterminated string");
        }

        /**
         * @brief 解析数组
         *
         * 格式：[value1, value2, ...]
         * 递增深度计数器，解析完成后递减
         */
        JsonValue parse_array() {
            next(); // 跳过 '['
            ++_depth;
            JsonArray arr;

            skip_whitespace();
            if (peek() == u8']') {
                // 空数组
                next();
                --_depth;
                return JsonValue{arr};
            }

            while (true) {
                arr.push_back(parse_value());
                skip_whitespace();

                if (peek() == u8']') {
                    // 数组结束
                    next();
                    --_depth;
                    return JsonValue{arr};
                }

                if (peek() != u8',') {
                    throw std::runtime_error("JSON String Parsing Error: Expected ',' or ']' in array");
                }
                next();
            }
        }

        JsonValue parse_object() {
            next(); // 跳过 '{'
            ++_depth;
            JsonObject obj;

            skip_whitespace();
            if (peek() == u8'}') {
                next();
                --_depth;
                return JsonValue{obj};
            }

            while (true) {
                skip_whitespace();

                if (peek() != u8'"') {
                    throw std::runtime_error("JSON String Parsing Error: Expected string key in object");
                }
                JsonValue key_val = parse_string();
                std::u8string key = key_val.as<JsonString>();

                skip_whitespace();
                if (peek() != u8':') {
                    throw std::runtime_error("JSON String Parsing Error: Expected ':' after key in object");
                }
                next();

                obj[key] = parse_value();

                skip_whitespace();
                if (peek() == u8'}') {
                    next();
                    --_depth;
                    return JsonValue{obj};
                }

                if (peek() != u8',') {
                    throw std::runtime_error("JSON String Parsing Error: Expected ',' or '}' in object");
                }
                next();
            }
        }
    };

    // ========== 顶层 parse 函数 ==========

    /**
     * @brief 解析 JSON 字符串（u8string 版本）
     * @param json_str JSON 字符串
     * @param max_depth 最大嵌套深度（默认 256）
     * @return 解析后的 JsonValue
     * @throw std::runtime_error 解析错误时
     */
    inline JsonValue parse(const std::u8string &json_str, size_t max_depth = JsonParser::default_max_depth) {
        return JsonParser::parse(json_str, max_depth);
    }

    /**
     * @brief 解析 JSON 字符串（std::string 版本）
     * @param json_str JSON 字符串
     * @param max_depth 最大嵌套深度（默认 256）
     * @return 解析后的 JsonValue
     * @throw std::runtime_error 解析错误时
     */
    inline JsonValue parse(const std::string &json_str, size_t max_depth = JsonParser::default_max_depth) {
        return JsonParser::parse(json_str, max_depth);
    }

}

#endif //TINY_JSON_JSON_H
