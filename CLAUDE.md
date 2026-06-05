# CLAUDE.md — tiny-json 项目指南

## 项目概述

tiny-json 是一个**轻量级、现代 C++20 单头文件 JSON 库**，零外部依赖，支持 JSON 的解析、构建、访问和序列化。内部统一使用 `std::u8string` 存储字符串，天然支持 UTF-8。

- **作者**: deeplakee
- **许可**: MIT (2026)
- **仓库**: `git@github.com:deeplakee/tiny-json.git`
- **核心文件**: `json.h`（唯一的库文件）

## 构建与测试

```bash
# 使用 CMake 构建（需要 CMake >= 3.29，C++20 编译器）
cmake -B build -G Ninja
cmake --build build

# 运行测试（当前无 CTest 集成，需手动运行可执行文件）
./build/universal_test   # 通用 JSON 功能测试（~30 个用例）
./build/utf_8_test       # UTF-8/Unicode 专项测试（~25 个用例）
./build/my_test          # 手动调试/演示（无断言，非正式测试）
```

Windows 开发环境使用 CLion + Ninja + Mingw。测试在 Windows 下需要 `SetConsoleOutputCP(CP_UTF8)` 以正确输出中文。

## 架构设计

### 核心类型体系

```
JsonValue  (包装器，对外统一接口)
  └─ JsonVariant = std::variant<JsonNull, JsonBool, JsonNumber, JsonString, JsonArray, JsonObject>
```

| JSON 类型 | C++ 类型 | 别名 |
|-----------|----------|------|
| null      | `std::nullptr_t` | `JsonNull` |
| bool      | `bool` | `JsonBool` |
| number    | `double` | `JsonNumber` |
| string    | `std::u8string` | `JsonString` |
| array     | `std::vector<JsonValue>` | `JsonArray` |
| object    | `std::unordered_map<std::u8string, JsonValue>` | `JsonObject` |

### 模块划分（均在 `json.h` 内）

1. **类型定义与概念**（lines 21-72）：`JsonVariant` 别名、C++20 concepts（`JsonTy`, `StringLike`, `IntegerLike`, `U8Stringlike`, `NormalStringLike`）
2. **`namespace json::util`**（lines 86-215）：工具函数
   - `join()` — 范围拼接（用于序列化）
   - `isSafeToConvertToInt()` — 判断 double 是否可安全转 int
   - `utf16_to_utf8()` — Unicode 码点转 UTF-8 字节
   - `utf8_char_count()` — 统计 UTF-8 字符数（非字节数）
   - `to_u8string()` / `to_std_string()` — `std::string` ↔ `std::u8string` 互转
   - `isspace()` / `isdigit()` — `char8_t` 字符分类
3. **`class JsonValue`**（lines 217-582）：核心 API 类
   - 构造：从 `nullptr`、`bool`、`double`、`int`、`const char*`、`std::string`、`std::u8string`、`JsonArray`、`JsonObject`
   - 类型检查：`type()`、`typeName()`、`is<T>()`
   - 值访问：`as<T>()`（类型不匹配抛异常）、`as_string()`（返回 `std::string`）
   - 数组操作：`operator[](int)`（null 自动提升为数组）、`push_back()`、`emplace_back()`、`at(index)`
   - 对象操作：`operator[](string)`（null 自动提升为对象）、`insert()`、`contains()`、`at(key)`
   - JSON Pointer：`resolve("/a/b/0")` 支持 RFC 6901 深层路径访问（`~0` 转义 `~`，`~1` 转义 `/`）
   - JSON Merge Patch：`merge(patch)` 支持 RFC 7396 合并（null 删除键，object 递归合并）
   - 序列化：`serialize(indent)` — 紧凑或格式化输出
   - 遍历：`as_array()`/`as_object()` 返回底层容器引用（兼容 range-based for）、`for_each_array()`/`for_each_object()` 回调方式
   - 大小：`size()` — 字符串返回字符数，数组/对象返回元素数
4. **`class JsonParser`**（lines 596-878）：递归下降解析器
   - 完整 JSON 语法支持 + `\uXXXX` 转义 + 代理对（surrogate pairs）
   - 入口：`parse(std::string)` / `parse(std::u8string)`
5. **辅助函数**（lines 584-595）：`array({...})`、`object({...})` 工厂函数

### 关键设计决策

1. **字符串统一为 `std::u8string`**：内部全部以 UTF-8 存储，外部 API 同时接受 `std::string` 和 `const char*`
2. **数字统一为 `double`**：不区分整数和浮点，序列化时若值为整数则省略小数点
3. **Null 自动提升**：对 null 值执行 `operator[]` 会自动创建 array 或 object（类似 JavaScript）
4. **异常驱动错误处理**：类型不匹配抛 `std::runtime_error`，越界抛 `std::out_of_range`
5. **C++20 特性**：concepts、`std::ranges`、fold expressions、structured bindings、`char8_t`

## 编码规范

- 语言：C++20
- 命名：类名 PascalCase，函数/变量 camelCase，类型别名 PascalCase，命名空间全小写
- 注释：中文注释，关键逻辑处有说明
- 头文件保护：`#ifndef TINY_JSON_JSON_H` / `#define` / `#endif`
- 所有实现内联在头文件中（header-only）
- 错误消息格式：`"JSON String Parsing Error: ..."` 或 `"JsonValue type mismatch: ..."`

## 测试框架

使用自定义轻量宏框架（`test.h`，基于 `<cassert>`）：

```cpp
TEST(name) { ... }          // 定义测试函数 test_##name()
RUN_TEST(name);             // 运行并打印 PASSED
ASSERT_EQ(a, b)             // 相等断言
ASSERT_TRUE(expr)           // 真值断言
ASSERT_FALSE(expr)          // 假值断言
ASSERT_DOUBLE_EQ(a, b)      // 浮点近似断言（epsilon=1e-6）
ASSERT_THROW(expr, type)    // 异常断言
```

**注意**：测试框架的局限性 — `assert` 失败直接终止进程，不会继续执行后续测试，无失败计数汇总。

### 代码风格与静态分析

- **`.clang-format`**：基于 LLVM 风格，缩进 4 空格，列宽 120，花括号不换行（Custom BreakBeforeBraces）
- **`.clang-tidy`**：启用了 bugprone、cert、cppcoreguidelines、modernize、performance、readability 等检查类别
- **clang-tidy 已知发现**：
  - `bugprone-branch-clone`：`utf8_char_count` 中 1 字节字符和无效字节的处理分支相同（`i += 1`）
  - `modernize-use-nodiscard`：`type()`、`typeName()`、`is<T>()`、`as_string()` 应标记 `[[nodiscard]]`

### 已修复项
- ~~`emplace_back` 错误消息写成 "Cannot push_back"~~ → 已修正为 "Cannot emplace_back"
- ~~`jsonTypeName` 使用硬编码 variant 索引~~ → 已改为使用 `JsonValueType` 枚举
- ~~CMakeLists.txt 缺少 `enable_testing()` / `add_test()`~~ → 已添加 CTest 集成
- ~~CMakeLists.txt 未设置编译器警告标志~~ → 已添加 `-Wall -Wextra -Wpedantic`
- ~~`parse_string()` 类型不一致~~ → 已改为直接使用 `std::u8string`
- ~~移动构造未标记 `noexcept`~~ → 已为 `JsonString`/`JsonArray`/`JsonObject` 移动构造添加 `noexcept`
- ~~测试文件中的 `system("pause")`~~ → 已移除
- ~~`universal_test.cpp` 的 `// utf-8` 占位注释~~ → 已移除
- ~~解析器缺少十六进制验证~~ → 已添加 `util::isxdigit()`，`\uXXXX` 转义现在会验证所有字符
- ~~`JsonTy` concept 的 `-Wunused-value` 警告~~ → 已通过添加 `-> bool` 返回类型修复
- ~~`object()` 工厂函数接受 `std::string` 键，需转换为 `u8string`~~ → 设计合理，接受 `std::string` 便于使用普通字符串字面量，内部转换到 `u8string` 是必要的
- ~~缺少 `#include <functional>`~~ → 已添加

## 可改进方向

- 考虑支持 `int64_t` 类型以精确表示大整数
- 优化 `serialize()` 性能（当前每次递归创建 `std::string`）
- 添加 CI/CD 流水线（GitHub Actions）
