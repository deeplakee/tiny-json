# Tiny JSON (C++)

一个**轻量级、现代 C++（C++20）实现的 JSON 库**，支持解析、构建、访问和序列化 JSON 数据，零依赖，单头文件即可使用。

## 特性

- 单头文件 (`json.h`)，开箱即用
- 基于 `std::variant` 的类型安全设计
- 支持：
  - `null`
  - `bool`
  - `number`（double）
  - `string`（UTF-8）
  - `array`
  - `object`
- 自动类型转换（如 `int -> number`）
- UTF-8 / UTF-16 转换支持（`\uXXXX`）
- 友好的 API
- 支持格式化输出（pretty print）

------

## 安装

只需将头文件加入项目：

```cpp
#include "json.h"
```

无需额外依赖。

------

## 快速开始

### 1. 构造 JSON

```cpp
#include "json.h"
using namespace json;

int main() {
    JsonValue obj = object({
        {"name", "Alice"},
        {"age", 18},
        {"is_student", true}
    });

    std::cout << obj.serialize() << std::endl;
}
```

输出：

```json
{"name":"Alice","age":18,"is_student":true}
```

------

### 2. 解析 JSON

```cpp
using namespace json;

auto j = parse(R"({
    "name": "Bob",
    "age": 25,
    "skills": ["C++", "Python"]
})");

std::cout << j["name"].as_string() << std::endl;
```

------

### 3. 访问数据

```cpp
JsonValue j = parse(R"({"a": 1, "b": [10, 20]})");

// object
int a = j["a"].as<JsonNumber>();

// array
int x = j["b"][0].as<JsonNumber>();
```

------

### 4. 修改 JSON

```cpp
JsonValue j;

j["name"] = "Charlie";
j["age"] = 30;

j["skills"].push_back("C++");
j["skills"].push_back("Rust");

std::cout << j.serialize(2) << std::endl;
```

输出（格式化）：

```json
{
  "name": "Charlie",
  "age": 30,
  "skills": [
    "C++",
    "Rust"
  ]
}
```

------

### 5. 类型检查

```cpp
if (j["age"].is<JsonNumber>()) {
    double age = j["age"].as<JsonNumber>();
}
```

------

### 6. 遍历

#### 数组（range-based for）

```cpp
for (auto& v : j["skills"].as_array()) {
    std::cout << v.as_string() << std::endl;
}
```

#### 对象（range-based for）

```cpp
for (auto& [key, value] : j.as_object()) {
    std::cout << (const char*)key.c_str() << ": "
              << value.serialize() << std::endl;
}
```

#### 数组（回调方式）

```cpp
j["skills"].for_each_array([](const JsonValue& v) {
    std::cout << v.as_string() << std::endl;
});
```

#### 对象（回调方式）

```cpp
j.for_each_object([](const std::u8string& key, JsonValue& value) {
    std::cout << (const char*)key.c_str() << ": "
              << value.serialize() << std::endl;
});
```

------

## 类型说明

| JSON 类型 | C++ 类型                                                   |
| --------- | ---------------------------------------------------------- |
| null      | `(JsonNull)std::nullptr_t`                                 |
| bool      | `(JsonBool)bool`                                           |
| number    | `(JsonNumber)double`                                       |
| string    | `(JsonString)std::u8string`                                |
| array     | `(JsonArray)std::vector<JsonValue>`                        |
| object    | `(JsonObject)std::unordered_map<std::u8string, JsonValue>` |

`JsonValue`类型是以上类型的包装器。

------

## API 概览

### 创建

```cpp
JsonValue v;
JsonValue v = nullptr;
JsonValue v = true;
JsonValue v = 3.14;
JsonValue v = "hello";
```

------

### 访问

```cpp
JsonNull n = v.as<JsonNull>();
JsonBool b = v.as<JsonBool>();
JsonNumber num = v.as<JsonNumber>();
JsonString str = v.as<JsonString>();
```

------

### 数组操作

```cpp
v.push_back(1);
v[0] = 10;
v.at(0);
```

------

### 对象操作

```cpp
v["key"] = "value";
v.insert("key", 123);
v.contains("key");
v.at("key");
```

------

### 迭代访问

```cpp
// range-based for
for (auto& val : v1.as_array()) {
    sum += val.as<JsonNumber>();
}

for (auto& [key, value] : v2.as_object()) {
    oss << (const char*)key.c_str() << ": "
              << value.serialize() << std::endl;
}

// 回调方式
v1.for_each_array([&sum](const JsonValue &val) {
    sum += val.as<JsonNumber>();
});

v2.for_each_object([&oss](const std::u8string& key, JsonValue& value) {
    oss << (const char*)key.c_str() << ": "
              << value.serialize() << std::endl;
});
```

------

### 序列化

```cpp
v.serialize();      // 紧凑
v.serialize(2);     // 格式化（缩进2）
```

------

## 注意事项

- 所有数字统一为 `double`
- 字符串内部使用 `std::u8string`
- `operator[]`：会自动创建对象或数组（类似 JS）
- 类型错误会抛出 `std::runtime_error`

------
