# 第七章：类的基础

---

第一部分讲的是 C++ 的"升级版 C"——那些让 C 写起来更舒服的特性。从这一章开始进入第二部分：类与对象。

类是 C++ 最核心的特性之一，也是和 C 差距最大的地方。Python 里已经有类的概念，所以这里不从零解释"什么是封装"，而是直接对比：C++ 的类和 Python 的类有什么相同，有什么不同，为什么会有这些不同。

---

## 7.1 从 Python 的类到 C++ 的类：对比切入

先来看同一个概念在两种语言里的写法。

**Python 版本**：

```python
class Token:
    def __init__(self, type, value):
        self.type = type
        self.value = value

    def to_string(self):
        return f"[{self.type}] \"{self.value}\""

t = Token("NUMBER", "42")
print(t.to_string())  # [NUMBER] "42"
```

**C++ 版本**：

```cpp
#include <string>
#include <iostream>

class Token {
public:
    std::string type;
    std::string value;

    Token(std::string type, std::string value)
        : type(std::move(type)), value(std::move(value))
    {}

    std::string to_string() const {
        return "[" + type + "] \"" + value + "\"";
    }
};

int main() {
    Token t("NUMBER", "42");
    std::cout << t.to_string() << "\n";  // [NUMBER] "42"
}
```

表面上很相似：都有数据成员、构造函数、成员函数。但有几个关键区别：

**区别一：必须声明类型**

Python 里 `self.type = type` 就直接创建了成员变量，类型是运行时决定的。C++ 里必须在类定义里声明每个成员变量的类型：`std::string type;`。

**区别二：访问控制**

C++ 有 `public`、`private`、`protected` 关键字，明确控制哪些东西外部可以访问。Python 里只有约定（以 `_` 开头表示"私有"），没有强制性的访问控制。

**区别三：构造函数的写法**

Python 的构造函数叫 `__init__`，C++ 的构造函数和类同名（`Token`）。C++ 还有初始化列表（`: type(...), value(...)`），这是 C++ 特有的语法，第八章会详细讲。

**区别四：`const` 成员函数**

C++ 的 `to_string()` 后面有一个 `const`，表示这个函数不会修改对象的状态。Python 没有这个概念——任何函数都可以修改 `self`。

**区别五：对象的创建方式**

Python 里 `t = Token("NUMBER", "42")` 返回的是一个对象引用，对象本身在堆上。C++ 里 `Token t("NUMBER", "42")` 默认在栈上创建对象，`Token* t = new Token("NUMBER", "42")` 才是在堆上。这个区别非常重要，第十五章讲智能指针时会深入讨论。

---

## 7.2 成员变量与成员函数

### 成员变量

成员变量（member variable）是类的数据部分，描述对象的状态：

```cpp
class Lexer {
    std::string m_source;  // 源代码
    int         m_pos;     // 当前位置
    int         m_line;    // 当前行号
};
```

命名约定：很多 C++ 项目用 `m_` 前缀表示成员变量（member），区别于局部变量和参数。这不是语言要求，但是一个有用的习惯——读代码时一眼就能看出哪些是成员变量。

成员变量可以在声明时给默认值（C++11 起）：

```cpp
class Lexer {
    std::string m_source;
    int m_pos    = 0;     // 默认值
    int m_line   = 1;     // 默认值
    int m_column = 1;     // 默认值
};
```

---

### 成员函数

成员函数（member function）是类的行为部分，定义对象能做什么：

```cpp
class Lexer {
public:
    // 成员函数：可以访问 m_source、m_pos 等成员变量
    char current() const {
        if (is_at_end()) return '\0';
        return m_source[m_pos];
    }

    bool is_at_end() const {
        return m_pos >= static_cast<int>(m_source.size());
    }
};
```

成员函数可以直接访问同一个类的成员变量，不需要传参数。这就是"封装"的核心——数据和操作数据的函数绑定在一起。

**在类外定义成员函数**

成员函数可以在类定义里声明，在类外实现：

```cpp
// lexer.h：只声明
class Lexer {
public:
    char current() const;
    bool is_at_end() const;
private:
    std::string m_source;
    int m_pos = 0;
};

// lexer.cpp：实现，用 Lexer:: 前缀表明属于 Lexer 类
char Lexer::current() const {
    if (is_at_end()) return '\0';
    return m_source[m_pos];
}

bool Lexer::is_at_end() const {
    return m_pos >= static_cast<int>(m_source.size());
}
```

这是大型项目里的标准做法：头文件只放接口（声明），源文件放实现。

---

## 7.3 访问控制：public、private、protected

C++ 用三个关键字控制成员的访问权限：

- `public`：任何地方都可以访问
- `private`：只有类自己的成员函数可以访问
- `protected`：类自己和子类可以访问（第十二章讲继承时再展开）

```cpp
class BankAccount {
public:
    // 公开接口：外部可以调用
    void deposit(double amount) {
        if (amount > 0) m_balance += amount;
    }

    double get_balance() const {
        return m_balance;
    }

private:
    // 私有数据：外部无法直接访问和修改
    double m_balance = 0.0;
    std::string m_owner;
};

BankAccount account;
account.deposit(100.0);          // 可以，deposit 是 public
double b = account.get_balance(); // 可以，get_balance 是 public
account.m_balance = 9999.0;      // 错误！m_balance 是 private
```

**为什么要 private？**

把数据设为 private，强迫外部代码通过公开接口操作对象，而不是直接修改数据。这样有几个好处：

- 可以在接口里做检查（`if (amount > 0)`），防止非法操作
- 内部实现可以改变，只要接口不变，外部代码不需要修改
- 读代码时，public 部分就是类的"说明书"，private 是实现细节

**struct 和 class 的区别**

C++ 里 `struct` 和 `class` 几乎完全相同，只有一个区别：`struct` 的成员默认是 `public`，`class` 的成员默认是 `private`。

```cpp
struct Point {
    double x;  // 默认 public
    double y;  // 默认 public
};

class Token {
    // 这里是 private！
    std::string value;
public:
    // 这里才是 public
    std::string get_value() const { return value; }
};
```

惯例上，`struct` 用于简单的数据容器（不需要封装），`class` 用于有复杂行为的对象（需要封装）。在 Taco 项目里，`Token` 用 `struct`（只是数据），`Lexer` 用 `class`（有复杂的内部逻辑）。

---

## 7.4 this 指针

在成员函数里，有一个隐式的指针叫 `this`，指向调用这个函数的对象本身：

```cpp
class Counter {
public:
    void increment() {
        this->count++;  // 等价于 count++
    }

    // 返回对象自身的引用，可以链式调用
    Counter& add(int n) {
        this->count += n;
        return *this;  // 解引用 this，返回对象本身
    }

    int get() const {
        return this->count;  // 等价于 count
    }

private:
    int count = 0;
};

Counter c;
c.add(5).add(3).add(2);  // 链式调用
std::cout << c.get();    // 10
```

大多数情况下不需要显式写 `this->`，编译器知道成员函数里的 `count` 就是 `this->count`。但有两种情况必须用 `this`：

**情况一：参数名和成员变量同名**

```cpp
class Token {
public:
    Token(std::string value) {
        this->value = value;  // this->value 是成员变量，value 是参数
    }
private:
    std::string value;
};
```

不过更好的做法是用初始化列表（下一章讲），或者给成员变量加 `m_` 前缀来避免歧义。

**情况二：返回对象自身**

```cpp
Counter& add(int n) {
    count += n;
    return *this;  // 必须用 this 才能返回对象自身的引用
}
```

---

### Taco 里的应用

在 Taco 项目里，`Lexer` 类用 `this` 的地方主要是构造函数。当前版本的 Lexer 用了成员变量初始化列表（下一章详细讲），所以 `this` 用得不多。但理解 `this` 的存在，有助于理解成员函数和普通函数的本质区别：

成员函数在底层其实就是一个普通函数，只不过编译器帮你把"调用这个函数的对象"作为第一个参数（`this`）传进去。这就是为什么 Python 里需要显式写 `self`，而 C++ 里 `this` 是隐式的。

```python
# Python：self 是显式参数
def to_string(self):
    return self.value
```

```cpp
// C++：this 是隐式指针
std::string to_string() const {
    return value;  // 等价于 this->value
}
```

---

## 7.5 回到项目：用 class 实现 Lexer

第六章定义了 `Token`，但只完成了一半：还需要一个东西，能读入源代码字符串，逐字符扫描，把它切成 Token 列表。这就是 `Lexer`。

`Token` 用 `struct` 就够了，因为它只是一捆数据，没有行为。`Lexer` 不一样：它有内部状态（读到第几个字符了、当前第几行第几列），还有一堆只在内部使用的辅助函数（`advance`、`peek`、`match`……），不希望外部代码随便碰这些细节，只暴露一个干净的接口——"给我源代码，还我 Token 列表"。这正是上面几节讲的封装：数据和操作数据的函数绑定在一起，用 `private` 藏起实现细节，只留 `public` 接口。所以 `Lexer` 用 `class` 来写。

按照 7.2 提到的惯例，`Lexer` 把声明和实现分开：`lexer.h` 只放接口，`lexer.cpp` 放具体实现。

### lexer.h

```cpp
#pragma once
#include "token.h"
#include <string>
#include <vector>
#include <unordered_map>

class Lexer {
public:
    // 构造函数：接收源代码字符串
    explicit Lexer(std::string source);

    // 核心接口：把源代码切成 Token 列表
    std::vector<Token> tokenize();

private:
    std::string m_source;   // 源代码
    int         m_pos;      // 当前读取位置
    int         m_line;     // 当前行号
    int         m_column;   // 当前列号

    // 关键字表：字符串 → TokenType
    static const std::unordered_map<std::string, TokenType> KEYWORDS;

    // 辅助函数
    char current() const;           // 当前字符
    char peek(int offset = 1) const;// 向前看 offset 个字符
    char advance();                 // 消费当前字符，前进一步
    bool is_at_end() const;         // 是否到达末尾
    bool match(char expected);      // 如果下一个字符匹配，消费并返回 true

    // 创建 Token
    Token make_token(TokenType type, const std::string& value) const;

    // 跳过空白和注释
    void skip_whitespace_and_comments();

    // 各类 Token 的读取函数
    Token read_number();
    Token read_string();
    Token read_identifier_or_keyword();
    Token read_taco_emoji();

    // 读取下一个 Token
    Token next_token();
};
```

---

### lexer.cpp

```cpp
#include "lexer.h"
#include <stdexcept>
#include <sstream>

// 关键字表：static 成员，所有 Lexer 实例共享一份
const std::unordered_map<std::string, TokenType> Lexer::KEYWORDS = {
    {"var",     TokenType::Var},
    {"func",    TokenType::Func},
    {"if",      TokenType::If},
    {"elseif",  TokenType::Elseif},
    {"else",    TokenType::Else},
    {"while",   TokenType::While},
    {"for",     TokenType::For},
    {"in",      TokenType::In},
    {"return",  TokenType::Return},
    {"true",    TokenType::True},
    {"false",   TokenType::False},
    {"nil",     TokenType::Nil},
    {"class",   TokenType::Class},
    {"struct",  TokenType::Struct},
    {"enum",    TokenType::Enum},
    {"extends", TokenType::Extends},
    {"self",    TokenType::Self},
    {"super",   TokenType::Super},
    {"switch",  TokenType::Switch},
    {"case",    TokenType::Case},
    {"default", TokenType::Default},
    {"import",  TokenType::Import},
    {"from",    TokenType::From},
    {"thread",  TokenType::Thread},
    {"channel", TokenType::Channel},
};

// 构造函数用的是"初始化列表"语法（: m_source(...), m_pos(0) ...）
// 这是 C++ 初始化成员变量的标准写法，比在函数体里逐个赋值更高效
// 下一章会专门讲这个语法，这里先照着写，能看懂"冒号后面是按成员变量顺序初始化"就够了
Lexer::Lexer(std::string source)
    : m_source(std::move(source))  // move 避免拷贝
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}

// ────────────────────────────────
// 辅助函数
// ────────────────────────────────

char Lexer::current() const {
    if (is_at_end()) return '\0';
    return m_source[m_pos];
}

char Lexer::peek(int offset) const {
    int idx = m_pos + offset;
    if (idx < 0 || idx >= static_cast<int>(m_source.size())) return '\0';
    return m_source[idx];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    return c;
}

bool Lexer::is_at_end() const {
    return m_pos >= static_cast<int>(m_source.size());
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (m_source[m_pos] != expected) return false;
    advance();
    return true;
}

Token Lexer::make_token(TokenType type, const std::string& value) const {
    return Token{type, value, m_line, m_column};
}

// ────────────────────────────────
// 跳过空白和注释
// ────────────────────────────────

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = current();

        // 跳过空白字符
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
            continue;
        }

        // 跳过单行注释 //
        if (c == '/' && peek() == '/') {
            while (!is_at_end() && current() != '\n') {
                advance();
            }
            continue;
        }

        break;
    }
}

// ────────────────────────────────
// 读取数字
// ────────────────────────────────

Token Lexer::read_number() {
    int start_line   = m_line;
    int start_column = m_column;
    std::string value;

    while (!is_at_end() && (std::isdigit(current()) || current() == '_')) {
        if (current() != '_') {   // 下划线只用于分隔，不加入值
            value += current();
        }
        advance();
    }

    // 小数部分
    if (current() == '.' && std::isdigit(peek())) {
        value += '.';
        advance();
        while (!is_at_end() && std::isdigit(current())) {
            value += advance();
        }
    }

    return Token{TokenType::Number, value, start_line, start_column};
}

// ────────────────────────────────
// 读取字符串
// ────────────────────────────────

Token Lexer::read_string() {
    int start_line   = m_line;
    int start_column = m_column;

    advance();  // 跳过开头的 "

    std::string value;
    while (!is_at_end() && current() != '"') {
        if (current() == '\\') {
            advance();  // 跳过反斜杠
            switch (current()) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                default:
                    value += '\\';
                    value += current();
            }
        } else {
            value += current();
        }
        advance();
    }

    if (is_at_end()) {
        // 字符串没有闭合
        throw std::runtime_error(
            "🌮 line " + std::to_string(start_line) +
            ": Unterminated string. Did you forget the closing \"?"
        );
    }

    advance();  // 跳过结尾的 "
    return Token{TokenType::String, value, start_line, start_column};
}

// ────────────────────────────────
// 读取标识符或关键字
// ────────────────────────────────

Token Lexer::read_identifier_or_keyword() {
    int start_line   = m_line;
    int start_column = m_column;
    std::string value;

    while (!is_at_end() && (std::isalnum(current()) || current() == '_')) {
        value += advance();
    }

    // 查关键字表，不在就是标识符
    auto it = KEYWORDS.find(value);
    TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::Identifier;

    return Token{type, value, start_line, start_column};
}

// ────────────────────────────────
// 读取 🌮 彩蛋
// ────────────────────────────────

Token Lexer::read_taco_emoji() {
    // 🌮 是 UTF-8 四字节序列：0xF0 0x9F 0x8C 0xAE
    // 调用时已经确认第一个字节是 0xF0，这里消费剩余三个字节
    int start_line   = m_line;
    int start_column = m_column;

    std::string value;
    value += advance();  // 0xF0
    value += advance();  // 0x9F
    value += advance();  // 0x8C
    value += advance();  // 0xAE

    return Token{TokenType::Taco, value, start_line, start_column};
}

// ────────────────────────────────
// 读取下一个 Token
// ────────────────────────────────

Token Lexer::next_token() {
    skip_whitespace_and_comments();

    if (is_at_end()) {
        return make_token(TokenType::EndOfFile, "");
    }

    int start_line   = m_line;
    int start_column = m_column;
    char c = current();

    // 数字
    if (std::isdigit(c)) {
        return read_number();
    }

    // 字符串
    if (c == '"') {
        return read_string();
    }

    // 标识符或关键字
    if (std::isalpha(c) || c == '_') {
        return read_identifier_or_keyword();
    }

    // 🌮 彩蛋（UTF-8 四字节，第一个字节是 0xF0）
    if (static_cast<unsigned char>(c) == 0xF0) {
        // 检查接下来三个字节是否匹配 🌮
        if (static_cast<unsigned char>(peek(1)) == 0x9F &&
            static_cast<unsigned char>(peek(2)) == 0x8C &&
            static_cast<unsigned char>(peek(3)) == 0xAE) {
            return read_taco_emoji();
        }
    }

    // 消费当前字符，处理单字符和双字符运算符
    advance();

    switch (c) {
        case '+': return Token{TokenType::Plus,         "+", start_line, start_column};
        case '-': return Token{TokenType::Minus,        "-", start_line, start_column};
        case '*': return Token{TokenType::Star,         "*", start_line, start_column};
        case '/': return Token{TokenType::Slash,        "/", start_line, start_column};
        case '%': return Token{TokenType::Percent,      "%", start_line, start_column};
        case '^': return Token{TokenType::Caret,        "^", start_line, start_column};
        case '?': return Token{TokenType::Question,     "?", start_line, start_column};
        case ':': return Token{TokenType::Colon,        ":", start_line, start_column};
        case ';': return Token{TokenType::Semicolon,    ";", start_line, start_column};
        case ',': return Token{TokenType::Comma,        ",", start_line, start_column};
        case '(': return Token{TokenType::LeftParen,    "(", start_line, start_column};
        case ')': return Token{TokenType::RightParen,   ")", start_line, start_column};
        case '{': return Token{TokenType::LeftBrace,    "{", start_line, start_column};
        case '}': return Token{TokenType::RightBrace,   "}", start_line, start_column};
        case '[': return Token{TokenType::LeftBracket,  "[", start_line, start_column};
        case ']': return Token{TokenType::RightBracket, "]", start_line, start_column};

        case '!':
            if (match('=')) return Token{TokenType::NotEqual,     "!=", start_line, start_column};
            return Token{TokenType::Not, "!", start_line, start_column};

        case '=':
            if (match('=')) return Token{TokenType::Equal,  "==", start_line, start_column};
            return Token{TokenType::Assign, "=", start_line, start_column};

        case '<':
            if (match('=')) return Token{TokenType::LessEqual,    "<=", start_line, start_column};
            return Token{TokenType::Less, "<", start_line, start_column};

        case '>':
            if (match('=')) return Token{TokenType::GreaterEqual, ">=", start_line, start_column};
            return Token{TokenType::Greater, ">", start_line, start_column};

        case '&':
            if (match('&')) return Token{TokenType::And, "&&", start_line, start_column};
            break;

        case '|':
            if (match('>')) return Token{TokenType::PipeArrow, "|>", start_line, start_column};
            break;

        case '.':
            if (current() == '.' && peek() == '.') {
                advance(); advance();
                return Token{TokenType::Ellipsis, "...", start_line, start_column};
            }
            return Token{TokenType::Dot, ".", start_line, start_column};
    }

    // 遇到无法识别的字符，报错
    throw std::runtime_error(
        "🌮 line " + std::to_string(start_line) +
        ": Unexpected character '" + c + "'."
    );
}

// ────────────────────────────────
// 核心接口
// ────────────────────────────────

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        Token t = next_token();
        tokens.push_back(t);
        if (t.type == TokenType::EndOfFile) break;
    }

    return tokens;
}
```

---

## 7.6 测试：把 `var x = 10;` 切成 Token 列表

### main.cpp

```cpp
#include <iostream>
#include "lexer.h"
#include "token.h"

int main() {
    std::string source = R"(
var x = 10 + 20;
var name = "Miguel";
var flag = true;

func greet(name) {
    print("Hola, " + name + "!");
}
)";

    Lexer lexer(source);

    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenize();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    // 打印每个 Token
    for (const auto& token : tokens) {
        std::cout
            << "[" << token_type_to_string(token.type) << "] "
            << "\"" << token.value << "\""
            << " (line " << token.line << ")\n";
    }

    return 0;
}
```

### 更新 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_executable(taco
    src/main.cpp
    src/token.cpp
    src/lexer.cpp
)

target_include_directories(taco PRIVATE src)
```

### 编译运行

```bash
cmake -B build
cmake --build build
./build/taco
```

### 输出

```
[VAR] "var" (line 2)
[IDENTIFIER] "x" (line 2)
[ASSIGN] "=" (line 2)
[NUMBER] "10" (line 2)
[PLUS] "+" (line 2)
[NUMBER] "20" (line 2)
[SEMICOLON] ";" (line 2)
[VAR] "var" (line 3)
[IDENTIFIER] "name" (line 3)
[ASSIGN] "=" (line 3)
[STRING] "Miguel" (line 3)
[SEMICOLON] ";" (line 3)
[VAR] "var" (line 4)
[IDENTIFIER] "flag" (line 4)
[ASSIGN] "=" (line 4)
[TRUE] "true" (line 4)
[SEMICOLON] ";" (line 4)
[FUNC] "func" (line 6)
[IDENTIFIER] "greet" (line 6)
[LEFT_PAREN] "(" (line 6)
[IDENTIFIER] "name" (line 6)
[RIGHT_PAREN] ")" (line 6)
[LEFT_BRACE] "{" (line 6)
[IDENTIFIER] "print" (line 7)
[LEFT_PAREN] "(" (line 7)
[STRING] "Hola, " (line 7)
[PLUS] "+" (line 7)
[IDENTIFIER] "name" (line 7)
[PLUS] "+" (line 7)
[STRING] "!" (line 7)
[RIGHT_PAREN] ")" (line 7)
[SEMICOLON] ";" (line 7)
[RIGHT_BRACE] "}" (line 8)
[EOF] "" (line 9)
```

词法分析器正确地把源代码切成了 Token 列表。关键字 `var`、`func`、`true` 被识别为对应的类型，而不是标识符。字符串的内容（`"Miguel"`）去掉了引号，只保留内容本身。

---

## 7.7 这个版本的局限性

v0 能正确识别 Token，但有几个明显的局限性：

**不支持字符串插值**

```taco
var greeting = "Hola, {name}!";
```

`"Hola, {name}!"` 现在被当成一个普通字符串，不会把 `{name}` 识别成插值表达式。字符串插值需要在读取字符串时，把 `{...}` 里的内容单独切出来，作为一个表达式处理。这需要解析器的配合，v1 再处理。

**没有行列号精确追踪**

当前的行号追踪是粗糙的——只在遇到 `\n` 时更新行号。列号的追踪也不够准确。v1 引入 AST 之后，会用行列号来生成精确的错误信息。

**没有从文件读取**

现在源代码是硬编码在 `main.cpp` 里的字符串。下一步要从 `.taco` 文件读取，并处理命令行参数。

**没有彩蛋逻辑**

🌮 Token 现在只是被识别出来，但没有任何特殊行为。彩蛋逻辑（Magic 8 Ball、魔法海螺等）会在 v6 的 REPL 里实现——在那之前，🌮 只是一个普通的 Token。

---

### v0 项目小结

到这里，v0 完成了解释器的第一层：词法分析器。

**Token** 是词法分析的输出单元，包含类型和原始文本，用 `struct` 定义。`enum class` 比普通 `enum` 更安全，是定义 TokenType 的正确方式。

**Lexer** 用 `class` 定义，核心逻辑是 `next_token()`：跳过空白和注释，然后根据当前字符决定读哪种 Token。关键字通过查表（`unordered_map`）来识别，避免了一大堆 `if/else`。这也是本章学到的封装思想第一次在 Taco 项目里落地——`Lexer` 把扫描逻辑的所有细节都藏在 `private` 里，外部只看得到 `tokenize()` 这一个接口。

**错误处理** 现在很简单：遇到无法识别的字符就抛出异常，打印带行号的错误信息。这符合 Taco 的设计——出错直接崩溃，信息要清晰。

---

## 小结

这一章分两部分：先讲 C++ 类的基础语法，再回到 Taco 项目，把 v0 的最后一块拼图补上。

**成员变量**描述对象的状态，**成员函数**描述对象的行为。两者绑定在一起，这就是封装。

**访问控制**（`public`/`private`）让类可以暴露稳定的接口，隐藏实现细节。`struct` 默认 `public`，`class` 默认 `private`——根据是否需要封装来选择。

**`this` 指针**是每个成员函数里隐式存在的，指向当前对象。大多数时候不需要显式写，但返回对象自身时必须用 `*this`。

学完这些之后，`Lexer` 类就不再是一堆看不懂的语法了：`public`/`private` 划清了对外接口和内部细节的界限，类外定义（`Lexer::xxx`）把声明和实现分开，构造函数完成了初始化。v0 到这里就完整了——`Lexer` 的构造函数里用了"初始化列表"语法（`: m_source(...), m_pos(0) ...`），当时只是先照着写，下一章会正式讲清楚它到底是什么、为什么比函数体里赋值更好。

---

下一章讲构造函数和析构函数——对象是怎么诞生和消亡的，以及 C++ 里最重要的设计模式之一：RAII。
