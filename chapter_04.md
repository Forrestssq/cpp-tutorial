# 第四章：字符串与输入输出

---

Taco 解释器的核心工作之一，是读取用户输入的代码（字符串），处理它，然后输出结果。这一章讲 C++ 里处理字符串和输入输出的工具。

C++ 在这个领域提供了两套东西：一套是从 C 继承来的（`char*`、`printf`、`scanf`），另一套是 C++ 自己的（`std::string`、`iostream`）。本书优先使用 C++ 的方式，但也会解释 C 的遗产，因为真实代码里两者都会遇到。

---

## 4.1 std::string vs C 字符串

### C 字符串的问题

C 里的字符串是 `char` 数组，以空字符 `'\0'` 结尾：

```c
char name[] = "Miguel";
// 实际存储：['M', 'i', 'g', 'u', 'e', 'l', '\0']
```

C 字符串的问题很多：

```c
// 拼接需要手动计算长度
char result[100];
strcpy(result, "Hello, ");
strcat(result, "World");  // 如果 result 太小，缓冲区溢出！

// 比较不能用 ==
char a[] = "hello";
char b[] = "hello";
if (a == b) { ... }  // 比较的是指针地址，不是内容！
if (strcmp(a, b) == 0) { ... }  // 正确方式，但很别扭

// 获取长度需要遍历
int len = strlen(name);  // O(n) 操作
```

缓冲区溢出（buffer overflow）是 C 程序里最常见的安全漏洞之一——往一个太小的数组里写入太多数据，会覆盖相邻的内存，导致程序崩溃或被攻击者利用。

---

### std::string：真正的字符串类

`std::string` 解决了 C 字符串的所有主要问题：

```cpp
#include <string>

std::string name = "Miguel";

// 拼接用 +
std::string greeting = "Hello, " + name + "!";

// 比较用 ==
if (name == "Miguel") { ... }  // 比较内容，符合直觉

// 长度是 O(1)
std::cout << name.size() << "\n";  // 6

// 不会溢出，自动管理内存
name += " Rivera";  // 自动扩展
```

---

### std::string 的常用操作

```cpp
std::string s = "Hello, World!";

// 长度
s.size();      // 13
s.length();    // 同上，两个函数等价
s.empty();     // false

// 访问字符
s[0];          // 'H'，不做边界检查
s.at(0);       // 'H'，做边界检查，越界抛异常

// 子串
s.substr(7, 5);      // "World"，从下标 7 开始，取 5 个字符
s.substr(7);         // "World!"，从下标 7 到结尾

// 查找
s.find("World");     // 7，返回第一次出现的位置
s.find("world");     // std::string::npos，找不到

// 是否包含（C++23 才有 contains，C++17 用 find）
s.find("World") != std::string::npos;  // true

// 替换
s.replace(7, 5, "Taco");  // "Hello, Taco!"

// 首尾操作
s.front();     // 'H'
s.back();      // '!'

// 插入和删除
s.insert(7, "Beautiful ");  // 在下标 7 处插入
s.erase(7, 10);             // 从下标 7 删除 10 个字符

// 转换大小写（标准库没有直接提供，需要借助算法）
#include <algorithm>
#include <cctype>
std::transform(s.begin(), s.end(), s.begin(), ::toupper);
```

---

### 字符串与数字的转换

```cpp
// 数字转字符串
std::string s = std::to_string(42);      // "42"
std::string s2 = std::to_string(3.14);   // "3.140000"

// 字符串转数字
int n = std::stoi("42");        // 42
double d = std::stod("3.14");   // 3.14
long l = std::stol("1000000");  // 1000000

// 非法输入会抛异常
try {
    int bad = std::stoi("hello");  // 抛出 std::invalid_argument
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
}
```

---

### 字符串字面量的类型

有几种不同的字符串字面量：

```cpp
"hello"           // const char*，C 风格字符串
"hello"s          // std::string（需要 using namespace std::string_literals）
R"(hello\nworld)" // 原始字符串字面量，\n 不会被转义
u8"你好"          // UTF-8 编码的字符串
```

原始字符串字面量在写正则表达式或 JSON 时很有用：

```cpp
// 普通字符串，需要大量转义
std::string json = "{ \"name\": \"Miguel\", \"age\": 12 }";

// 原始字符串，不需要转义
std::string json = R"({ "name": "Miguel", "age": 12 })";
```

---

### C 字符串和 std::string 的互转

实际项目里经常需要在两者之间转换：

```cpp
// std::string → const char*
std::string s = "hello";
const char* cstr = s.c_str();  // 返回 C 风格字符串指针

// const char* → std::string
const char* cstr2 = "world";
std::string s2 = cstr2;  // 隐式转换，直接赋值即可
std::string s3(cstr2);   // 或者用构造函数
```

注意：`c_str()` 返回的指针指向 `std::string` 内部的数据，`std::string` 对象销毁后，这个指针就变成悬空指针，不能再使用。

---

## 4.2 iostream：cin 与 cout

### cout：标准输出

`std::cout` 是标准输出流，通常对应终端屏幕：

```cpp
#include <iostream>

std::cout << "Hello\n";           // 输出字符串
std::cout << 42 << "\n";         // 输出整数
std::cout << 3.14 << "\n";       // 输出浮点数
std::cout << true << "\n";       // 输出 1（bool 默认输出 0/1）

// 链式输出
std::cout << "Name: " << "Miguel" << ", Age: " << 12 << "\n";
```

`"\n"` 和 `std::endl` 的区别：

```cpp
std::cout << "Hello\n";        // 换行
std::cout << "Hello" << std::endl;  // 换行 + 刷新缓冲区
```

`std::endl` 会刷新输出缓冲区，确保内容立刻显示在屏幕上。在大多数情况下，`"\n"` 就够了，而且更快——频繁刷新缓冲区会降低性能。只在确实需要立刻看到输出时（比如打印调试信息后程序崩溃了）才用 `std::endl`。

---

### cin：标准输入

`std::cin` 读取用户输入：

```cpp
int n;
std::cin >> n;  // 读取一个整数

std::string name;
std::cin >> name;  // 读取一个单词（遇到空格停止）

std::string line;
std::getline(std::cin, line);  // 读取一整行（包括空格）
```

`>>` 运算符会跳过前导空白（空格、换行、制表符），然后读取对应类型的数据。`std::getline` 读取一整行，直到遇到换行符（换行符本身被丢弃）。

一个常见的陷阱：混合使用 `>>` 和 `getline` 时，`>>` 不会消耗换行符，导致 `getline` 读到一个空行：

```cpp
int n;
std::cin >> n;           // 读取数字，但换行符留在缓冲区里

std::string line;
std::getline(std::cin, line);  // 读到空行！因为缓冲区里还有换行符
```

解决方法：在用 `getline` 之前，先忽略残留的换行符：

```cpp
std::cin >> n;
std::cin.ignore();  // 忽略一个字符（换行符）
std::getline(std::cin, line);  // 现在才能正确读到下一行
```

---

### cerr 和 clog：错误输出

除了 `cout`，标准库还提供了 `cerr` 和 `clog`：

```cpp
std::cerr << "Error: file not found\n";  // 标准错误输出，不缓冲
std::clog << "Info: starting...\n";      // 标准错误输出，有缓冲
```

`cerr` 和 `clog` 输出到标准错误流（stderr），而 `cout` 输出到标准输出流（stdout）。终端里两者看起来一样，但可以分别重定向：

```bash
./taco 2>/dev/null    # 丢弃错误输出，保留正常输出
./taco > output.txt   # 把正常输出写到文件，错误输出仍然显示
```

在 Taco 解释器里，错误信息用 `cerr` 输出，正常结果用 `cout` 输出。

---

## 4.3 格式化输出

### printf 风格（C 遗产）

C 的 `printf` 依然可以在 C++ 里用：

```cpp
#include <cstdio>

printf("Name: %s, Age: %d\n", "Miguel", 12);
printf("Pi: %.2f\n", 3.14159);  // 保留 2 位小数
```

`printf` 的格式符：
- `%d`：整数
- `%f`：浮点数
- `%s`：字符串（C 风格）
- `%c`：字符
- `%x`：十六进制整数

`printf` 的问题：类型不安全。如果格式符和参数类型不匹配，会产生未定义行为（undefined behavior）——程序可能输出乱码，可能崩溃，也可能看起来正常但实际上在悄悄损坏内存。编译器会警告这类错误，但不会报错。

---

### iostream 风格

`cout <<` 是类型安全的——编译器知道每个参数的类型，选择正确的输出方式：

```cpp
std::cout << "Name: " << "Miguel" << ", Age: " << 12 << "\n";
```

但 `cout` 的格式控制比较麻烦，需要用操纵符（manipulator）：

```cpp
#include <iomanip>

// 保留 2 位小数
std::cout << std::fixed << std::setprecision(2) << 3.14159 << "\n";

// 十六进制
std::cout << std::hex << 255 << "\n";  // ff

// 宽度和填充
std::cout << std::setw(10) << std::setfill('0') << 42 << "\n";  // 0000000042
```

这些操纵符写起来比 `printf` 格式符啰嗦很多。

---

### std::format（C++20）和 fmtlib（第三方库）

C++20 引入了 `std::format`，结合了 `printf` 的简洁和 `cout` 的类型安全：

```cpp
#include <format>  // C++20

std::string s = std::format("Name: {}, Age: {}", "Miguel", 12);
std::cout << s << "\n";

// 格式控制
std::cout << std::format("Pi: {:.2f}\n", 3.14159);  // Pi: 3.14
std::cout << std::format("{:>10}\n", "right");       // 右对齐，宽度 10
std::cout << std::format("{:0>5}\n", 42);            // 00042
```

如果编译器不支持 C++20，可以用 `fmtlib`——`std::format` 的原型就是从 fmtlib 来的，API 几乎完全相同：

```cmake
# CMakeLists.txt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.1.1
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(taco PRIVATE fmt::fmt)
```

```cpp
#include <fmt/format.h>

fmt::print("Name: {}, Age: {}\n", "Miguel", 12);
std::string s = fmt::format("Pi: {:.2f}", 3.14159);
```

本书使用 `std::format`（C++17 的编译器用 fmtlib 代替），它是目前格式化输出的最佳选择。

---

## 4.4 文件 I/O：fstream

### 读写文件

C++ 用 `fstream` 头文件里的类来处理文件：

```cpp
#include <fstream>
#include <string>
#include <iostream>
```

**写文件**：

```cpp
std::ofstream file("output.txt");  // 创建并打开文件
if (!file) {
    std::cerr << "Failed to open file\n";
    return 1;
}

file << "Hello, Taco!\n";
file << 42 << "\n";
// file 析构时自动关闭，不需要手动 close()
```

**读文件**：

```cpp
std::ifstream file("input.txt");
if (!file) {
    std::cerr << "File not found\n";
    return 1;
}

// 逐行读取
std::string line;
while (std::getline(file, line)) {
    std::cout << line << "\n";
}
```

**读取整个文件内容**：

```cpp
std::ifstream file("script.taco");
std::string content(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>()
);
// content 现在包含文件的全部内容
```

这种写法有点奇怪，但是很常用的惯用法。也可以用更直观的方式：

```cpp
std::ifstream file("script.taco");
std::ostringstream buffer;
buffer << file.rdbuf();
std::string content = buffer.str();
```

---

### 文件打开模式

`ofstream` 默认会覆盖已有文件。如果想追加内容：

```cpp
std::ofstream file("log.txt", std::ios::app);  // 追加模式
file << "New log entry\n";
```

常用的打开模式：

```cpp
std::ios::in      // 读（ifstream 默认）
std::ios::out     // 写（ofstream 默认，覆盖）
std::ios::app     // 追加
std::ios::binary  // 二进制模式
std::ios::trunc   // 清空文件（out 默认包含）
```

可以组合使用：

```cpp
// 读写都支持，二进制模式
std::fstream file("data.bin", std::ios::in | std::ios::out | std::ios::binary);
```

---

### Taco 的文件读取函数

在 Taco 里，需要读取 `.taco` 脚本文件。封装一个辅助函数：

```cpp
// src/utils.h
#pragma once
#include <string>
#include <optional>

// 读取文件内容，失败返回空 optional
std::optional<std::string> read_file(const std::string& path);
```

```cpp
// src/utils.cpp
#include "utils.h"
#include <fstream>
#include <sstream>

std::optional<std::string> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;  // 文件打开失败
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
```

使用：

```cpp
auto content = read_file("script.taco");
if (!content) {
    std::cerr << "Error: cannot open file\n";
    return 1;
}
std::cout << *content;  // 解引用 optional 获取字符串
```

`std::optional` 是 C++17 的特性，表示一个"可能有值，也可能没有值"的类型。第十九章会详细介绍。这里只需要知道：`std::nullopt` 表示没有值，`*content` 获取值。

---

## 4.5 filesystem：现代文件系统操作

C++17 引入了 `<filesystem>` 头文件，提供了跨平台的文件系统操作：

```cpp
#include <filesystem>
namespace fs = std::filesystem;  // 别名，简化书写
```

---

### 常用操作

```cpp
// 检查文件是否存在
fs::path p = "script.taco";
if (fs::exists(p)) {
    std::cout << "File exists\n";
}

// 获取文件信息
std::cout << fs::file_size(p) << " bytes\n";
std::cout << p.extension() << "\n";    // ".taco"
std::cout << p.filename() << "\n";     // "script.taco"
std::cout << p.stem() << "\n";         // "script"（不含扩展名）
std::cout << p.parent_path() << "\n";  // 父目录

// 列出目录内容
for (const auto& entry : fs::directory_iterator(".")) {
    std::cout << entry.path().filename() << "\n";
}

// 递归列出
for (const auto& entry : fs::recursive_directory_iterator(".")) {
    if (entry.path().extension() == ".taco") {
        std::cout << entry.path() << "\n";
    }
}

// 创建目录
fs::create_directory("output");
fs::create_directories("a/b/c");  // 递归创建

// 复制、移动、删除
fs::copy("src.taco", "dst.taco");
fs::rename("old.taco", "new.taco");
fs::remove("file.taco");
fs::remove_all("directory");  // 递归删除

// 获取当前目录
fs::path cwd = fs::current_path();
std::cout << cwd << "\n";
```

---

### 路径操作

`fs::path` 是跨平台的路径类，自动处理不同系统的路径分隔符（`/` 和 `\`）：

```cpp
fs::path p = "src";
p /= "lexer.cpp";   // 用 /= 拼接路径
// p 现在是 "src/lexer.cpp"（Linux/Mac）或 "src\lexer.cpp"（Windows）

fs::path absolute = fs::absolute(p);  // 转换为绝对路径
fs::path canonical = fs::canonical(p); // 解析符号链接，转换为真实路径
```

---

### 在 Taco 里的应用

Taco 解释器运行脚本时，需要处理文件路径：

```cpp
int main(int argc, char* argv[]) {
    if (argc < 2) {
        // 没有参数，启动 REPL
        start_repl();
        return 0;
    }

    fs::path script_path = argv[1];

    // 检查文件存在且是 .taco 文件
    if (!fs::exists(script_path)) {
        std::cerr << "🌮 Error: file not found: " << script_path << "\n";
        return 1;
    }

    if (script_path.extension() != ".taco") {
        std::cerr << "🌮 Warning: expected .taco file\n";
    }

    auto content = read_file(script_path.string());
    if (!content) {
        std::cerr << "🌮 Error: cannot read file\n";
        return 1;
    }

    run(*content);
    return 0;
}
```

---

## 小结

这一章讲了 C++ 的字符串和输入输出：

**std::string** 是 C 字符串的现代替代品，自动管理内存，支持 `+` 拼接和 `==` 比较，内置丰富的操作方法。

**iostream** 提供了类型安全的输入输出。`cout` 用于正常输出，`cerr` 用于错误输出。`"\n"` 比 `std::endl` 更高效。

**std::format**（或 fmtlib）是格式化字符串的最佳方式，兼具 `printf` 的简洁和 `cout` 的类型安全。

**fstream** 处理文件读写，结合 RAII，文件会在对象析构时自动关闭。

**filesystem**（C++17）提供跨平台的文件系统操作，`fs::path` 自动处理不同系统的路径差异。

---

下一章是第一部分的收尾：介绍解释器的架构，定义 Taco 语言的设计，并完成第一个版本（v0）——一个能把 Taco 代码切成 Token 的词法分析器。
