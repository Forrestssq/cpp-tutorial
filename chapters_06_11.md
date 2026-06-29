# 第六章：项目 v0——词法分析器

---

前五章打下了基础：C++ 的语法、构建系统、字符串处理，以及解释器的整体架构。现在开始动手。

v0 的目标很简单：把一段 Taco 源代码字符串，切成一个个有意义的 Token。不求值，不解析语法树，只做词法分析。

```
输入："var x = 10 + 20;"
输出：[VAR "var"] [IDENTIFIER "x"] [ASSIGN "="] [NUMBER "10"] [PLUS "+"] [NUMBER "20"] [SEMICOLON ";"] [EOF]
```

---

## 6.1 什么是 Token

词法分析器（Lexer）做的事情，类似人类读代码时的直觉：

看到 `var`，知道这是一个关键字。
看到 `x`，知道这是一个标识符。
看到 `10`，知道这是一个数字。
看到 `+`，知道这是一个运算符。

每一个这样的"有意义的单元"，就是一个 Token。

Token 有两个基本属性：
- **类型**（TokenType）：这个词是什么种类
- **值**（value）：这个词的原始文本

```
源代码：var x = 10 + 20;

Token 列表：
  Token { type: VAR,        value: "var" }
  Token { type: IDENTIFIER, value: "x"   }
  Token { type: ASSIGN,     value: "="   }
  Token { type: NUMBER,     value: "10"  }
  Token { type: PLUS,       value: "+"   }
  Token { type: NUMBER,     value: "20"  }
  Token { type: SEMICOLON,  value: ";"   }
  Token { type: EOF,        value: ""    }
```

注意最后有一个 `EOF`（End of File）Token。这是一个哨兵值，告诉后续的语法分析器"输入结束了"，避免在处理 Token 列表时越界。

---

## 6.2 用结构体和枚举实现 Token

先定义 Token 的数据结构。这一章用的都是 C++ 的基础特性——结构体和枚举——还没有涉及类和继承。

### token.h

```cpp
#pragma once
#include <string>

// TokenType 枚举：用 enum class 而不是普通 enum
// enum class 更安全：不会隐式转换成 int，不同枚举的值不会互相冲突
enum class TokenType {
    // 字面量
    Number,       // 42, 3.14, 1_000_000
    String,       // "hello"
    True,         // true
    False,        // false
    Nil,          // nil

    // 标识符和关键字
    Identifier,   // x, name, greet
    Var,          // var
    Func,         // func
    If,           // if
    Elseif,       // elseif
    Else,         // else
    While,        // while
    For,          // for
    In,           // in
    Return,       // return
    Class,        // class
    Struct,       // struct
    Enum,         // enum
    Extends,      // extends
    Self,         // self
    Super,        // super
    Switch,       // switch
    Case,         // case
    Default,      // default
    Import,       // import
    From,         // from
    Thread,       // thread
    Channel,      // channel

    // 运算符
    Plus,         // +
    Minus,        // -
    Star,         // *
    Slash,        // /
    Percent,      // %
    Caret,        // ^
    Equal,        // ==
    NotEqual,     // !=
    Greater,      // >
    Less,         // <
    GreaterEqual, // >=
    LessEqual,    // <=
    And,          // &&
    Or,           // ||
    Not,          // !
    Assign,       // =
    Question,     // ?
    Colon,        // :
    Dot,          // .
    Ellipsis,     // ...
    PipeArrow,    // |>

    // 分隔符
    LeftParen,    // (
    RightParen,   // )
    LeftBrace,    // {
    RightBrace,   // }
    LeftBracket,  // [
    RightBracket, // ]
    Semicolon,    // ;
    Comma,        // ,

    // 彩蛋
    Taco,         // 🌮

    // 特殊
    EndOfFile,
};

// Token 结构体
struct Token {
    TokenType   type;
    std::string value;   // 原始文本
    int         line;    // 行号（从 1 开始）
    int         column;  // 列号（从 1 开始）
};

// 把 TokenType 转成字符串，方便调试
std::string token_type_to_string(TokenType type);
```

为什么用 `enum class` 而不是普通的 `enum`？

普通 `enum` 有两个问题：

```cpp
// 普通 enum：会隐式转换成 int，不同枚举的值可能冲突
enum Color { Red, Green, Blue };
enum Direction { North, South, East, West };

int c = Red;   // 可以，但这通常不是你想要的
if (Red == North) { ... }  // 比较两个不相关的枚举，编译器不报错！
```

`enum class` 解决了这两个问题：

```cpp
// enum class：类型安全
enum class Color { Red, Green, Blue };
enum class Direction { North, South, East, West };

int c = Color::Red;            // 错误！不能隐式转换
if (Color::Red == Direction::North) { ... }  // 错误！类型不匹配
```

用 `enum class` 写 TokenType，编译器会帮我们检查：如果不小心把 `TokenType::Number` 当 `int` 用，会直接报错，而不是悄悄产生 bug。

---

### token.cpp

```cpp
#include "token.h"

std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::Number:       return "NUMBER";
        case TokenType::String:       return "STRING";
        case TokenType::True:         return "TRUE";
        case TokenType::False:        return "FALSE";
        case TokenType::Nil:          return "NIL";
        case TokenType::Identifier:   return "IDENTIFIER";
        case TokenType::Var:          return "VAR";
        case TokenType::Func:         return "FUNC";
        case TokenType::If:           return "IF";
        case TokenType::Elseif:       return "ELSEIF";
        case TokenType::Else:         return "ELSE";
        case TokenType::While:        return "WHILE";
        case TokenType::For:          return "FOR";
        case TokenType::In:           return "IN";
        case TokenType::Return:       return "RETURN";
        case TokenType::Class:        return "CLASS";
        case TokenType::Struct:       return "STRUCT";
        case TokenType::Enum:         return "ENUM";
        case TokenType::Extends:      return "EXTENDS";
        case TokenType::Self:         return "SELF";
        case TokenType::Super:        return "SUPER";
        case TokenType::Switch:       return "SWITCH";
        case TokenType::Case:         return "CASE";
        case TokenType::Default:      return "DEFAULT";
        case TokenType::Import:       return "IMPORT";
        case TokenType::From:         return "FROM";
        case TokenType::Thread:       return "THREAD";
        case TokenType::Channel:      return "CHANNEL";
        case TokenType::Plus:         return "PLUS";
        case TokenType::Minus:        return "MINUS";
        case TokenType::Star:         return "STAR";
        case TokenType::Slash:        return "SLASH";
        case TokenType::Percent:      return "PERCENT";
        case TokenType::Caret:        return "CARET";
        case TokenType::Equal:        return "EQUAL";
        case TokenType::NotEqual:     return "NOT_EQUAL";
        case TokenType::Greater:      return "GREATER";
        case TokenType::Less:         return "LESS";
        case TokenType::GreaterEqual: return "GREATER_EQUAL";
        case TokenType::LessEqual:    return "LESS_EQUAL";
        case TokenType::And:          return "AND";
        case TokenType::Or:           return "OR";
        case TokenType::Not:          return "NOT";
        case TokenType::Assign:       return "ASSIGN";
        case TokenType::Question:     return "QUESTION";
        case TokenType::Colon:        return "COLON";
        case TokenType::Dot:          return "DOT";
        case TokenType::Ellipsis:     return "ELLIPSIS";
        case TokenType::PipeArrow:    return "PIPE_ARROW";
        case TokenType::LeftParen:    return "LEFT_PAREN";
        case TokenType::RightParen:   return "RIGHT_PAREN";
        case TokenType::LeftBrace:    return "LEFT_BRACE";
        case TokenType::RightBrace:   return "RIGHT_BRACE";
        case TokenType::LeftBracket:  return "LEFT_BRACKET";
        case TokenType::RightBracket: return "RIGHT_BRACKET";
        case TokenType::Semicolon:    return "SEMICOLON";
        case TokenType::Comma:        return "COMMA";
        case TokenType::Taco:         return "TACO";
        case TokenType::EndOfFile:    return "EOF";
    }
    return "UNKNOWN";
}
```

---

## 6.3 实现词法分析器

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

## 6.4 测试：把 `var x = 10;` 切成 Token 列表

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

## 6.5 这个版本的局限性

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

## 小结

v0 完成了解释器的第一层：词法分析器。

**Token** 是词法分析的输出单元，包含类型和原始文本。`enum class` 比普通 `enum` 更安全，是定义 TokenType 的正确方式。

**Lexer** 的核心逻辑是 `next_token()`：跳过空白和注释，然后根据当前字符决定读哪种 Token。关键字通过查表（`unordered_map`）来识别，避免了一大堆 `if/else`。

**错误处理** 现在很简单：遇到无法识别的字符就抛出异常，打印带行号的错误信息。这符合 Taco 的设计——出错直接崩溃，信息要清晰。

---

下一章开始进入第二部分：类与对象。学完类和继承之后，第十一章会用这些知识构建 AST，并实现语法分析器（v1）。
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

## 小结

这一章介绍了 C++ 类的基础：

**成员变量**描述对象的状态，**成员函数**描述对象的行为。两者绑定在一起，这就是封装。

**访问控制**（`public`/`private`）让类可以暴露稳定的接口，隐藏实现细节。`struct` 默认 `public`，`class` 默认 `private`——根据是否需要封装来选择。

**`this` 指针**是每个成员函数里隐式存在的，指向当前对象。大多数时候不需要显式写，但返回对象自身时必须用 `*this`。

---

下一章讲构造函数和析构函数——对象是怎么诞生和消亡的，以及 C++ 里最重要的设计模式之一：RAII。
# 第八章：构造与析构

---

上一章看到了类的基本结构。这一章深入讲对象的生命周期：它是怎么被创建的，又是怎么被销毁的。

这是 C++ 和 Python 差距最大的地方之一。Python 有垃圾回收，对象什么时候销毁不需要关心。C++ 没有垃圾回收，但它用一套精妙的机制来弥补这个缺失——RAII。

---

## 8.1 构造函数：对象如何诞生

构造函数（constructor）在对象被创建时自动调用，负责初始化对象的状态。

```cpp
class Lexer {
public:
    // 构造函数：和类同名，没有返回类型
    Lexer(std::string source) {
        m_source = std::move(source);
        m_pos    = 0;
        m_line   = 1;
        m_column = 1;
    }

private:
    std::string m_source;
    int m_pos;
    int m_line;
    int m_column;
};

// 创建对象时，构造函数自动调用
Lexer lexer("var x = 10;");
```

**和 Python 的 `__init__` 对比**：

```python
class Lexer:
    def __init__(self, source):
        self.source = source
        self.pos = 0
        self.line = 1
        self.column = 1
```

功能完全相同。区别在于：
- Python 的 `__init__` 是特殊方法名，C++ 的构造函数是和类同名的函数
- Python 需要显式写 `self`，C++ 里是隐式的 `this`
- C++ 的构造函数没有返回类型（连 `void` 都没有）

---

### 默认构造函数

如果没有定义任何构造函数，编译器会自动生成一个**默认构造函数**（default constructor），它什么都不做：

```cpp
class Point {
public:
    double x;
    double y;
    // 没有定义构造函数
};

Point p;   // 使用默认构造函数，x 和 y 的值未定义（是内存里的随机数！）
Point p2{};  // 值初始化，x 和 y 被初始化为 0
```

注意：`Point p` 和 `Point p2{}` 的行为不同。对于内置类型（`int`、`double` 等），`p` 的成员变量值是未定义的（读取它是未定义行为），`p2` 的成员变量被初始化为 0。

一旦定义了任何构造函数，编译器就不再自动生成默认构造函数：

```cpp
class Lexer {
public:
    Lexer(std::string source) { ... }  // 定义了带参数的构造函数
};

Lexer lexer;  // 错误！没有默认构造函数
```

如果既需要带参数的构造函数，又需要默认构造函数，可以显式定义，或者用 `= default`：

```cpp
class Token {
public:
    Token() = default;  // 显式要求编译器生成默认构造函数
    Token(std::string type, std::string value)
        : m_type(std::move(type)), m_value(std::move(value)) {}

private:
    std::string m_type;
    std::string m_value;
};

Token t1;              // 使用默认构造函数
Token t2("NUMBER", "42");  // 使用带参数的构造函数
```

---

### explicit 关键字

构造函数可以触发隐式类型转换，有时候这不是我们想要的：

```cpp
class Lexer {
public:
    Lexer(std::string source) { ... }
};

void run(Lexer lexer) { ... }

run("var x = 10;");  // 可以！"var x = 10;" 隐式转换成 Lexer
```

这里 `"var x = 10;"` 是一个 `const char*`，它先被转换成 `std::string`，再被用来构造 `Lexer`。这个隐式转换可能让代码难以理解。

用 `explicit` 关键字禁止隐式转换：

```cpp
class Lexer {
public:
    explicit Lexer(std::string source) { ... }
};

run("var x = 10;");     // 错误！不能隐式转换
run(Lexer("var x = 10;"));  // 正确：显式构造
```

对于只有一个参数的构造函数，`explicit` 几乎是标配——可以防止意外的隐式转换。

---

## 8.2 析构函数：对象如何消亡

析构函数（destructor）在对象被销毁时自动调用。

```cpp
class FileReader {
public:
    FileReader(const std::string& path) {
        m_file = std::fopen(path.c_str(), "r");
        if (!m_file) {
            throw std::runtime_error("Cannot open file: " + path);
        }
    }

    // 析构函数：~ 加类名，没有参数，没有返回类型
    ~FileReader() {
        if (m_file) {
            std::fclose(m_file);  // 自动关闭文件
            m_file = nullptr;
        }
    }

    // ... 其他成员函数

private:
    FILE* m_file = nullptr;
};
```

析构函数何时被调用？

```cpp
void process_file() {
    FileReader reader("script.taco");  // 构造函数调用，文件打开
    // ... 使用 reader
}   // 函数结束，reader 离开作用域，析构函数自动调用，文件关闭
```

**Python 没有析构函数的直接对应物**。Python 有 `__del__`，但它的调用时机是不确定的（取决于垃圾回收器），不能用于可靠的资源清理。C++ 的析构函数是确定性的——对象离开作用域，析构函数立刻调用。

---

### 什么时候需要析构函数

如果类持有需要手动释放的资源，就需要析构函数：

- 动态分配的内存（`new` 出来的）
- 文件句柄
- 网络连接
- 锁（mutex）
- 任何需要配对操作的资源（open/close、lock/unlock、acquire/release）

如果类只包含普通成员变量（`int`、`std::string`、`std::vector` 等），不需要定义析构函数——编译器自动生成的析构函数会正确销毁所有成员变量。

---

## 8.3 初始化列表

回到 Lexer 的构造函数。上一节用的是赋值方式初始化：

```cpp
Lexer::Lexer(std::string source) {
    m_source = std::move(source);  // 赋值
    m_pos    = 0;
    m_line   = 1;
    m_column = 1;
}
```

C++ 提供了另一种方式：**初始化列表**（member initializer list）：

```cpp
Lexer::Lexer(std::string source)
    : m_source(std::move(source))  // 直接初始化
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}
```

冒号 `:` 后面是初始化列表，每个成员变量用括号括起来要初始化的值。

**为什么要用初始化列表？**

有几个原因：

**原因一：效率**

用赋值方式，成员变量先被默认构造（对于 `std::string`，这会创建一个空字符串），然后再被赋值。用初始化列表，成员变量直接用给定的值构造，省去了默认构造这一步。

对于 `int` 这类简单类型，差别不大。但对于 `std::string`、`std::vector` 这类复杂类型，初始化列表更高效。

**原因二：必要性**

有些成员变量必须用初始化列表：

- `const` 成员变量：`const` 变量不能赋值，只能初始化
- 引用成员变量：引用必须在声明时绑定，不能用赋值
- 没有默认构造函数的成员变量

```cpp
class Config {
public:
    Config(int max_tokens, const std::string& filename)
        : m_max_tokens(max_tokens)   // const int 必须用初始化列表
        , m_filename(filename)       // const std::string& 引用
    {}

private:
    const int m_max_tokens;          // const，不能赋值
    const std::string& m_filename;   // 引用，不能重新绑定
};
```

**初始化顺序**

成员变量的初始化顺序取决于它们在类里的**声明顺序**，而不是初始化列表里的顺序。

```cpp
class Example {
    int m_a;
    int m_b;
public:
    Example(int a, int b)
        : m_b(b)   // 虽然 m_b 写在前面
        , m_a(a)   // 但 m_a 先初始化（因为声明在前）
    {}
};
```

为了避免混淆，最好让初始化列表的顺序和声明顺序一致。

---

## 8.4 RAII 初步：资源与对象生命周期绑定

RAII（Resource Acquisition Is Initialization）是 C++ 里最重要的设计模式。名字有点绕，但思想很简单：

> 用对象的生命周期来管理资源。对象构造时获取资源，对象析构时释放资源。

这样，只要对象的生命周期管理好了，资源就自动管理好了——不需要手动释放，也不会忘记释放。

### 没有 RAII 的世界

```cpp
void process_file(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return;

    // 处理文件...
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f)) {
        // ... 处理每一行
        if (some_error_condition) {
            std::fclose(f);  // 如果忘了这行，内存泄漏！
            return;
        }
    }

    std::fclose(f);  // 正常情况下关闭
}
```

每个 `return` 之前都需要手动关闭文件。如果有异常、有多个 `return`，很容易漏掉。这是 C 代码里常见的 bug 来源。

### 有 RAII 的世界

```cpp
class FileGuard {
public:
    explicit FileGuard(const std::string& path)
        : m_file(std::fopen(path.c_str(), "r"))
    {
        if (!m_file) {
            throw std::runtime_error("Cannot open: " + path);
        }
    }

    ~FileGuard() {
        if (m_file) std::fclose(m_file);
    }

    FILE* get() const { return m_file; }

    // 禁止拷贝（文件句柄不应该被复制）
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;

private:
    FILE* m_file;
};

void process_file(const std::string& path) {
    FileGuard guard(path);  // 打开文件

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), guard.get())) {
        if (some_error_condition) {
            return;  // guard 离开作用域，析构函数自动关闭文件
        }
    }
    // 函数结束，guard 析构，文件自动关闭
}
```

不管函数从哪里 `return`，甚至抛出异常，`FileGuard` 的析构函数都会被调用，文件一定被关闭。这就是 RAII 的魔力。

### RAII 无处不在

标准库里的容器都是 RAII 的：

```cpp
{
    std::vector<int> v = {1, 2, 3};  // 构造，分配内存
    v.push_back(4);
    // ... 使用 v
}  // v 析构，内存自动释放

{
    std::string s = "hello";  // 构造，分配内存
    s += " world";
}  // s 析构，内存自动释放
```

`std::mutex` 的 RAII 包装：

```cpp
std::mutex mu;

void safe_increment(int& count) {
    std::lock_guard<std::mutex> lock(mu);  // 加锁
    count++;
    // 函数结束，lock 析构，自动解锁
    // 不需要手动 mu.unlock()
}
```

第十六到十七章讲智能指针时，还会看到更多 RAII 的应用。

---

### 在 Taco 里的应用

Taco 的 `Lexer` 类虽然简单，但已经用到了 RAII 的思想：

```cpp
Lexer::Lexer(std::string source)
    : m_source(std::move(source))
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}
```

`m_source` 是 `std::string`，它自己管理内存。`Lexer` 不需要析构函数——当 `Lexer` 对象销毁时，`m_source` 的析构函数会自动被调用，内存自动释放。

这就是 RAII 的另一面：不只是自己写析构函数，而是尽量用已经实现好了 RAII 的类型，让它们帮你管理资源。

---

## *More About 构造函数*：委托构造与 = delete / = default

> 第一次读可以跳过。

### 委托构造

如果多个构造函数有重复的初始化逻辑，可以用委托构造（delegating constructor）：

```cpp
class Token {
public:
    // 完整构造函数
    Token(std::string type, std::string value, int line, int column)
        : m_type(std::move(type))
        , m_value(std::move(value))
        , m_line(line)
        , m_column(column)
    {}

    // 委托构造：调用完整构造函数，line 和 column 用默认值
    Token(std::string type, std::string value)
        : Token(std::move(type), std::move(value), 0, 0)
    {}

private:
    std::string m_type;
    std::string m_value;
    int m_line;
    int m_column;
};
```

委托构造避免了重复代码，初始化逻辑只写一次。

### = delete 和 = default

`= delete` 显式删除某个函数，让编译器报错而不是生成默认版本：

```cpp
class Lexer {
public:
    explicit Lexer(std::string source);

    // 禁止拷贝：Lexer 不应该被复制
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;

    // 允许移动（第十七章讲）
    Lexer(Lexer&&) = default;
    Lexer& operator=(Lexer&&) = default;
};

Lexer a("var x = 10;");
Lexer b = a;  // 错误！拷贝构造函数被删除
```

`= default` 显式要求编译器生成默认实现：

```cpp
class Token {
public:
    Token() = default;      // 生成默认构造函数
    ~Token() = default;     // 生成默认析构函数
    Token(const Token&) = default;  // 生成默认拷贝构造函数
};
```

这在需要明确表达意图时很有用：显式写 `= default` 比什么都不写更清晰——让读代码的人知道"这不是忘了写，而是故意让编译器生成"。

---

## 小结

这一章讲了对象的生命周期：

**构造函数**在对象创建时自动调用，负责初始化。`explicit` 防止意外的隐式转换，是单参数构造函数的标配。

**析构函数**在对象销毁时自动调用，负责清理资源。如果类持有需要手动释放的资源，就需要析构函数。

**初始化列表**是初始化成员变量的正确方式，比赋值更高效，而且对 `const` 成员和引用成员是唯一选择。

**RAII** 是 C++ 最重要的设计模式：用对象的生命周期管理资源。构造时获取，析构时释放。不需要手动管理，也不会忘记释放。标准库里的 `vector`、`string`、`mutex` 都是 RAII 的。

---

下一章讲拷贝与赋值——当一个对象被复制时，C++ 做了什么，以及什么时候这会变得危险。
# 第九章：拷贝与赋值

---

上一章讲了对象的诞生（构造）和消亡（析构）。这一章讲另一件事：对象的复制。

在 Python 里，赋值几乎总是创建一个引用，指向同一个对象：

```python
a = [1, 2, 3]
b = a        # b 和 a 指向同一个列表
b.append(4)
print(a)     # [1, 2, 3, 4]，a 也变了
```

C++ 里则不同。默认情况下，赋值会创建一个真正的副本。但当对象里有指针时，这个"默认"行为会带来大麻烦。

---

## 9.1 拷贝构造函数

当一个对象被用来初始化另一个同类型对象时，**拷贝构造函数**（copy constructor）会被调用：

```cpp
Token t1("NUMBER", "42");
Token t2 = t1;   // 拷贝构造：用 t1 初始化 t2
Token t3(t1);    // 同上，另一种写法

void print_token(Token t) { ... }  // 传值时也会触发拷贝构造
print_token(t1);
```

如果没有定义拷贝构造函数，编译器会自动生成一个——它会逐个拷贝所有成员变量。对于 `Token` 这样的简单类，这个默认行为完全够用：

```cpp
class Token {
public:
    std::string type;
    std::string value;
    int line;
    int column;
};

Token t1{"NUMBER", "42", 1, 5};
Token t2 = t1;
// t2.type == "NUMBER"，t2.value == "42"，t2.line == 1，t2.column == 5
// t2 是 t1 的完整副本，修改 t2 不影响 t1
```

---

## 9.2 拷贝赋值运算符

拷贝赋值运算符（copy assignment operator）在对象已经存在的情况下被赋值时调用：

```cpp
Token t1{"NUMBER", "42", 1, 5};
Token t2{"STRING", "hello", 2, 3};

t2 = t1;   // 拷贝赋值：t2 已存在，把 t1 的内容复制给 t2
```

编译器同样会自动生成一个拷贝赋值运算符，逐个拷贝成员变量。

拷贝构造函数和拷贝赋值运算符的区别：

```cpp
Token t2 = t1;  // 拷贝构造（t2 是新创建的）
t2 = t1;        // 拷贝赋值（t2 已存在）
```

---

## 9.3 Rule of Three

这里有一个重要的规则：**Rule of Three**（三法则）。

> 如果一个类需要自定义以下三个中的任何一个，那么它通常需要自定义全部三个：
> 1. 析构函数
> 2. 拷贝构造函数
> 3. 拷贝赋值运算符

为什么？因为需要自定义析构函数，通常意味着类里有需要手动管理的资源（比如裸指针）。如果有裸指针，默认的拷贝行为就会出问题——它只拷贝指针的值（地址），而不是指针指向的数据。

---

## 9.4 为什么拷贝有时候很危险

来看一个典型的例子，一个简单的字符串类（不用 `std::string`，手动管理内存）：

```cpp
class MyString {
public:
    MyString(const char* str) {
        m_length = std::strlen(str);
        m_data = new char[m_length + 1];  // 在堆上分配内存
        std::strcpy(m_data, str);
    }

    ~MyString() {
        delete[] m_data;  // 释放内存
    }

    void print() const {
        std::cout << m_data << "\n";
    }

private:
    char* m_data;
    int   m_length;
};
```

这个类有析构函数，但没有自定义拷贝构造函数和拷贝赋值运算符。会发生什么？

```cpp
MyString a("hello");
MyString b = a;   // 使用编译器生成的默认拷贝构造函数
```

默认拷贝构造函数做的事情：把 `a.m_data`（一个指针地址）复制给 `b.m_data`，把 `a.m_length` 复制给 `b.m_length`。

现在 `a.m_data` 和 `b.m_data` 指向**同一块内存**：

```
a.m_data ──┐
           ▼
         [h][e][l][l][o][\0]
           ▲
b.m_data ──┘
```

这就是**浅拷贝**（shallow copy）。问题来了：

```cpp
{
    MyString b = a;
}  // b 析构，delete[] b.m_data，释放了那块内存

a.print();  // 未定义行为！a.m_data 指向已经被释放的内存
```

`b` 析构时，`delete[] b.m_data` 释放了那块内存。但 `a.m_data` 还指向那里。之后访问 `a`，就是访问已释放的内存，程序可能崩溃，也可能输出乱码，这是一种严重的 bug。

更糟糕的是，当 `a` 自己析构时，`delete[] a.m_data` 会**再次释放同一块内存**（double free），这是未定义行为，通常会导致崩溃。

---

### 正确的做法：深拷贝

解决方法是实现**深拷贝**（deep copy）——不只复制指针，而是复制指针指向的数据：

```cpp
class MyString {
public:
    MyString(const char* str) {
        m_length = std::strlen(str);
        m_data = new char[m_length + 1];
        std::strcpy(m_data, str);
    }

    // 拷贝构造函数：深拷贝
    MyString(const MyString& other) {
        m_length = other.m_length;
        m_data = new char[m_length + 1];  // 分配新内存
        std::strcpy(m_data, other.m_data); // 复制数据
    }

    // 拷贝赋值运算符：深拷贝
    MyString& operator=(const MyString& other) {
        if (this == &other) return *this;  // 自我赋值检查

        delete[] m_data;  // 释放旧内存

        m_length = other.m_length;
        m_data = new char[m_length + 1];
        std::strcpy(m_data, other.m_data);

        return *this;
    }

    ~MyString() {
        delete[] m_data;
    }

private:
    char* m_data;
    int   m_length;
};
```

现在 `MyString b = a` 会分配新的内存，复制数据，两个对象互相独立：

```
a.m_data ──→ [h][e][l][l][o][\0]

b.m_data ──→ [h][e][l][l][o][\0]  （独立的副本）
```

注意拷贝赋值运算符里的**自我赋值检查**：`if (this == &other) return *this;`。如果写了 `a = a`，没有这个检查，`delete[] m_data` 会先释放自己的数据，然后再试图复制已经释放的数据——又是未定义行为。

---

### 现代 C++ 的建议

手动管理内存、手动实现深拷贝，这是 C++ 很容易出错的地方。现代 C++ 的建议是：**尽量不要手动管理内存**。

用 `std::string` 而不是 `char*`，用 `std::vector` 而不是裸数组，用 `std::unique_ptr` 而不是裸指针。这些标准库类都已经正确实现了深拷贝，用它们就不需要自己写拷贝构造函数和拷贝赋值运算符。

在 Taco 项目里，`Token` 的成员都是 `std::string` 和 `int`，`Lexer` 的成员是 `std::string` 和 `int`——都不涉及裸指针，所以不需要手动实现拷贝。

Rule of Three 在现代 C++ 里通常这样理解：**如果你觉得需要自定义析构函数，先想想能不能换成用标准库类来管理资源，从而完全避免手动实现拷贝**。

---

## *More About 拷贝*：深拷贝 vs 浅拷贝的底层图景

> 第一次读可以跳过。

### 浅拷贝的完整图景

浅拷贝只复制对象表面的内容：

```
对象 a：
  [m_data: 0x1234] [m_length: 5]
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存

浅拷贝后对象 b：
  [m_data: 0x1234] [m_length: 5]  ← m_data 和 a 相同
         │
         ▼（和 a 指向同一块内存）
       [h][e][l][l][o][\0]
```

浅拷贝之后：
- 修改 `b.m_data[0] = 'H'` 会同时影响 `a`
- `b` 析构后，`a.m_data` 变成悬空指针
- `a` 析构时，double free

### 深拷贝的完整图景

深拷贝复制对象的全部内容，包括指针指向的数据：

```
对象 a：
  [m_data: 0x1234] [m_length: 5]
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存 A

深拷贝后对象 b：
  [m_data: 0x5678] [m_length: 5]  ← m_data 指向不同位置
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存 B（全新分配，内容相同）
```

深拷贝之后：
- 修改 `b` 不影响 `a`
- `b` 析构时，只释放内存 B
- `a` 析构时，只释放内存 A

### std::string 怎么做的

`std::string` 内部通常包含一个指向字符数据的指针（或者对短字符串用内联存储优化）。它实现了正确的深拷贝：

```cpp
std::string a = "hello";
std::string b = a;  // 深拷贝：b 有自己独立的字符数组

b[0] = 'H';
std::cout << a;  // "hello"，a 不受影响
std::cout << b;  // "Hello"
```

这就是为什么用 `std::string` 而不是 `char*` 可以避免很多问题——`std::string` 已经帮你处理好了所有的内存管理细节。

---

## 小结

这一章讲了 C++ 的拷贝机制：

**拷贝构造函数**在用一个对象初始化另一个对象时调用，**拷贝赋值运算符**在对已存在的对象赋值时调用。

**Rule of Three**：如果需要自定义析构函数，通常也需要自定义拷贝构造函数和拷贝赋值运算符。

**浅拷贝**只复制指针值，导致两个对象共享同一块内存，会产生 double free 和悬空指针等严重 bug。**深拷贝**分配新内存，复制数据，两个对象独立。

**现代 C++ 的建议**：用标准库类（`std::string`、`std::vector`、智能指针）代替裸指针，从根本上避免手动实现深拷贝。

---

下一章讲运算符重载——让自定义类型支持 `+`、`==`、`<<` 等运算符，让代码读起来更自然。
# 第十章：运算符重载

---

运算符重载（operator overloading）让自定义类型可以使用 `+`、`==`、`<<` 这样的运算符，让代码读起来像操作内置类型一样自然。

Python 里也有运算符重载，通过 `__add__`、`__eq__`、`__str__` 等魔法方法实现。C++ 的机制不同，但思路相同。

---

## 10.1 什么是运算符重载

当写 `a + b` 时，编译器实际上在调用一个函数。对于内置类型（`int`、`double`），这个函数是语言内置的。对于自定义类型，可以定义这个函数的行为——这就是运算符重载。

```cpp
// 这两行等价
Token result = t1 + t2;
Token result = operator+(t1, t2);  // 编译器看到的实际调用
```

运算符重载有两种写法：**成员函数**和**非成员函数**。

---

## 10.2 常用运算符重载

### == 和 != ：相等比较

在 Taco 里，比较两个 Token 是否相同很常见（比如检查当前 Token 是不是某个关键字）：

```cpp
// 成员函数写法
class Token {
public:
    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }

    bool operator!=(const Token& other) const {
        return !(*this == other);  // 复用 == 的逻辑
    }
};

Token t1{TokenType::Var, "var", 1, 1};
Token t2{TokenType::Var, "var", 1, 1};
Token t3{TokenType::Identifier, "x", 1, 5};

if (t1 == t2) { ... }  // true
if (t1 != t3) { ... }  // true
```

也可以只比较类型，不比较值：

```cpp
// 更常用的写法：直接和 TokenType 比较
bool operator==(TokenType t) const {
    return type == t;
}

// 使用
if (current_token == TokenType::Var) { ... }
```

---

### << ：输出运算符

重载 `<<` 让对象可以直接用 `std::cout` 输出：

```cpp
// 非成员函数写法（推荐）
// 因为左边是 std::ostream，不是 Token，不能写成 Token 的成员函数
std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "[" << token_type_to_string(token.type) << "] "
       << "\"" << token.value << "\""
       << " (line " << token.line << ")";
    return os;  // 返回 os 支持链式调用
}

// 使用
Token t{TokenType::Number, "42", 1, 5};
std::cout << t << "\n";  // [NUMBER] "42" (line 1)

// 链式调用
std::cout << "Token: " << t << ", next: " << t2 << "\n";
```

`<<` 运算符必须返回 `std::ostream&`，才能支持链式调用（`cout << a << b << c`）。

---

### + ：加法（字符串拼接的例子）

假设要实现一个简单的值类型，支持字符串拼接：

```cpp
class TacoString {
public:
    explicit TacoString(std::string s) : m_value(std::move(s)) {}

    // 成员函数写法
    TacoString operator+(const TacoString& other) const {
        return TacoString(m_value + other.m_value);
    }

    std::string value() const { return m_value; }

private:
    std::string m_value;
};

TacoString a("Hello, ");
TacoString b("World!");
TacoString c = a + b;
std::cout << c.value() << "\n";  // Hello, World!
```

---

### [] ：下标运算符

让对象支持 `obj[index]` 的访问方式：

```cpp
class TokenStream {
public:
    // 只读版本（const）
    const Token& operator[](int index) const {
        return m_tokens[index];
    }

    // 可写版本（非 const）
    Token& operator[](int index) {
        return m_tokens[index];
    }

    int size() const { return static_cast<int>(m_tokens.size()); }

private:
    std::vector<Token> m_tokens;
};

TokenStream stream;
// ...
Token first = stream[0];  // 使用 [] 运算符
```

---

### 布尔转换：operator bool

让对象可以在 `if` 条件里使用：

```cpp
class LexerResult {
public:
    explicit operator bool() const {
        return m_success;
    }

    bool m_success;
    std::vector<Token> m_tokens;
    std::string m_error;
};

LexerResult result = run_lexer("var x = 10;");
if (result) {  // 使用 operator bool
    // 成功
} else {
    std::cerr << result.m_error << "\n";
}
```

注意 `explicit` 关键字：它防止 `LexerResult` 在不期望的地方被隐式转换成 `bool`（比如用来做算术运算）。

---

## 10.3 什么时候该重载，什么时候不该

运算符重载可以让代码更简洁，但滥用会让代码更难懂。

**应该重载的情况**：

- 运算符的语义对于这个类型来说很自然（`+` 对字符串，`==` 对值类型）
- 代码读起来更清晰，而不是更神秘
- 标准库或常见约定期望这个类型支持某个运算符（比如放入 `std::map` 需要 `<`，`std::cout` 输出需要 `<<`）

**不应该重载的情况**：

- 运算符的含义不明显（`&` 重载成"查找"，`*` 重载成"执行"）
- 只是为了让代码看起来酷，但实际上增加了理解负担
- 有更清晰的命名函数可以代替

一个常见的反例是把 `+` 重载成完全不相关的操作：

```cpp
// 糟糕的设计
Lexer operator+(const Lexer& a, const std::string& extra_source) {
    // 把 extra_source 追加到 a 的源代码？
    // 这让人完全看不懂
}
```

**Python 对比**：

Python 的魔法方法和 C++ 的运算符重载思路相同：

```python
class Token:
    def __eq__(self, other):
        return self.type == other.type and self.value == other.value

    def __repr__(self):
        return f'[{self.type}] "{self.value}"'
```

```cpp
// C++ 对应
bool operator==(const Token& other) const { ... }
std::ostream& operator<<(std::ostream& os, const Token& t) { ... }
```

`__repr__` 在 C++ 里对应 `<<` 运算符（没有完全对应的，但 `<<` 是最接近的输出接口）。

---

### Taco 项目里的运算符重载

在 Taco 解释器里，目前最有用的是：

```cpp
// Token 的相等比较：检查 Token 类型
bool Token::operator==(TokenType t) const {
    return type == t;
}

// Token 的输出：调试时打印 Token 列表
std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "[" << token_type_to_string(token.type) << "] \""
       << token.value << "\"";
    return os;
}
```

用这两个，`main.cpp` 的调试输出可以改成：

```cpp
for (const auto& token : tokens) {
    std::cout << token << "\n";  // 直接用 << 输出
}

// 检查当前 token 是不是 var 关键字
if (tokens[0] == TokenType::Var) { ... }
```

---

## 小结

**运算符重载**让自定义类型可以使用 C++ 的运算符，让代码读起来更自然。

常用的重载：`==`/`!=` 用于相等比较，`<<` 用于输出，`[]` 用于下标访问，`bool` 转换用于条件判断。

**成员函数**还是**非成员函数**的选择：如果运算符的左操作数是这个类型，用成员函数；如果左操作数是其他类型（比如 `<<` 的左边是 `ostream`），用非成员函数。

**什么时候重载**：运算符语义自然、代码更清晰时才重载。不要为了"酷"而重载，不明显的运算符只会让人困惑。

---

下一章是第二部分的项目章节：用类来重构词法分析器，并实现语法分析器和 AST（v1）。
# 第十一章：项目 v1——语法分析器与 AST

---

第二部分学完了类、构造析构、拷贝、运算符重载。现在用这些知识做一件真正有意义的事：把 Token 列表变成一棵抽象语法树（AST），并对树上的节点求值。

v1 结束时，Taco 能运行这样的代码：

```taco
var x = 10 + 20;
var y = x * 2;
var name = "Miguel";
print(x);       // 30
print(y);       // 60
print(name);    // Miguel
```

---

## 11.1 什么是抽象语法树（AST）

词法分析器把源代码变成 Token 列表。Token 列表是线性的，没有层次结构——它不知道 `10 + 20` 是一个完整的表达式，也不知道 `var x = 10 + 20` 里 `x` 是变量名、`10 + 20` 是初始值。

语法分析器（Parser）读取 Token 列表，按照语法规则把它们组织成有层次的树形结构，就是 AST。

```
源代码：var x = 10 + 20;

Token 列表（线性）：
[VAR] [x] [=] [10] [+] [20] [;]

AST（树形）：
VarDecl
├── name: "x"
└── value: BinaryExpr
    ├── left:  NumberLiteral(10)
    ├── op:    "+"
    └── right: NumberLiteral(20)
```

AST 捕捉了代码的语义结构：变量声明包含变量名和初始值，初始值是一个二元表达式，二元表达式有左操作数、运算符、右操作数。

---

## 11.2 用类表示 AST 节点

AST 节点天然适合用继承来表示：所有节点共享一个基类 `Expr`，不同种类的节点是子类。

v1 只实现基础表达式：数字、字符串、布尔值、nil、变量引用、二元运算、变量声明、函数调用（只支持 `print`）。

### ast.h

```cpp
#pragma once
#include <string>
#include <vector>
#include <memory>

// 前向声明，避免循环包含
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// ────────────────────────────────
// 基类：所有 AST 节点的父类
// ────────────────────────────────

struct Expr {
    virtual ~Expr() = default;

    // 每个节点都能把自己转成字符串（用于调试）
    virtual std::string to_string() const = 0;
};

// ────────────────────────────────
// 字面量节点
// ────────────────────────────────

struct NumberLiteral : Expr {
    double value;

    explicit NumberLiteral(double v) : value(v) {}

    std::string to_string() const override {
        // 去掉多余的小数点（42.0 显示为 42）
        if (value == static_cast<int>(value)) {
            return std::to_string(static_cast<int>(value));
        }
        return std::to_string(value);
    }
};

struct StringLiteral : Expr {
    std::string value;

    explicit StringLiteral(std::string v) : value(std::move(v)) {}

    std::string to_string() const override {
        return "\"" + value + "\"";
    }
};

struct BoolLiteral : Expr {
    bool value;

    explicit BoolLiteral(bool v) : value(v) {}

    std::string to_string() const override {
        return value ? "true" : "false";
    }
};

struct NilLiteral : Expr {
    std::string to_string() const override { return "nil"; }
};

// ────────────────────────────────
// 变量引用节点
// ────────────────────────────────

struct Identifier : Expr {
    std::string name;

    explicit Identifier(std::string n) : name(std::move(n)) {}

    std::string to_string() const override { return name; }
};

// ────────────────────────────────
// 二元表达式节点
// ────────────────────────────────

struct BinaryExpr : Expr {
    ExprPtr     left;
    std::string op;
    ExprPtr     right;

    BinaryExpr(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l))
        , op(std::move(o))
        , right(std::move(r))
    {}

    std::string to_string() const override {
        return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
    }
};

// ────────────────────────────────
// 一元表达式节点（! 和 -）
// ────────────────────────────────

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr     operand;

    UnaryExpr(std::string o, ExprPtr expr)
        : op(std::move(o))
        , operand(std::move(expr))
    {}

    std::string to_string() const override {
        return "(" + op + operand->to_string() + ")";
    }
};

// ────────────────────────────────
// 语句节点（有副作用，不返回值）
// ────────────────────────────────

struct VarDecl : Expr {
    std::string name;
    ExprPtr     value;

    VarDecl(std::string n, ExprPtr v)
        : name(std::move(n))
        , value(std::move(v))
    {}

    std::string to_string() const override {
        return "var " + name + " = " + value->to_string();
    }
};

struct PrintStmt : Expr {
    ExprPtr argument;

    explicit PrintStmt(ExprPtr arg) : argument(std::move(arg)) {}

    std::string to_string() const override {
        return "print(" + argument->to_string() + ")";
    }
};

// 整个程序：一组语句
struct Program : Expr {
    std::vector<ExprPtr> statements;

    std::string to_string() const override {
        std::string result;
        for (const auto& stmt : statements) {
            result += stmt->to_string() + "\n";
        }
        return result;
    }
};
```

这里用了 `std::unique_ptr<Expr>`（别名 `ExprPtr`）来管理 AST 节点的所有权。第十六、十七章会详细讲智能指针，现在只需要知道：`unique_ptr` 会在对象销毁时自动释放内存，不需要手动 `delete`。

---

## 11.3 实现递归下降解析器

语法分析器用**递归下降**（recursive descent）方法：对每种语法结构写一个函数，函数之间互相调用，自然形成递归。

这是最直观的解析器实现方式，也是大多数手写解析器（包括 V8、Clang 等真实编译器）采用的方式。

### parser.h

```cpp
#pragma once
#include "token.h"
#include "ast.h"
#include <vector>
#include <stdexcept>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // 核心接口：解析整个程序
    std::unique_ptr<Program> parse();

private:
    std::vector<Token> m_tokens;
    int m_pos;  // 当前读取位置

    // ── 辅助函数 ──
    const Token& current() const;
    const Token& peek(int offset = 1) const;
    const Token& advance();
    bool is_at_end() const;

    // 如果当前 Token 类型匹配，消费并返回 true
    bool match(TokenType type);

    // 期望当前 Token 是某种类型，不是就报错
    const Token& expect(TokenType type, const std::string& message);

    // 报错
    [[noreturn]] void error(const std::string& message);

    // ── 解析函数（从上到下，优先级从低到高）──
    ExprPtr parse_statement();
    ExprPtr parse_var_decl();
    ExprPtr parse_print_stmt();
    ExprPtr parse_expression();
    ExprPtr parse_comparison();
    ExprPtr parse_addition();
    ExprPtr parse_multiplication();
    ExprPtr parse_unary();
    ExprPtr parse_primary();
};
```

### parser.cpp

```cpp
#include "parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))
    , m_pos(0)
{}

// ────────────────────────────────
// 辅助函数
// ────────────────────────────────

const Token& Parser::current() const {
    return m_tokens[m_pos];
}

const Token& Parser::peek(int offset) const {
    int idx = m_pos + offset;
    if (idx >= static_cast<int>(m_tokens.size())) {
        return m_tokens.back();  // 返回 EOF Token
    }
    return m_tokens[idx];
}

const Token& Parser::advance() {
    const Token& t = m_tokens[m_pos];
    if (!is_at_end()) m_pos++;
    return t;
}

bool Parser::is_at_end() const {
    return current().type == TokenType::EndOfFile;
}

bool Parser::match(TokenType type) {
    if (current().type == type) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& message) {
    if (current().type != type) {
        error(message);
    }
    return advance();
}

void Parser::error(const std::string& message) {
    throw std::runtime_error(
        "🌮 line " + std::to_string(current().line) + ": " + message
    );
}

// ────────────────────────────────
// 解析整个程序
// ────────────────────────────────

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();

    while (!is_at_end()) {
        program->statements.push_back(parse_statement());
    }

    return program;
}

// ────────────────────────────────
// 语句
// ────────────────────────────────

ExprPtr Parser::parse_statement() {
    if (current().type == TokenType::Var) {
        return parse_var_decl();
    }

    // print(...) 是内置函数，单独处理
    if (current().type == TokenType::Identifier &&
        current().value == "print") {
        return parse_print_stmt();
    }

    // 其他情况：把表达式当语句
    auto expr = parse_expression();
    expect(TokenType::Semicolon, "Expected ';' after expression.");
    return expr;
}

ExprPtr Parser::parse_var_decl() {
    expect(TokenType::Var, "Expected 'var'.");

    const Token& name_token = expect(
        TokenType::Identifier,
        "Expected variable name after 'var'."
    );
    std::string name = name_token.value;

    expect(TokenType::Assign, "Expected '=' after variable name.");

    ExprPtr value = parse_expression();

    expect(TokenType::Semicolon, "Expected ';' after variable declaration.");

    return std::make_unique<VarDecl>(name, std::move(value));
}

ExprPtr Parser::parse_print_stmt() {
    advance();  // 消费 "print"

    expect(TokenType::LeftParen, "Expected '(' after 'print'.");
    ExprPtr arg = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after print argument.");
    expect(TokenType::Semicolon, "Expected ';' after print statement.");

    return std::make_unique<PrintStmt>(std::move(arg));
}

// ────────────────────────────────
// 表达式（优先级从低到高）
// ────────────────────────────────

// expression → comparison
ExprPtr Parser::parse_expression() {
    return parse_comparison();
}

// comparison → addition (("==" | "!=" | "<" | ">" | "<=" | ">=") addition)*
ExprPtr Parser::parse_comparison() {
    ExprPtr left = parse_addition();

    while (current().type == TokenType::Equal      ||
           current().type == TokenType::NotEqual   ||
           current().type == TokenType::Less       ||
           current().type == TokenType::Greater    ||
           current().type == TokenType::LessEqual  ||
           current().type == TokenType::GreaterEqual) {
        std::string op = advance().value;
        ExprPtr right = parse_addition();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// addition → multiplication (("+" | "-") multiplication)*
ExprPtr Parser::parse_addition() {
    ExprPtr left = parse_multiplication();

    while (current().type == TokenType::Plus ||
           current().type == TokenType::Minus) {
        std::string op = advance().value;
        ExprPtr right = parse_multiplication();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// multiplication → unary (("*" | "/" | "%" | "^") unary)*
ExprPtr Parser::parse_multiplication() {
    ExprPtr left = parse_unary();

    while (current().type == TokenType::Star    ||
           current().type == TokenType::Slash   ||
           current().type == TokenType::Percent ||
           current().type == TokenType::Caret) {
        std::string op = advance().value;
        ExprPtr right = parse_unary();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// unary → ("!" | "-") unary | primary
ExprPtr Parser::parse_unary() {
    if (current().type == TokenType::Not ||
        current().type == TokenType::Minus) {
        std::string op = advance().value;
        ExprPtr operand = parse_unary();
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    }

    return parse_primary();
}

// primary → NUMBER | STRING | "true" | "false" | "nil" | IDENTIFIER | "(" expression ")"
ExprPtr Parser::parse_primary() {
    const Token& token = current();

    if (token.type == TokenType::Number) {
        advance();
        return std::make_unique<NumberLiteral>(std::stod(token.value));
    }

    if (token.type == TokenType::String) {
        advance();
        return std::make_unique<StringLiteral>(token.value);
    }

    if (token.type == TokenType::True) {
        advance();
        return std::make_unique<BoolLiteral>(true);
    }

    if (token.type == TokenType::False) {
        advance();
        return std::make_unique<BoolLiteral>(false);
    }

    if (token.type == TokenType::Nil) {
        advance();
        return std::make_unique<NilLiteral>();
    }

    if (token.type == TokenType::Identifier) {
        advance();
        return std::make_unique<Identifier>(token.value);
    }

    // 括号表达式
    if (token.type == TokenType::LeftParen) {
        advance();  // 消费 "("
        ExprPtr expr = parse_expression();
        expect(TokenType::RightParen, "Expected ')' after expression.");
        return expr;
    }

    error("Unexpected token: '" + token.value + "'.");
}
```

---

### 为什么这样分层？

`parse_addition` 调用 `parse_multiplication`，`parse_multiplication` 调用 `parse_unary`，`parse_unary` 调用 `parse_primary`。这个调用层级对应的是**运算符优先级**：

```
优先级（从低到高）：
  == != < > <= >=   （比较）
  + -               （加减）
  * / % ^           （乘除）
  ! -               （一元）
  字面量、括号       （基本）
```

优先级高的运算符在调用链的更深处处理，所以它们先被"绑定"。这就是为什么 `2 + 3 * 4` 会解析成 `2 + (3 * 4)` 而不是 `(2 + 3) * 4`。

---

## 11.4 实现求值器

有了 AST，现在实现求值器——遍历 AST，对每个节点求值。

v1 的求值器很简单：没有变量作用域，用一个全局 `map` 存所有变量。

### evaluator.h

```cpp
#pragma once
#include "ast.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <iostream>

// Taco 的值类型：数字、字符串、布尔、nil
using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;

// 把 TacoValue 转成字符串（用于 print）
std::string value_to_string(const TacoValue& val);

class Evaluator {
public:
    // 求值入口
    TacoValue evaluate(const Expr* expr);

private:
    // 变量环境：变量名 → 值
    std::unordered_map<std::string, TacoValue> m_env;

    // 各类节点的求值
    TacoValue eval_number(const NumberLiteral* node);
    TacoValue eval_string(const StringLiteral* node);
    TacoValue eval_bool(const BoolLiteral* node);
    TacoValue eval_nil(const NilLiteral* node);
    TacoValue eval_identifier(const Identifier* node);
    TacoValue eval_binary(const BinaryExpr* node);
    TacoValue eval_unary(const UnaryExpr* node);
    TacoValue eval_var_decl(const VarDecl* node);
    TacoValue eval_print(const PrintStmt* node);
    TacoValue eval_program(const Program* node);
};
```

### evaluator.cpp

```cpp
#include "evaluator.h"
#include <stdexcept>
#include <cmath>

// ────────────────────────────────
// TacoValue 工具函数
// ────────────────────────────────

std::string value_to_string(const TacoValue& val) {
    if (std::holds_alternative<double>(val)) {
        double d = std::get<double>(val);
        if (d == static_cast<int>(d)) {
            return std::to_string(static_cast<int>(d));
        }
        return std::to_string(d);
    }
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    }
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? "true" : "false";
    }
    return "nil";
}

static bool is_truthy(const TacoValue& val) {
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val);
    }
    if (std::holds_alternative<std::nullptr_t>(val)) {
        return false;  // nil 是 falsy
    }
    return true;  // 其他值都是 truthy
}

// ────────────────────────────────
// 求值入口：根据节点类型分发
// ────────────────────────────────

TacoValue Evaluator::evaluate(const Expr* expr) {
    // 用 dynamic_cast 判断节点类型（第十四章会详细讲）
    if (auto* node = dynamic_cast<const NumberLiteral*>(expr))
        return eval_number(node);
    if (auto* node = dynamic_cast<const StringLiteral*>(expr))
        return eval_string(node);
    if (auto* node = dynamic_cast<const BoolLiteral*>(expr))
        return eval_bool(node);
    if (auto* node = dynamic_cast<const NilLiteral*>(expr))
        return eval_nil(node);
    if (auto* node = dynamic_cast<const Identifier*>(expr))
        return eval_identifier(node);
    if (auto* node = dynamic_cast<const BinaryExpr*>(expr))
        return eval_binary(node);
    if (auto* node = dynamic_cast<const UnaryExpr*>(expr))
        return eval_unary(node);
    if (auto* node = dynamic_cast<const VarDecl*>(expr))
        return eval_var_decl(node);
    if (auto* node = dynamic_cast<const PrintStmt*>(expr))
        return eval_print(node);
    if (auto* node = dynamic_cast<const Program*>(expr))
        return eval_program(node);

    throw std::runtime_error("🌮 Unknown AST node type.");
}

// ────────────────────────────────
// 字面量求值（直接返回值）
// ────────────────────────────────

TacoValue Evaluator::eval_number(const NumberLiteral* node) {
    return node->value;
}

TacoValue Evaluator::eval_string(const StringLiteral* node) {
    return node->value;
}

TacoValue Evaluator::eval_bool(const BoolLiteral* node) {
    return node->value;
}

TacoValue Evaluator::eval_nil(const NilLiteral* node) {
    return nullptr;
}

// ────────────────────────────────
// 变量引用：从环境里查找
// ────────────────────────────────

TacoValue Evaluator::eval_identifier(const Identifier* node) {
    auto it = m_env.find(node->name);
    if (it == m_env.end()) {
        throw std::runtime_error(
            "🌮 '" + node->name + "' is not defined."
        );
    }
    return it->second;
}

// ────────────────────────────────
// 二元表达式
// ────────────────────────────────

TacoValue Evaluator::eval_binary(const BinaryExpr* node) {
    TacoValue left  = evaluate(node->left.get());
    TacoValue right = evaluate(node->right.get());
    const std::string& op = node->op;

    // 算术运算（要求两边都是数字）
    if (op == "+" || op == "-" || op == "*" ||
        op == "/" || op == "%" || op == "^") {

        // 字符串拼接：+ 两边都是字符串时
        if (op == "+" &&
            std::holds_alternative<std::string>(left) &&
            std::holds_alternative<std::string>(right)) {
            return std::get<std::string>(left) + std::get<std::string>(right);
        }

        if (!std::holds_alternative<double>(left) ||
            !std::holds_alternative<double>(right)) {
            throw std::runtime_error(
                "🌮 Operator '" + op + "' requires numbers."
            );
        }

        double l = std::get<double>(left);
        double r = std::get<double>(right);

        if (op == "+") return l + r;
        if (op == "-") return l - r;
        if (op == "*") return l * r;
        if (op == "/") {
            if (r == 0) throw std::runtime_error("🌮 Division by zero. Nobody can.");
            return l / r;
        }
        if (op == "%") return std::fmod(l, r);
        if (op == "^") return std::pow(l, r);
    }

    // 比较运算
    if (op == "==" || op == "!=") {
        bool equal = (left == right);
        return op == "==" ? equal : !equal;
    }

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!std::holds_alternative<double>(left) ||
            !std::holds_alternative<double>(right)) {
            throw std::runtime_error(
                "🌮 Operator '" + op + "' requires numbers."
            );
        }
        double l = std::get<double>(left);
        double r = std::get<double>(right);
        if (op == "<")  return l < r;
        if (op == ">")  return l > r;
        if (op == "<=") return l <= r;
        if (op == ">=") return l >= r;
    }

    // 逻辑运算
    if (op == "&&") return is_truthy(left) && is_truthy(right);
    if (op == "||") return is_truthy(left) || is_truthy(right);

    throw std::runtime_error("🌮 Unknown operator: '" + op + "'.");
}

// ────────────────────────────────
// 一元表达式
// ────────────────────────────────

TacoValue Evaluator::eval_unary(const UnaryExpr* node) {
    TacoValue val = evaluate(node->operand.get());

    if (node->op == "!") {
        return !is_truthy(val);
    }
    if (node->op == "-") {
        if (!std::holds_alternative<double>(val)) {
            throw std::runtime_error("🌮 Unary '-' requires a number.");
        }
        return -std::get<double>(val);
    }

    throw std::runtime_error("🌮 Unknown unary operator: '" + node->op + "'.");
}

// ────────────────────────────────
// 变量声明：求值后存入环境
// ────────────────────────────────

TacoValue Evaluator::eval_var_decl(const VarDecl* node) {
    TacoValue val = evaluate(node->value.get());
    m_env[node->name] = val;
    return val;
}

// ────────────────────────────────
// print 语句
// ────────────────────────────────

TacoValue Evaluator::eval_print(const PrintStmt* node) {
    TacoValue val = evaluate(node->argument.get());
    std::cout << value_to_string(val) << "\n";
    return nullptr;  // print 不返回值
}

// ────────────────────────────────
// 程序：依次求值所有语句
// ────────────────────────────────

TacoValue Evaluator::eval_program(const Program* node) {
    TacoValue last = nullptr;
    for (const auto& stmt : node->statements) {
        last = evaluate(stmt.get());
    }
    return last;
}
```

---

## 11.5 测试：把 `var x = 10 + 20;` 解析成 AST 并运行

### main.cpp

```cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"

// 读取文件内容
std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("🌮 Cannot open file: " + path);
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

void run(const std::string& source) {
    // 词法分析
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    // 语法分析
    Parser parser(std::move(tokens));
    auto program = parser.parse();

    // 求值
    Evaluator evaluator;
    evaluator.evaluate(program.get());
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // 没有参数：简单的 REPL（v6 会做成完整版）
        std::cout << "🌮 Taco 0.1.0\n";
        std::cout << "   It works on my machine.\n";

        std::string line;
        while (true) {
            std::cout << "> ";
            if (!std::getline(std::cin, line)) break;
            if (line == "exit") {
                std::cout << "🌮 Fine.\n";
                break;
            }
            try {
                run(line);
            } catch (const std::exception& e) {
                std::cerr << e.what() << "\n";
            }
        }
    } else {
        // 有参数：运行文件
        try {
            std::string source = read_file(argv[1]);
            run(source);
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
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
    src/parser.cpp
    src/evaluator.cpp
)

target_include_directories(taco PRIVATE src)
```

### 测试文件 test.taco

```taco
var x = 10 + 20;
var y = x * 2;
var name = "Miguel";
var greeting = "Hola, " + name + "!";
var flag = true;

print(x);
print(y);
print(name);
print(greeting);
print(flag);
print(1 + 2 * 3);
print((1 + 2) * 3);
```

### 运行

```bash
cmake -B build
cmake --build build
./build/taco test.taco
```

### 输出

```
30
60
Miguel
Hola, Miguel!
true
7
9
```

也可以启动简单的 REPL：

```bash
./build/taco
🌮 Taco 0.1.0
   It works on my machine.
> var x = 10 + 20;
> print(x);
30
> print("Hello, " + "World!");
Hello, World!
> exit
🌮 Fine.
```

---

## 11.6 这个版本的局限性

v1 能运行基础的 Taco 程序，但有几个明显的不足：

**没有控制流**

```taco
if (x > 10) { print("big"); }  // 不支持
while (x > 0) { x = x - 1; }  // 不支持
```

控制流需要更多的 AST 节点类型，v2 会加。

**变量没有作用域**

所有变量都在同一个全局环境里。如果有函数，函数里的变量应该和外面的变量隔离，但现在做不到。

**`dynamic_cast` 的局限**

现在用 `dynamic_cast` 来判断节点类型，这是可以工作的，但不够优雅。第十三章学完多态之后，v2 会改用虚函数的方式——给每个节点加一个 `evaluate()` 虚函数，让每个节点知道如何求值自己。

**只有 print 内置函数**

其他内置函数（`input`、`type` 等）还没有实现，v4 会系统地加入标准库。

---

## 小结

v1 实现了解释器的第二层和第三层：语法分析器和求值器。

**AST** 用继承体系表示：`Expr` 是基类，各种节点是子类。`unique_ptr` 管理节点的生命周期，不需要手动释放内存。

**递归下降解析器**对每种语法结构写一个函数，函数层级对应运算符优先级。这是最直观、最容易理解的解析器实现方式。

**求值器**遍历 AST，对每个节点求值。`std::variant` 表示 Taco 的值类型，可以是数字、字符串、布尔或 nil。

**`std::variant`** 是 C++17 的特性，表示"可以是这几种类型之一"的值。第十九章会详细介绍，现在只需要知道用法。

---

第二部分到这里结束。第三部分进入继承和多态，学完之后 v2 会用虚函数重构求值器，并加入控制流。
