# 第五章：解释器是什么

---

前四章讲的都是工具——C++ 的语法、构建系统、字符串处理。从这一章开始，进入本书的核心：**构建 Taco 解释器**。

在写代码之前，先把"解释器是什么"、"Taco 长什么样"、"这本书要做什么"这三件事讲清楚。

---

## 5.1 语言、编译器、解释器的关系

### 什么是编程语言

编程语言是一种形式化的符号系统，让人类能够以一种精确的方式描述计算过程。

"形式化"意味着语言有严格的规则——哪些写法是合法的，哪些不是；合法的写法代表什么含义。自然语言（中文、英文）有歧义，而编程语言不允许歧义。

---

### 编译器 vs 解释器

一段程序代码，要想让计算机执行，需要把它翻译成计算机能理解的机器码。翻译的方式主要有两种：

**编译器**（Compiler）：把整个源文件翻译成机器码，生成一个独立的可执行文件。翻译发生在程序运行之前。

```
源代码 → 编译器 → 可执行文件 → 运行
```

C 和 C++ 就是编译型语言。`g++ main.cpp -o main` 这个命令完成的就是编译，生成的 `main` 文件可以直接运行，不再需要 C++ 环境。

**解释器**（Interpreter）：直接读取源代码，边理解边执行，不生成独立的可执行文件。执行和翻译同时发生。

```
源代码 → 解释器 → 执行
```

Python 就是典型的解释型语言。运行 `python script.py` 时，Python 解释器读取 `script.py`，理解每一行的含义，然后执行对应的操作。

**混合方式**：很多现代语言介于两者之间。Python 实际上会先把源代码编译成字节码（`.pyc` 文件），再由虚拟机解释执行字节码。Java 也类似——先编译成字节码，再由 JVM 执行。这种方式结合了两者的优点：比纯解释快，比编译到机器码更便于跨平台。

Taco 使用**纯解释**方式：直接读取源代码，构建语法树，然后执行语法树。这是最简单的实现方式，适合这本书的教学目标。如果以后想让 Taco 更快，可以加入字节码编译——那是进阶的话题。

---

### 解释器内部发生了什么

解释器不是黑盒，它的内部有清晰的结构。以执行这行 Taco 代码为例：

```taco
var x = 10 + 20;
```

解释器内部经历以下步骤：

**第一步：词法分析（Lexical Analysis）**

把源代码字符串切成一个个有意义的单元，叫做 **Token**（词法单元）：

```
"var x = 10 + 20;" 
→ [var] [x] [=] [10] [+] [20] [;]
```

每个 Token 有类型（关键字、标识符、数字、运算符、分号……）和值。负责这个工作的模块叫**词法分析器**（Lexer）或**扫描器**（Scanner）。

**第二步：语法分析（Parsing）**

把 Token 列表组织成有层次结构的**抽象语法树**（Abstract Syntax Tree，AST）：

```
VarDeclaration
├── name: "x"
└── value: BinaryExpression
    ├── left: NumberLiteral(10)
    ├── operator: "+"
    └── right: NumberLiteral(20)
```

AST 反映的是代码的语法结构。负责这个工作的模块叫**语法分析器**（Parser）。

**第三步：求值（Evaluation）**

遍历 AST，执行每个节点代表的操作：

```
执行 VarDeclaration：
  求值 BinaryExpression：
    求值 NumberLiteral(10) → 10
    求值 NumberLiteral(20) → 20
    计算 10 + 20 → 30
  把 30 存储到变量 x
```

负责这个工作的模块叫**求值器**（Evaluator）或**解释器**（Interpreter）——虽然有时候整个系统也叫解释器。

---

整个流程：

```
源代码字符串
    ↓ 词法分析器（Lexer）
Token 列表
    ↓ 语法分析器（Parser）
抽象语法树（AST）
    ↓ 求值器（Evaluator）
结果
```

这三层结构是绝大多数解释器和编译器的基础架构。理解了这个架构，就理解了接下来每一章在做什么。

---

## 5.2 Taco 语言设计：我们要实现什么

在开始写解释器之前，需要先定义 Taco 是什么样的语言。

### Taco 的定位

Taco 是一门**动态类型的脚本语言**，定位是：

> 住在终端里的语言。不用 import，不用项目，打开就写，写完就跑。语法是人话，数据处理像写英语，系统操作像呼吸一样自然。

Taco 的竞争对手是 bash 脚本和 Python 一次性脚本：
- 比 bash 可读性强，语法是真正的编程语言
- 比 Python 更轻量，shell 操作开箱即用，不需要 `import os`

---

### Taco 的语法

**变量声明**：

```taco
var x = 10;
var name = "Miguel";
var flag = true;
var nothing = nil;
```

**基本数据类型**：
- `number`：整数和浮点数统一（`10`、`3.14`）
- `string`：字符串（`"hello"`）
- `bool`：布尔值（`true`、`false`）
- `nil`：空值

**运算符**：

```taco
// 算术
x + y;  x - y;  x * y;  x / y;  x % y;  x ^ y;

// 比较
x == y;  x != y;  x > y;  x < y;  x >= y;  x <= y;

// 逻辑
x && y;  x || y;  !x;

// 字符串拼接
"Hello, " + name + "!";

// 字符串插值
"Hello, {name}!";

// 三元
x > 0 ? "positive" : "negative";
```

**控制流**：

```taco
// if/elseif/else
if (x > 90) {
    print("A");
} elseif (x > 80) {
    print("B");
} else {
    print("C");
}

// while
while (x > 0) {
    x = x - 1;
}

// C 风格 for
for (var i = 0; i < 10; i++) {
    print(i);
}

// range 风格
for i in range(0, 10) {
    print(i);
}

// switch（默认不穿透）
switch x {
    case 1 { print("one"); }
    case 2 { print("two"); }
    default { print("other"); }
}
```

**函数**：

```taco
func greet(name, greeting: "Hola") {
    print("{greeting}, {name}!");
}

greet("Miguel");              // Hola, Miguel!
greet("Dante", greeting: "Hey");  // Hey, Dante!

// 闭包
var double = { x in x * 2 };

// 多返回值
func minmax(arr) {
    return arr.getFirst(), arr.getLast();
}
var min, max = minmax([3, 1, 4, 1, 5]);
```

**数据结构**：

```taco
// array（从 0 开始）
var arr = [1, 2, 3, 4, 5];
var first = arr[0];
var merged = [...arr, 6, 7];

// map
var person = {"name": "Miguel", "age": 12};
var name = person["name"];
var name2 = person.name;  // 点语法

// pipeline
arr
    .filter { x in x % 2 == 0 }
    .map { x in x * 2 }
    .each { x in print(x) };
```

**OOP**：

```taco
// class：引用类型，支持继承
class Animal {
    var name;
    func init(name) { self.name = name; }
    func speak() { print("{self.name} makes a sound"); }
}

class Dog extends Animal {
    func speak() {
        super.speak();
        print("{self.name} also barks");
    }
}

// struct：值类型，不支持继承，自动 init
struct Point {
    var x;
    var y;
    func distance() { return (self.x ^ 2 + self.y ^ 2) ^ 0.5; }
}

// enum
enum Direction { North, South, East, West }
var d = Direction.North;
```

**并发**：

```taco
var ch = channel();
var t = thread {
    ch.send(42);
};
var val = ch.receive();
t.join();
```

**模块**：

```taco
import utils;
from io import readFile;
import taco.net;
```

**彩蛋**：

```taco
🌮 "Will my code compile?";  // Magic 8 Ball
🌮🌮 "Same question";        // 魔法海螺
🌮🌮🌮;                      // 随机笑话
🌮🌮🌮🌮;                    // Unix 时间戳
```

---

### 我们会实现哪些部分

Taco 的完整特性集很丰富，但这本书的重点是**学 C++**，不是构建一个完整的生产级语言。所以 Taco 的实现是**渐进的**：

| 版本 | 实现的特性 | 对应章节 |
|------|------------|----------|
| v0 | 词法分析器，能识别 Token | 第六章 |
| v1 | 语法分析器，能构建 AST，能求值基本表达式 | 第十一章 |
| v2 | 控制流：if/while/for/switch | 第十五章 |
| v3 | 函数、闭包、作用域 | 第二十章 |
| v4 | array、map、enum、内置标准库 | 第二十五章 |
| v5 | 解释器内部模板化，值类型用 variant | 第二十九章 |
| v6 | REPL、多线程 | 第三十三章 |
| v7 | 网络支持，fetch() | 第三十六章 |

OOP（class、struct）、模块系统、VSCode 插件等高级特性放在综合收尾部分。

每次项目进化，都有具体的动机：学到新的 C++ 特性，发现旧版本的不足，用新特性来改进。

---

## 5.3 解释器的三层架构：词法、语法、求值

现在把架构具体化，对应到 Taco 项目的代码结构。

### 项目结构

```
taco/
  CMakeLists.txt
  src/
    main.cpp          # 程序入口，解析命令行参数
    token.h           # Token 的定义
    token.cpp         # Token 相关工具函数
    lexer.h           # 词法分析器接口
    lexer.cpp         # 词法分析器实现
    ast.h             # AST 节点定义
    parser.h          # 语法分析器接口
    parser.cpp        # 语法分析器实现
    evaluator.h       # 求值器接口
    evaluator.cpp     # 求值器实现
    environment.h     # 变量环境（作用域）
    environment.cpp
    value.h           # Taco 值类型
    value.cpp
    error.h           # 错误处理
    error.cpp
    builtin.h         # 内置函数
    builtin.cpp
  tests/
    test_lexer.cpp
    test_parser.cpp
    test_evaluator.cpp
```

---

### 第一层：Token

> Tips: 这个章节里面的所有内容都只是展示，项目还没有开始构建哦。

Token 是词法分析的输出，语法分析的输入。一个 Token 包含两件事：

- **类型**（TokenType）：这个词是什么——关键字、数字、字符串、运算符……
- **值**（value）：这个词的原始文本内容

```cpp
// token.h
#pragma once
#include <string>

enum class TokenType {
    // 字面量
    Number,       // 42, 3.14
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
    Arrow,        // =>（箭头函数，暂时保留）
    Question,     // ?
    Colon,        // :
    Dot,          // .
    DotDot,       // ..（range）
    Ellipsis,     // ...（展开）
    Pipe,         // |（pipeline，如需要）

    // 分隔符
    LeftParen,    // (
    RightParen,   // )
    LeftBrace,    // {
    RightBrace,   // }
    LeftBracket,  // [
    RightBracket, // ]
    Semicolon,    // ;
    Comma,        // ,

    // 特殊
    Taco,         // 🌮
    EndOfFile,    // 文件结束
};

struct Token {
    TokenType type;
    std::string value;  // 原始文本
    int line;           // 行号，用于错误报告
    int column;         // 列号，用于错误报告
};
```

---

### 第二层：AST 节点

AST 节点表示代码的语法结构。每种语法结构对应一种节点类型：

```
var x = 10 + 20;
→
VarDecl {
    name: "x",
    value: BinaryExpr {
        left: NumberExpr { value: 10 },
        op: "+",
        right: NumberExpr { value: 20 }
    }
}
```

AST 是一棵树：节点可以包含其他节点。树的根节点代表整个程序，叶节点代表最基本的元素（数字、字符串、变量名）。

在 C++ 里，AST 节点天然适合用继承来实现——所有节点共享一个基类，不同的节点类型是子类。这是第十一章引入类和继承之后要做的事情。

---

### 第三层：求值器

求值器遍历 AST，对每个节点求值：

```
BinaryExpr { left: 10, op: "+", right: 20 }
→ 求值 left → 10
→ 求值 right → 20
→ 计算 10 + 20 → 30
```

求值器需要一个**环境**（Environment）来存储变量：

```
var x = 10;  → 环境: {x: 10}
var y = 20;  → 环境: {x: 10, y: 20}
print(x + y); → 从环境里取 x=10, y=20, 求值 10+20=30
```

当进入一个新的作用域（函数体、if 块），创建一个新的子环境；离开时销毁子环境。

---

## 5.4 整本书的项目路线图

现在把整本书的主线梳理一遍，建立全局视角。

### 七次进化

Taco 从一个只能切 Token 的程序，一步步进化成一个完整的解释器：

**v0：词法分析器**（第一部分）

```
输入：var x = 10 + 20;
输出：[VAR][x][=][10][+][20][;]
```

涉及 C++ 知识：结构体、枚举、字符串处理、基本函数。

**v1：语法分析器 + 基础求值**（第二部分）

```
输入：var x = 10 + 20;
输出：30（x 被赋值并打印）
```

涉及 C++ 知识：类、构造函数、析构函数、运算符重载。

**v2：控制流**（第三部分）

```taco
// 能运行这样的代码
var score = 85;
if (score >= 90) { print("A"); }
elseif (score >= 80) { print("B"); }
else { print("C"); }
```

涉及 C++ 知识：继承、多态、虚函数、vtable。

**v3：函数、闭包、作用域**（第四部分）

```taco
// 能运行这样的代码
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n-1) + fib(n-2);
}
print(fib(10));
```

涉及 C++ 知识：智能指针、RAII、移动语义。

**v4：array、map、内置库**（第五部分）

```taco
// 能运行这样的代码
var nums = [1, 2, 3, 4, 5];
nums.filter { x in x % 2 == 0 }.each { x in print(x); };
```

涉及 C++ 知识：STL 容器、迭代器、算法、Lambda。

**v5：模板化重构**（第六部分）

把解释器内部的值类型和容器用模板重构，代码更通用，性能略有提升。

涉及 C++ 知识：函数模板、类模板、模板特化、`std::variant`。

**v6：REPL + 多线程**（第七部分）

```
🌮 Taco 0.1.0
   It works on my machine.
> var x = 10;
> print(x + 20);
30
>
```

涉及 C++ 知识：`std::thread`、`std::mutex`、`std::channel`、`std::atomic`。

**v7：网络支持**（第八部分）

```taco
var res = fetchUrl("https://api.github.com/users/torvalds");
print(res);
```

涉及 C++ 知识：socket、Asio、cpp-httplib。

---

### 每一章的结构

本书每一章的结构是一致的：

1. **概念介绍**：这个 C++ 特性是什么，为什么需要它
2. **详细讲解**：语法、用法、底层原理
3. **More About**（部分章节）：更深入的细节，第一次读可以跳过
4. **项目章节**：用刚学到的特性改进 Taco

项目章节会展示完整的代码，并解释"为什么这样做，而不是那样做"。每次改进都有真实的动机，不是为了用新特性而用新特性。

---

## 小结

这一章建立了全书的框架：

**解释器的三层架构**：词法分析（源码 → Token）、语法分析（Token → AST）、求值（AST → 结果）。接下来每一个项目章节都在这个框架里。

**Taco 的设计**：动态类型、终端脚本、pipeline 风格、带有个性的彩蛋。

**七次进化**：从只能切 Token 的 v0，到能运行脚本、支持 REPL 和网络请求的 v7。每次进化对应一部分新学的 C++ 知识。

---

下一章开始动手：实现 Taco 的词法分析器（v0）。
