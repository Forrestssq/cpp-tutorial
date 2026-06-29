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
# 第三十八章：VSCode 插件与 Language Server

---

Taco 可以运行了，但写 Taco 脚本的体验还不够好——所有关键字都是同一个颜色，没有自动补全，写错了没有提示。

这一章给 Taco 加上编辑器支持：先做语法高亮（不需要 C++，只是配置文件），然后实现一个基础的 Language Server（需要 C++，让编辑器能和解释器通信），最后打包发布到 VSCode Marketplace。

---

## 38.1 LSP 是什么

在 LSP 出现之前，每个编辑器（VSCode、Vim、Emacs、Sublime Text……）和每种语言都需要单独集成。N 个编辑器 × M 种语言 = N×M 个集成点，每个都要独立实现。

**Language Server Protocol（LSP）** 是 Microsoft 在 2016 年提出的协议，把语言智能和编辑器解耦：

```
┌─────────┐   LSP（JSON-RPC）   ┌────────────────┐
│  编辑器  │ <─────────────────> │ Language Server │
│(Client) │                     │   (Server)     │
└─────────┘                     └────────────────┘
```

- **编辑器（LSP Client）**：VSCode、Vim、Emacs 等，发送用户操作（光标移动、打字、保存）作为请求
- **Language Server**：独立进程，理解某种语言，回答编辑器的问题（这个词是什么、这里有没有错误、补全建议是什么）

有了 LSP，一个 Language Server 可以为所有支持 LSP 的编辑器服务。现在已经有几百种语言的 Language Server（clangd 为 C++，rust-analyzer 为 Rust，pyright 为 Python……）。

### LSP 通信格式

LSP 用 JSON-RPC 2.0 通过 stdin/stdout 通信（或 TCP socket）。每条消息有一个 header 和 body：

```
Content-Length: 89\r\n
\r\n
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"processId":12345,...}}
```

编辑器发送请求，Language Server 发送响应：

```json
// 编辑器 → Server：初始化
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
        "processId": 12345,
        "rootUri": "file:///home/user/project",
        "capabilities": { ... }
    }
}

// Server → 编辑器：初始化响应
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "capabilities": {
            "textDocumentSync": 1,
            "completionProvider": {},
            "hoverProvider": true
        }
    }
}
```

常用的 LSP 方法：

| 方法 | 方向 | 含义 |
|------|------|------|
| `initialize` | Client → Server | 初始化，交换能力列表 |
| `textDocument/didOpen` | Client → Server | 用户打开了文件 |
| `textDocument/didChange` | Client → Server | 用户修改了文件 |
| `textDocument/completion` | Client → Server | 请求自动补全 |
| `textDocument/hover` | Client → Server | 鼠标悬停，请求信息 |
| `textDocument/publishDiagnostics` | Server → Client | 推送错误/警告 |

---

## 38.2 语法高亮：TextMate grammar

语法高亮不需要 Language Server，只需要一个 **TextMate grammar** 文件（`.tmLanguage.json`）。这是一个正则表达式规则集，定义哪些词应该用什么颜色显示。

### VSCode 插件结构

```
taco-language/
  package.json          # 插件配置（名字、版本、贡献点）
  language-configuration.json  # 括号匹配、注释、缩进规则
  syntaxes/
    taco.tmLanguage.json  # TextMate 语法文件
  server/
    src/
      server.cpp        # Language Server 实现
    CMakeLists.txt
  client/
    src/
      extension.ts      # VSCode 插件入口（TypeScript）
  package-lock.json
```

### package.json

```json
{
    "name": "taco-language",
    "displayName": "Taco Language",
    "description": "Language support for the Taco scripting language",
    "version": "0.1.0",
    "publisher": "your-name",
    "engines": {
        "vscode": "^1.85.0"
    },
    "categories": ["Programming Languages"],
    "contributes": {
        "languages": [
            {
                "id": "taco",
                "aliases": ["Taco", "taco"],
                "extensions": [".taco"],
                "configuration": "./language-configuration.json"
            }
        ],
        "grammars": [
            {
                "language": "taco",
                "scopeName": "source.taco",
                "path": "./syntaxes/taco.tmLanguage.json"
            }
        ]
    },
    "main": "./client/out/extension.js",
    "scripts": {
        "compile": "tsc -p ./client/tsconfig.json",
        "vscode:prepublish": "npm run compile"
    },
    "dependencies": {
        "vscode-languageclient": "^9.0.1"
    },
    "devDependencies": {
        "@types/vscode": "^1.85.0",
        "typescript": "^5.3.3"
    }
}
```

### language-configuration.json

```json
{
    "comments": {
        "lineComment": "//"
    },
    "brackets": [
        ["{", "}"],
        ["[", "]"],
        ["(", ")"]
    ],
    "autoClosingPairs": [
        { "open": "{", "close": "}" },
        { "open": "[", "close": "]" },
        { "open": "(", "close": ")" },
        { "open": "\"", "close": "\"" }
    ],
    "surroundingPairs": [
        ["{", "}"],
        ["[", "]"],
        ["(", ")"],
        ["\"", "\""]
    ],
    "indentationRules": {
        "increaseIndentPattern": "^.*\\{\\s*$",
        "decreaseIndentPattern": "^\\s*\\}"
    }
}
```

### taco.tmLanguage.json

TextMate grammar 用正则表达式匹配代码里的不同部分，给每部分打上 scope（范围标签），编辑器主题根据 scope 决定颜色。

```json
{
    "$schema": "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json",
    "name": "Taco",
    "scopeName": "source.taco",
    "patterns": [
        { "include": "#comments" },
        { "include": "#strings" },
        { "include": "#keywords" },
        { "include": "#numbers" },
        { "include": "#operators" },
        { "include": "#functions" },
        { "include": "#variables" },
        { "include": "#builtins" }
    ],
    "repository": {
        "comments": {
            "name": "comment.line.double-slash.taco",
            "match": "//.*$"
        },
        "strings": {
            "name": "string.quoted.double.taco",
            "begin": "\"",
            "end": "\"",
            "patterns": [
                {
                    "name": "constant.character.escape.taco",
                    "match": "\\\\[nrt\\\\\"']"
                },
                {
                    "name": "meta.embedded.expression.taco",
                    "begin": "\\{",
                    "end": "\\}",
                    "patterns": [{ "include": "$self" }]
                }
            ]
        },
        "keywords": {
            "name": "keyword.control.taco",
            "match": "\\b(var|func|if|elseif|else|while|for|in|return|class|struct|enum|extends|self|super|switch|case|default|import|from|thread|channel|mutex|true|false|nil)\\b"
        },
        "numbers": {
            "name": "constant.numeric.taco",
            "match": "\\b([0-9][0-9_]*\\.?[0-9_]*)\\b"
        },
        "operators": {
            "patterns": [
                {
                    "name": "keyword.operator.comparison.taco",
                    "match": "(==|!=|>=|<=|>|<)"
                },
                {
                    "name": "keyword.operator.logical.taco",
                    "match": "(&&|\\|\\||!)"
                },
                {
                    "name": "keyword.operator.arithmetic.taco",
                    "match": "(\\+|-|\\*|/|%|\\^)"
                },
                {
                    "name": "keyword.operator.assignment.taco",
                    "match": "="
                },
                {
                    "name": "keyword.operator.other.taco",
                    "match": "(\\?\\.|\\?\\?|\\.\\.\\.)"
                }
            ]
        },
        "functions": {
            "name": "entity.name.function.taco",
            "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*\\()"
        },
        "builtins": {
            "name": "support.function.builtin.taco",
            "match": "\\b(print|input|type|number|string|bool|cat|echo|ls|mkdir|rm|mv|cp|pwd|cd|exists|exec|env|fetchUrl|postData|parseJson|toJson|range)\\b"
        },
        "variables": {
            "name": "variable.other.taco",
            "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b"
        }
    }
}
```

这个 grammar 实现了：
- 注释（`//`）：灰色
- 字符串（双引号）：绿色，支持字符串插值（`{expr}` 内部也有高亮）
- 关键字（`var`、`func`、`if`……）：蓝色
- 数字：橙色
- 运算符：高亮
- 函数调用：函数名高亮
- 内置函数（`print`、`fetchUrl`……）：特殊颜色

---

## 38.3 实现基础 Language Server

语法高亮不需要 Language Server，只是静态规则。但自动补全、错误提示、悬停信息需要真正理解代码——需要 Language Server。

### Language Server 的架构

Taco 的 Language Server 是一个独立的 C++ 可执行文件，通过 stdin/stdout 和 VSCode 通信。VSCode 插件（TypeScript）启动这个进程，然后通过 LSP 协议和它交互。

```
VSCode
  └── extension.ts（LSP Client）
        │  stdin/stdout
        ▼
  taco-lsp（C++ Language Server）
        └── 复用 Taco 的 Lexer、Parser
```

Language Server 内部复用 Taco 的 Lexer 和 Parser——这就是把解释器拆成独立模块的好处。

### LSP 消息解析

LSP 消息通过 stdin 逐条读取，格式是 header + JSON body：

```cpp
// lsp_server.cpp

#include <iostream>
#include <string>
#include <sstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 读取一条 LSP 消息（阻塞直到读到完整消息）
json read_message() {
    // 读取 header 行
    int content_length = -1;
    std::string line;

    while (std::getline(std::cin, line)) {
        // 移除 \r（Windows 换行符）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            // 空行：header 结束，读取 body
            break;
        }

        // 解析 Content-Length
        const std::string prefix = "Content-Length: ";
        if (line.substr(0, prefix.size()) == prefix) {
            content_length = std::stoi(line.substr(prefix.size()));
        }
    }

    if (content_length < 0) {
        throw std::runtime_error("Missing Content-Length header");
    }

    // 读取 body
    std::string body(content_length, '\0');
    std::cin.read(body.data(), content_length);

    return json::parse(body);
}

// 发送一条 LSP 消息
void send_message(const json& msg) {
    std::string body = msg.dump();
    std::cout << "Content-Length: " << body.size() << "\r\n";
    std::cout << "\r\n";
    std::cout << body;
    std::cout.flush();
}

// 发送响应
void send_response(const json& id, const json& result) {
    send_message({
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    });
}

// 发送错误响应
void send_error(const json& id, int code, const std::string& message) {
    send_message({
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", {{"code", code}, {"message", message}}}
    });
}

// 发送通知（没有 id，Server 主动发给 Client）
void send_notification(const std::string& method, const json& params) {
    send_message({
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    });
}
```

### 文档管理

Language Server 需要维护当前打开的所有文档的内容：

```cpp
// 存储所有打开的文档内容
std::unordered_map<std::string, std::string> open_documents;

void handle_did_open(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    auto text = params["textDocument"]["text"].get<std::string>();
    open_documents[uri] = text;

    // 打开时立刻做一次诊断
    publish_diagnostics(uri, text);
}

void handle_did_change(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    // contentChanges 是增量更新列表，这里取全量（simpler）
    auto changes = params["contentChanges"];
    if (!changes.empty()) {
        open_documents[uri] = changes.back()["text"].get<std::string>();
    }

    // 每次改动后重新做诊断
    publish_diagnostics(uri, open_documents[uri]);
}
```

### 错误诊断（Diagnostics）

Diagnostics 是 LSP 里最有价值的功能：实时显示代码错误。

Taco 的词法分析器和语法分析器在遇到错误时会抛出异常。Language Server 捕获这些异常，把错误位置和信息转成 LSP Diagnostic，推送给编辑器：

```cpp
// Taco 错误结构（在 error.h 里定义）
struct TacoError {
    std::string message;
    int line;    // 1-indexed
    int column;  // 0-indexed
};

// 把 Taco 错误转成 LSP Diagnostic
json make_diagnostic(const TacoError& err, int severity = 1) {
    // severity: 1=Error, 2=Warning, 3=Information, 4=Hint
    return {
        {"range", {
            {"start", {{"line", err.line - 1}, {"character", err.column}}},
            {"end",   {{"line", err.line - 1}, {"character", err.column + 1}}}
        }},
        {"severity", severity},
        {"source", "taco"},
        {"message", err.message}
    };
}

void publish_diagnostics(const std::string& uri, const std::string& source) {
    json diagnostics = json::array();

    try {
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.tokenize();  // 可能抛出 LexError

        // 语法分析
        Parser parser(tokens);
        auto ast = parser.parse();      // 可能抛出 ParseError

        // 也可以做更深的语义分析，比如检查未定义变量
        // 但那需要一个不执行、只分析的 pass，暂时不实现

    } catch (const LexError& e) {
        diagnostics.push_back(make_diagnostic({e.message(), e.line(), e.column()}));
    } catch (const ParseError& e) {
        diagnostics.push_back(make_diagnostic({e.message(), e.line(), e.column()}));
    } catch (...) {
        // 其他错误忽略
    }

    send_notification("textDocument/publishDiagnostics", {
        {"uri", uri},
        {"diagnostics", diagnostics}
    });
}
```

这样，用户一写错了（比如漏了括号、拼错了关键字），编辑器里立刻出现红色波浪线。

### 自动补全（Completion）

自动补全返回一个候选项列表。对于 Taco，最简单的补全策略是：返回所有关键字和内置函数。

```cpp
// 所有 Taco 关键字
static const std::vector<std::string> TACO_KEYWORDS = {
    "var", "func", "if", "elseif", "else", "while", "for", "in",
    "return", "class", "struct", "enum", "extends", "self", "super",
    "switch", "case", "default", "import", "from", "thread", "channel",
    "mutex", "true", "false", "nil"
};

// 所有内置函数
static const std::vector<std::string> TACO_BUILTINS = {
    "print", "input", "type", "number", "string", "bool",
    "cat", "echo", "ls", "mkdir", "rm", "mv", "cp", "pwd", "cd", "exists",
    "exec", "env", "fetchUrl", "postData", "parseJson", "toJson",
    "range", "random"
};

json handle_completion(const json& params) {
    json items = json::array();

    // 关键字补全
    for (const auto& kw : TACO_KEYWORDS) {
        items.push_back({
            {"label", kw},
            {"kind", 14},          // 14 = Keyword
            {"detail", "keyword"}
        });
    }

    // 内置函数补全
    for (const auto& fn : TACO_BUILTINS) {
        items.push_back({
            {"label", fn},
            {"kind", 3},           // 3 = Function
            {"detail", "built-in function"}
        });
    }

    // 更高级：扫描当前文档，找用户定义的变量和函数名
    // ... （从 open_documents 里取当前文件内容，跑 lexer，提取标识符）

    return items;
}
```

### 悬停信息（Hover）

鼠标悬停在某个词上时，显示文档或类型信息：

```cpp
// 内置函数的文档字符串
static const std::unordered_map<std::string, std::string> BUILTIN_DOCS = {
    {"print",     "print(value)\n\nPrint value to stdout."},
    {"fetchUrl",  "fetchUrl(url: string) -> string\n\nSend HTTP GET request. Returns response body."},
    {"postData",  "postData(url: string, body: string, contentType?: string) -> string\n\nSend HTTP POST request."},
    {"parseJson", "parseJson(str: string) -> any\n\nParse JSON string into Taco value (map, array, etc.)."},
    {"range",     "range(start: number, end: number) -> array\n\nGenerate integer sequence [start, end)."},
    // ... 其他内置函数 ...
};

json handle_hover(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    auto line = params["position"]["line"].get<int>();
    auto character = params["position"]["character"].get<int>();

    // 从文档里取当前行
    auto& source = open_documents[uri];
    std::istringstream ss(source);
    std::string current_line;
    for (int i = 0; i <= line; i++) {
        std::getline(ss, current_line);
    }

    // 找光标位置的词
    // 向左找词的开始，向右找词的结束
    int start = character;
    while (start > 0 && (std::isalnum(current_line[start-1]) || current_line[start-1] == '_')) {
        start--;
    }
    int end = character;
    while (end < (int)current_line.size()
           && (std::isalnum(current_line[end]) || current_line[end] == '_')) {
        end++;
    }

    std::string word = current_line.substr(start, end - start);

    auto it = BUILTIN_DOCS.find(word);
    if (it != BUILTIN_DOCS.end()) {
        return {
            {"contents", {
                {"kind", "markdown"},
                {"value", "```taco\n" + it->second + "\n```"}
            }}
        };
    }

    return nullptr;  // 没有信息，不显示悬停框
}
```

### 主循环

把所有处理程序组装起来：

```cpp
int main() {
    // 禁用 stdout 缓冲（LSP 要求立即发送）
    std::cout.setf(std::ios::unitbuf);

    while (true) {
        json msg;
        try {
            msg = read_message();
        } catch (const std::exception& e) {
            // stdin 关闭（编辑器退出），退出 server
            break;
        }

        std::string method = msg.value("method", "");
        json id = msg.contains("id") ? msg["id"] : json(nullptr);
        json params = msg.value("params", json(nullptr));

        if (method == "initialize") {
            // 声明 server 的能力
            send_response(id, {
                {"capabilities", {
                    {"textDocumentSync", 1},          // 全量同步
                    {"completionProvider", json::object()},
                    {"hoverProvider", true}
                }},
                {"serverInfo", {
                    {"name", "taco-lsp"},
                    {"version", "0.1.0"}
                }}
            });

        } else if (method == "initialized") {
            // 通知：不需要回复

        } else if (method == "textDocument/didOpen") {
            handle_did_open(params);

        } else if (method == "textDocument/didChange") {
            handle_did_change(params);

        } else if (method == "textDocument/didClose") {
            auto uri = params["textDocument"]["uri"].get<std::string>();
            open_documents.erase(uri);

        } else if (method == "textDocument/completion") {
            send_response(id, handle_completion(params));

        } else if (method == "textDocument/hover") {
            auto result = handle_hover(params);
            send_response(id, result.is_null() ? json(nullptr) : result);

        } else if (method == "shutdown") {
            send_response(id, nullptr);

        } else if (method == "exit") {
            break;

        } else if (!id.is_null()) {
            // 未知请求：返回错误
            send_error(id, -32601, "Method not found: " + method);
        }
        // 未知通知（没有 id）：忽略
    }

    return 0;
}
```

---

## 38.4 自动补全与错误提示

把 Language Server 和 VSCode 插件连起来，需要写一个 TypeScript 的 client 端：

```typescript
// client/src/extension.ts
import * as path from 'path';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
    // Language Server 可执行文件的路径
    const serverPath = context.asAbsolutePath(
        path.join('server', 'build', 'taco-lsp')
    );

    const serverOptions: ServerOptions = {
        command: serverPath,
        transport: TransportKind.stdio   // 用 stdin/stdout 通信
    };

    const clientOptions: LanguageClientOptions = {
        // 只对 .taco 文件激活
        documentSelector: [{ scheme: 'file', language: 'taco' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.taco')
        }
    };

    client = new LanguageClient(
        'taco-language-server',
        'Taco Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
    console.log('Taco Language Server started.');
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) return undefined;
    return client.stop();
}
```

当用户打开 `.taco` 文件时，VSCode 自动启动 `taco-lsp` 进程，通过 stdin/stdout 建立 LSP 连接。之后：

- 每次文件内容变化 → 触发诊断 → 错误实时显示在编辑器里
- 按 `Ctrl+Space` → 请求补全 → 出现关键字和内置函数列表
- 鼠标悬停在内置函数上 → 显示文档字符串

效果大致如下：

```
// 写错了 elseif 的拼写：
if (x > 10) {
    print("big");
} elsief (x > 5) {       // ← 红色波浪线："Unexpected token 'elsief'"
    print("medium");
}

// 输入 fetch 后按 Ctrl+Space：
fetch                    // → 补全建议：fetchUrl
```

---

## 38.5 发布到 VSCode Marketplace

### 安装发布工具

```bash
npm install -g @vscode/vsce
```

### 打包插件

```bash
cd taco-language
npm install
npm run compile
# 编译 Language Server
cd server && cmake -B build && cmake --build build && cd ..
# 打包成 .vsix 文件
vsce package
# 生成 taco-language-0.1.0.vsix
```

`.vsix` 是 VSCode 插件的包格式，可以直接在 VSCode 里安装（Extensions → Install from VSIX）。

### 发布到 Marketplace

1. 在 https://marketplace.visualstudio.com 创建账号
2. 在 Azure DevOps 创建 Personal Access Token（PAT），权限选 Marketplace → Manage
3. 登录：`vsce login your-publisher-name`
4. 发布：`vsce publish`

发布后，任何人都可以在 VSCode 的扩展市场搜索 "Taco" 并安装。

### 在 package.json 里补充元信息

发布前完善插件的元信息：

```json
{
    "name": "taco-language",
    "displayName": "Taco Language",
    "description": "Syntax highlighting and language support for Taco (.taco) scripting language",
    "version": "0.1.0",
    "publisher": "your-publisher-name",
    "license": "MIT",
    "repository": {
        "type": "git",
        "url": "https://github.com/your-name/taco"
    },
    "bugs": {
        "url": "https://github.com/your-name/taco/issues"
    },
    "icon": "images/taco-icon.png",
    "keywords": ["taco", "scripting", "language"],
    "categories": ["Programming Languages"]
}
```

---

## 小结

**TextMate grammar**（`taco.tmLanguage.json`）是语法高亮的核心，用正则表达式规则把代码里的不同元素打上 scope 标签，主题根据 scope 决定颜色。不需要 C++，纯配置文件。

**Language Server Protocol（LSP）** 把语言智能和编辑器解耦。Language Server 是一个独立进程，通过 stdin/stdout 用 JSON-RPC 和编辑器通信。主要功能：

- `textDocument/publishDiagnostics`：推送错误信息（实时红线）
- `textDocument/completion`：提供自动补全列表
- `textDocument/hover`：悬停显示文档

Taco 的 Language Server 用 C++ 实现，内部复用 Lexer 和 Parser 来做语法错误检测，这体现了把解释器模块化的价值。

**VSCode 插件**：TypeScript 写的 client 负责启动 Language Server 进程，把 LSP 消息转发给 VSCode 的 Extension API。用户无感知，打开 `.taco` 文件就自动生效。

整个工具链：写一次 Language Server（C++），所有支持 LSP 的编辑器（VSCode、Neovim、Emacs、Helix……）都能用。这是现代语言工具链的标准做法。
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
# 第四十章：接下来去哪里

---

这是全书的最后一章。

不讲新的 C++ 特性，不加新的 Taco 功能。回头看走过的路，然后往前看：C++ 还有哪些没讲到的重要内容，Taco 还能往哪里走，这门语言的未来是什么样的。

---

## 40.1 没讲到的重要内容

这本书覆盖了现代 C++ 的核心，但 C++ 是一门极其宽广的语言，还有很多重要的东西没有触及。

### 异常处理（Exception Handling）

Taco 的错误处理哲学是"直接崩溃"，所以 C++ 的异常机制在 Taco 内部只是作为内部错误传播的工具，没有专门讲。但在真实项目里，异常是主流的错误处理机制之一。

```cpp
// 抛出异常
void parse_number(const std::string& s) {
    try {
        size_t pos;
        double v = std::stod(s, &pos);
        if (pos != s.size()) {
            throw std::invalid_argument("not a number: " + s);
        }
    } catch (const std::invalid_argument& e) {
        // 处理特定类型的异常
        std::cerr << "Error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        // 处理所有标准异常
        std::cerr << "Unexpected error: " << e.what() << "\n";
    } catch (...) {
        // 处理所有异常（不推荐，除非是最外层的兜底）
        std::cerr << "Unknown error.\n";
    }
}
```

C++ 异常的争议：有些代码库（Google、游戏引擎）禁用异常，用错误码或者 `std::expected`（C++23）替代。理由是：异常会让控制流变得隐式，难以推理；在嵌入式、实时系统里，异常的开销不可接受。

哪种方式更好，至今仍有争论。了解两种方式，根据项目需求做选择。

**推荐阅读**：《A Tour of C++》（Bjarne Stroustrup）第 3 章；《Effective C++》条款 8-9（Scott Meyers）。

### 协程（Coroutine，C++20）

C++20 加入了语言级别的协程支持。`co_await`、`co_yield`、`co_return` 三个关键字，让异步代码写起来像同步代码。

第三十五章介绍 Asio 时提到了协程，但没有深讲。C++20 协程是一个相当复杂的特性——标准只提供了底层机制（promise type、awaitable concept），没有提供高层抽象，需要库来封装（Asio、cppcoro、libcopp）。

```cpp
// C++20 协程（需要 Asio 1.20+ 或其他协程库）
asio::awaitable<std::string> fetch_and_parse(std::string url) {
    auto body = co_await async_fetch(url);     // 暂停，等待 HTTP 响应
    auto json = co_await async_parse(body);    // 暂停，等待 JSON 解析
    co_return json["result"].get<std::string>();
}
```

**推荐阅读**：《C++20: The Complete Guide》（Nicolai Josuttis）；David Mazières 的《My tutorial and take on C++20 coroutines》。

### 模块（Module，C++20）

C++20 引入了语言级别的模块系统，解决头文件的种种问题：

- **编译速度**：头文件每次被 include 都重新编译；模块只编译一次
- **宏污染**：头文件里的宏会泄漏到所有 include 它的地方；模块不会
- **循环依赖**：头文件容易形成循环依赖；模块有明确的导入关系

```cpp
// 定义一个模块（token.cppm）
export module taco.token;

import std;  // 导入标准库

export enum class TokenType { ... };  // export 关键字：对外可见
export struct Token { ... };

// 内部实现（不 export 的不对外可见）
static std::string escape_string(const std::string& s) { ... }
```

```cpp
// 使用模块
import taco.token;    // 导入，不是 #include

Token t;
```

模块在 C++20 编译器里已经可用，但工具链支持（CMake、构建系统、IDE）还在完善中。

**推荐**：等编译器和工具链的支持成熟后迁移（大概 2025-2026 年是比较好的时机）。

### `std::expected`（C++23）

`std::expected<T, E>` 是一个"要么有值，要么有错误"的类型，是异常的一个替代方案：

```cpp
// C++23
std::expected<double, std::string> parse_number(const std::string& s) {
    try {
        return std::stod(s);
    } catch (...) {
        return std::unexpected("not a number: " + s);
    }
}

// 使用
auto result = parse_number("42.0");
if (result) {
    std::cout << "Parsed: " << *result << "\n";
} else {
    std::cerr << "Error: " << result.error() << "\n";
}
```

比异常更显式（函数签名里写明了可能出错），比错误码更方便（可以用 monadic 操作链式处理）：

```cpp
auto final = parse_number(input)
    .and_then([](double v) -> std::expected<int, std::string> {
        if (v < 0) return std::unexpected("negative number");
        return static_cast<int>(v);
    })
    .transform([](int n) { return n * 2; });
```

### ranges（C++20）

第二十二章提到了 ranges，但只是预览。`std::ranges` 提供了更好的算法接口，以及 views（惰性求值的视图）：

```cpp
#include <ranges>
#include <vector>
#include <iostream>

std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// 传统算法（三步走）
std::vector<int> temp;
std::copy_if(nums.begin(), nums.end(), std::back_inserter(temp),
             [](int n) { return n % 2 == 0; });
std::vector<int> result;
std::transform(temp.begin(), temp.end(), std::back_inserter(result),
               [](int n) { return n * n; });

// ranges（pipeline 风格，惰性求值）
auto result2 = nums
    | std::views::filter([](int n) { return n % 2 == 0; })
    | std::views::transform([](int n) { return n * n; });

for (int n : result2) std::cout << n << " ";
// 4 16 36 64 100
```

注意：`result2` 不是一个容器，而是一个惰性视图（view）——遍历时才真正计算，不创建中间容器。这和 Taco 的 `.filter { }.map { }` 在语义上接近，但 Taco 的版本是严格求值的（每步都创建新 array）。

---

## 40.2 Taco 的下一步：垃圾回收、字节码虚拟机、JIT

上一章简要介绍了 Taco 可以扩展的方向。这里多说一点关于实现路径的细节，供有兴趣继续做的读者参考。

### 从 shared_ptr 迁移到 GC

Taco 目前用 `shared_ptr` 管理值对象。要替换成 GC，大致步骤：

**Step 1**：把所有堆分配的 Taco 对象（TacoArray、TacoMap、TacoFunction、Environment）注册到一个全局的 GC 堆里：

```cpp
class GcHeap {
public:
    // 分配一个新的 GC 对象
    template<typename T, typename... Args>
    T* alloc(Args&&... args) {
        auto* obj = new T(std::forward<Args>(args)...);
        all_objects_.insert(obj);
        maybe_collect();
        return obj;
    }

    void collect();  // 触发 mark-sweep

private:
    std::unordered_set<GcObject*> all_objects_;
    std::unordered_set<GcObject*> roots_;  // 根对象集合（全局变量、栈上的值）
};
```

**Step 2**：TacoValue 改用裸指针（不是 `shared_ptr`），由 GcHeap 统一管理：

```cpp
// 之前：TacoValue 持有 shared_ptr<TacoArray>
// 之后：TacoValue 持有 TacoArray*（GC 管理的裸指针）
```

**Step 3**：实现 mark-sweep：

```cpp
void GcHeap::collect() {
    // Mark phase：从根集合出发，标记所有可达对象
    for (GcObject* root : roots_) {
        mark(root);
    }

    // Sweep phase：回收所有未标记的对象
    for (auto it = all_objects_.begin(); it != all_objects_.end(); ) {
        if (!(*it)->is_marked()) {
            delete *it;
            it = all_objects_.erase(it);
        } else {
            (*it)->unmark();  // 清除标记，为下次 GC 准备
            ++it;
        }
    }
}
```

这是 Lua 5.0 之前的做法。Lua 5.1 开始用增量 GC（interleave mark-sweep 和程序执行，减少停顿），Lua 5.4 加了分代 GC。

**推荐阅读**：《Crafting Interpreters》（Robert Nystrom）第 26 章：Garbage Collection。这本书可能是实现解释器最好的入门书，免费在线阅读。

### 字节码虚拟机

把 Taco 改成字节码 VM 是一个很大的工程，但思路清晰：

**Step 1**：定义字节码指令集：

```cpp
enum class OpCode : uint8_t {
    // 常量
    LOAD_CONST,    // 从常量池加载
    LOAD_NIL,
    LOAD_TRUE,
    LOAD_FALSE,

    // 局部变量
    LOAD_LOCAL,    // 从局部变量槽加载
    STORE_LOCAL,

    // 全局变量
    LOAD_GLOBAL,   // 从全局环境加载
    STORE_GLOBAL,

    // 算术
    ADD, SUB, MUL, DIV, MOD, POW,
    NEG,           // 取反

    // 比较
    EQ, NEQ, LT, LE, GT, GE,

    // 控制流
    JUMP,          // 无条件跳转
    JUMP_IF_FALSE, // 条件跳转
    LOOP,          // 回跳（用于循环）

    // 函数
    CALL,          // 调用函数
    RETURN,

    // 其他
    PRINT,
    POP,           // 丢弃栈顶
};
```

**Step 2**：写编译器（AST → 字节码）：

```cpp
class Compiler {
public:
    Chunk compile(const Program& ast);

private:
    void compile_expr(const Expr& expr);
    void compile_stmt(const Stmt& stmt);

    void emit(OpCode op);
    void emit(OpCode op, uint8_t arg);
    int emit_jump(OpCode op);     // 发射跳转指令，返回位置用于回填
    void patch_jump(int offset);  // 回填跳转目标

    Chunk current_chunk_;
};
```

**Step 3**：写 VM（执行字节码）：

```cpp
class VM {
public:
    InterpretResult run(const Chunk& chunk);

private:
    const uint8_t* ip_;          // 指令指针（instruction pointer）
    std::vector<TacoValue> stack_;  // 操作数栈
    std::vector<TacoValue> globals_; // 全局变量

    TacoValue pop() { auto v = stack_.back(); stack_.pop_back(); return v; }
    void push(TacoValue v) { stack_.push_back(std::move(v)); }
};

InterpretResult VM::run(const Chunk& chunk) {
    ip_ = chunk.code.data();

    for (;;) {
        OpCode op = static_cast<OpCode>(*ip_++);

        switch (op) {
        case OpCode::LOAD_CONST: {
            uint8_t idx = *ip_++;
            push(chunk.constants[idx]);
            break;
        }
        case OpCode::ADD: {
            auto b = pop();
            auto a = pop();
            push(TacoValue(a.as_number() + b.as_number()));
            break;
        }
        case OpCode::JUMP_IF_FALSE: {
            uint16_t offset = (ip_[0] << 8) | ip_[1];
            ip_ += 2;
            if (!pop().as_bool()) ip_ += offset;
            break;
        }
        case OpCode::RETURN:
            return InterpretResult::OK;
        // ...
        }
    }
}
```

**推荐阅读**：《Crafting Interpreters》第二部分（第 14-30 章）完整实现了一个字节码 VM，是目前最好的实践参考。

---

## 40.3 网络编程深入：Boost.Asio、gRPC

第八部分只是网络编程的入门。如果想深入：

### Boost.Asio 与高并发服务器

独立 Asio 是 Boost.Asio 的子集。Boost.Asio 功能更完整：

```cpp
// 一个用 Asio 协程实现的 echo 服务器
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/awaitable.hpp>
using namespace boost::asio;

awaitable<void> handle_client(ip::tcp::socket socket) {
    char buf[1024];
    for (;;) {
        // 读数据（暂停协程，等数据到达）
        size_t n = co_await socket.async_read_some(
            buffer(buf), use_awaitable
        );
        // 回写（echo）
        co_await async_write(socket, buffer(buf, n), use_awaitable);
    }
}

awaitable<void> listener(io_context& io) {
    ip::tcp::acceptor acceptor(io, {ip::tcp::v4(), 8080});
    for (;;) {
        auto socket = co_await acceptor.async_accept(use_awaitable);
        // 为每个连接启动一个协程
        co_spawn(io, handle_client(std::move(socket)), detached);
    }
}

int main() {
    io_context io;
    co_spawn(io, listener(io), detached);
    io.run();  // 单线程处理所有连接！
}
```

这个 echo 服务器用单线程处理所有客户端连接（协程切换，不是线程切换），可以轻松处理数万个并发连接。

### gRPC：微服务 RPC

gRPC 是 Google 开发的 RPC 框架，基于 HTTP/2 和 Protobuf：

```protobuf
// taco_service.proto
syntax = "proto3";

service TacoRunner {
    rpc Run(RunRequest) returns (RunResponse);
    rpc RunStream(RunRequest) returns (stream OutputLine);
}

message RunRequest {
    string code = 1;
}

message RunResponse {
    bool success = 1;
    string output = 2;
    string error = 3;
}

message OutputLine {
    string line = 1;
}
```

生成 C++ 代码后，可以把 Taco 解释器包装成一个网络服务——别人发来 Taco 代码，服务端执行并返回结果。这就是云 IDE（比如 Replit）的基本思路。

---

## 40.4 值得读的书与资源

把这本书读完，是一个起点，不是终点。下面是继续深入的资源。

### C++ 书籍

**入门到进阶**

- **《A Tour of C++》**（Bjarne Stroustrup）——C++ 之父写的概览，简洁，现代，适合有基础的读者。第三版覆盖 C++20。

- **《C++ Primer》**（Lippman 等）——最系统的 C++ 入门书，适合想彻底学清楚每个细节的读者。

**进阶**

- **《Effective C++》+《More Effective C++》**（Scott Meyers）——55 条实践建议，每条都有原理解释。经典，虽然部分条款已经被现代 C++ 取代，但思维方式仍然有价值。

- **《Effective Modern C++》**（Scott Meyers）——专门针对 C++11/14 的 42 条建议，覆盖 `auto`、move semantics、lambda、智能指针、并发。

- **《C++ Templates: The Complete Guide》**（Vandevoorde、Josuttis）——模板的圣经。

**高性能**

- **《C++ High Performance》**（Björn Andrist、Viktor Sehr）——写出高性能 C++ 代码的方法：缓存友好性、SIMD、并行算法、profile 工具。

### 编译器与解释器

- **《Crafting Interpreters》**（Robert Nystrom）——免费在线阅读（craftinginterpreters.com）。实现两个 Lox 解释器：一个树遍历（Java），一个字节码 VM（C）。是 Taco 最直接的进阶路线。

- **《Writing a Compiler in Go》**（Thorsten Ball）——相同作者的系列（《Writing An Interpreter In Go》是前传），Go 语言，很清晰。

- **《Engineering a Compiler》**（Cooper、Torczon）——编译原理教材，系统、严谨，比 龙书（CLRS）更现代。

### 并发

- **《C++ Concurrency in Action》**（Anthony Williams）——C++ 并发编程的权威参考。作者是 C++ 并发标准的参与者。

- **《The Art of Multiprocessor Programming》**（Herlihy、Shavit）——并发数据结构的理论基础，从锁到无锁，严谨但不枯燥。

### 网络编程

- **《UNIX Network Programming》**（W. Richard Stevens）——网络编程的圣经，用 C 写，讲的是 socket API 和网络编程的每个细节。仍然是最好的底层参考。

- **《Boost.Asio C++ Network Programming》**——Asio 的实践书籍。

### 在线资源

- **cppreference.com**——C++ 标准库的权威参考，每个函数都有说明、示例和异常规范。书签收好。

- **Compiler Explorer（godbolt.org）**——把 C++ 代码编译成汇编，实时看，对理解编译器优化极有帮助。

- **C++ Weekly（Jason Turner，YouTube）**——每周一个 C++ 小技巧或特性讲解，几分钟一集，质量高。

- **cppcon 演讲**——YouTube 上有大量 CppCon 的录播，很多都是顶级 C++ 工程师的分享。

---

## 40.5 C++ 的未来：C++23 及之后

C++ 标准每三年更新一次：C++11 → C++14 → C++17 → C++20 → C++23 → C++26……

### C++23 已经发布的主要内容

**`std::expected<T, E>`**：函数式错误处理，前面介绍过。

**`std::print` 和 `std::println`**：基于 fmtlib 的格式化输出，终于进入标准：

```cpp
#include <print>
std::println("Hello, {}!", name);  // 不需要 "\n"
std::print("{:>10.2f}", 3.14159);  // 格式化选项
```

**`std::mdspan`**：多维数组视图，高性能数值计算的基础：

```cpp
// 把一维数组当多维数组用，零开销
float data[6] = {1, 2, 3, 4, 5, 6};
auto matrix = std::mdspan(data, 2, 3);  // 2x3 矩阵
matrix[1, 2] = 42.0f;  // 访问 row=1, col=2
```

**`std::flat_map` 和 `std::flat_set`**：基于排序 vector 的有序映射和集合，比 `std::map` 和 `std::set` 有更好的缓存性能（连续内存），但插入较慢：

```cpp
std::flat_map<std::string, int> m;
m["hello"] = 1;
// 内部是两个排序 vector：keys 和 values，对应位置
```

**Ranges 扩展**：更多 range 算法和 views，`std::views::zip`、`std::views::slide`、`std::ranges::fold_left`……

### C++26（进行中）

C++26 的一些重要提案：

**反射（Reflection）**：在编译期检查和操作类型的结构。可以不用手写序列化代码——直接反射出类的所有字段，自动生成 JSON 序列化/反序列化：

```cpp
// 未来（C++26 reflection 的想象写法）
struct Point { int x, y; };

// 自动生成：不需要手写 to_json/from_json
json j = reflect_to_json(Point{1, 2});  // {"x": 1, "y": 2}
```

**执行器（Executors / senders/receivers）**：统一的异步执行模型，可能最终合并 Asio、线程池、协程等不同的并发方式。这是 C++ 社区争论最激烈的提案之一，进展缓慢但很重要。

**Contracts**：前置条件、后置条件、不变量的语言级别支持：

```cpp
// 想象的 contracts 语法
double sqrt(double x)
    pre (x >= 0.0)     // 前置条件
    post(result >= 0.0)  // 后置条件
{
    return std::sqrt(x);
}
```

### C++ 的地位

C++ 不会消失。它占据的领域——系统软件、游戏引擎、高频交易、嵌入式、科学计算——对性能和控制有极高要求，Rust 在部分场景里是竞争者，但 C++ 庞大的代码库、工具链、生态系统的惯性让它仍然是这些领域的主流。

每次有人说"C++ 要被 XXX 替代了"，C++ 又发布了一个新标准，变得更现代、更安全、更好用。

---

## 最后

这本书建立了一个框架：从零实现一门语言的解释器。过程里学到的每一个 C++ 特性，都有真实的动机——不是"学这个因为重要"，而是"遇到了这个问题，这个特性解决了它"。

Taco 的七次进化对应的知识路径：

```
基础语法 → 类与继承 → 多态 → 智能指针/RAII → STL → 模板 → 并发 → 网络
    ↕           ↕         ↕          ↕             ↕        ↕       ↕       ↕
  词法器  →   AST  →  求值器  →   闭包/作用域 → 标准库 → 重构  → REPL  → HTTP
```

每条横线上面是 C++ 知识，下面是 Taco 的对应进化。这个对应关系不是偶然的——语言特性存在是因为有真实的工程问题，了解问题才能理解特性。

C++ 是一门工具，目的是解决问题。工具越熟，解决问题的方式越多，越优雅，越高效。

---

🌮

```taco
// 写在最后
var 答案 = 🌮 "Will I become a good C++ programmer?";
print(答案);
// 🎱 It is certain.
```
