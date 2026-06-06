# Tiny JSON (C++)

一个**轻量级、现代 C++20 单头文件 JSON 库**，零依赖，支持 JSON 的解析、构建、访问和序列化。

## 特性

- **单头文件** — 只需 `#include "json.h"`，零外部依赖
- **类型安全** — 基于 `std::variant`，编译期类型约束
- **原生 UTF-8** — 内部统一使用 `std::u8string`，完整 Unicode 支持
- **完整 JSON 规范** — 支持所有 JSON 数据类型
- **JSON Pointer (RFC 6901)** — 深层路径访问
- **JSON Merge Patch (RFC 7396)** — 递归合并
- **安全解析** — 可配置嵌套深度限制、数组大小限制、UTF-8 验证

------

## 环境要求

- C++20 或更高版本
- 支持的编译器：GCC 10+、Clang 13+、MSVC 19.29+

## 安装

将 `json.h` 复制到项目中即可：

```cpp
#include "json.h"
```

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
    // {"name":"Alice","age":18,"is_student":true}
}
```

### 2. 解析 JSON

```cpp
auto j = parse(R"({
    "name": "Bob",
    "age": 25,
    "skills": ["C++", "Python"]
})");

std::cout << j["name"].as_string() << std::endl;  // Bob
```

### 3. 访问数据

```cpp
JsonValue j = parse(R"({"a": 1, "b": [10, 20]})");

double a = j["a"].as<JsonNumber>();       // 1
double x = j["b"][0].as<JsonNumber>();    // 10
```

### 4. JSON Pointer 深层访问（RFC 6901）

```cpp
JsonValue j = parse(R"({"user": {"name": "Alice", "scores": [95, 88, 92]}})");

std::string name  = j.resolve("/user/name").as_string();       // "Alice"
double      score = j.resolve("/user/scores/1").as<JsonNumber>(); // 88

// 转义规则：~0 代表 ~，~1 代表 /
// 例如键 "a/b" 用 "/a~1b" 访问
```

### 5. 修改 JSON

```cpp
JsonValue j;
j["name"] = "Charlie";
j["age"] = 30;
j["skills"].push_back("C++");
j["skills"].emplace_back("Rust");  // 原地构造，避免临时对象

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

### 6. 类型检查

```cpp
if (j["age"].is<JsonNumber>()) {
    double age = j["age"].as<JsonNumber>();
}

// 获取类型名称
std::string name = j["name"].type_name();  // "string"
```

### 7. 遍历

```cpp
// 数组 — range-based for
for (auto& v : j["skills"].as_array()) {
    std::cout << v.as_string() << std::endl;
}

// 对象 — range-based for
for (auto& [key, value] : j.as_object()) {
    std::cout << util::to_std_string(key) << ": "
              << value.serialize() << std::endl;
}

// 数组 — 回调方式
j["skills"].for_each_array([](const JsonValue& v) {
    std::cout << v.as_string() << std::endl;
});

// 对象 — 回调方式
j.for_each_object([](const std::u8string& key, JsonValue& value) {
    std::cout << util::to_std_string(key) << ": "
              << value.serialize() << std::endl;
});
```

### 8. JSON Merge Patch（RFC 7396）

```cpp
JsonValue target = object({{"a", 1}, {"b", 2}});
JsonValue patch  = object({{"b", nullptr}, {"c", 3}});
target.merge(patch);
// 结果：{"a": 1, "c": 3}（b 被 null 删除，c 被添加）

// 嵌套 object 递归合并
JsonValue base   = object({{"a", object({{"x", 1}, {"y", 2}})}});
JsonValue update = object({{"a", object({{"y", 99}, {"z", 3}})}});
base.merge(update);
// 结果：{"a": {"x": 1, "y": 99, "z": 3}}
```

------

## 类型说明

| JSON 类型 | C++ 类型 | 别名 |
|-----------|----------|------|
| null      | `std::nullptr_t` | `JsonNull` |
| bool      | `bool` | `JsonBool` |
| number    | `double` | `JsonNumber` |
| string    | `std::u8string` | `JsonString` |
| array     | `std::vector<JsonValue>` | `JsonArray` |
| object    | `std::unordered_map<std::u8string, JsonValue>` | `JsonObject` |

`JsonValue` 是以上类型的包装器，支持从所有 JSON 类型自动构造。

------

## API 概览

### 类型检查

```cpp
j.type()              // 返回 JsonValueType 枚举
j.type_name()         // 返回类型名称字符串（如 "null"、"bool"）
j.is<JsonNumber>()    // 检查是否为指定类型
```

### 值访问

```cpp
j.as<JsonNumber>()    // 获取指定类型引用（类型不匹配抛异常）
j.as_string()         // 获取字符串值（返回 std::string，方便外部使用）
j.size()              // 数组/对象返回元素数，字符串返回 UTF-8 字符数
```

### 数组操作

```cpp
j.push_back(value)         // 添加元素
j.emplace_back(args...)    // 原地构造元素
j[0]                       // 下标访问（自动扩容）
j.at(0)                    // 带边界检查的访问
```

### 对象操作

```cpp
j["key"] = value           // 设置/创建键值对
j.insert("key", value)     // 插入键值对
j.contains("key")          // 检查键是否存在
j.at("key")                // 带边界检查的访问
```

### JSON Pointer（RFC 6901）

```cpp
j.resolve("/a/b/0")        // 等价于 j["a"]["b"][0]
j.resolve("/a~1b")         // 访问键 "a/b"（~1 转义 /）
j.resolve("/a~0b")         // 访问键 "a~b"（~0 转义 ~）
j.resolve("")              // 引用根值自身
```

### JSON Merge Patch（RFC 7396）

```cpp
j.merge(patch)             // 将 patch 合并到 j（原地修改）
                           // null 删除键，object 递归合并，其他直接覆盖
```

### 迭代访问

```cpp
// range-based for
for (auto& val : j.as_array()) { ... }
for (auto& [key, val] : j.as_object()) { ... }

// 回调方式
j.for_each_array([](const JsonValue& val) { ... });
j.for_each_object([](const std::u8string& key, JsonValue& val) { ... });
```

### 序列化

```cpp
j.serialize()       // 紧凑格式
j.serialize(2)      // 格式化输出（缩进 2 空格）
```

### 工厂函数

```cpp
JsonValue arr = array({1, 2, 3});
JsonValue obj = object({{"key", "value"}});
JsonValue j   = parse(json_string);
JsonValue j   = parse(json_string, 512);  // 自定义最大嵌套深度（默认 256）
```

------

## 安全限制

| 限制项 | 默认值 | 说明 |
|--------|--------|------|
| 最大嵌套深度 | 256 | 防止栈溢出，可通过 `parse(json, max_depth)` 自定义 |
| 最大数组大小 | 1,000,000 | 防止无限扩容，`operator[]` 自动扩容时检查 |
| UTF-8 验证 | 启用 | 解析时拒绝非法 UTF-8 序列 |
| NaN / Inf | 序列化为 `null` | 符合 JSON 规范 |

------

## 错误处理

类型不匹配或越界访问时抛出标准异常：

```cpp
try {
    double n = j.as<JsonNumber>();  // 若 j 不是 number，抛异常
} catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    // "JsonValue type mismatch: requested 'd' but actual type is 'string'"
}

try {
    j.at(999);  // 数组越界
} catch (const std::out_of_range& e) {
    std::cerr << e.what() << std::endl;
    // "JsonArray index out of range"
}
```

------

## 构建与测试

```bash
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

------

## 注意事项

- 所有数字统一为 `double`，序列化时若值为整数则省略小数点
- 字符串内部使用 `std::u8string`，外部 API 同时接受 `std::string` 和 `const char*`
- `operator[]` 对 null 值自动创建对象或数组（类似 JavaScript 行为）
- 需要 C++20 支持（concepts、`std::ranges`、`char8_t`）
