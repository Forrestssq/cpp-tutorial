# 第二十九章：项目 v5——解释器内部模板化

---

第六部分学了三章模板：函数模板、类模板、特化与 `if constexpr`。现在用这些知识对 Taco 解释器做一次内部重构——v5 不增加新的语言功能，而是改进内部实现。

v5 做三件事：

1. **用 `std::variant` 重新设计 TacoValue**：v1 里已经用过 `variant`，但随着值类型增多，需要系统地重新设计
2. **用模板重构环境（Environment）**：让符号表可以存储不同类型的映射
3. **用模板实现通用的 AST 节点访问器（Visitor）**：用模板取代 `dynamic_cast` 链，实现干净的 Visitor 模式

重构之后，代码更干净，扩展更容易，性能略有提升。

---

## 29.1 用 std::variant 实现 Taco 值类型

### v4 的 TacoValue 回顾

v4 的 `TacoValue` 是这样的：

```cpp
// v4 的实现——已经可以工作，但有几个问题
struct TacoArray;
struct TacoMap;
struct TacoFunction;
struct TacoNative;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>
>;
```

这个设计工作得很好，但有几个摩擦点：
- 辅助函数（`is_truthy`、`value_to_string`、运算符）全部堆在 `value.cpp` 里，越来越难维护
- `std::get_if<T>` 的使用散落在各处，同样的模式重复很多次
- 需要一个统一的"访问"（visit）接口，对任意类型的值都能做某种操作

`std::visit` 是 `variant` 的标准配套工具，配合模板可以写出非常整洁的值处理代码。

### std::visit 的用法

`std::visit` 接收一个**访问者**（visitor）和一个 `variant`，用 `variant` 当前持有的实际类型调用访问者：

```cpp
#include <variant>

using Value = std::variant<int, double, std::string>;

Value v = 42;

// 访问者：一个有 operator() 的对象
struct Printer {
    void operator()(int n)                { std::cout << "int: " << n << "\n"; }
    void operator()(double d)             { std::cout << "double: " << d << "\n"; }
    void operator()(const std::string& s) { std::cout << "string: " << s << "\n"; }
};

std::visit(Printer{}, v);  // 输出：int: 42

v = 3.14;
std::visit(Printer{}, v);  // 输出：double: 3.14

v = std::string("hello");
std::visit(Printer{}, v);  // 输出：string: hello
```

访问者可以用一个重载了多个 `operator()` 的结构体，但写起来有点繁琐。C++17 里有一个更简洁的技巧：**overloaded**：

```cpp
// 把多个 lambda 合并成一个重载集合
template<typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;  // 把所有基类的 operator() 引入当前作用域
};

// C++17 的推导指南：让编译器推导模板参数
template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// 使用
Value v = 42;
std::visit(overloaded{
    [](int n)                { std::cout << "int: " << n << "\n"; },
    [](double d)             { std::cout << "double: " << d << "\n"; },
    [](const std::string& s) { std::cout << "string: " << s << "\n"; },
}, v);
```

`overloaded` 继承自所有传入的 lambda 类型，把它们的 `operator()` 全都引入，形成一个重载集。这是模板继承的实际应用。

### 重新设计 value.h

把 `overloaded` 工具和 TacoValue 的辅助函数整合进来：

```cpp
// value.h（v5 版本）
#pragma once
#include <variant>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <stdexcept>

// ── 前向声明 ──────────────────────────────────────────────────
struct TacoArray;
struct TacoMap;
struct TacoFunction;
struct TacoNative;
class  Environment;

// ── TacoValue：所有 Taco 值类型的 variant ─────────────────────
using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>
>;

// ── overloaded 工具 ───────────────────────────────────────────
// 把多个 lambda 合并成一个可以重载的访问者
template<typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// ── 辅助函数 ─────────────────────────────────────────────────
std::string  taco_to_string(const TacoValue& v);
bool         taco_is_truthy(const TacoValue& v);
bool         taco_equal(const TacoValue& a, const TacoValue& b);
TacoValue    taco_add(const TacoValue& a, const TacoValue& b);  // + 运算，支持数字和字符串拼接

// 类型检查辅助
template<typename T>
bool taco_is(const TacoValue& v) {
    return std::holds_alternative<T>(v);
}

// 安全提取（提取失败抛运行时错误）
template<typename T>
const T& taco_get(const TacoValue& v, const std::string& context = "") {
    if (auto* p = std::get_if<T>(&v)) {
        return *p;
    }
    throw std::runtime_error(
        context.empty()
            ? "Type error"
            : "Type error in " + context
    );
}

template<typename T>
T& taco_get_mut(TacoValue& v, const std::string& context = "") {
    if (auto* p = std::get_if<T>(&v)) {
        return *p;
    }
    throw std::runtime_error(
        context.empty()
            ? "Type error"
            : "Type error in " + context
    );
}
```

模板函数 `taco_is<T>` 和 `taco_get<T>` 比直接写 `holds_alternative` 和 `get_if` 清晰得多，而且集中了错误信息的格式。

### 辅助函数的实现

```cpp
// value.cpp（v5 版本）
#include "value.h"
#include "taco_array.h"
#include "taco_map.h"
#include "taco_function.h"

std::string taco_to_string(const TacoValue& v) {
    return std::visit(overloaded{
        [](double d) -> std::string {
            // 整数时不显示小数点
            if (d == static_cast<long long>(d) && !std::isinf(d))
                return std::to_string(static_cast<long long>(d));
            return std::to_string(d);
        },
        [](const std::string& s) -> std::string {
            return s;
        },
        [](bool b) -> std::string {
            return b ? "true" : "false";
        },
        [](std::nullptr_t) -> std::string {
            return "nil";
        },
        [](const std::shared_ptr<TacoArray>& arr) -> std::string {
            return arr->to_string();
        },
        [](const std::shared_ptr<TacoMap>& map) -> std::string {
            return map->to_string();
        },
        [](const std::shared_ptr<TacoFunction>& fn) -> std::string {
            return "<func " + fn->name + ">";
        },
        [](const std::shared_ptr<TacoNative>&) -> std::string {
            return "<native>";
        },
    }, v);
}

bool taco_is_truthy(const TacoValue& v) {
    return std::visit(overloaded{
        [](double d)                              { return d != 0.0; },
        [](const std::string& s)                 { return !s.empty(); },
        [](bool b)                               { return b; },
        [](std::nullptr_t)                       { return false; },
        [](const std::shared_ptr<TacoArray>& a)  { return true; },
        [](const std::shared_ptr<TacoMap>& m)    { return true; },
        [](const std::shared_ptr<TacoFunction>&) { return true; },
        [](const std::shared_ptr<TacoNative>&)   { return true; },
    }, v);
}
```

这种写法的好处是：**所有类型必须有对应的处理分支**。如果将来 `TacoValue` 加了新的类型，但 `overloaded` 里没有新分支，编译器会直接报错——强制你不能忘记处理新类型。这是 `variant` + `visit` 相比用继承 + 虚函数的一个优势：你不能漏掉某个类型。

---

## 29.2 用模板重构符号表与环境

### v4 的 Environment

v4 的环境是一个 `unordered_map<string, TacoValue>` 加上父环境的 `shared_ptr`：

```cpp
class Environment {
public:
    void define(const std::string& name, TacoValue value);
    TacoValue get(const std::string& name);
    void assign(const std::string& name, TacoValue value);

    std::shared_ptr<Environment> parent;

private:
    std::unordered_map<std::string, TacoValue> vars_;
};
```

这个设计是够用的，但有一个问题：`get` 找不到变量时怎么办？要么抛异常，要么返回 nil——两种方案都有缺点（抛异常开销大，返回 nil 会掩盖错误）。

v5 用模板改进 `get`，返回 `std::optional<TacoValue>`：

```cpp
// environment.h（v5 版本）
#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include "value.h"

class Environment {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr)
        : parent_(std::move(parent)) {}

    // 在当前作用域定义新变量
    void define(const std::string& name, TacoValue value) {
        vars_[name] = std::move(value);
    }

    // 查找变量：当前作用域找不到就往上找，找到返回值，找不到返回空
    std::optional<TacoValue> lookup(const std::string& name) const {
        auto it = vars_.find(name);
        if (it != vars_.end()) {
            return it->second;
        }
        if (parent_) {
            return parent_->lookup(name);
        }
        return std::nullopt;
    }

    // 获取变量（找不到抛错误）
    TacoValue get(const std::string& name) const {
        auto result = lookup(name);
        if (!result) {
            throw std::runtime_error("Undefined variable: " + name);
        }
        return *result;
    }

    // 赋值：找到变量的作用域，在那里修改
    bool assign(const std::string& name, TacoValue value) {
        auto it = vars_.find(name);
        if (it != vars_.end()) {
            it->second = std::move(value);
            return true;
        }
        if (parent_) {
            return parent_->assign(name, std::move(value));
        }
        return false;  // 变量不存在
    }

    // 创建子作用域
    std::shared_ptr<Environment> make_child() {
        return std::make_shared<Environment>(shared_from_this());
    }

    std::shared_ptr<Environment> parent() const { return parent_; }

private:
    std::unordered_map<std::string, TacoValue> vars_;
    std::shared_ptr<Environment> parent_;
};
```

`make_child()` 的实现用到了 `shared_from_this()`，这要求 `Environment` 继承自 `std::enable_shared_from_this<Environment>`：

```cpp
class Environment : public std::enable_shared_from_this<Environment> {
    // ... 同上
};
```

`enable_shared_from_this` 让对象可以安全地获取指向自己的 `shared_ptr`，不会创建第二套引用计数。这是在对象内部需要 `shared_ptr<this>` 时的标准做法。

### 模板化的注册接口

内置函数注册也可以用模板简化。v4 里每次注册都要手动包装 lambda：

```cpp
// v4 的手动注册方式
env->define("print", TacoValue{std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args) -> TacoValue {
        // ...
        return nullptr;
    }
)});
```

用模板写一个辅助函数，让注册更简洁：

```cpp
// 注册内置函数的辅助模板
template<typename Fn>
void register_builtin(Environment& env, const std::string& name, Fn fn) {
    env.define(name, TacoValue{
        std::make_shared<TacoNative>(std::function<TacoValue(std::vector<TacoValue>)>(fn))
    });
}

// 使用
void register_builtins(Environment& env) {
    register_builtin(env, "print", [](std::vector<TacoValue> args) -> TacoValue {
        for (auto& a : args) std::cout << taco_to_string(a);
        std::cout << "\n";
        return nullptr;
    });

    register_builtin(env, "type", [](std::vector<TacoValue> args) -> TacoValue {
        if (args.empty()) return std::string("nil");
        return std::visit(overloaded{
            [](double)                            -> TacoValue { return std::string("number"); },
            [](const std::string&)                -> TacoValue { return std::string("string"); },
            [](bool)                              -> TacoValue { return std::string("bool"); },
            [](std::nullptr_t)                    -> TacoValue { return std::string("nil"); },
            [](const std::shared_ptr<TacoArray>&) -> TacoValue { return std::string("array"); },
            [](const std::shared_ptr<TacoMap>&)   -> TacoValue { return std::string("map"); },
            [](const std::shared_ptr<TacoFunction>&) -> TacoValue { return std::string("func"); },
            [](const std::shared_ptr<TacoNative>&)   -> TacoValue { return std::string("func"); },
        }, args[0]);
    });

    register_builtin(env, "len", [](std::vector<TacoValue> args) -> TacoValue {
        if (args.empty()) throw std::runtime_error("len() requires an argument");
        return std::visit(overloaded{
            [](const std::string& s)              -> TacoValue { return (double)s.size(); },
            [](const std::shared_ptr<TacoArray>& a) -> TacoValue { return (double)a->elements.size(); },
            [](const std::shared_ptr<TacoMap>& m)   -> TacoValue { return (double)m->entries.size(); },
            [](const auto&) -> TacoValue {
                throw std::runtime_error("len() only works on string, array, and map");
                return nullptr;
            },
        }, args[0]);
    });
}
```

`register_builtin` 用 `Fn` 接收任意可调用类型（lambda、函数指针等），内部转换成 `std::function`。这比每次都手写 `make_shared<TacoNative>` 简洁很多。

---

## 29.3 用模板实现通用的 AST 节点访问器

### 访问者模式（Visitor Pattern）

v2 里用了一个简化版：让每个 AST 节点有 `evaluate()` 虚函数。这个设计有个问题：如果需要对同一棵 AST 做多种操作（求值、打印、类型检查、代码生成……），就要在每个节点类里加很多虚函数，节点类会越来越臃肿。

标准的访问者模式把操作从节点里分离出来：

- **节点类**只负责存储数据和调用 `accept(visitor)`
- **访问者类**实现对每种节点的具体操作

```
AST 节点：NumberLiteral, BinaryExpr, IfStmt, ...
  每个节点：accept(Visitor&)
  
访问者：Evaluator, Printer, TypeChecker, ...
  每个访问者：对每种节点实现 visit() 方法
```

这样，新增操作只需要写一个新的访问者类，不需要修改任何节点类。

### 用模板实现访问者基类

在不用模板的情况下，访问者模式需要手动列出所有节点类型：

```cpp
// 传统方式：为每种节点类型写一个 visit 函数
struct Visitor {
    virtual TacoValue visit(const NumberLiteralNode&) = 0;
    virtual TacoValue visit(const StringLiteralNode&) = 0;
    virtual TacoValue visit(const BinaryExprNode&) = 0;
    virtual TacoValue visit(const IfStmtNode&) = 0;
    // ... 每增加一种节点，这里就要加一行
};
```

用模板可以稍微改进——用 `if constexpr` 配合类型特征，让节点的 `accept` 函数更通用：

```cpp
// ast.h（v5 的新方式）
#pragma once
#include <memory>
#include <string>
#include "value.h"

// 前向声明访问者
class Evaluator;
class ASTPrinter;

// ── 基类 ──────────────────────────────────────────────────────
struct ASTNode {
    int line = 0;    // 出错时的行号
    virtual ~ASTNode() = default;

    // 求值：由子类实现
    virtual TacoValue evaluate(Evaluator& eval) const = 0;

    // 打印 AST（调试用）
    virtual std::string dump(int indent = 0) const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

// ── 表达式基类 ────────────────────────────────────────────────
struct Expr : ASTNode {};
using ExprPtr = std::unique_ptr<Expr>;

// ── 语句基类 ─────────────────────────────────────────────────
struct Stmt : ASTNode {};
using StmtPtr = std::unique_ptr<Stmt>;

// ── 字面量节点 ────────────────────────────────────────────────

struct NumberLiteralNode : Expr {
    double value;
    explicit NumberLiteralNode(double v, int ln = 0) : value(v) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Number(" + std::to_string(value) + ")";
    }
};

struct StringLiteralNode : Expr {
    std::string value;
    explicit StringLiteralNode(std::string v, int ln = 0) : value(std::move(v)) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "String(\"" + value + "\")";
    }
};

struct BoolLiteralNode : Expr {
    bool value;
    explicit BoolLiteralNode(bool v, int ln = 0) : value(v) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + std::string(value ? "True" : "False");
    }
};

struct NilLiteralNode : Expr {
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Nil";
    }
};

// ── 变量节点 ─────────────────────────────────────────────────

struct VariableNode : Expr {
    std::string name;
    explicit VariableNode(std::string n, int ln = 0) : name(std::move(n)) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Var(" + name + ")";
    }
};

// ── 二元表达式 ────────────────────────────────────────────────

struct BinaryExprNode : Expr {
    ExprPtr left;
    std::string op;
    ExprPtr right;

    BinaryExprNode(ExprPtr l, std::string o, ExprPtr r, int ln = 0)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) { line = ln; }

    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Binary(" + op + ")\n"
             + left->dump(indent + 2) + "\n"
             + right->dump(indent + 2);
    }
};
```

### 用 if constexpr 实现通用的 dump

为了演示 `if constexpr` 的实际应用，来写一个通用的缩进打印函数：

```cpp
// 通用的 AST 打印辅助：根据节点类型选择不同的打印方式
template<typename Node>
std::string dump_node(const Node& node, int indent) {
    std::string prefix(indent, ' ');

    if constexpr (std::is_same_v<Node, NumberLiteralNode>) {
        return prefix + "Number(" + std::to_string(node.value) + ")";
    } else if constexpr (std::is_same_v<Node, StringLiteralNode>) {
        return prefix + "String(\"" + node.value + "\")";
    } else if constexpr (std::is_same_v<Node, BinaryExprNode>) {
        return prefix + "Binary(" + node.op + ")\n"
             + node.left->dump(indent + 2) + "\n"
             + node.right->dump(indent + 2);
    } else {
        // 兜底：只打印类型名
        return prefix + "[Unknown Node]";
    }
}
```

这里 `if constexpr` 根据 `Node` 的类型在编译期决定打印格式——对 `NumberLiteralNode` 访问 `.value`，对 `BinaryExprNode` 访问 `.op`、`.left`、`.right`。如果用普通 `if`，所有分支都会被编译，但 `NumberLiteralNode` 没有 `.op` 成员，会编译失败。

---

## 29.4 模板在这里解决了什么问题

回顾一下 v5 里模板的三个应用，以及它们各自解决了什么问题：

**`overloaded` + `std::visit`**

解决了 `variant` 处理代码重复的问题。每次处理 `TacoValue` 都要 `if (holds_alternative<double>)...` 一长串，`visit` + `overloaded` 把所有类型的处理集中在一个地方，而且如果漏掉某种类型，编译器会报错。

**`taco_is<T>` 和 `taco_get<T>` 模板函数**

把散落在求值器各处的 `get_if`/`holds_alternative` 调用统一成一个接口，错误信息也更清晰。`context` 参数让错误定位更精准。

**`register_builtin<Fn>` 模板函数**

把每次注册内置函数都要写的 `make_shared<TacoNative>(function<...>(lambda))` 封装起来，调用端只需要写内置函数的名字和实现 lambda。新增内置函数从十行代码变成三行。

**`if constexpr` 在 `dump_node` 里**

让一个函数可以根据节点类型采用不同的打印逻辑，而且每个分支只访问该类型实际存在的成员，不需要基类预先声明所有可能用到的成员。

---

## 29.5 性能对比：重构前后的差异

v5 的重构对性能的影响主要体现在两个地方：

### 1. `std::visit` vs `dynamic_cast` 链

v1 里用过 `dynamic_cast` 来识别节点类型：

```cpp
// v1 的方式：O(n)，每次要试每种类型直到匹配
TacoValue evaluate(const Expr* e) {
    if (auto* n = dynamic_cast<const NumberLiteralNode*>(e)) return eval_number(n);
    if (auto* n = dynamic_cast<const BinaryExprNode*>(e))   return eval_binary(n);
    // ...
}
```

v2 改成了虚函数（查 vtable 一次，`O(1)`）。`std::variant` + `std::visit` 也是 `O(1)` 的——`variant` 内部存了一个标记（通常是整数），`visit` 根据这个标记做一次跳转（类似 switch，但更安全）。

两者性能相近，但 `variant` 的好处是**不需要基类**：`TacoValue` 里的 `double`、`std::string` 不是类，不需要继承任何东西，内存布局也更紧凑。

### 2. 内存布局

用继承实现的多态值类型，每个值都需要：
- 堆上一个对象（`new`）
- 一个 vtable 指针（通常 8 字节）
- 引用计数（如果用 `shared_ptr`）

`std::variant` 的内存布局：
- 一块栈上的内存，大小等于最大成员类型的大小
- 一个标记整数（通常 1-4 字节）
- 没有堆分配，没有 vtable，没有引用计数

对于像 `double` 和 `std::string` 这样的基本类型，`variant` 直接存在栈上，比 `shared_ptr` 包装的堆对象快很多（省去了分配和引用计数的开销）。

用 `<chrono>` 做一个简单的对比测试：

```cpp
#include <chrono>
#include <iostream>
#include <variant>
#include <string>

using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;

// 求和：用 variant 存数字
double sum_variant(int n) {
    TacoValue acc = 0.0;
    for (int i = 0; i < n; ++i) {
        std::get<double>(acc) += i;
    }
    return std::get<double>(acc);
}

int main() {
    const int N = 10'000'000;

    auto t1 = std::chrono::high_resolution_clock::now();
    double result = sum_variant(N);
    auto t2 = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "Sum: " << result << "\n";
    std::cout << "Time: " << ms << " ms\n";

    return 0;
}
```

在 Release 模式（`-O2`）下，`variant` 里的 `double` 几乎和裸 `double` 一样快——编译器能优化掉大部分 `variant` 的开销。

### 真正的性能瓶颈在哪里

测量之后会发现，Taco v5 的性能瓶颈不在 `variant`，而在：

1. **字符串操作**：每次字符串拼接都创建新的 `std::string`，对于 `"Hello, " + name + "!"` 这类插值，中间有多次临时字符串
2. **`unordered_map` 的变量查找**：每次变量读写都要做哈希和比较
3. **AST 节点的虚函数调用**：虚函数调用有 vtable 查找和间接跳转的开销

这些才是真正值得优化的地方。字节码虚拟机（把 AST 编译成指令数组，然后解释执行）可以消除虚函数调用的开销，但那是更高级的话题，留给第三十七章讨论。

---

## 测试 v5

用一段稍复杂的 Taco 脚本测试 v5 的核心功能——重点验证类型处理和错误信息：

```taco
// test_v5.taco

// 基本类型和 type()
var n = 42;
var s = "hello";
var b = true;
var arr = [1, 2, 3];
var m = {"key": "value"};

print(type(n));    // number
print(type(s));    // string
print(type(b));    // bool
print(type(arr));  // array
print(type(m));    // map

// 类型安全的运算
func safe_add(a, b) {
    if (type(a) != type(b)) {
        return "type mismatch";
    }
    return a + b;
}

print(safe_add(1, 2));       // 3
print(safe_add("a", "b"));   // ab
print(safe_add(1, "b"));     // type mismatch

// len() 对不同类型的行为
print(len("hello"));         // 5
print(len([1, 2, 3, 4]));    // 4
print(len({"a": 1, "b": 2})); // 2

// variant 值的完整生命周期：创建、传递、返回
func make_pair(k, v) {
    return {"key": k, "value": v};
}

var pair = make_pair("name", "Miguel");
print(pair["key"]);    // name
print(pair["value"]);  // Miguel

// 闭包 + 类型
func counter(start) {
    var count = start;
    return {
        inc:  func() { count = count + 1; return count; },
        dec:  func() { count = count - 1; return count; },
        get:  func() { return count; },
    };
}

var c = counter(10);
print(c["inc"]());  // 11
print(c["inc"]());  // 12
print(c["dec"]());  // 11
print(c["get"]());  // 11
```

### 预期输出

```
number
string
bool
array
map
3
ab
type mismatch
5
4
2
name
Miguel
11
12
11
11
```

---

## 小结

v5 是一次内部重构，没有新的语言特性，但代码质量有明显提升。

**`overloaded` + `std::visit`** 是 `std::variant` 的标准配套用法。`overloaded` 本身只有几行代码（模板继承 + using），但它让 `visit` 可以直接接收 lambda 列表，不需要手写结构体。所有类型必须有对应的处理分支，漏掉会在编译时报错——比虚函数更安全。

**`taco_is<T>` 和 `taco_get<T>`** 把类型检查和提取操作封装成模板函数，错误信息集中，调用端干净。

**`register_builtin<Fn>`** 用模板接收任意可调用类型，让注册内置函数的代码从繁琐的样板代码变成一行。

**`if constexpr`** 在需要对不同类型做不同操作、但又不想为每种类型写单独函数的场合非常有用。它是模板里的"条件编译"，没有被选中的分支根本不会编译。

**性能上**，`variant` 里的基本类型（`double`、`bool`）直接存在栈上，没有堆分配，在 Release 模式下编译器能充分优化。真正的性能瓶颈在字符串操作和变量查找，不在类型分发本身。

---

第六部分到这里结束。第七部分进入多线程，学完之后 v6 会用线程实现 REPL，让 Taco 有一个交互式的游乐场。
