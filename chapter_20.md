# 第二十章：项目 v3——函数、闭包与环境

---

第四部分学完了所有权、智能指针、移动语义、常用标准库工具。现在用这些知识做 v3 最核心的事：**实现用户定义函数和闭包**。

v3 结束时，Taco 能运行这样的代码：

```taco
// 基础函数
func greet(name) {
    print("Hola, " + name + "!");
}
greet("Miguel");

// 递归
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
print(fib(10));  // 55

// 闭包：函数捕获外部变量
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}

var counter = makeCounter();
print(counter());  // 1
print(counter());  // 2
print(counter());  // 3

// 命名参数
func greet2(name, greeting: "Hola") {
    print(greeting + ", " + name + "!");
}
greet2("Dante");              // Hola, Dante!
greet2("Miguel", greeting: "Hey");  // Hey, Miguel!

// 多返回值
func minmax(arr) {
    return arr[0], arr[arr.len() - 1];
}
var lo, hi = minmax([3, 1, 4, 1, 5, 9]);
print(lo);  // 3
print(hi);  // 9
```

---

## 20.1 函数是什么：从解释器的角度

在实现函数之前，先想清楚函数在解释器里是什么东西。

函数是一个**值**——就像数字、字符串一样，函数可以被赋给变量、作为参数传递、从函数里返回。这就是"函数是一等公民"（first-class function）的含义。

在 Taco 里，函数值包含：
- **参数列表**：函数接受哪些参数，各自的默认值是什么
- **函数体**：一组语句，也就是 AST 节点列表
- **闭包环境**：函数定义时所在的作用域（用于捕获外部变量）

当调用函数时，解释器：
1. 创建一个新的作用域，以闭包环境为父作用域
2. 把实参绑定到形参名上，存入新作用域
3. 在这个新作用域里依次执行函数体的每条语句
4. 如果遇到 `return`，捕获返回值，退出

---

## 20.2 环境（Environment）：作用域的实现

作用域是函数实现的核心。v2 用一个简单的 `vector<unordered_map>` 实现作用域链，但这个设计有一个致命问题：**不支持闭包**。

来看一个例子：

```taco
func makeCounter() {
    var count = 0;     // count 在 makeCounter 的作用域里

    return func() {
        count = count + 1;  // 内层函数访问外层的 count
        return count;
    };
}

var counter = makeCounter();
// makeCounter 已经返回，它的局部作用域应该销毁了
// 但 counter 还要访问 count！
counter();  // 还能访问到 count，因为闭包捕获了外层作用域
```

v2 的 `vector<map>` 实现里，当 `makeCounter` 返回时，它的作用域从 `vector` 里弹出，`count` 就消失了。之后调用 `counter()` 就找不到 `count`。

解决方案：**把每个作用域做成堆上的对象，用 `shared_ptr` 管理，闭包持有对外层作用域的共享引用**。这样即使外层函数返回，只要还有闭包引用那个作用域，它就不会被销毁。

### 新的 Environment 设计

```cpp
// environment.h
#pragma once
#include "value.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>

class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;

    // 创建全局环境（没有父环境）
    Environment() : m_parent(nullptr) {}

    // 创建子环境（有父环境）
    explicit Environment(Ptr parent)
        : m_parent(std::move(parent))
    {}

    // 在当前作用域定义新变量
    void define(const std::string& name, TacoValue value) {
        m_vars[name] = std::move(value);
    }

    // 查找变量：先在当前作用域找，找不到就往上找
    TacoValue get(const std::string& name) const {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) {
            return it->second;
        }
        if (m_parent) {
            return m_parent->get(name);  // 递归向上查找
        }
        throw std::runtime_error("🌮 '" + name + "' is not defined.");
    }

    // 修改变量：在定义它的那个作用域里修改
    void set(const std::string& name, TacoValue value) {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) {
            it->second = std::move(value);
            return;
        }
        if (m_parent) {
            m_parent->set(name, std::move(value));  // 递归向上修改
            return;
        }
        throw std::runtime_error("🌮 '" + name + "' is not defined.");
    }

    // 获取父环境
    Ptr parent() const { return m_parent; }

private:
    Ptr m_parent;  // shared_ptr：可以和闭包共享
    std::unordered_map<std::string, TacoValue> m_vars;
};
```

**为什么 `m_parent` 用 `shared_ptr`？**

当外层函数返回时，它的 `Environment` 对象本来应该销毁。但如果内层函数（闭包）还持有一个指向外层 `Environment` 的 `shared_ptr`，那个 `Environment` 就不会被销毁——它的引用计数还不是 0。

这正是我们想要的：只要闭包还存在，它捕获的外层作用域就存在。当闭包也被销毁时，引用计数降到 0，外层作用域才被释放。

---

## 20.3 函数值的表示

函数是一个值，所以它要能存在 `TacoValue` 里。首先定义函数的数据结构：

```cpp
// function.h
#pragma once
#include "value.h"
#include "ast.h"
#include "environment.h"
#include <string>
#include <vector>
#include <functional>

// 参数：名字 + 可选的默认值
struct Param {
    std::string              name;
    std::unique_ptr<Expr>    default_value;  // nullptr 表示没有默认值
    bool                     is_named;       // 是否是命名参数
};

// 用户定义的函数
struct TacoFunction {
    std::string              name;        // 函数名（匿名函数为空）
    std::vector<Param>       params;      // 参数列表
    std::vector<ExprPtr>     body;        // 函数体（语句列表）
    Environment::Ptr         closure;     // 闭包环境（定义时的作用域）

    // 使用 shared_ptr 包装，让 TacoValue 可以存放函数
    using Ptr = std::shared_ptr<TacoFunction>;
};

// 内置函数：用 C++ lambda 实现
using NativeFunction = std::function<TacoValue(std::vector<TacoValue>)>;

struct TacoNative {
    std::string    name;
    NativeFunction fn;
    using Ptr = std::shared_ptr<TacoNative>;
};
```

现在更新 `TacoValue`，加入函数类型：

```cpp
// value.h（更新版）
#pragma once
#include <variant>
#include <string>
#include <memory>

// 前向声明
struct TacoFunction;
struct TacoNative;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,  // 用户定义函数
    std::shared_ptr<TacoNative>     // 内置函数
>;

std::string value_to_string(const TacoValue& val);
bool        is_truthy(const TacoValue& val);
bool        values_equal(const TacoValue& a, const TacoValue& b);
```

用 `shared_ptr<TacoFunction>` 而不是直接存 `TacoFunction`，是因为：
1. `TacoValue` 是 `variant`，如果直接存 `TacoFunction`，`TacoFunction` 里有 `vector<ExprPtr>`（独占指针的 vector），`variant` 的拷贝就会失败
2. `shared_ptr` 让函数对象可以被多个变量引用（把函数赋给多个变量），不需要深拷贝

---

## 20.4 更新 AST：函数定义和调用

新增两种 AST 节点：函数定义（`FuncDecl`）和函数调用（完整版的 `CallExpr`）。

```cpp
// ast.h 新增部分

// 函数声明：func name(params) { body }
struct FuncDecl : Expr {
    std::string          name;
    std::vector<Param>   params;
    std::vector<ExprPtr> body;
    bool                 is_anonymous;  // 匿名函数

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "func " + name + "(...)";
    }
};

// 函数调用：callee(args) 或 callee(named: value)
struct CallExpr : Expr {
    ExprPtr              callee;       // 被调用的表达式（可以是变量、方法等）
    std::vector<ExprPtr> args;         // 位置参数
    std::vector<std::pair<std::string, ExprPtr>> named_args;  // 命名参数

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "(call)"; }
};

// 多返回值赋值：var a, b = func()
struct MultiAssign : Expr {
    std::vector<std::string> names;
    ExprPtr                  value;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "(multi-assign)"; }
};
```

---

## 20.5 实现 RAII 管理函数调用栈

函数调用时需要创建新作用域，函数返回时销毁这个作用域。这正是 RAII 的使用场景：

```cpp
// evaluator.h（新增）

class Evaluator {
public:
    // ...

    // 当前环境
    Environment::Ptr current_env() const { return m_env; }

    // 进入新作用域
    void push_env(Environment::Ptr env) {
        m_env = std::move(env);
    }

    // 退出当前作用域，恢复父作用域
    void pop_env() {
        if (m_env->parent()) {
            m_env = m_env->parent();
        }
    }

private:
    Environment::Ptr m_env;  // 当前环境
};

// RAII 作用域守卫：构造时进入新作用域，析构时退出
class ScopeGuard {
public:
    ScopeGuard(Evaluator& eval, Environment::Ptr new_env)
        : m_eval(eval)
        , m_old_env(eval.current_env())
    {
        eval.push_env(std::move(new_env));
    }

    ~ScopeGuard() {
        m_eval.push_env(m_old_env);  // 恢复旧环境
    }

    // 禁止拷贝和移动（守卫不应该被复制）
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

private:
    Evaluator&       m_eval;
    Environment::Ptr m_old_env;
};
```

`ScopeGuard` 是一个经典的 RAII 守卫：

```cpp
// 使用示例
TacoValue call_function(Evaluator& eval, TacoFunction::Ptr fn,
                         std::vector<TacoValue> args) {
    // 创建函数的执行环境，父环境是闭包环境
    auto fn_env = std::make_shared<Environment>(fn->closure);

    // 绑定参数
    for (int i = 0; i < static_cast<int>(fn->params.size()); i++) {
        fn_env->define(fn->params[i].name, args[i]);
    }

    // RAII 守卫：进入函数作用域
    ScopeGuard guard(eval, fn_env);
    // 现在 eval.current_env() 是 fn_env

    TacoValue result = nullptr;
    try {
        for (const auto& stmt : fn->body) {
            result = eval.evaluate(stmt.get());
        }
    } catch (ReturnException& ret) {
        result = ret.value;  // 捕获 return 的值
    }

    // guard 析构，自动恢复之前的环境
    return result;
}
```

不管函数是正常返回还是通过 `return` 跳出，`ScopeGuard` 的析构函数都会恢复之前的环境。这就是 RAII 处理异常的价值。

---

## 20.6 实现闭包

闭包的关键：**函数在定义时捕获当前环境，存在函数对象里**。

```cpp
// ast.cpp

TacoValue FuncDecl::evaluate(Evaluator& eval) const {
    // 创建函数对象，捕获当前环境
    auto fn = std::make_shared<TacoFunction>();
    fn->name      = name;
    fn->closure   = eval.current_env();  // 捕获当前环境！
    fn->is_anonymous = is_anonymous;

    // 拷贝参数列表（params 里有 default_value，需要深拷贝 AST 节点）
    // 为简单起见，这里存 shared_ptr 而不是 unique_ptr
    for (const auto& p : params) {
        Param copy;
        copy.name     = p.name;
        copy.is_named = p.is_named;
        // default_value 的拷贝需要 clone 机制，暂时简化处理
        fn->params.push_back(std::move(copy));
    }

    // 函数体：存对语句的引用（不拷贝，因为 FuncDecl 的生命周期足够长）
    // 更严格的实现应该把 body 拷贝进去，这里简化
    for (const auto& stmt : body) {
        fn->body.push_back(clone_expr(stmt.get()));  // 需要 clone 函数
    }

    if (!name.empty()) {
        // 具名函数：把自己存到当前作用域里
        eval.current_env()->define(name, fn);
    }

    return fn;  // 返回函数值
}
```

闭包的神奇之处：`fn->closure = eval.current_env()` 这一行。函数对象持有对当前环境的 `shared_ptr`，当函数被存到变量里并传出去时，那个环境因为有引用而不会被销毁。

**来看闭包的具体执行过程**：

```taco
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}
var counter = makeCounter();
```

1. 调用 `makeCounter()`：
   - 创建 `makeCounter` 的执行环境 `env_mk`，父环境是全局环境
   - 在 `env_mk` 里定义 `count = 0`

2. 执行 `return func() { ... }`：
   - 创建匿名函数对象 `fn_inner`
   - `fn_inner.closure = env_mk`（捕获 `makeCounter` 的环境）
   - 返回 `fn_inner`

3. `makeCounter()` 返回：
   - `makeCounter` 的调用栈帧弹出
   - 但 `env_mk` 没有销毁！因为 `fn_inner.closure` 持有它的 `shared_ptr`

4. `var counter = makeCounter()`：
   - `counter` 现在持有 `fn_inner`（通过 `shared_ptr`）
   - `fn_inner` 持有 `env_mk`（通过 `shared_ptr`）

5. 调用 `counter()`：
   - 创建新环境 `env_call`，父环境是 `fn_inner.closure`（即 `env_mk`）
   - 执行 `count = count + 1`：在 `env_call` 里找 `count`，找不到，向上找，在 `env_mk` 里找到！
   - 修改 `env_mk` 里的 `count`，从 0 变成 1
   - 返回 1

6. 再次调用 `counter()`：
   - `env_mk` 里的 `count` 已经是 1
   - 执行后变成 2，返回 2

这就是闭包的工作原理：内层函数通过 `shared_ptr` 持有外层的环境，每次调用都能访问并修改同一份 `count`。

---

## 20.7 实现函数调用

```cpp
// ast.cpp

TacoValue CallExpr::evaluate(Evaluator& eval) const {
    // 先求值被调用的表达式
    TacoValue callee_val = eval.evaluate(callee.get());

    // 求值所有位置参数
    std::vector<TacoValue> arg_vals;
    for (const auto& arg : args) {
        arg_vals.push_back(eval.evaluate(arg.get()));
    }

    // 求值命名参数
    std::vector<std::pair<std::string, TacoValue>> named_vals;
    for (const auto& [name, expr] : named_args) {
        named_vals.push_back({name, eval.evaluate(expr.get())});
    }

    // 分派：内置函数还是用户定义函数
    if (std::holds_alternative<std::shared_ptr<TacoNative>>(callee_val)) {
        auto& native = std::get<std::shared_ptr<TacoNative>>(callee_val);
        return native->fn(arg_vals);
    }

    if (std::holds_alternative<std::shared_ptr<TacoFunction>>(callee_val)) {
        auto& fn = std::get<std::shared_ptr<TacoFunction>>(callee_val);
        return call_user_function(eval, fn, arg_vals, named_vals);
    }

    throw std::runtime_error("🌮 '" + value_to_string(callee_val) +
                             "' is not a function.");
}

static TacoValue call_user_function(
    Evaluator& eval,
    const std::shared_ptr<TacoFunction>& fn,
    const std::vector<TacoValue>& args,
    const std::vector<std::pair<std::string, TacoValue>>& named_args)
{
    // 创建函数的执行环境，父环境是闭包环境
    auto fn_env = std::make_shared<Environment>(fn->closure);

    // 处理参数绑定
    int pos_idx = 0;  // 当前处理到第几个位置参数

    for (const auto& param : fn->params) {
        TacoValue val = nullptr;

        if (param.is_named) {
            // 命名参数：先从 named_args 里找
            bool found = false;
            for (const auto& [name, v] : named_args) {
                if (name == param.name) {
                    val = v;
                    found = true;
                    break;
                }
            }
            // 没找到就用默认值
            if (!found) {
                if (param.default_value) {
                    val = eval.evaluate(param.default_value.get());
                } else {
                    throw std::runtime_error(
                        "🌮 Missing argument for parameter '" + param.name + "'."
                    );
                }
            }
        } else {
            // 位置参数
            if (pos_idx < static_cast<int>(args.size())) {
                val = args[pos_idx++];
            } else if (param.default_value) {
                val = eval.evaluate(param.default_value.get());
            } else {
                throw std::runtime_error(
                    "🌮 Missing argument for parameter '" + param.name + "'."
                );
            }
        }

        fn_env->define(param.name, std::move(val));
    }

    // RAII 守卫：进入函数作用域
    ScopeGuard guard(eval, fn_env);

    // 执行函数体
    TacoValue result = nullptr;
    try {
        for (const auto& stmt : fn->body) {
            result = eval.evaluate(stmt.get());
        }
    } catch (ReturnException& ret) {
        return ret.value;
    }

    return result;
}
```

---

## 20.8 多返回值的实现

Taco 支持多返回值：

```taco
func minmax(arr) {
    return arr[0], arr[arr.len() - 1];
}
var lo, hi = minmax([3, 1, 4, 1, 5]);
```

实现思路：多返回值在内部用一个特殊的"元组"值表示（用 `vector<TacoValue>` 包装），解构赋值时拆开：

更新 `TacoValue`，加入数组类型（同时也支持 Taco 的 array 类型）：

```cpp
using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>,
    std::shared_ptr<std::vector<TacoValue>>  // array 和多返回值都用这个
>;
```

`ReturnStmt` 的求值：

```cpp
TacoValue ReturnStmt::evaluate(Evaluator& eval) const {
    if (!value) {
        throw ReturnException{nullptr};
    }

    // 检查是否是多返回值（逗号分隔的表达式列表）
    if (auto* multi = dynamic_cast<const MultiReturn*>(value.get())) {
        auto arr = std::make_shared<std::vector<TacoValue>>();
        for (const auto& expr : multi->values) {
            arr->push_back(eval.evaluate(expr.get()));
        }
        throw ReturnException{arr};
    }

    throw ReturnException{eval.evaluate(value.get())};
}
```

`MultiAssign` 的求值：

```cpp
TacoValue MultiAssign::evaluate(Evaluator& eval) const {
    TacoValue result = eval.evaluate(value.get());

    // 期望结果是一个 array（多返回值）
    if (!std::holds_alternative<std::shared_ptr<std::vector<TacoValue>>>(result)) {
        throw std::runtime_error(
            "🌮 Expected multiple return values, got a single value."
        );
    }

    auto& arr = *std::get<std::shared_ptr<std::vector<TacoValue>>>(result);

    if (arr.size() != names.size()) {
        throw std::runtime_error(
            "🌮 Expected " + std::to_string(names.size()) +
            " values, got " + std::to_string(arr.size()) + "."
        );
    }

    for (int i = 0; i < static_cast<int>(names.size()); i++) {
        eval.current_env()->define(names[i], arr[i]);
    }

    return result;
}
```

---

## 20.9 内置函数的实现

内置函数用 C++ lambda 实现，存为 `TacoNative`：

```cpp
// builtins.cpp
#include "builtins.h"
#include "value.h"
#include <iostream>
#include <stdexcept>

void register_builtins(Environment::Ptr env) {
    // print
    env->define("print", std::make_shared<TacoNative>(TacoNative{
        "print",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 print() takes 1 argument.");
            }
            std::cout << value_to_string(args[0]) << "\n";
            return nullptr;
        }
    }));

    // type：返回值的类型名
    env->define("type", std::make_shared<TacoNative>(TacoNative{
        "type",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 type() takes 1 argument.");
            }
            return std::visit([](auto&& v) -> std::string {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>)       return "number";
                if constexpr (std::is_same_v<T, std::string>)  return "string";
                if constexpr (std::is_same_v<T, bool>)         return "bool";
                if constexpr (std::is_same_v<T, std::nullptr_t>) return "nil";
                return "function";
            }, args[0]);
        }
    }));

    // input：读取用户输入
    env->define("input", std::make_shared<TacoNative>(TacoNative{
        "input",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (!args.empty()) {
                std::cout << value_to_string(args[0]);
            }
            std::string line;
            std::getline(std::cin, line);
            return line;
        }
    }));

    // number：把值转成数字
    env->define("number", std::make_shared<TacoNative>(TacoNative{
        "number",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 number() takes 1 argument.");
            }
            return std::visit([](auto&& v) -> TacoValue {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>)      return v;
                if constexpr (std::is_same_v<T, std::string>) {
                    try { return std::stod(v); }
                    catch (...) {
                        throw std::runtime_error(
                            "🌮 Cannot convert '" + v + "' to number."
                        );
                    }
                }
                if constexpr (std::is_same_v<T, bool>) return v ? 1.0 : 0.0;
                throw std::runtime_error("🌮 Cannot convert nil to number.");
                return nullptr;
            }, args[0]);
        }
    }));

    // string：把值转成字符串
    env->define("string", std::make_shared<TacoNative>(TacoNative{
        "string",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 string() takes 1 argument.");
            }
            return value_to_string(args[0]);
        }
    }));
}
```

---

## 20.10 测试：斐波那契数列与闭包

创建 `test_v3.taco`：

```taco
// 斐波那契（递归）
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}

for i in range(0, 11) {
    print(fib(i));
}
// 0 1 1 2 3 5 8 13 21 34 55

// 闭包：计数器
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}

var c1 = makeCounter();
var c2 = makeCounter();

print(c1());  // 1
print(c1());  // 2
print(c2());  // 1（c2 有自己的 count，和 c1 独立）
print(c1());  // 3

// 闭包：累加器
func makeAdder(n) {
    return func(x) {
        return x + n;
    };
}

var add5 = makeAdder(5);
var add10 = makeAdder(10);

print(add5(3));   // 8
print(add10(3));  // 13

// 命名参数
func greet(name, greeting: "Hola") {
    print(greeting + ", " + name + "!");
}

greet("Miguel");
greet("Dante", greeting: "Hey");

// 高阶函数：把函数作为参数传递
func apply(f, x) {
    return f(x);
}

func double(x) { return x * 2; }
func square(x) { return x * x; }

print(apply(double, 5));  // 10
print(apply(square, 5));  // 25
```

### 运行结果

```
0
1
1
2
3
5
8
13
21
34
55
1
2
1
3
8
13
Hola, Miguel!
Hey, Dante!
10
25
```

---

## 20.11 这个版本的局限性

**还没有 array 和 map 的内置方法**

`arr.filter`、`arr.map`、`str.getLines()` 这些还不支持，v4 会系统地实现标准库。

**参数列表的 AST 节点没有完全实现**

`Param` 里的 `default_value` 用了 `unique_ptr<Expr>`，但在 `FuncDecl::evaluate` 里克隆 AST 节点需要实现一个 `clone` 函数，这里做了简化处理。完整实现会在 v4 里补全。

**没有尾调用优化**

Taco 里的递归函数，每次调用都会在 C++ 的调用栈上创建一个新帧。调用层次太深（比如 `fib(100)`）会导致 C++ 的栈溢出。真实的解释器通常会实现尾调用优化（TCO）或者用显式栈来避免这个问题。

**没有 OOP**

`class`、`struct`、`enum` 还没实现，这些会在综合收尾章节里加入。

---

## 小结

v3 是 Taco 解释器最核心的一步进化。

**Environment（作用域链）** 用 `shared_ptr` 管理，支持闭包捕获外层变量。每个作用域是堆上的一个对象，内层函数持有对外层作用域的共享引用，外层函数返回后作用域仍然存活。

**函数是值**：函数被表示为 `TacoFunction`（用 `shared_ptr` 存在 `TacoValue` 里），包含参数列表、函数体 AST、闭包环境三个部分。

**RAII 管理函数调用栈**：`ScopeGuard` 在构造时进入新作用域，在析构时（不管正常返回还是异常）恢复旧作用域。这让函数调用的资源管理完全自动化。

**内置函数** 用 `std::function` 包装 C++ lambda，统一注册到全局环境，和用户定义函数的调用方式一致。

**闭包** 在函数定义时捕获当前环境（`fn->closure = eval.current_env()`），调用时以闭包环境为父环境创建新作用域，实现对外层变量的访问和修改。

---

第四部分到这里结束。第五部分进入 STL，学完之后 v4 会用 STL 容器和算法实现 Taco 的 array、map，以及完整的内置标准库和 pipeline 风格。
