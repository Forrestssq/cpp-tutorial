# 第三十七章：Taco 完整版

---

第九部分是全书的收尾。不再引入新的 C++ 知识点，而是回头看：我们建了什么，学了什么，还能往哪里走。

这一章回顾 Taco 的七次进化，看整个解释器的完整架构，然后实现一个一直缺席的功能：**模块系统**（`import`）。

---

## 37.1 回顾七次进化

从第六章的 v0 到第三十六章的 v7，Taco 用了全书的篇幅一步一步构建出来。每一步都有具体的动机，而不是为了演示 C++ 特性而演示。

### v0：词法分析器（第六章）

**问题**：源代码是一个字符串，无法直接处理。

**解决**：词法分析器（Lexer）把源代码切成 Token 列表。

**用到的 C++**：结构体、枚举、`std::string`、`std::vector`、基本函数。

```taco
// v0 能做的事
// 输入：var x = 10 + 20;
// 输出：[Var, Identifier("x"), Assign, Number(10), Plus, Number(20), Semicolon]
```

**局限**：只能切 Token，不知道 Token 之间的关系，不能执行任何代码。

### v1：语法分析器与 AST（第十一章）

**问题**：Token 列表是线性的，无法表达表达式的结构（`10 + 20 * 3` 中乘法优先于加法）。

**解决**：递归下降解析器把 Token 列表解析成抽象语法树（AST）。

**用到的 C++**：类、继承、`std::unique_ptr`、多态（虚函数雏形）。

```
// AST 的树形结构
BinaryExpr(Plus)
  ├── NumberLiteral(10)
  └── BinaryExpr(Multiply)
        ├── NumberLiteral(20)
        └── NumberLiteral(3)
```

**局限**：能构建 AST，能对基本表达式求值，但没有控制流、没有变量。

### v2：求值器与控制流（第十五章）

**问题**：解释器只能算表达式，不能跑程序（没有 if/while/for）。

**解决**：用继承体系设计完整的 AST 节点层次，用虚函数实现 `evaluate()`，实现 if/elseif/else、while、for（C 风格和 range 风格）、switch。

**用到的 C++**：继承、虚函数、多态、`dynamic_cast`、RAII 雏形。

```taco
// v2 能运行：
if (x > 10) {
    print("big");
} else {
    print("small");
}
for i in range(0, 5) { print(i); }
```

**局限**：有控制流了，但没有函数，程序无法抽象和复用。

### v3：函数、闭包与环境（第二十章）

**问题**：没有函数，所有逻辑都是平铺的，无法复用。

**解决**：实现 Environment（作用域链）、用户定义函数（`TacoFunction`）、闭包（函数捕获定义时的环境）、递归、命名参数、多返回值。

**用到的 C++**：`shared_ptr`（共享所有权管理作用域链）、移动语义、RAII（`ScopeGuard` 管理函数调用栈）、`std::function`（内置函数）。

```taco
// v3 能运行：
func makeCounter() {
    var count = 0;
    return func() { count = count + 1; return count; };
}
var c = makeCounter();
print(c());  // 1
print(c());  // 2
```

**局限**：没有内置数据结构（array、map），字符串方法缺失，无法处理集合数据。

### v4：array、map 与标准库（第二十五章）

**问题**：没有集合类型，Taco 写不了大部分实用脚本。

**解决**：用 `std::vector` 实现 TacoArray，用 `std::map` 实现 TacoMap，实现 pipeline 方法（`filter`、`map`、`each`、`reduce`），实现内置标准库（文件、系统、随机数）。

**用到的 C++**：STL 容器（`vector`、`map`、`unordered_map`）、算法（`transform`、`sort`、`find_if`）、Lambda 表达式、`std::function`。

```taco
// v4 能运行：
var nums = [1, 2, 3, 4, 5];
nums.filter { n in n % 2 == 0 }.map { n in n * n }.each { n in print(n) };
// 4 16
```

**局限**：值类型系统是一个大的 `std::variant` 手写版本，类型检查分散，性能有优化空间。

### v5：解释器模板化（第二十九章）

**问题**：v4 的值类型（`TacoValue`）是手写的 tagged union，类型检查代码散落在各处，容易出错，难以扩展。

**解决**：用 `std::variant` 重构 TacoValue，用 `std::visit` + 模板实现类型分派，用类模板重构 Environment，用 Visitor 模式实现 AST 节点遍历。

**用到的 C++**：`std::variant`、`std::visit`、函数模板、类模板、模板特化、`if constexpr`。

```cpp
// v5 之后的值类型
using TacoVariant = std::variant<
    std::monostate,                          // nil
    bool,                                    // bool
    double,                                  // number
    std::string,                             // string
    std::shared_ptr<TacoArray>,              // array
    std::shared_ptr<TacoMap>,               // map
    std::shared_ptr<TacoFunction>,           // function
    std::shared_ptr<TacoNative>             // native function
>;
```

**局限**：没有 REPL，没有并发，只能以文件模式运行脚本。

### v6：REPL 与并发（第三十三章）

**问题**：Taco 只能运行脚本文件，没有交互式环境；没有并发支持。

**解决**：实现 REPL（Read-Eval-Print Loop），用线程分离输入和求值，用 `std::condition_variable` 实现 channel，用 `std::mutex` 保护共享状态，实现 Taco 的 `thread`、`channel`、`mutex` 关键字。

**用到的 C++**：`std::thread`、`std::mutex`、`std::lock_guard`、`std::condition_variable`、`std::atomic`、`std::future`/`std::promise`。

```taco
// v6 能运行：
var ch = channel();
var t = thread { ch.send(42); };
var val = ch.receive();
print(val);  // 42
t.join();
```

**局限**：没有网络支持，不能和外部世界交互。

### v7：网络支持（第三十六章）

**问题**：Taco 是一个孤立的本地工具，不能调用 API、不能发 HTTP 请求。

**解决**：用 cpp-httplib 实现 `fetchUrl()` 和 `postData()`，用 nlohmann/json 实现 `parseJson()` 和 `toJson()`。

**用到的 C++**：第三方库集成（FetchContent）、cmake find_package、库封装设计。

```taco
// v7 能运行：
var user = parseJson(fetchUrl("https://api.github.com/users/torvalds"));
print("Name: " + user["name"]);
```

---

## 37.2 最终版本的完整架构图

```
┌─────────────────────────────────────────────────────┐
│                   main.cpp                          │
│  解析命令行参数：taco script.taco 或 taco（REPL）    │
└───────────────┬─────────────────────┬───────────────┘
                │                     │
         脚本模式                   REPL 模式
                │                     │
                ▼                     ▼
┌───────────────────────────────────────────────────────────┐
│                     前端：词法 + 语法                      │
│                                                           │
│  源代码字符串                                              │
│       │                                                   │
│       ▼                                                   │
│  Lexer（lexer.cpp）                                        │
│  源代码 → Token 列表                                       │
│       │                                                   │
│       ▼                                                   │
│  Parser（parser.cpp）                                      │
│  Token 列表 → AST（unique_ptr 树）                         │
└───────────────────────┬───────────────────────────────────┘
                        │
                        ▼
┌───────────────────────────────────────────────────────────┐
│                     后端：求值器                           │
│                                                           │
│  Evaluator（evaluator.cpp）                               │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  AST 节点 → evaluate() → TacoValue                  │  │
│  │                                                     │  │
│  │  Environment（environment.h）                        │  │
│  │  作用域链：shared_ptr<Env> → parent                  │  │
│  │                                                     │  │
│  │  TacoValue（value.h，std::variant）                  │  │
│  │  nil | bool | double | string | array |             │  │
│  │  map | TacoFunction | TacoNative                    │  │
│  └─────────────────────────────────────────────────────┘  │
└───────────────────────┬───────────────────────────────────┘
                        │
                        ▼
┌───────────────────────────────────────────────────────────┐
│                     内置库                                │
│                                                           │
│  builtin.cpp      核心内置（print, input, type, ...）     │
│  builtin_io.cpp   文件系统（cat, ls, mkdir, ...）         │
│  builtin_net.cpp  网络（fetchUrl, postData, parseJson）   │
│                                                           │
│  第三方库：                                               │
│    cpp-httplib    HTTP 客户端/服务器                       │
│    nlohmann/json  JSON 解析                               │
└───────────────────────────────────────────────────────────┘
```

### 数据流

一个 Taco 程序的完整执行过程：

```
源代码: var x = 10 + 20; print(x);
   │
   ▼ Lexer
Token: [Var, Id("x"), Assign, Num(10), Plus, Num(20), Semicolon,
        Id("print"), LParen, Id("x"), RParen, Semicolon]
   │
   ▼ Parser
AST:
  Program
  ├── VarDecl("x", BinaryExpr(Plus, Num(10), Num(20)))
  └── ExprStmt(CallExpr(Id("print"), [Id("x")]))
   │
   ▼ Evaluator
执行 VarDecl：
  计算 BinaryExpr(Plus, 10, 20) → TacoValue(30.0)
  在环境里定义 "x" = 30.0
执行 ExprStmt：
  查找 "print" → TacoNative(print_fn)
  查找 "x" → 30.0
  调用 print_fn([30.0]) → 输出 "30"
   │
   ▼ 输出
30
```

---

## 37.3 模块系统的实现：`import`

模块系统一直是个缺口。Taco 支持 `import utils;` 和 `from io import readFile;` 这样的语法（设计文档里有），但之前没有实现。现在补上。

### 设计决策

Taco 的模块系统非常简单：**一个 `.taco` 文件就是一个模块**，`import` 就是把那个文件求值一遍，把它的顶层定义导入当前作用域。

这和 Python 早期的 `import` 机制类似，也是很多脚本语言（Lua、早期 Ruby）的做法。不用 namespace，不用符号表分离，简单直接。

### 模块查找顺序

`import utils;` 会按以下顺序查找 `utils.taco`：

1. 当前脚本所在目录
2. 环境变量 `TACO_PATH` 指定的目录列表
3. Taco 安装目录的 `lib/` 下（标准库）

### 词法分析：新 Token

v7 的词法分析器已经识别 `import` 和 `from`（在 token.h 里定义了 `Import` 和 `From`），这里不需要改词法分析器。

### 语法分析：解析 import 语句

```cpp
// parser.cpp：解析 import 语句

// import utils;
// import utils as u;
// from io import readFile;
// from io import readFile, writeFile;

ExprPtr Parser::parse_import_stmt() {
    // import ...
    expect(TokenType::Import, "Expected 'import'.");

    auto node = std::make_unique<ImportStmt>();

    if (current().type == TokenType::Identifier) {
        node->module_name = advance().value;

        // import utils as u
        if (current().type == TokenType::Identifier
            && current().value == "as") {
            advance();  // 消费 "as"
            node->alias = expect(TokenType::Identifier,
                                 "Expected alias name after 'as'.").value;
        }
    }

    expect(TokenType::Semicolon, "Expected ';' after import.");
    return node;
}

// from io import readFile;
// from io import readFile, writeFile;
ExprPtr Parser::parse_from_import_stmt() {
    // from ...
    expect(TokenType::From, "Expected 'from'.");

    auto node = std::make_unique<FromImportStmt>();
    node->module_name = expect(TokenType::Identifier,
                               "Expected module name.").value;

    // import ...
    if (current().type != TokenType::Import) {
        throw ParseError("Expected 'import' after module name.", current());
    }
    advance();

    // 读取导入的名字列表
    do {
        node->names.push_back(
            expect(TokenType::Identifier, "Expected name to import.").value
        );
        if (current().type == TokenType::Comma) advance();
        else break;
    } while (true);

    expect(TokenType::Semicolon, "Expected ';' after import.");
    return node;
}
```

### AST 节点

```cpp
// ast.h（新增）

struct ImportStmt : Stmt {
    std::string module_name;    // "utils"
    std::string alias;          // "u"（可选）

    TacoValue evaluate(Evaluator& eval) override;
};

struct FromImportStmt : Stmt {
    std::string module_name;           // "io"
    std::vector<std::string> names;    // ["readFile", "writeFile"]

    TacoValue evaluate(Evaluator& eval) override;
};
```

### 求值：加载模块

```cpp
// evaluator.cpp

// 模块缓存：避免同一个模块被 import 多次重复执行
static std::unordered_map<std::string, std::shared_ptr<Environment>>
    module_cache;

// 查找模块文件
std::string find_module_file(const std::string& module_name,
                             const std::string& current_dir) {
    // 1. 当前目录
    std::string local = current_dir + "/" + module_name + ".taco";
    if (std::filesystem::exists(local)) return local;

    // 2. TACO_PATH 环境变量
    const char* taco_path = std::getenv("TACO_PATH");
    if (taco_path) {
        std::string path_str(taco_path);
        std::stringstream ss(path_str);
        std::string dir;
        // TACO_PATH 用冒号分隔（Unix 风格）
        while (std::getline(ss, dir, ':')) {
            std::string candidate = dir + "/" + module_name + ".taco";
            if (std::filesystem::exists(candidate)) return candidate;
        }
    }

    // 3. 标准库目录（可执行文件旁边的 lib/）
    // 这里简化，留给读者实现

    throw std::runtime_error(
        "Module not found: '" + module_name
        + "'. (Searched in: " + current_dir + ")"
    );
}

// 加载并执行模块，返回模块的环境
std::shared_ptr<Environment> load_module(const std::string& module_name,
                                          const std::string& current_dir,
                                          Evaluator& eval) {
    std::string filepath = find_module_file(module_name, current_dir);

    // 检查缓存
    auto it = module_cache.find(filepath);
    if (it != module_cache.end()) {
        return it->second;
    }

    // 读取文件
    std::ifstream f(filepath);
    if (!f) {
        throw std::runtime_error("Cannot open module file: " + filepath);
    }
    std::string source((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());

    // 在独立的环境里执行模块
    // 模块的父环境是全局环境（可以使用内置函数），但和当前脚本隔离
    auto module_env = std::make_shared<Environment>(eval.global_env());

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto program = parser.parse();

    // 用模块的目录作为当前目录（模块内的相对路径基于模块自身位置）
    std::string module_dir = std::filesystem::path(filepath).parent_path();
    Evaluator module_eval(module_env, module_dir);
    module_eval.evaluate(*program);

    // 缓存模块环境
    module_cache[filepath] = module_env;
    return module_env;
}

// import utils;
TacoValue ImportStmt::evaluate(Evaluator& eval) {
    auto module_env = load_module(module_name,
                                  eval.current_dir(),
                                  eval);

    // 把模块的所有顶层定义导入当前环境
    std::string prefix = alias.empty() ? "" : alias + ".";

    for (const auto& [name, value] : module_env->bindings()) {
        // 跳过以 _ 开头的"私有"定义
        if (!name.empty() && name[0] == '_') continue;

        std::string import_name = alias.empty() ? name : (alias + "." + name);
        eval.current_env()->define(import_name, value);
    }

    return TacoValue();  // import 语句不返回值
}

// from io import readFile;
TacoValue FromImportStmt::evaluate(Evaluator& eval) {
    auto module_env = load_module(module_name,
                                  eval.current_dir(),
                                  eval);

    for (const auto& name : names) {
        auto val = module_env->get(name);
        if (!val.has_value()) {
            throw std::runtime_error(
                "Module '" + module_name
                + "' has no export named '" + name + "'"
            );
        }
        eval.current_env()->define(name, *val);
    }

    return TacoValue();
}
```

### 测试模块系统

```taco
// math_utils.taco（模块文件）

// 私有辅助函数（以 _ 开头，import 时不导出）
func _clamp(x, lo, hi) {
    if (x < lo) { return lo; }
    if (x > hi) { return hi; }
    return x;
}

func square(x) { return x * x; }
func cube(x) { return x * x * x; }
func abs(x) { return x >= 0 ? x : -x; }

func factorial(n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

func isPrime(n) {
    if (n < 2) { return false; }
    for i in range(2, n) {
        if (n % i == 0) { return false; }
    }
    return true;
}

var PI = 3.14159265358979;
var E  = 2.71828182845905;
```

```taco
// main.taco

import math_utils;

print(math_utils.square(5));      // 25
print(math_utils.factorial(10));  // 3628800
print(math_utils.PI);             // 3.14159...
print(math_utils.isPrime(17));    // true

// _clamp 不会被导入
// print(math_utils._clamp(5, 0, 10));  // 报错：not defined
```

```taco
// 用 from 导入特定名字

from math_utils import square, PI;

print(square(7));  // 49
print(PI);         // 3.14159...
```

```taco
// 用别名

import math_utils as m;

print(m.square(4));   // 16
print(m.cube(3));     // 27
```

---

## 37.4 还能怎么扩展：垃圾回收、字节码、标准库

v7 是这本书的终点，但 Taco 作为一个解释器还有很多可以改进的方向。这里简要介绍，为有兴趣继续的读者指路。

### 垃圾回收（Garbage Collection）

Taco 目前用 `shared_ptr` 管理内存。`shared_ptr` 基于引用计数，有一个著名的问题：**循环引用**导致内存泄漏。

```taco
// 这在 Taco 里会产生循环引用
var a = {};
var b = {};
a["other"] = b;
b["other"] = a;
// a 和 b 的引用计数都不会降到 0，永远不释放
```

用 `weak_ptr` 可以打破一些循环，但不能解决所有情况。真实的解释器（Python、Ruby、Lua）用专门的垃圾回收器。

**最简单的 GC：标记-清除（Mark-Sweep）**

1. 从根集合（全局变量、调用栈上的值）出发
2. 遍历所有可达对象，标记为"存活"
3. 清除所有未标记的对象

实现标记-清除 GC 需要把所有堆对象放在一个全局列表里统一管理，放弃 `shared_ptr`，改用裸指针。工程量不小，但思路清晰。

**更高效的 GC：分代回收（Generational GC）**

大多数对象"活得很短"（局部变量、临时值）。分代回收把对象分成年轻代和老年代，优先回收年轻代，减少 GC 停顿时间。这是 Python、Java 的做法。

### 字节码虚拟机（Bytecode VM）

Taco 目前是**树遍历解释器**（tree-walking interpreter）：每次执行都遍历 AST 树，每个节点调用 `evaluate()`。这很简单，但有性能问题：

- AST 节点分散在堆上，缓存不友好
- 每个节点求值都有虚函数调用的开销
- 无法做很多编译期优化

改进方案：**编译成字节码**，用字节码虚拟机（VM）执行。

```
源代码 → Lexer → Parser → AST → 字节码编译器 → 字节码 → VM
```

字节码是一种紧凑的、为解释器设计的指令集：

```
// var x = 10 + 20; print(x);
PUSH_CONST  10     // 把 10 压栈
PUSH_CONST  20     // 把 20 压栈
ADD                // 弹出两个，相加，结果压栈
SET_LOCAL   0      // 把栈顶存到局部变量 0（x）
GET_LOCAL   0      // 把局部变量 0 压栈
CALL_NATIVE print  // 调用内置 print
```

字节码 VM 的优势：

- 字节码是连续内存，CPU 缓存友好
- 避免了虚函数调用的间接层
- 可以做很多编译期优化（常量折叠、死代码消除）

这是 CPython、Lua、Ruby（YARV）的实现方式。

### JIT 编译（Just-In-Time Compilation）

更进一步：在运行时把热点字节码编译成机器码。这是 V8（JavaScript）、PyPy（Python）、LuaJIT（Lua）的做法。实现复杂度高，但性能可以接近原生代码。

对于 Taco 这样的小解释器，JIT 不是必要的，但了解它的存在很重要。

### 标准库扩展

Taco 的标准库目前非常有限。可以参考 Python 或者 Lua 的标准库设计，添加：

- `taco.math`：更多数学函数（sin、cos、log、sqrt……）
- `taco.string`：更丰富的字符串操作
- `taco.io`：文件读写的更多模式（append、binary）
- `taco.net`：更完整的网络支持（WebSocket、UDP）
- `taco.db`：SQLite 接口

每个标准库模块都是一个 `.taco` 文件（用 `from taco.math import sqrt;` 导入），或者一个 C++ 内置函数集合。

### 类型系统

Taco 是动态类型的，类型错误在运行时才发现。可以加一个**可选的类型标注**：

```taco
// 想象中的 Taco 类型标注（目前不支持）
func add(x: number, y: number) -> number {
    return x + y;
}
```

加类型标注不一定要做全量静态类型检查，也可以是运行时检查（在函数调用时验证参数类型）。

---

## 小结

这一章是 Taco 的完整总结。

七次进化对应七组 C++ 知识：词法器（基础语法）→ AST（类与继承）→ 求值器（多态）→ 闭包（智能指针、RAII）→ 标准库（STL、Lambda）→ 模板化（variant、模板）→ REPL（多线程）→ 网络（第三方库）。

模块系统让 Taco 的代码可以分文件组织，`import`/`from import` 语法和 Python 风格一致，实现简单：找到 `.taco` 文件，在独立环境里执行，把顶层定义导入当前作用域，用文件路径做缓存键避免重复执行。

Taco 还有很多可以探索的方向：GC 替换 `shared_ptr` 解决循环引用，字节码 VM 提升性能，类型标注增强安全性，标准库扩展实用性。这些都是真实语言实现里的核心问题，有兴趣的读者可以从 Crafting Interpreters（Robert Nystrom）开始深入。

---

下一章讲 VSCode 插件：给 Taco 加上语法高亮和基础的 Language Server，让开发体验更好。
