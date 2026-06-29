# 第三十九章：C++ 生态与工程实践

---

Taco 的功能已经完整。这一章讲的是**工程**：怎么保证代码质量，怎么调试，怎么发布。

这些技能在任何 C++ 项目里都用得到，不只是 Taco。

---

## 39.1 常用第三方库概览：nlohmann/json、spdlog、Catch2

C++ 的标准库相比 Python 要精简得多，很多实用功能要靠第三方库。下面是写 C++ 项目时经常会用到的几个。

### nlohmann/json：JSON 处理

Taco 的 v7 已经用过了。几个常用模式：

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 解析
json j = json::parse(R"({"name": "Miguel", "scores": [88, 92, 76]})");

// 访问（带默认值）
std::string name = j.value("name", "unknown");
int first_score = j["scores"][0];

// 构建
json resp;
resp["status"] = "ok";
resp["data"]["user"] = "Miguel";
resp["data"]["scores"] = {88, 92, 76};
std::cout << resp.dump(2);  // pretty-print

// 序列化/反序列化自定义类型（需要实现 to_json/from_json）
struct Config {
    std::string host;
    int port;
};

void to_json(json& j, const Config& c) {
    j = {{"host", c.host}, {"port", c.port}};
}

void from_json(const json& j, Config& c) {
    j.at("host").get_to(c.host);
    j.at("port").get_to(c.port);
}

// 使用：
Config cfg{"localhost", 8080};
json j_cfg = cfg;                   // 自动调用 to_json
Config restored = j_cfg;           // 自动调用 from_json
```

### spdlog：日志库

`std::cout` 的问题：没有日志级别（debug/info/warn/error），多线程写会乱，没有时间戳，性能不好。spdlog 解决这些问题：

```cmake
# CMakeLists.txt
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(taco PRIVATE spdlog::spdlog)
```

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

int main() {
    // 默认 logger（输出到终端，带颜色）
    spdlog::info("Taco interpreter starting...");
    spdlog::debug("Parsing file: {}", filename);
    spdlog::warn("Deprecated feature used at line {}", line);
    spdlog::error("Failed to load module: {}", module_name);

    // 设置全局日志级别
    spdlog::set_level(spdlog::level::debug);  // 调试时开 debug 级别

    // 自定义 logger（同时输出到终端和文件）
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "taco.log", true  // true = 每次启动清空日志文件
    );

    auto logger = std::make_shared<spdlog::logger>(
        "taco",
        spdlog::sinks_init_list{console_sink, file_sink}
    );
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);

    spdlog::info("Logger initialized.");
}
```

在 Taco 里，调试模式（`taco --debug script.taco`）可以打开 debug 级别日志，输出词法分析、语法分析、求值的详细过程，帮助排查脚本问题。

### fmtlib：格式化字符串

fmtlib 是 C++20 `<format>` 的前身，也是 spdlog 格式化的基础。`{}` 占位符，支持各种格式选项：

```cpp
#include <fmt/format.h>
#include <fmt/ranges.h>   // 用于格式化容器

// 基本用法
std::string s = fmt::format("Hello, {}! You are {} years old.", "Miguel", 12);

// 格式选项
fmt::print("{:>10}\n", "right");     // 右对齐，宽度 10
fmt::print("{:0>5}\n", 42);         // 左填零，宽度 5：00042
fmt::print("{:.2f}\n", 3.14159);    // 2 位小数：3.14
fmt::print("{:#x}\n", 255);         // 16进制：0xff

// 格式化容器（需要 fmt/ranges.h）
std::vector<int> v = {1, 2, 3, 4, 5};
fmt::print("{}\n", v);              // [1, 2, 3, 4, 5]
```

### Catch2：单元测试框架

下一节详细介绍。

---

## 39.2 测试：用 Catch2 给 Taco 写单元测试

软件没有测试，就没有信心改动它。Taco 是一个相当复杂的系统，单元测试可以保证每个组件在修改后仍然正确工作。

### 引入 Catch2

```cmake
# CMakeLists.txt
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

# 测试可执行文件（和主程序分开）
add_executable(taco_tests
    tests/test_lexer.cpp
    tests/test_parser.cpp
    tests/test_evaluator.cpp
    # 共享 taco 的源文件（除了 main.cpp）
    src/lexer.cpp
    src/parser.cpp
    src/evaluator.cpp
    src/value.cpp
    src/environment.cpp
    src/builtin.cpp
)
target_link_libraries(taco_tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(taco_tests PRIVATE src)

# 让 CMake 的 test 命令运行 Catch2 测试
include(CTest)
include(Catch)
catch_discover_tests(taco_tests)
```

### 测试词法分析器

```cpp
// tests/test_lexer.cpp
#include <catch2/catch_test_macros.hpp>
#include "lexer.h"
#include "token.h"

TEST_CASE("Lexer tokenizes numbers", "[lexer]") {
    SECTION("integer") {
        Lexer lex("42");
        auto tokens = lex.tokenize();
        REQUIRE(tokens.size() == 2);  // 42, EOF
        REQUIRE(tokens[0].type == TokenType::Number);
        REQUIRE(tokens[0].value == "42");
    }

    SECTION("float") {
        Lexer lex("3.14");
        auto tokens = lex.tokenize();
        REQUIRE(tokens[0].type == TokenType::Number);
        REQUIRE(tokens[0].value == "3.14");
    }

    SECTION("underscore separator") {
        Lexer lex("1_000_000");
        auto tokens = lex.tokenize();
        // 词法分析器应该去掉下划线，得到 "1000000"
        REQUIRE(tokens[0].value == "1000000");
    }
}

TEST_CASE("Lexer tokenizes strings", "[lexer]") {
    SECTION("basic string") {
        Lexer lex(R"("hello world")");
        auto tokens = lex.tokenize();
        REQUIRE(tokens[0].type == TokenType::String);
        REQUIRE(tokens[0].value == "hello world");
    }

    SECTION("string with escape") {
        Lexer lex(R"("line1\nline2")");
        auto tokens = lex.tokenize();
        REQUIRE(tokens[0].value == "line1\nline2");
    }
}

TEST_CASE("Lexer tokenizes keywords", "[lexer]") {
    Lexer lex("var func if while return");
    auto tokens = lex.tokenize();
    REQUIRE(tokens[0].type == TokenType::Var);
    REQUIRE(tokens[1].type == TokenType::Func);
    REQUIRE(tokens[2].type == TokenType::If);
    REQUIRE(tokens[3].type == TokenType::While);
    REQUIRE(tokens[4].type == TokenType::Return);
}

TEST_CASE("Lexer handles line numbers", "[lexer]") {
    Lexer lex("var x = 1;\nvar y = 2;");
    auto tokens = lex.tokenize();
    // x 在第 1 行，y 在第 2 行
    REQUIRE(tokens[1].line == 1);  // x
    // 找到 y 所在的 token
    auto y_token = std::find_if(tokens.begin(), tokens.end(),
        [](const Token& t) { return t.value == "y"; });
    REQUIRE(y_token != tokens.end());
    REQUIRE(y_token->line == 2);
}

TEST_CASE("Lexer rejects invalid input", "[lexer]") {
    SECTION("unterminated string") {
        Lexer lex(R"("unclosed)");
        REQUIRE_THROWS_AS(lex.tokenize(), LexError);
    }

    SECTION("invalid character") {
        Lexer lex("@invalid");
        REQUIRE_THROWS_AS(lex.tokenize(), LexError);
    }
}
```

### 测试求值器

求值器测试最有价值：直接验证 Taco 脚本的执行结果。

```cpp
// tests/test_evaluator.cpp
#include <catch2/catch_test_macros.hpp>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "environment.h"
#include "builtin.h"

// 辅助函数：运行一段 Taco 代码，返回最后一个表达式的值
TacoValue run(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto ast = parser.parse();

    auto global = std::make_shared<Environment>();
    register_builtins(*global);

    Evaluator eval(global, ".");
    return eval.evaluate(*ast);
}

// 辅助函数：运行代码，捕获 print 的输出
std::string capture_output(const std::string& source) {
    // 替换 print 的实现，把输出写入字符串
    std::string output;
    // ... （通过依赖注入或全局状态捕获输出）
    return output;
}

TEST_CASE("Arithmetic expressions", "[evaluator]") {
    REQUIRE(run("1 + 2").as_number() == 3.0);
    REQUIRE(run("10 - 3").as_number() == 7.0);
    REQUIRE(run("4 * 5").as_number() == 20.0);
    REQUIRE(run("10 / 4").as_number() == 2.5);
    REQUIRE(run("10 % 3").as_number() == 1.0);
    REQUIRE(run("2 ^ 10").as_number() == 1024.0);
}

TEST_CASE("Comparison operators", "[evaluator]") {
    REQUIRE(run("1 < 2").as_bool() == true);
    REQUIRE(run("2 > 1").as_bool() == true);
    REQUIRE(run("1 == 1").as_bool() == true);
    REQUIRE(run("1 != 2").as_bool() == true);
    REQUIRE(run("2 >= 2").as_bool() == true);
    REQUIRE(run("1 <= 2").as_bool() == true);
}

TEST_CASE("String operations", "[evaluator]") {
    REQUIRE(run(R"("hello" + " world")").as_string() == "hello world");

    SECTION("string interpolation") {
        REQUIRE(run(R"(var name = "Miguel"; "Hola, {name}!")").as_string()
                == "Hola, Miguel!");
    }
}

TEST_CASE("Variable declaration and assignment", "[evaluator]") {
    REQUIRE(run("var x = 42; x").as_number() == 42.0);
    REQUIRE(run("var x = 1; x = 2; x").as_number() == 2.0);
}

TEST_CASE("If expression", "[evaluator]") {
    REQUIRE(run("var x = 10; if (x > 5) { 1 } else { 0 }").as_number() == 1.0);
    REQUIRE(run("var x = 3; if (x > 5) { 1 } else { 0 }").as_number() == 0.0);
}

TEST_CASE("While loop", "[evaluator]") {
    std::string code = R"(
        var i = 0;
        var sum = 0;
        while (i < 5) {
            sum = sum + i;
            i = i + 1;
        }
        sum
    )";
    REQUIRE(run(code).as_number() == 10.0);  // 0+1+2+3+4 = 10
}

TEST_CASE("Function definition and call", "[evaluator]") {
    SECTION("basic function") {
        std::string code = R"(
            func add(a, b) { return a + b; }
            add(3, 4)
        )";
        REQUIRE(run(code).as_number() == 7.0);
    }

    SECTION("recursion") {
        std::string code = R"(
            func fib(n) {
                if (n <= 1) { return n; }
                return fib(n - 1) + fib(n - 2);
            }
            fib(10)
        )";
        REQUIRE(run(code).as_number() == 55.0);
    }

    SECTION("closure") {
        std::string code = R"(
            func makeCounter() {
                var count = 0;
                return func() {
                    count = count + 1;
                    return count;
                };
            }
            var c = makeCounter();
            c(); c(); c()
        )";
        REQUIRE(run(code).as_number() == 3.0);
    }
}

TEST_CASE("Array operations", "[evaluator]") {
    SECTION("basic access") {
        REQUIRE(run("[1, 2, 3][0]").as_number() == 1.0);
        REQUIRE(run("[1, 2, 3][2]").as_number() == 3.0);
    }

    SECTION("len") {
        REQUIRE(run("[1, 2, 3].len()").as_number() == 3.0);
    }

    SECTION("push and pop") {
        std::string code = R"(
            var arr = [1, 2, 3];
            arr.push(4);
            arr.len()
        )";
        REQUIRE(run(code).as_number() == 4.0);
    }

    SECTION("filter") {
        std::string code = R"(
            [1, 2, 3, 4, 5].filter { x in x % 2 == 0 }.len()
        )";
        REQUIRE(run(code).as_number() == 2.0);
    }

    SECTION("reduce") {
        std::string code = R"(
            [1, 2, 3, 4, 5].reduce { (acc, x) in acc + x }
        )";
        REQUIRE(run(code).as_number() == 15.0);
    }
}

TEST_CASE("Map operations", "[evaluator]") {
    SECTION("access by key") {
        REQUIRE(run(R"({"x": 1, "y": 2}["x"])").as_number() == 1.0);
    }

    SECTION("dot access") {
        REQUIRE(run(R"(var m = {"name": "Miguel"}; m.name)").as_string()
                == "Miguel");
    }

    SECTION("getKeys") {
        std::string code = R"({"a": 1, "b": 2}.getKeys().len())";
        REQUIRE(run(code).as_number() == 2.0);
    }
}

TEST_CASE("Error handling", "[evaluator]") {
    REQUIRE_THROWS(run("undefined_var"));
    REQUIRE_THROWS(run("1 + \"string\""));  // 类型错误
    REQUIRE_THROWS(run("[1, 2, 3][10]"));   // 越界
}
```

### 运行测试

```bash
# 编译并运行测试
cmake -B build && cmake --build build
cd build && ctest --verbose

# 或者直接运行测试可执行文件
./build/taco_tests

# 只运行特定标签的测试
./build/taco_tests [lexer]
./build/taco_tests [evaluator]

# 输出更详细的信息
./build/taco_tests -v

# 测试失败时显示更多上下文
./build/taco_tests --show-catchup
```

Catch2 的输出：

```
===============================================================================
All tests passed (47 assertions in 18 test cases)
```

或者（有失败时）：

```
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
taco_tests is a Catch2 v3.5.4 host application.
Run with -? for options

───────────────────────────────────────────────────────────────────────────────
tests/test_evaluator.cpp:89: FAILED:
  REQUIRE( run(code).as_number() == 55.0 )
with expansion:
  54.0 == 55.0

===============================================================================
test cases:  18 | 17 passed | 1 failed
assertions:  47 | 46 passed | 1 failed
```

---

## 39.3 调试工具：gdb、valgrind、AddressSanitizer

C++ 有三类常见的运行时问题：**逻辑错误**（输出结果不对）、**内存错误**（越界、use-after-free）、**内存泄漏**。三类问题用不同的工具。

### gdb：调试逻辑错误

gdb 是 Linux 下的调试器，可以设置断点、单步执行、查看变量值。

编译时加 `-g` 生成调试符号（CMake 的 `Debug` 模式自动加）：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

基本 gdb 使用：

```bash
gdb ./build/taco

# 在 gdb 里：
(gdb) run script.taco       # 运行
(gdb) break evaluator.cpp:120  # 在第 120 行设断点
(gdb) break Evaluator::evaluate  # 在函数入口设断点
(gdb) continue              # 继续运行到下一个断点
(gdb) next                  # 单步（不进入函数）
(gdb) step                  # 单步（进入函数）
(gdb) print current_env     # 打印变量
(gdb) info locals           # 打印所有局部变量
(gdb) backtrace             # 查看调用栈
(gdb) quit                  # 退出
```

程序崩溃时，gdb 会停在崩溃点，可以查看调用栈和变量：

```bash
# 调试 core dump
gdb ./build/taco core
(gdb) backtrace
```

VSCode 有 C/C++ 插件，可以用图形界面调试，不需要记 gdb 命令。配置 `.vscode/launch.json`：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Taco",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/taco",
            "args": ["${workspaceFolder}/test.taco"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "MIMode": "gdb"
        }
    ]
}
```

### AddressSanitizer：内存错误

AddressSanitizer（ASan）是编译器内置的内存检查工具，能检测：

- 越界访问（buffer overflow）
- use-after-free（访问已释放的内存）
- use-after-scope（访问已出作用域的变量）
- double free

性能开销约 2x，适合在测试环境里开启：

```cmake
# CMakeLists.txt（Debug 模式下开启 ASan）
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(taco PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(taco PRIVATE -fsanitize=address)

    target_compile_options(taco_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(taco_tests PRIVATE -fsanitize=address)
endif()
```

当有内存错误时，ASan 打印详细的错误报告：

```
=================================================================
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000000110
READ of size 8 at 0x602000000110 thread T0
    #0 0x55555557a3c0 in TacoValue::as_number() src/value.h:45
    #1 0x55555557c1f0 in BinaryExpr::evaluate(Evaluator&) src/evaluator.cpp:89
    ...
0x602000000110 is located 0 bytes inside of 32-byte region [0x602000000110,0x602000000130)
freed by thread T0 here:
    #0 0x7ffff7c12b20 in operator delete(void*) ...
    #1 0x55555557b8a0 in ~TacoValue() src/value.h:30
    ...
```

### valgrind：内存泄漏

valgrind 检测内存泄漏和内存错误，比 ASan 慢（10x 以上），但不需要重新编译：

```bash
valgrind --leak-check=full --show-leak-kinds=all ./build/taco script.taco

# 输出（无泄漏时）：
# ==12345== LEAK SUMMARY:
# ==12345==    definitely lost: 0 bytes in 0 blocks
# ==12345==    indirectly lost: 0 bytes in 0 blocks
# ==12345==      possibly lost: 0 bytes in 0 blocks
# ==12345==    still reachable: 1,024 bytes in 3 blocks  ← 正常（全局对象）
```

`definitely lost`：真实的内存泄漏，一定要修。`still reachable`：程序退出时仍可访问的内存，通常是全局对象，不用担心。

### UndefinedBehaviorSanitizer

UBSan 检测 C++ 里的未定义行为：整数溢出、空指针解引用、越界数组访问……

```cmake
target_compile_options(taco PRIVATE -fsanitize=undefined)
target_link_options(taco PRIVATE -fsanitize=undefined)
```

可以同时开启 ASan 和 UBSan：

```cmake
target_compile_options(taco PRIVATE -fsanitize=address,undefined)
target_link_options(taco PRIVATE -fsanitize=address,undefined)
```

---

## 39.4 代码风格：clang-format、clang-tidy

代码风格统一可以减少 code review 时的噪音，让团队专注于逻辑而不是格式。

### clang-format：自动格式化

clang-format 根据配置文件自动格式化 C++ 代码：

```yaml
# .clang-format（放在项目根目录）
BasedOnStyle: LLVM       # 基于 LLVM 风格（可选：Google、Mozilla、WebKit、Chromium）
IndentWidth: 4           # 4 空格缩进
ColumnLimit: 100         # 每行最多 100 字符
PointerAlignment: Left   # int* ptr 而不是 int *ptr
AllowShortFunctionsOnASingleLine: Empty    # 空函数允许单行：{}
AllowShortIfStatementsOnASingleLine: Never # if 不允许单行
SortIncludes: CaseSensitive  # 自动排序 #include
```

使用：

```bash
# 格式化单个文件（修改原文件）
clang-format -i src/evaluator.cpp

# 格式化所有 .cpp 和 .h 文件
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# 只检查（不修改），输出差异（用于 CI）
clang-format --dry-run --Werror src/evaluator.cpp
```

在 VSCode 里安装 C/C++ 插件后，保存时自动格式化（设置 `"editor.formatOnSave": true`）。

### clang-tidy：静态分析

clang-tidy 是一个 linter，检测代码里的潜在问题：

```yaml
# .clang-tidy（放在项目根目录）
Checks: >
    clang-diagnostic-*,
    clang-analyzer-*,
    cppcoreguidelines-*,
    modernize-*,
    performance-*,
    readability-*,
    -modernize-use-trailing-return-type,
    -cppcoreguidelines-avoid-magic-numbers,
    -readability-magic-numbers

WarningsAsErrors: ""
HeaderFilterRegex: "src/.*"
```

```bash
# 分析单个文件（需要 compile_commands.json）
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy src/evaluator.cpp -p build/

# 自动修复部分问题
clang-tidy src/evaluator.cpp -p build/ --fix
```

clang-tidy 的常见检查：

- `modernize-use-auto`：建议用 `auto`（`std::vector<int>::iterator it` → `auto it`）
- `modernize-use-nullptr`：用 `nullptr` 替换 `NULL` 或 `0`
- `modernize-use-override`：重写虚函数时加 `override`
- `performance-unnecessary-copy-initialization`：不必要的拷贝
- `cppcoreguidelines-avoid-c-arrays`：用 `std::array` 替代 C 数组
- `readability-identifier-naming`：命名规范检查

### 在 CI 里强制执行

```yaml
# .github/workflows/lint.yml
name: Lint
on: [push, pull_request]
jobs:
  clang-format:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Run clang-format
        run: |
          find src -name "*.cpp" -o -name "*.h" | \
          xargs clang-format --dry-run --Werror
```

---

## 39.5 发布到 GitHub：README、CI、二进制发布

### README.md

一个好的 README 是项目的门面。Taco 的 README 应该包含：

```markdown
# 🌮 Taco

> A simple, spicy scripting language.

## Quick Start

```bash
# 安装
brew install taco          # macOS
apt install taco           # Ubuntu

# 运行脚本
taco hello.taco

# REPL
taco
```

## Hello, World

```taco
print("¡Hola, mundo!");
```

## Features

- Dynamic typing, clean syntax
- First-class functions and closures
- Array and map with pipeline methods
- Built-in HTTP support: fetchUrl(), postData()
- Concurrent: thread, channel, mutex
- Interactive REPL

## Building from Source

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/taco --version
```

Requires: CMake 3.20+, C++17 compiler, OpenSSL (optional, for HTTPS).

## Language Reference

[Taco Language Spec](docs/spec.md)

## License

MIT
```

### GitHub Actions CI

```yaml
# .github/workflows/ci.yml
name: CI
on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build-and-test:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      - name: Install OpenSSL (Ubuntu)
        if: runner.os == 'Linux'
        run: sudo apt-get install -y libssl-dev

      - name: Install OpenSSL (macOS)
        if: runner.os == 'macOS'
        run: brew install openssl

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release

      - name: Test
        run: |
          cd build
          ctest --config Release --output-on-failure

      - name: Run integration test
        run: |
          echo 'print("hello from CI");' > /tmp/test.taco
          ./build/taco /tmp/test.taco
```

### 二进制发布

用 GitHub Actions 在 tag 触发时自动构建并发布二进制：

```yaml
# .github/workflows/release.yml
name: Release
on:
  push:
    tags:
      - 'v*'   # 触发条件：推送 v0.1.0 这样的 tag

jobs:
  build:
    strategy:
      matrix:
        include:
          - os: ubuntu-latest
            artifact: taco-linux-x86_64
          - os: macos-latest
            artifact: taco-macos-x86_64
          - os: windows-latest
            artifact: taco-windows-x86_64.exe

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      - name: Build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build --config Release

      - name: Rename binary
        run: |
          # Linux/macOS
          cp build/taco ${{ matrix.artifact }}   # 或者 .exe 版本

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: ${{ matrix.artifact }}
          path: ${{ matrix.artifact }}

  release:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - uses: actions/download-artifact@v4

      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            taco-linux-x86_64/taco-linux-x86_64
            taco-macos-x86_64/taco-macos-x86_64
            taco-windows-x86_64.exe/taco-windows-x86_64.exe
          generate_release_notes: true
```

发布流程：

```bash
# 打 tag 触发自动发布
git tag v0.1.0
git push origin v0.1.0

# GitHub Actions 自动：
# 1. 在 Linux、macOS、Windows 上构建
# 2. 运行测试
# 3. 创建 GitHub Release，上传三个平台的二进制文件
```

---

## 小结

**第三方库**：nlohmann/json 处理 JSON，spdlog 处理日志，fmtlib 处理格式化字符串，Catch2 做单元测试。这四个库几乎在所有 C++ 项目里都会用到。

**Catch2 单元测试**：`TEST_CASE` + `SECTION` 组织测试，`REQUIRE` 断言结果，`REQUIRE_THROWS_AS` 断言异常。Taco 的测试覆盖 Lexer、Parser、Evaluator 三层，每个语言特性有对应的测试用例。

**调试工具三件套**：
- gdb：调试逻辑错误，断点、单步、查看变量
- AddressSanitizer：编译时插桩，检测内存越界、use-after-free
- valgrind：运行时检测内存泄漏

**代码质量工具**：
- clang-format：自动格式化，`.clang-format` 配置风格
- clang-tidy：静态分析，发现潜在问题，建议现代 C++ 写法

**发布工程**：GitHub Actions 实现 CI（每次 push 自动构建测试）和 CD（打 tag 自动发布多平台二进制）。好的 README 是项目的入口，包含快速开始、特性列表、构建说明。
