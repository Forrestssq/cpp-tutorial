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

我们首先需要一个容器来储存 `TokenType` 。用什么来储存类似于 `Number`、 `String`、 `True` 这样的数据呢？

我们很容易想到用 `enum` 枚举类型。

但普通 `enum` 有几个问题：

```cpp
// 普通 enum：会隐式转换成 int，不同枚举的值可能冲突
enum Color { Red, Green, Blue };
enum Direction { North, South, East, West };

int c = Red;   // 可以，但这通常不是你想要的
if (Red == North) { ... }  // 比较两个不相关的枚举，编译器不报错！
```

另一个隐患是枚举成员的名字会污染外层作用域。比如同时定义两个枚举，如果都有一个叫Red的成员，会直接撞名报错：

```cpp
enum Color { Red, Green, Blue };
enum TrafficLight { Red, Yellow, Green };  // 错误！Red 和 Green 重复定义
```

为了解决这些问题，在 `C++11` 中引入了一个强类型枚举类型 `enum class` 。

> `enum class` 既不是 `enum` 也不是 `class`，而是借用了一下名字（为了不新建一个关键字），只是它的语法跟 `enum` 大致一样而已。

#### enum class 和 enum 的语法差异

第一，声明语法多了一个关键字：

```cpp
enum Color { Red, Green, Blue };          // enum
enum class Color { Red, Green, Blue };    // 多了 class
enum struct Color { Red, Green, Blue };   // 或者多了 struct，效果完全等价
```

第二，访问成员的写法不一样：

```cpp
enum Color { Red, Green, Blue };
Color c = Red;              // 直接用 Red，不用加前缀

enum class Direction { North, South };
Direction d = North;              // 错误！必须带前缀
Direction d = Direction::North;   // 正确，必须加 Direction::
```

老式 `enum` 的成员名直接暴露在外层作用域；`enum class` 的成员名被限定在类型名内部，访问时强制要求加 `类型名::` 前缀，因此不同枚举之间的成员不会撞名。

第三，隐式类型转换的规则不一样：

```cpp
enum Color { Red, Green, Blue };
int x = Red;          // 合法，隐式转 int

enum class Direction { North, South };
int y = North;                                 // 错误！不允许隐式转换
int y = static_cast<int>(Direction::North);    // 必须显式转换才行
```

> `Tips`: **static_cast**
>
> 它是C++提供的几种"类型转换"操作符之一，专门用来做显式的、编译期检查的类型转换。语法长这样：

```cpp
 static_cast<目标类型>(要转换的东西)
```

> static_cast不需要加std::，因为它根本不是std命名空间里的一个函数或对象，而是C++语言本身内置的一个关键字，属于语法层面的东西，就跟if、for、return一样，是语言自带的，不需要任何命名空间前缀。


第四，前向声明的能力不一样：

```cpp
enum Color;              // 错误！老式 enum（未指定底层类型时）不能前向声明
enum class Color;        // 正确！enum class 可以直接前向声明
```

两者相同的一点是，指定底层类型的语法完全一样：

```cpp
enum Color : char { Red, Green, Blue };          // 老式 enum 也支持
enum class Direction : char { North, South };    // enum class 也支持
```

结论是：`enum class` 借用了 `enum` 的基本骨架（关键字、花括号列举成员这套写法），但在成员访问、隐式转换、前向声明三处做了更严格的规则限制，并不是"语法完全一样"，而是"长得像但规则更严格"。

> `Tips`: **前向声明（Forward Declaration）**
> 
> 前向声明指的是：先告诉编译器"这个东西存在，它叫这个名字"，但暂时不给出完整定义，把细节留到后面再补。
> 
> 函数前向声明的例子：

```cpp
// 前向声明：只说"这个函数存在，长这样"
int add(int a, int b);

int main() {
    int result = add(3, 5);   // 编译器已经知道 add 长什么样，可以放心调用
    return 0;
}

// 真正的实现，放在后面
int add(int a, int b) {
    return a + b;
}
```

> 如果没有前面那行声明，编译器在 `main` 里看到 `add(3, 5)` 时还不认识这个函数（因为定义在后面），会报错找不到。前向声明相当于提前给编译器打个招呼。

`enum class` 的前向声明：

```cpp
enum class Color;        // 前向声明：只说"有一个叫 Color 的枚举类型"

// ... 中间可能隔着很多代码，甚至在另一个文件里 ...

enum class Color { Red, Green, Blue };  // 真正的完整定义在这里给出
```

> 这在大型项目里很有用，比如 `token.h` 和 `ast.h` 这类文件经常需要互相引用对方的类型，如果两个头文件互相 `#include`，容易陷入循环包含的死结。这时可以先做前向声明，告诉编译器"这个类型存在，我先用着"，等真正需要访问具体成员时，再正式包含对应的头文件。
> 
> 为什么 enum class 能前向声明而 enum 不行？老式 `enum` 在没有指定底层类型时，编译器需要先看到全部枚举成员，才能推算出用多大的内存存储这个枚举；而 `enum class` 默认底层类型固定为 `int`，不需要靠成员推算，所以可以先声明、后定义。

### token.h

在header里面我们需要：
1. 用 `enum class` 定义一个 `TokenType` 类型；
2. 用 `struct` 定义一个 `Token` 结构体，用来放：原始的值 + 对应的 `TokenType` +  `Token` 所在的行与列的编号，方便报错；
3. 另外需要一个函数，接受一个 `TokenType` ，返回它被转换成的 `string` 名，方便查看、打印。

那么开始吧。

所需的代码是仓库的 `codeSources/v0-token.h`。  

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

用 `enum class` 写 `TokenType` ，编译器会帮我们检查：如果不小心把 `TokenType::Number` 当 `int` 用，会直接报错（因为 `enum TokenType` 或 `enum class TokenType` 声明完之后，就产生了 `TokenType` 这个类型名，地位与 `int` 等同），而不是悄悄产生 bug。

---

### token.cpp

然后写这个函数的实现。

> `Tips` `switch` 语法。
> `C++` 中的 `switch` 语法和 `C` 中的一样，都是：

```cpp
switch (表达式) {
  case 值1:
    语句;
    break;
  case 值2:
    语句;
    return 值;  // 用 return 时可以不写 break
  default:
    语句;
    break;
}
```

> 注意每个 `case` 后面要 `break;` ，否则会和 `C` 一样穿透。

> 该文件在 `codeSources/v0-token.cpp`

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

## 6.3 还差最后一步：怎么把 Token 扫描出来

到这里，`Token` 和 `TokenType` 都已经就绪，但词法分析器还缺最后一块拼图：一个能读入源代码字符串、逐字符扫描、把它切成 Token 列表的东西——`Lexer`。

`Lexer` 不能再用 `struct` 写了。它需要维护内部状态（读到第几个字符、第几行第几列），还需要一堆只在内部使用的辅助函数，又不想把这些细节暴露给外部代码。这种"数据 + 行为绑在一起，并且要隐藏内部细节"的需求，单靠 `struct` 和 `enum` 是不够的，需要 C++ 真正的面向对象工具——`class`。

下一章先系统讲 `class` 的基础语法，讲完之后会立刻回到这里，把 `Lexer` 实现出来，完成 v0 的最后一步。

---

## 小结

这一章定义了 Taco 词法分析的基本数据结构。

**Token** 是词法分析的输出单元，包含类型和原始文本，用 `struct` 定义——它只是一捆数据，不需要复杂行为，`struct` 足够。

**TokenType** 用 `enum class` 定义。比起普通 `enum`，`enum class` 不会隐式转换成 `int`，不同枚举值之间也不会互相误比较，更安全。

接下来要做的，是把这些 Token 真正"扫描"出来——这需要学完类的基础，下一章见。
