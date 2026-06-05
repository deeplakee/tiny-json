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

namespace json {
    class JsonValue;

    enum class JsonValueType {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    using JsonNull = std::nullptr_t;
    using JsonBool = bool;
    using JsonNumber = double;
    using JsonString = std::u8string;
    using JsonArray = std::vector<JsonValue>;
    using JsonObject = std::unordered_map<std::u8string, JsonValue>;

    using JsonVariant = std::variant<
        JsonNull,
        JsonBool,
        JsonNumber,
        JsonString,
        JsonArray,
        JsonObject
    >;

    template<typename T>
    concept JsonTy = requires
    {
        []<std::size_t... Is>(std::index_sequence<Is...>) -> bool {
            return (std::same_as<T, std::variant_alternative_t<Is, JsonVariant> > || ...);
        }(std::make_index_sequence<std::variant_size_v<JsonVariant> >{});
    };

    template<typename T>
    concept IntegerLike = std::integral<T> && !std::same_as<T, bool>;

    template<typename T>
    concept U8Stringlike = std::same_as<std::decay_t<T>, std::u8string> ||
                           std::same_as<std::decay_t<T>, const char8_t *> ||
                           std::same_as<std::decay_t<T>, char8_t *>;

    template<typename T>
    concept NormalStringLike = std::same_as<std::decay_t<T>, std::string> ||
                               std::same_as<std::decay_t<T>, const char *> ||
                               std::same_as<std::decay_t<T>, char *>;

    template<typename T>
    concept StringLike = NormalStringLike<T> || U8Stringlike<T>;


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

    namespace util {
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

        inline std::string utf16_to_utf8(uint32_t codepoint) {
            std::string result;

            if (codepoint <= 0x7F) {
                result += static_cast<char>(codepoint);
            } else if (codepoint <= 0x7FF) {
                result += static_cast<char>(0xC0 | (codepoint >> 6));
                result += static_cast<char>(0x80 | (codepoint & 0x3F));
            } else if (codepoint <= 0xFFFF) {
                result += static_cast<char>(0xE0 | (codepoint >> 12));
                result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (codepoint & 0x3F));
            } else {
                result += static_cast<char>(0xF0 | (codepoint >> 18));
                result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (codepoint & 0x3F));
            }

            return result;
        }

        inline size_t utf8_char_count(const std::u8string &str) {
            size_t count = 0;
            for (size_t i = 0; i < str.size();) {
                const char8_t c = str[i];
                if ((c & 0x80) == 0) i += 1; // 1字节字符
                else if ((c & 0xE0) == 0xC0) i += 2; // 2字节字符
                else if ((c & 0xF0) == 0xE0) i += 3; // 3字节字符
                else if ((c & 0xF8) == 0xF0) i += 4; // 4字节字符
                else i += 1; // 无效字节，跳过
                count++;
            }
            return count;
        }

        namespace string_cov_detail {
            inline std::u8string to_u8string_impl(const std::string &s) {
                return {reinterpret_cast<const char8_t *>(s.data()), s.size()};
            }

            inline std::u8string to_u8string_impl(const char *s) {
                return {reinterpret_cast<const char8_t *>(s)};
            }

            inline std::u8string to_u8string_impl(std::string &&s) {
                return {reinterpret_cast<const char8_t *>(s.data()), s.size()};
            }

            // UTF-8 类型直接透传
            template<U8Stringlike T>
            std::u8string to_u8string_impl(T &&s) {
                return std::u8string(std::forward<T>(s));
            }

            inline std::string to_std_string_impl(const std::u8string &s) {
                return {reinterpret_cast<const char *>(s.data()), s.size()};
            }

            inline std::string to_std_string_impl(const char8_t *s) {
                return {reinterpret_cast<const char *>(s)};
            }

            inline std::string to_std_string_impl(std::u8string &&s) {
                return {reinterpret_cast<const char *>(s.data()), s.size()};
            }

            // 普通字符串类型直接透传
            template<NormalStringLike T>
            std::string to_std_string_impl(T &&s) {
                return std::string(std::forward<T>(s));
            }
        }

        template<StringLike T>
        std::u8string to_u8string(T &&s) {
            return string_cov_detail::to_u8string_impl(std::forward<T>(s));
        }

        template<StringLike T>
        std::string to_std_string(T &&s) {
            return string_cov_detail::to_std_string_impl(std::forward<T>(s));
        }

        // 判断字符是否为空白字符
        inline bool isspace(char8_t c) {
            return c == u8' ' || c == u8'\t' || c == u8'\n' || c == u8'\r';
        }

        // 判断字符是否为数字
        inline bool isdigit(char8_t c) {
            return c >= u8'0' && c <= u8'9';
        }

        // 判断字符是否为十六进制数字
        inline bool isxdigit(char8_t c) {
            return isdigit(c) ||
                   (c >= u8'a' && c <= u8'f') ||
                   (c >= u8'A' && c <= u8'F');
        }
    }

    class JsonValue {
    public:
        JsonValue() : _value{nullptr} {
        }

        JsonValue(JsonNull) : _value{nullptr} {
        }

        JsonValue(JsonBool b) : _value{b} {
        }

        JsonValue(JsonNumber n) : _value{n} {
        }

        JsonValue(const int n) : _value{static_cast<JsonNumber>(n)} {
        }

        JsonValue(const char *s) : _value{util::to_u8string(s)} {
        }

        JsonValue(const char8_t *s) : _value{s} {
        }

        JsonValue(const std::string &s) : _value{util::to_u8string(s)} {
        }

        JsonValue(std::string &&s) : _value{util::to_u8string(s)} {
        }

        JsonValue(const JsonString &s) : _value{s} {
        }

        JsonValue(JsonString &&s) noexcept : _value{std::move(s)} {
        }

        JsonValue(const JsonArray &arr) : _value{arr} {
        }

        JsonValue(JsonArray &&arr) noexcept : _value{std::move(arr)} {
        }

        JsonValue(const JsonObject &obj) : _value{obj} {
        }

        JsonValue(JsonObject &&obj) noexcept : _value{std::move(obj)} {
        }

        JsonValueType type() const {
            return static_cast<JsonValueType>(_value.index());
        }

        std::string typeName() const {
            return jsonTypeName(_value);
        }

        template<JsonTy T>
        constexpr bool is() const {
            return std::holds_alternative<T>(_value);
        }

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

        // 特殊处理：获取字符串为 std::string
        std::string as_string() const {
            if (!is<JsonString>()) {
                throw std::runtime_error("JsonValue type mismatch: not a string");
            }
            return util::to_std_string(std::get<JsonString>(_value));
        }

        // for JsonString
        // for JsonArray
        // for JsonObject
        size_t size() const {
            if (is<JsonString>()) return util::utf8_char_count(std::get<JsonString>(_value));
            if (is<JsonArray>()) return std::get<JsonArray>(_value).size();
            if (is<JsonObject>()) return std::get<JsonObject>(_value).size();
            throw std::runtime_error("JsonValue type mismatch: Type does not support size()");
        }

        // for JsonArray
        JsonValue &at(const size_t index) {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            auto &arr = std::get<JsonArray>(_value);
            if (index >= arr.size()) {
                throw std::out_of_range("JsonArray index out of range");
            }
            return arr[index];
        }

        // for JsonArray
        const JsonValue &at(const size_t index) const {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            auto &arr = std::get<JsonArray>(_value);
            if (index >= arr.size()) {
                throw std::out_of_range("JsonArray index out of range");
            }
            return arr[index];
        }

        // for JsonArray
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
                arr.resize(idx + 1, JsonValue(nullptr));
            }
            return arr[idx];
        }

        // for JsonArray
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

        // for JsonArray
        template<typename... Args>
        void emplace_back(Args &&... args) {
            if (is<JsonNull>()) {
                _value = JsonArray();
            } else if (!is<JsonArray>()) {
                throw std::runtime_error("Cannot emplace_back to non-JsonArray value");
            }

            std::get<JsonArray>(_value).emplace_back(std::forward<Args>(args)...);
        }

        // for JsonObject
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

        // for JsonObject
        template<StringLike T>
        JsonValue &at(T &&key) {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            auto u8key = util::to_u8string(std::forward<T>(key));
            auto &obj = std::get<JsonObject>(_value);
            if (!obj.contains(u8key)) throw std::out_of_range("JsonObject key not found");
            return obj[u8key];
        }

        // for JsonObject
        template<StringLike T>
        const JsonValue &at(T &&key) const {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            auto u8key = util::to_u8string(std::forward<T>(key));
            auto &obj = std::get<JsonObject>(_value);
            if (!obj.contains(u8key)) throw std::out_of_range("JsonObject key not found");
            return obj.at(u8key);
        }

        // for JsonObject
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

        // for JsonObject
        template<StringLike T>
        bool contains(T &&key) const {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            auto u8key = util::to_u8string(std::forward<T>(key));
            return std::get<JsonObject>(_value).contains(u8key);
        }

        std::string serialize(const int indent = 0) const {
            if (indent <= 0) {
                switch (type()) {
                    case JsonValueType::Null:
                        return "null";
                    case JsonValueType::Bool:
                        return std::get<JsonBool>(_value) ? "true" : "false";
                    case JsonValueType::Number: {
                        const auto num = std::get<JsonNumber>(_value);
                        if (util::isSafeToConvertToInt(num)) return std::to_string(static_cast<int>(num));
                        return std::to_string(num);
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
            return serialize_with_indent(indent, 0);
        }


        // 基于范围的 for 循环支持
        template<typename Func>
        void for_each_array(Func &&func) {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            for (auto &arr = std::get<JsonArray>(_value); auto &elem: arr) {
                func(elem);
            }
        }

        template<typename Func>
        void for_each_array(Func &&func) const {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            for (const auto &arr = std::get<JsonArray>(_value); const auto &elem: arr) {
                func(elem);
            }
        }

        template<typename Func>
        void for_each_object(Func &&func) {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            for (auto &obj = std::get<JsonObject>(_value); auto &[key, value]: obj) {
                func(key, value);
            }
        }

        template<typename Func>
        void for_each_object(Func &&func) const {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            for (const auto &obj = std::get<JsonObject>(_value); const auto &[key, value]: obj) {
                func(key, value);
            }
        }

        // 迭代器支持：直接访问底层容器，兼容 range-based for
        // 用法：for (auto& v : j.as_array())
        //       for (auto& [key, val] : j.as_object())

        JsonArray &as_array() {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            return std::get<JsonArray>(_value);
        }

        const JsonArray &as_array() const {
            if (!is<JsonArray>()) throw std::runtime_error("Not a JsonArray value");
            return std::get<JsonArray>(_value);
        }

        JsonObject &as_object() {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            return std::get<JsonObject>(_value);
        }

        const JsonObject &as_object() const {
            if (!is<JsonObject>()) throw std::runtime_error("Not a JsonObject value");
            return std::get<JsonObject>(_value);
        }

    private:
        JsonVariant _value;

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
                    return std::to_string(num);
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

    inline JsonValue array(const std::initializer_list<JsonValue> elements) {
        return {JsonArray(elements)};
    }

    inline JsonValue object(const std::initializer_list<std::pair<const std::string, JsonValue> > elements) {
        JsonObject obj;
        for (const auto &[key, value]: elements) {
            obj[util::to_u8string(key)] = value;
        }
        return {obj};
    }

    class JsonParser {
    public:
        static JsonValue parse(const std::u8string &json_str) {
            JsonParser parser(json_str);
            JsonValue result = parser.parse_value();
            parser.skip_whitespace();
            if (parser._pos < parser._str.size()) {
                throw std::runtime_error("JSON String Parsing Error: Unexpected characters after JSON value");
            }
            return result;
        }

        // 重载：支持从 std::string 解析
        static JsonValue parse(const std::string &json_str) {
            return parse(util::to_u8string(json_str));
        }

    private:
        std::u8string _str;
        size_t _pos;

        explicit JsonParser(std::u8string json_str) : _str(std::move(json_str)), _pos(0) {
        }

        void skip_whitespace() {
            while (_pos < _str.size() && util::isspace(_str[_pos])) {
                _pos++;
            }
        }

        [[nodiscard]] char8_t peek() const {
            if (_pos >= _str.size()) {
                throw std::runtime_error("JSON String Parsing Error: Unexpected end of JSON String");
            }
            return _str[_pos];
        }

        char8_t next() {
            if (_pos >= _str.size()) {
                throw std::runtime_error("JSON String Parsing Error: Unexpected end of JSON String");
            }
            return _str[_pos++];
        }

        JsonValue parse_value() {
            skip_whitespace();
            if (_pos >= _str.size()) {
                throw std::runtime_error("JSON String Parsing Error: Unexpected end of JSON String");
            }

            switch (const char8_t c = peek()) {
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

        JsonValue parse_null() {
            if (_str.substr(_pos, 4) == u8"null") {
                _pos += 4;
                return JsonValue{nullptr};
            }
            throw std::runtime_error("JSON String Parsing Error: Invalid null value");
        }

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

        JsonValue parse_number() {
            const size_t start = _pos;

            if (peek() == u8'-') {
                next();
            }

            if (peek() == u8'0') {
                next();
            } else if (util::isdigit(peek())) {
                while (_pos < _str.size() && util::isdigit(_str[_pos])) {
                    next();
                }
            } else {
                throw std::runtime_error("JSON String Parsing Error: Invalid number format");
            }

            if (_pos < _str.size() && peek() == u8'.') {
                next();
                if (_pos >= _str.size() || !util::isdigit(peek())) {
                    throw std::runtime_error("JSON String Parsing Error: Invalid number format");
                }
                while (_pos < _str.size() && util::isdigit(_str[_pos])) {
                    next();
                }
            }

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

            // 将 u8string 转换为普通字符串以便使用 stod
            std::string num_str;
            num_str.reserve(_pos - start);
            for (size_t i = start; i < _pos; ++i) {
                num_str.push_back(static_cast<char>(_str[i]));
            }
            return JsonValue{std::stod(num_str)};
        }

        JsonValue parse_string() {
            next(); // 跳过开头的引号
            std::u8string raw_result;

            while (_pos < _str.size()) {
                char8_t c = next();
                if (c == u8'"') {
                    return JsonValue{std::move(raw_result)};
                }
                if (c == u8'\\') {
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
                    raw_result += c;
                }
            }
            throw std::runtime_error("JSON String Parsing Error: Unterminated string");
        }

        JsonValue parse_array() {
            next(); // 跳过 '['
            JsonArray arr;

            skip_whitespace();
            if (peek() == u8']') {
                next();
                return JsonValue{arr};
            }

            while (true) {
                arr.push_back(parse_value());
                skip_whitespace();

                if (peek() == u8']') {
                    next();
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
            JsonObject obj;

            skip_whitespace();
            if (peek() == u8'}') {
                next();
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
                    return JsonValue{obj};
                }

                if (peek() != u8',') {
                    throw std::runtime_error("JSON String Parsing Error: Expected ',' or '}' in object");
                }
                next();
            }
        }
    };

    inline JsonValue parse(const std::u8string &json_str) {
        return JsonParser::parse(json_str);
    }

    inline JsonValue parse(const std::string &json_str) {
        return JsonParser::parse(json_str);
    }
}

#endif //TINY_JSON_JSON_H
