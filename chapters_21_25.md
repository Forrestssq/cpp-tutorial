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
# 第二十一章：容器

---

第五部分进入 STL（Standard Template Library，标准模板库）。STL 是 C++ 标准库的核心，提供了一套经过精心设计、高度优化的数据结构和算法。

在 Python 里，列表（`list`）、字典（`dict`）、集合（`set`）是内置类型，用起来很自然。C++ 的 STL 容器提供了同样的功能，但因为是静态类型，有一些不同的使用方式和性能特性。

这一章讲最常用的几种容器，下一章讲迭代器，再下一章讲算法。理解了这三章，再看 v4 的代码会非常清晰。

---

## 21.1 vector：最常用的序列容器

`std::vector<T>` 是动态数组——可以在尾部高效地添加或删除元素，支持随机访问（通过下标 `[]`）。它是 C++ 里最常用的容器，在很多场景下 `vector` 是默认选择。

### 基本用法

```cpp
#include <vector>

// 创建
std::vector<int> nums;               // 空 vector
std::vector<int> nums2(5);           // 5 个元素，初始值 0
std::vector<int> nums3(5, 42);       // 5 个元素，初始值 42
std::vector<int> nums4 = {1, 2, 3, 4, 5};  // 初始化列表

// 添加元素
nums.push_back(10);   // 在尾部添加
nums.push_back(20);
nums.push_back(30);

// 访问元素
int first = nums[0];        // 下标访问，不检查边界
int second = nums.at(1);    // 带边界检查，越界抛 std::out_of_range
int last = nums.back();     // 最后一个元素
int front = nums.front();   // 第一个元素

// 大小
nums.size();     // 元素个数
nums.empty();    // 是否为空
nums.capacity(); // 当前分配的容量（可能大于 size）

// 删除
nums.pop_back();   // 删除最后一个元素
nums.clear();      // 删除所有元素

// 遍历
for (int n : nums) {
    std::cout << n << "\n";
}

// 或者用迭代器（下一章详讲）
for (auto it = nums.begin(); it != nums.end(); ++it) {
    std::cout << *it << "\n";
}
```

### vector 的内存模型

`vector` 在内存里是一块连续的数组，这意味着：
- 随机访问 `O(1)`：`nums[i]` 直接计算内存地址
- 尾部添加 `O(1)` 均摊：大多数时候很快，偶尔需要扩容（分配更大的数组，把旧数据复制过去）
- 中间插入 `O(n)`：需要把后面的元素往后移

```
vector 内部：
  [ptr] [size] [capacity]
    │
    ▼
  [1][2][3][4][5][ ][ ][ ]  ← 连续内存，capacity=8，size=5
```

**扩容策略**：当 `size == capacity` 时，`push_back` 会分配一块更大的内存（通常是当前容量的 2 倍），把旧数据移过去，释放旧内存。这就是为什么说尾部添加是 `O(1)` "均摊"——偶尔有 `O(n)` 的扩容操作，但平均下来还是 `O(1)`。

如果预先知道元素个数，用 `reserve` 避免重复扩容：

```cpp
std::vector<Token> tokens;
tokens.reserve(1000);  // 预分配 1000 个元素的空间
// 之后的 push_back 不会触发扩容（直到超过 1000）
```

### vector 在 Taco 里的应用

`vector` 在 Taco 里到处都是：

```cpp
// Token 列表
std::vector<Token> tokens = lexer.tokenize();

// AST 节点列表（Program 里的语句）
std::vector<ExprPtr> statements;

// 函数参数列表
std::vector<TacoValue> args;

// 作用域链（v2 的实现）
std::vector<std::unordered_map<std::string, TacoValue>> m_scopes;
```

v4 里，Taco 的 array 类型内部就用 `vector<TacoValue>` 实现：

```cpp
// Taco 的 array 值
using TacoArray = std::vector<TacoValue>;
```

---

## 21.2 deque、list：其他序列容器

### deque：双端队列

`std::deque<T>`（double-ended queue）在头部和尾部都能高效地添加/删除元素，但内存不连续（分段存储），随机访问比 `vector` 稍慢。

```cpp
#include <deque>

std::deque<int> dq = {1, 2, 3};

dq.push_back(4);   // 在尾部添加
dq.push_front(0);  // 在头部添加
dq.pop_front();    // 删除头部
dq.pop_back();     // 删除尾部

// 支持下标访问
int n = dq[1];
```

什么时候用 `deque` 而不是 `vector`：需要频繁在**头部**插入/删除时。`vector` 头部插入是 `O(n)`（所有元素要移动），`deque` 头部插入是 `O(1)`。

在 Taco 的词法分析里，REPL 的历史记录可以用 `deque` 管理（新条目加到头部，超过限制就从尾部删除）。

### list：双向链表

`std::list<T>` 是双向链表，每个节点单独分配内存，任意位置的插入/删除都是 `O(1)`（如果有迭代器指向那个位置），但不支持随机访问（不能用 `[]`），遍历也比 `vector` 慢（因为内存不连续，缓存不友好）。

```cpp
#include <list>

std::list<int> lst = {1, 2, 3, 4, 5};

lst.push_front(0);  // 头部添加
lst.push_back(6);   // 尾部添加

// 不支持 lst[2]
// 遍历必须用迭代器
for (int n : lst) {
    std::cout << n << "\n";
}

// 任意位置插入：先找到位置，再插入
auto it = lst.begin();
std::advance(it, 2);  // 移动到第 3 个元素
lst.insert(it, 99);   // 在这个位置插入 99
```

**实际使用**：`list` 在现代 C++ 里用得越来越少。因为 CPU 的缓存效果，即使是需要频繁中间插入的场景，`vector` 实际上往往也比 `list` 快（内存连续，缓存命中率高）。在大多数情况下，先用 `vector`，性能真的不够了再考虑 `list`。

---

## 21.3 map 与 unordered_map

### map：有序关联容器

`std::map<K, V>` 是基于红黑树实现的有序映射，键按照 `<` 运算符排序：

```cpp
#include <map>

std::map<std::string, int> word_count;

// 插入
word_count["hello"] = 1;
word_count["world"] = 2;
word_count.insert({"foo", 3});

// 访问
int n = word_count["hello"];   // 如果不存在，插入默认值！
int m = word_count.at("hello");  // 如果不存在，抛异常

// 查找
auto it = word_count.find("hello");
if (it != word_count.end()) {
    std::cout << it->first << ": " << it->second << "\n";
}

// 遍历（按键排序的顺序）
for (const auto& [key, value] : word_count) {
    std::cout << key << ": " << value << "\n";
}

// 删除
word_count.erase("hello");

// 大小
word_count.size();
word_count.count("hello");  // 存在返回 1，不存在返回 0
word_count.contains("hello");  // C++20，更直观
```

`map` 的特点：
- 键自动排序
- 查找、插入、删除：`O(log n)`
- 内存开销较大（每个节点单独分配，有指针开销）

### unordered_map：哈希映射

`std::unordered_map<K, V>` 基于哈希表，不保证顺序，但平均操作复杂度是 `O(1)`：

```cpp
#include <unordered_map>

std::unordered_map<std::string, int> env;

env["x"] = 10;
env["name"] = 42;

// 用法和 map 基本相同，但遍历顺序不确定
for (const auto& [key, value] : env) {
    std::cout << key << ": " << value << "\n";
}
```

`unordered_map` 的特点：
- 不排序
- 查找、插入、删除：平均 `O(1)`，最坏 `O(n)`（哈希冲突严重时）
- 内存效率比 `map` 稍好

**什么时候用哪个**：

- 需要有序遍历（比如打印所有变量按字母排序）：用 `map`
- 只需要快速查找，不关心顺序：用 `unordered_map`（更快）

Taco 的 `Environment` 用 `unordered_map<string, TacoValue>`——变量查找是热路径，需要尽可能快，不需要排序。

Taco 的关键字表（`Lexer::KEYWORDS`）也用 `unordered_map`——词法分析里频繁查找关键字。

### map 在 Taco 里的应用

Taco 语言的 `map` 类型（字典），内部用 `unordered_map<string, TacoValue>` 实现：

```cpp
// Taco 的 map 值
using TacoMap = std::unordered_map<std::string, TacoValue>;
```

```taco
// Taco 代码
var person = {"name": "Miguel", "age": 12};
print(person["name"]);
```

在 C++ 里对应：

```cpp
auto person = std::make_shared<TacoMap>();
(*person)["name"] = std::string("Miguel");
(*person)["age"] = 12.0;
```

---

## 21.4 set 与 unordered_set

### set：有序集合

`std::set<T>` 是有序集合，不允许重复元素：

```cpp
#include <set>

std::set<int> s = {3, 1, 4, 1, 5, 9, 2, 6};
// 自动去重并排序：{1, 2, 3, 4, 5, 6, 9}

s.insert(7);
s.erase(3);
bool exists = s.count(5) > 0;  // 或 s.contains(5)（C++20）

for (int n : s) {
    std::cout << n << "\n";  // 1 2 4 5 6 7 9
}
```

### unordered_set：哈希集合

`std::unordered_set<T>` 是哈希集合，不排序，平均 `O(1)` 查找：

```cpp
#include <unordered_set>

std::unordered_set<std::string> keywords = {
    "var", "func", "if", "else", "while", "for", "return"
};

bool is_keyword = keywords.count("var") > 0;  // true
bool is_keyword2 = keywords.count("print") > 0;  // false
```

在 Taco 词法分析器里，关键字集合可以用 `unordered_set` 或 `unordered_map`——前者只需要检查是否是关键字，后者同时能获取对应的 `TokenType`。

---

## 21.5 stack、queue、priority_queue

这三种是基于其他容器实现的适配器，提供特定的访问接口。

### stack：栈

`std::stack<T>` 是后进先出（LIFO）的栈，默认用 `deque` 实现：

```cpp
#include <stack>

std::stack<int> s;
s.push(1);
s.push(2);
s.push(3);

int top = s.top();   // 3（查看但不删除）
s.pop();             // 删除顶部元素
s.size();            // 2
s.empty();           // false
```

栈在 Taco 里的一个用途：解析器可以用栈来处理括号匹配，或者在将来实现字节码虚拟机时用作操作数栈。

### queue：队列

`std::queue<T>` 是先进先出（FIFO）的队列：

```cpp
#include <queue>

std::queue<std::string> q;
q.push("first");
q.push("second");
q.push("third");

std::string front = q.front();  // "first"
q.pop();                         // 删除 "first"
std::string next = q.front();   // "second"
```

v6 的 REPL 里，输入线程和求值线程之间可以用 `queue` 传递命令（配合 `mutex` 保护）。

### priority_queue：优先队列

`std::priority_queue<T>` 是堆实现的优先队列，默认是最大堆（最大的元素先出）：

```cpp
#include <queue>

std::priority_queue<int> pq;
pq.push(3);
pq.push(1);
pq.push(4);
pq.push(1);
pq.push(5);

while (!pq.empty()) {
    std::cout << pq.top() << " ";  // 5 4 3 1 1
    pq.pop();
}
```

最小堆：

```cpp
// 用 greater<int> 实现最小堆
std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
min_pq.push(3);
min_pq.push(1);
min_pq.push(4);
// min_pq.top() == 1
```

---

## 21.6 如何选择容器

选择容器的核心问题是：**这个容器最常做什么操作，那个操作的复杂度是多少？**

| 需求 | 推荐容器 | 理由 |
|------|----------|------|
| 按顺序存储，随机访问 | `vector` | O(1) 随机访问，内存连续 |
| 按顺序存储，频繁头部操作 | `deque` | 头尾都是 O(1) |
| 频繁任意位置插入/删除 | `list` | O(1) 插入（但遍历慢） |
| 键值映射，需要排序 | `map` | O(log n) 操作，有序 |
| 键值映射，不需要排序 | `unordered_map` | O(1) 平均，更快 |
| 去重，需要排序 | `set` | O(log n)，有序 |
| 去重，不需要排序 | `unordered_set` | O(1) 平均，更快 |
| 后进先出 | `stack` | 语义明确 |
| 先进先出 | `queue` | 语义明确 |
| 按优先级出队 | `priority_queue` | O(log n) 插入和取出 |

**经验规则**：如果不确定用什么，先用 `vector`。`vector` 在绝大多数场景下都是合理的选择，而且它的内存连续性让它有很好的缓存效果。性能真的不够了，再用性能分析工具找瓶颈，再换容器。

---

## *More About 容器*：底层数据结构与复杂度分析

> 第一次读可以跳过。

### vector 的摊还分析

`vector::push_back` 的平均 `O(1)` 复杂度可以用摊还分析证明。

假设当前容量是 $n$，每次扩容容量翻倍：
- 前 $n/2$ 次 `push_back`：每次 $O(1)$，共 $O(n/2)$
- 第 $n/2 + 1$ 次：触发扩容，复制 $n/2$ 个元素，$O(n/2)$
- 前 $n$ 次 `push_back`：共 $O(n/2) + O(n/2) = O(n)$，平均 $O(1)$

如果每次只扩容固定数量（比如 +100），摊还分析会得到 $O(n)$ 的平均复杂度。所以容量翻倍是最优策略。

### unordered_map 的哈希冲突

`unordered_map` 的 `O(1)` 是平均情况。最坏情况下，所有元素哈希到同一个桶，退化成链表，查找是 `O(n)`。

现代标准库的实现会在负载因子（元素数/桶数）超过阈值时自动扩容（重新哈希），保持平均 `O(1)`。

对于自定义类型，需要提供哈希函数：

```cpp
struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// 自定义哈希
struct PointHash {
    std::size_t operator()(const Point& p) const {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);  // 组合两个哈希
    }
};

std::unordered_map<Point, std::string, PointHash> map;
```

### map 的红黑树

`std::map` 基于红黑树（一种自平衡二叉搜索树）。红黑树保证树的高度是 $O(\log n)$，所以查找、插入、删除都是 $O(\log n)$。

红黑树的每个节点存储：
- 键值对 `(key, value)`
- 左右子节点指针
- 父节点指针
- 颜色（红或黑）

每个节点单独分配，内存不连续，这是 `map` 比 `unordered_map` 慢（对于随机查找）的原因之一——缓存命中率低。

---

## 小结

**`vector`** 是最常用的序列容器，内存连续，随机访问 $O(1)$，尾部添加摊还 $O(1)$。不确定用什么容器时，先用 `vector`。

**`deque`** 头尾添加都是 $O(1)$，适合需要双端操作的场景。

**`list`** 任意位置插入/删除 $O(1)$，但随机访问不支持，遍历也慢，实际上很少用。

**`map`** 有序键值映射，$O(\log n)$ 操作，需要排序时用。**`unordered_map`** 哈希映射，平均 $O(1)$，不需要排序时优先用。

**`set`/`unordered_set`** 分别对应有序和无序的集合。

**`stack`、`queue`、`priority_queue`** 是适配器，语义明确的特殊用途容器。

---

下一章讲迭代器——STL 容器和算法之间的桥梁，也是范围 for 循环的底层机制。
# 第二十二章：迭代器

---

迭代器（iterator）是 STL 的核心设计之一，它是容器和算法之间的桥梁。理解迭代器，就理解了为什么 STL 的算法可以对任何容器工作，以及范围 for 循环的底层是什么。

---

## 22.1 迭代器是什么

迭代器是一种对象，它"指向"容器里的某个元素，可以通过它访问那个元素，也可以移动到下一个元素。

从行为上看，迭代器非常像指针：

```cpp
std::vector<int> v = {10, 20, 30, 40, 50};

// 用迭代器遍历，和用指针遍历数组很像
for (auto it = v.begin(); it != v.end(); ++it) {
    std::cout << *it << "\n";  // * 解引用，获取元素
}

// 用裸指针遍历数组（对比）
int arr[] = {10, 20, 30, 40, 50};
for (int* p = arr; p != arr + 5; ++p) {
    std::cout << *p << "\n";
}
```

`begin()` 返回指向第一个元素的迭代器，`end()` 返回指向最后一个元素**之后**位置的迭代器（一个哨兵，不能解引用）。

```
v:    [10][20][30][40][50][ ]
       ↑                   ↑
     begin()             end()
```

这种"左闭右开"区间 `[begin, end)` 是 STL 的标准约定：包含 `begin`，不包含 `end`。

---

### 迭代器的基本操作

```cpp
std::vector<int> v = {10, 20, 30, 40, 50};

auto it = v.begin();

*it;        // 解引用：10
++it;       // 前进一步，it 现在指向 20
*it;        // 20
--it;       // 后退一步，it 现在指向 10（随机访问迭代器支持）
it += 3;    // 向前 3 步，it 现在指向 40（随机访问迭代器支持）
it - v.begin();  // 计算距离：3（随机访问迭代器支持）

// 比较
it == v.end();   // false
it != v.end();   // true
```

---

## 22.2 迭代器的种类

不同的容器提供不同能力的迭代器，按能力从弱到强分五种：

### 输入迭代器（Input Iterator）

只能单向前进，只能读取，不能修改，每个位置只能访问一次：

```cpp
// 典型例子：istream_iterator，从输入流读取
std::istream_iterator<int> it(std::cin);
std::istream_iterator<int> end;  // 默认构造表示结束

while (it != end) {
    std::cout << *it << "\n";
    ++it;
}
```

### 输出迭代器（Output Iterator）

只能单向前进，只能写入，每个位置只能写一次：

```cpp
// 典型例子：ostream_iterator，向输出流写入
std::vector<int> v = {1, 2, 3, 4, 5};
std::copy(v.begin(), v.end(),
          std::ostream_iterator<int>(std::cout, " "));
// 输出：1 2 3 4 5
```

### 前向迭代器（Forward Iterator）

可以单向前进，可以读写，可以多次访问同一位置。`std::forward_list`（单链表）的迭代器是前向迭代器：

```cpp
std::forward_list<int> fl = {1, 2, 3};
for (auto it = fl.begin(); it != fl.end(); ++it) {
    *it *= 2;  // 可以修改
}
// fl: {2, 4, 6}
```

### 双向迭代器（Bidirectional Iterator）

可以双向前进和后退。`std::list`、`std::map`、`std::set` 的迭代器是双向迭代器：

```cpp
std::list<int> lst = {1, 2, 3, 4, 5};
auto it = lst.end();
--it;   // 后退到最后一个元素
*it;    // 5
--it;   // 后退到倒数第二个
*it;    // 4
```

### 随机访问迭代器（Random Access Iterator）

支持随机访问（像数组下标一样）、加减操作、比较大小。`std::vector`、`std::deque`、裸指针的迭代器是随机访问迭代器：

```cpp
std::vector<int> v = {10, 20, 30, 40, 50};
auto it = v.begin();

it += 3;    // 跳到第 4 个元素
*it;        // 40

it - v.begin();  // 3（计算距离）

v.begin() < v.end();  // true（可以比较大小）
```

**为什么要分这么多种？**

STL 算法根据需要的迭代器类型选择最高效的实现。`std::advance` 把迭代器向前移动 n 步：对随机访问迭代器直接 `it += n`（`O(1)`），对双向/前向迭代器循环 `++it` n 次（`O(n)`）。这种差异让算法对不同容器自动使用最优策略。

---

## 22.3 范围 for 循环的本质

范围 for 循环：

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
for (int n : v) {
    std::cout << n << "\n";
}
```

实际上是以下代码的语法糖：

```cpp
{
    auto __begin = v.begin();
    auto __end   = v.end();
    for (; __begin != __end; ++__begin) {
        int n = *__begin;
        std::cout << n << "\n";
    }
}
```

编译器会把范围 for 展开成这种形式。只要一个类型提供了 `begin()` 和 `end()` 函数（或者有对应的全局函数 `begin(v)` 和 `end(v)`），就可以用范围 for 遍历它。

这意味着**任何自定义类型都可以支持范围 for**，只要实现了对应的迭代器接口。

---

### 为自定义类实现迭代器支持

假设要为 Taco 的 `Environment` 添加迭代支持，可以遍历所有变量：

```cpp
class Environment {
public:
    // 内部迭代器类型：直接暴露 unordered_map 的迭代器
    using iterator = std::unordered_map<std::string, TacoValue>::iterator;
    using const_iterator = std::unordered_map<std::string, TacoValue>::const_iterator;

    iterator begin() { return m_vars.begin(); }
    iterator end()   { return m_vars.end(); }
    const_iterator begin() const { return m_vars.begin(); }
    const_iterator end()   const { return m_vars.end(); }

private:
    std::unordered_map<std::string, TacoValue> m_vars;
    // ...
};

// 使用
Environment env;
env.define("x", 10.0);
env.define("name", std::string("Miguel"));

for (const auto& [key, value] : env) {
    std::cout << key << " = " << value_to_string(value) << "\n";
}
```

---

### const 迭代器

每种容器通常提供两套迭代器：普通迭代器（可以修改元素）和 `const_iterator`（只能读取）：

```cpp
std::vector<int> v = {1, 2, 3};

// 普通迭代器：可以修改
for (auto it = v.begin(); it != v.end(); ++it) {
    *it *= 2;  // 可以修改
}

// const_iterator：只能读
const std::vector<int>& cv = v;
for (auto it = cv.begin(); it != cv.end(); ++it) {
    // *it *= 2;  // 错误！不能修改
    std::cout << *it << "\n";
}

// 明确使用 cbegin/cend 获取 const_iterator
for (auto it = v.cbegin(); it != v.cend(); ++it) {
    // 即使 v 不是 const，这里也不能修改
}
```

**经验规则**：如果不需要修改元素，用 `const auto&` 在范围 for 里遍历，或者用 `cbegin()/cend()`，这会触发 `const_iterator`，让代码意图更清晰，也防止意外修改。

---

### 反向迭代器

很多容器还提供反向迭代器，从尾部向头部遍历：

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// rbegin 指向最后一个元素，rend 指向第一个元素之前
for (auto it = v.rbegin(); it != v.rend(); ++it) {
    std::cout << *it << "\n";  // 5 4 3 2 1
}

// 或者范围 for 配合 std::ranges::reverse（C++20）
```

---

## *More About 迭代器*：迭代器设计哲学与 ranges

> 第一次读可以跳过。

### 为什么迭代器设计成"指针风格"

STL 的设计者 Alexander Stepanov 把迭代器设计成像指针一样，是刻意的——他想让算法既能用于容器，也能用于裸数组：

```cpp
int arr[] = {5, 3, 1, 4, 2};

// std::sort 可以接受裸指针（它们是随机访问迭代器）
std::sort(arr, arr + 5);

// 也可以接受 vector 的迭代器
std::vector<int> v = {5, 3, 1, 4, 2};
std::sort(v.begin(), v.end());
```

裸指针满足随机访问迭代器的所有要求（支持 `*p`、`++p`、`p + n`、`p1 - p2` 等），所以直接可以用。

### C++20 的 ranges

传统 STL 算法用一对迭代器 `[begin, end)` 表示范围，使用起来有点冗长，而且容易传错（`sort(v.begin(), v.end())` 而不是 `sort(v.begin(), v2.end())`）。

C++20 引入了 `std::ranges`，让算法直接接受容器：

```cpp
#include <ranges>
#include <algorithm>

std::vector<int> v = {5, 3, 1, 4, 2};

// 传统：传一对迭代器
std::sort(v.begin(), v.end());

// C++20 ranges：直接传容器
std::ranges::sort(v);

// ranges 还支持惰性管道（类似 Taco 的 pipeline）
auto result = v
    | std::views::filter([](int n) { return n % 2 == 0; })
    | std::views::transform([](int n) { return n * 2; });

for (int n : result) {
    std::cout << n << "\n";
}
```

这和 Taco 的 pipeline 风格非常相似：

```taco
v.filter { n in n % 2 == 0 }.map { n in n * 2 }.each { n in print(n); };
```

Taco 的 pipeline 实现（v4）在精神上和 `std::views` 类似，只不过 Taco 是动态类型的，不需要模板。

---

## 小结

**迭代器**是容器和算法之间的桥梁，行为类似指针：`*it` 解引用，`++it` 前进，`it != end()` 判断是否到达末尾。

**五种迭代器**从弱到强：输入、输出、前向、双向、随机访问。容器提供对应能力的迭代器，算法根据需要的能力类型选择最优实现。

**范围 for 循环**是一对 `begin()/end()` 调用加上循环的语法糖。任何提供了 `begin()/end()` 的类型都可以用范围 for 遍历。

**`const_iterator`** 用于只读遍历，通过 `cbegin()/cend()` 或 `const` 容器的 `begin()/end()` 获取。

---

下一章讲算法库——STL 提供的几十个通用算法，用迭代器对容器进行各种操作，是"避免手写循环"的核心工具。
# 第二十三章：算法库

---

STL 算法库（`<algorithm>`、`<numeric>`）提供了几十个通用算法，可以对任何支持迭代器的容器操作。这些算法是 C++ 里"避免手写循环"的核心工具——大多数循环都能用一个算法调用替代，代码更短、意图更清晰、而且算法本身经过高度优化。

这一章系统地介绍常用算法，按功能分类。

---

## 23.1 不修改序列的算法

这类算法只读取容器里的元素，不改变它们。

### find 和 find_if

```cpp
#include <algorithm>
#include <vector>

std::vector<int> v = {10, 20, 30, 40, 50};

// find：查找特定值，返回迭代器
auto it = std::find(v.begin(), v.end(), 30);
if (it != v.end()) {
    std::cout << "Found: " << *it << " at index " << (it - v.begin()) << "\n";
    // Found: 30 at index 2
}

// find_if：查找满足条件的第一个元素
auto it2 = std::find_if(v.begin(), v.end(), [](int n) {
    return n > 25;
});
// *it2 == 30

// find_if_not：查找第一个不满足条件的元素
auto it3 = std::find_if_not(v.begin(), v.end(), [](int n) {
    return n < 40;
});
// *it3 == 10（第一个不小于 40 的）... 其实是第一个不小于 40 的，即 10 不满足 < 40
// 其实 10 < 40 是 true，find_if_not 找第一个让条件为 false 的
// 这里 40 < 40 是 false，所以 *it3 == 40
```

### count 和 count_if

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};

// count：计算某个值出现的次数
int n = std::count(v.begin(), v.end(), 2);  // 3

// count_if：计算满足条件的元素个数
int evens = std::count_if(v.begin(), v.end(), [](int n) {
    return n % 2 == 0;
});  // 3（2, 2, 4, 2 里有 4 个偶数... 实际是 4 个：2, 2, 4, 2）
```

### all_of、any_of、none_of

```cpp
std::vector<int> v = {2, 4, 6, 8, 10};

// all_of：所有元素都满足条件？
bool all_even = std::all_of(v.begin(), v.end(), [](int n) {
    return n % 2 == 0;
});  // true

// any_of：至少有一个元素满足条件？
bool any_gt5 = std::any_of(v.begin(), v.end(), [](int n) {
    return n > 5;
});  // true

// none_of：没有元素满足条件？
bool none_negative = std::none_of(v.begin(), v.end(), [](int n) {
    return n < 0;
});  // true
```

这三个算法非常直观，而且有短路求值——`any_of` 一找到就停，`all_of` 一发现不满足就停。

### for_each

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// for_each：对每个元素执行操作（和范围 for 类似，但可以传给其他函数）
std::for_each(v.begin(), v.end(), [](int n) {
    std::cout << n << " ";
});
// 1 2 3 4 5

// for_each 的返回值是传入的函数对象（可以用来积累状态）
int sum = 0;
std::for_each(v.begin(), v.end(), [&sum](int n) {
    sum += n;
});
// sum == 15
```

大多数情况下范围 for 更简洁，`for_each` 的优势是可以把函数对象传递给其他函数，或者在某些需要迭代器的接口里使用。

### min_element 和 max_element

```cpp
std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

auto min_it = std::min_element(v.begin(), v.end());
auto max_it = std::max_element(v.begin(), v.end());

std::cout << *min_it << "\n";  // 1
std::cout << *max_it << "\n";  // 9

// 获取下标
int min_idx = min_it - v.begin();  // 1（第一个 1 的位置）
```

### equal 和 mismatch

```cpp
std::vector<int> a = {1, 2, 3, 4, 5};
std::vector<int> b = {1, 2, 3, 4, 5};
std::vector<int> c = {1, 2, 99, 4, 5};

// equal：两个序列是否完全相同
bool same = std::equal(a.begin(), a.end(), b.begin());  // true
bool diff = std::equal(a.begin(), a.end(), c.begin());  // false

// mismatch：找到第一个不同的位置
auto [it_a, it_c] = std::mismatch(a.begin(), a.end(), c.begin());
std::cout << *it_a << " vs " << *it_c << "\n";  // 3 vs 99
```

---

## 23.2 修改序列的算法

这类算法会改变容器的内容（但通常不改变容器的大小）。

### transform

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
std::vector<int> result(v.size());

// transform：对每个元素应用函数，结果存入另一个容器
std::transform(v.begin(), v.end(), result.begin(), [](int n) {
    return n * n;
});
// result: {1, 4, 9, 16, 25}

// 也可以原地修改
std::transform(v.begin(), v.end(), v.begin(), [](int n) {
    return n * 2;
});
// v: {2, 4, 6, 8, 10}

// 两个序列合并：transform 的二元版本
std::vector<int> a = {1, 2, 3};
std::vector<int> b = {10, 20, 30};
std::vector<int> sum(3);
std::transform(a.begin(), a.end(), b.begin(), sum.begin(),
               [](int x, int y) { return x + y; });
// sum: {11, 22, 33}
```

### copy 和 copy_if

```cpp
std::vector<int> src = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
std::vector<int> dst;
dst.resize(src.size());

// copy：复制整个序列
std::copy(src.begin(), src.end(), dst.begin());

// copy_if：只复制满足条件的元素
std::vector<int> evens;
std::copy_if(src.begin(), src.end(), std::back_inserter(evens),
             [](int n) { return n % 2 == 0; });
// evens: {2, 4, 6, 8, 10}
```

`std::back_inserter` 是一个输出迭代器适配器，每次"写入"都调用 `push_back`，让目标容器自动扩展大小。这在不知道结果大小时非常有用。

### replace 和 replace_if

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2};

// replace：把所有等于某值的元素换成新值
std::replace(v.begin(), v.end(), 2, 99);
// v: {1, 99, 3, 99, 4, 99}

// replace_if：把满足条件的元素换成新值
std::replace_if(v.begin(), v.end(), [](int n) { return n > 50; }, 0);
// v: {1, 0, 3, 0, 4, 0}
```

### remove 和 remove_if

`remove` 和 `remove_if` 比较特殊——它们不真正删除元素，而是把不需要删除的元素移到容器前面，然后返回一个迭代器，指向"逻辑末尾"：

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};

// remove：把等于 2 的元素移走
auto new_end = std::remove(v.begin(), v.end(), 2);
// v 的内容变成：{1, 3, 4, 5, ?, ?, ?}（? 是垃圾值）
// new_end 指向第 5 个位置

// 要真正删除，配合 erase
v.erase(new_end, v.end());
// v: {1, 3, 4, 5}

// 更常见的写法：erase-remove 惯用法
v = {1, 2, 3, 2, 4, 2, 5};
v.erase(std::remove(v.begin(), v.end(), 2), v.end());
// v: {1, 3, 4, 5}
```

这个"erase-remove 惯用法"（erase-remove idiom）是 C++ 里删除容器元素的标准方式，看起来有点奇怪但很高效。

### fill 和 generate

```cpp
std::vector<int> v(10);

// fill：用指定值填充
std::fill(v.begin(), v.end(), 42);
// v: {42, 42, 42, ..., 42}

// fill_n：填充 n 个元素
std::fill_n(v.begin(), 5, 99);
// v: {99, 99, 99, 99, 99, 42, 42, 42, 42, 42}

// generate：用函数生成值
int counter = 0;
std::generate(v.begin(), v.end(), [&counter]() { return counter++; });
// v: {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
```

---

## 23.3 排序相关算法

### sort

```cpp
std::vector<int> v = {5, 3, 1, 4, 2};

// 默认：升序排序
std::sort(v.begin(), v.end());
// v: {1, 2, 3, 4, 5}

// 自定义比较器：降序
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
// v: {5, 4, 3, 2, 1}

// 对结构体排序
struct Person {
    std::string name;
    int age;
};

std::vector<Person> people = {
    {"Miguel", 12}, {"Dante", 3}, {"Imelda", 70}
};

// 按年龄排序
std::sort(people.begin(), people.end(), [](const Person& a, const Person& b) {
    return a.age < b.age;
});
// Dante(3), Miguel(12), Imelda(70)

// 按姓名排序
std::sort(people.begin(), people.end(), [](const Person& a, const Person& b) {
    return a.name < b.name;
});
// Dante, Imelda, Miguel
```

`std::sort` 的平均复杂度是 $O(n \log n)$，标准库实现通常是 introsort（快排 + 堆排 + 插排的混合）。

### stable_sort

```cpp
// stable_sort：排序时保持相等元素的相对顺序（稳定排序）
std::vector<Person> people = {
    {"Alice", 25}, {"Bob", 25}, {"Charlie", 30}
};

// Alice 和 Bob 年龄相同
std::stable_sort(people.begin(), people.end(), [](const Person& a, const Person& b) {
    return a.age < b.age;
});
// Alice 仍然在 Bob 前面（相对顺序保持）
```

`stable_sort` 比 `sort` 稍慢（$O(n \log^2 n)$ 或 $O(n \log n)$ 取决于内存），但在需要稳定性时是正确的选择。

### partial_sort 和 nth_element

```cpp
std::vector<int> v = {5, 3, 1, 4, 2};

// partial_sort：只排序前 k 个元素（找最小的 k 个）
std::partial_sort(v.begin(), v.begin() + 3, v.end());
// v 的前 3 个是排好序的最小值：{1, 2, 3, ?, ?}（? 是剩余元素，顺序未定）

// nth_element：让第 n 个位置的元素就是排好序后应该在的那个元素
// 比 sort 快：O(n) 而不是 O(n log n)
v = {5, 3, 1, 4, 2};
std::nth_element(v.begin(), v.begin() + 2, v.end());
// v[2] 是排好序后的第 3 小的元素（1-indexed 是第 3 个）
// 即 v[2] == 3
// v[0], v[1] 都比 3 小（不保证排序），v[3], v[4] 都比 3 大（不保证排序）
```

`nth_element` 常用于"找第 k 小的元素"这类问题，比完整排序快得多。

### binary_search、lower_bound、upper_bound

这三个用于**已排序**的序列：

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// binary_search：序列里是否存在某个值（O(log n)）
bool found = std::binary_search(v.begin(), v.end(), 5);  // true
bool found2 = std::binary_search(v.begin(), v.end(), 11); // false

// lower_bound：第一个不小于目标值的位置
auto lb = std::lower_bound(v.begin(), v.end(), 5);
// *lb == 5，lb - v.begin() == 4

// upper_bound：第一个大于目标值的位置
auto ub = std::upper_bound(v.begin(), v.end(), 5);
// *ub == 6，ub - v.begin() == 5

// 找所有等于 5 的元素的范围
auto range = std::equal_range(v.begin(), v.end(), 5);
// range.first == lower_bound，range.second == upper_bound
```

---

## 23.4 数值算法

`<numeric>` 头文件里的数值算法：

```cpp
#include <numeric>

std::vector<int> v = {1, 2, 3, 4, 5};

// accumulate：累积计算（求和、求积等）
int sum = std::accumulate(v.begin(), v.end(), 0);     // 15（从 0 开始累加）
int product = std::accumulate(v.begin(), v.end(), 1,
                              [](int acc, int n) { return acc * n; }); // 120

// reduce（C++17）：和 accumulate 类似，但可以并行
int sum2 = std::reduce(v.begin(), v.end());  // 15，默认从 0 开始

// inner_product：内积（点积）
std::vector<int> a = {1, 2, 3};
std::vector<int> b = {4, 5, 6};
int dot = std::inner_product(a.begin(), a.end(), b.begin(), 0);
// 1*4 + 2*5 + 3*6 = 32

// partial_sum：前缀和
std::vector<int> prefix(v.size());
std::partial_sum(v.begin(), v.end(), prefix.begin());
// prefix: {1, 3, 6, 10, 15}

// iota：填充连续整数
std::vector<int> nums(10);
std::iota(nums.begin(), nums.end(), 0);  // 从 0 开始
// nums: {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}

std::iota(nums.begin(), nums.end(), 1);  // 从 1 开始
// nums: {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
```

---

## 23.5 其他常用算法

### rotate

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// rotate：将 [first, middle) 旋转到末尾
std::rotate(v.begin(), v.begin() + 2, v.end());
// 原来 [1,2] 移到末尾：{3, 4, 5, 1, 2}
```

### partition

```cpp
std::vector<int> v = {1, 5, 2, 8, 3, 7, 4, 6};

// partition：把满足条件的元素移到前面，不满足的移到后面
auto mid = std::partition(v.begin(), v.end(), [](int n) {
    return n % 2 == 0;  // 偶数在前
});
// v 的前半段是偶数，后半段是奇数（顺序不保证）
// mid 指向后半段的开始

// stable_partition：保持相对顺序的 partition
```

### unique

```cpp
std::vector<int> v = {1, 1, 2, 2, 3, 3, 2, 2};

// unique：移除相邻的重复元素（只处理相邻重复！）
auto new_end = std::unique(v.begin(), v.end());
v.erase(new_end, v.end());
// v: {1, 2, 3, 2}（注意 2 出现两次，因为不是相邻的）

// 要去掉所有重复，先排序
v = {3, 1, 4, 1, 5, 9, 2, 6, 5};
std::sort(v.begin(), v.end());
v.erase(std::unique(v.begin(), v.end()), v.end());
// v: {1, 2, 3, 4, 5, 6, 9}
```

### merge

```cpp
std::vector<int> a = {1, 3, 5, 7};
std::vector<int> b = {2, 4, 6, 8};
std::vector<int> merged(a.size() + b.size());

// merge：合并两个已排序的序列
std::merge(a.begin(), a.end(), b.begin(), b.end(), merged.begin());
// merged: {1, 2, 3, 4, 5, 6, 7, 8}
```

### shuffle

```cpp
#include <random>

std::vector<int> v = {1, 2, 3, 4, 5};
std::mt19937 rng(std::random_device{}());

// shuffle：随机打乱
std::shuffle(v.begin(), v.end(), rng);
// v 的顺序被随机打乱
```

---

## 23.6 避免手写循环的思维方式

用算法库的好处不只是代码更短——它让代码的**意图更清晰**。

来看一个例子：统计 Taco Token 列表里各种类型的数量。

**手写循环版本**：

```cpp
std::unordered_map<TokenType, int> type_counts;
for (const auto& token : tokens) {
    type_counts[token.type]++;
}
```

**算法版本**：

```cpp
// 用 for_each 加 lambda
std::unordered_map<TokenType, int> type_counts;
std::for_each(tokens.begin(), tokens.end(), [&type_counts](const Token& t) {
    type_counts[t.type]++;
});
```

这两个版本差不多，手写循环其实更清晰。但有些场景算法版本明显更好：

**找到所有标识符**：

```cpp
// 手写循环
std::vector<std::string> identifiers;
for (const auto& t : tokens) {
    if (t.type == TokenType::Identifier) {
        identifiers.push_back(t.value);
    }
}

// 算法版本：意图一眼就懂
std::vector<std::string> identifiers;
std::transform(
    tokens.begin(), tokens.end(),
    std::back_inserter(identifiers),
    // 但这里不对，transform 不能做过滤...
    // 需要先 copy_if 再 transform，或者用 ranges
);

// 其实这个场景手写循环更好
// ranges 版本（C++20）更清晰：
auto idents = tokens
    | std::views::filter([](const Token& t) {
          return t.type == TokenType::Identifier;
      })
    | std::views::transform([](const Token& t) {
          return t.value;
      });
```

**对 Token 值排序**：

```cpp
std::vector<std::string> values;
for (const auto& t : tokens) {
    values.push_back(t.value);
}
std::sort(values.begin(), values.end());

// 用算法组合（虽然也可以，但未必更简洁）
```

**结论**：不是所有情况都适合用算法替代手写循环，要看哪种更清晰。但了解这些算法，在合适的时候使用它们，是 C++ 代码质量的标志之一。

---

## 小结

**不修改序列的算法**：`find`/`find_if`、`count`/`count_if`、`all_of`/`any_of`/`none_of`、`for_each`、`min_element`/`max_element`。

**修改序列的算法**：`transform`（映射）、`copy_if`（过滤复制）、`replace`/`replace_if`、`remove`/`remove_if`（配合 `erase` 使用）、`fill`/`generate`。

**排序算法**：`sort`（不稳定）、`stable_sort`（稳定）、`partial_sort`（部分排序）、`nth_element`（第 k 小）、`binary_search`/`lower_bound`/`upper_bound`（已排序序列查找）。

**数值算法**：`accumulate`/`reduce`（累积）、`inner_product`（内积）、`partial_sum`（前缀和）、`iota`（连续填充）。

**其他**：`rotate`（旋转）、`partition`（分区）、`unique`（去重相邻）、`merge`（合并有序序列）、`shuffle`（随机打乱）。

---

下一章讲 Lambda 表达式——它是算法的最佳搭档，也是理解 Taco 闭包语法的 C++ 基础。
# 第二十四章：Lambda 表达式

---

Lambda 表达式（lambda expression）是 C++11 引入的特性，让你可以在需要的地方直接定义匿名函数，而不需要单独命名一个函数或定义一个函数对象类。Lambda 和算法库是绝配——传给 `sort`、`find_if`、`transform` 的比较器和谓词，用 lambda 写最自然。

Lambda 也是理解 Taco 闭包语法的基础——Taco 的 `{ x in x * 2 }` 和 C++ 的 `[](int x) { return x * 2; }` 在设计思想上完全一致。

---

## 24.1 Lambda 的语法

Lambda 的完整语法：

```cpp
[capture_list](parameter_list) -> return_type {
    function_body
}
```

各个部分：
- **`[capture_list]`**：捕获列表，声明从外部作用域捕获哪些变量
- **`(parameter_list)`**：参数列表，和普通函数一样
- **`-> return_type`**：返回类型，大多数情况下可以省略（编译器自动推导）
- **`{ function_body }`**：函数体

---

### 最简单的 lambda

```cpp
// 没有参数，没有返回值
auto greet = []() {
    std::cout << "Hello!\n";
};
greet();  // Hello!

// 有参数
auto add = [](int a, int b) {
    return a + b;
};
int sum = add(3, 4);  // 7

// 省略参数列表的括号（无参数时可省）
auto say_hi = [] {
    std::cout << "Hi!\n";
};
say_hi();
```

### 立即调用

Lambda 可以在定义后立刻调用：

```cpp
int result = [](int a, int b) {
    return a + b;
}(3, 4);  // 立刻调用，传入 3 和 4
// result == 7
```

---

### 与算法配合

这是 lambda 最常见的用途：

```cpp
std::vector<int> v = {5, 3, 8, 1, 9, 2, 7, 4, 6};

// sort：自定义比较器
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a > b;  // 降序
});

// find_if：自定义条件
auto it = std::find_if(v.begin(), v.end(), [](int n) {
    return n > 5;  // 找第一个大于 5 的
});

// transform：自定义映射
std::vector<int> squares(v.size());
std::transform(v.begin(), v.end(), squares.begin(), [](int n) {
    return n * n;
});

// count_if：自定义计数条件
int count = std::count_if(v.begin(), v.end(), [](int n) {
    return n % 2 == 0;  // 偶数个数
});
```

---

## 24.2 捕获列表：值捕获与引用捕获

Lambda 可以"捕获"外部作用域里的变量——这就是为什么 lambda 叫做"闭包"（closure）。

### 值捕获

```cpp
int x = 10;
int y = 20;

// 值捕获：捕获 x 和 y 的当前值（拷贝）
auto add_xy = [x, y](int z) {
    return x + y + z;
};

x = 100;  // 修改 x，但 lambda 捕获的是原来的值
y = 200;

int result = add_xy(5);
// result == 10 + 20 + 5 == 35（不是 100 + 200 + 5）
// 因为 lambda 捕获的是定义时的 x=10, y=20
```

值捕获是拷贝，lambda 内部有自己的变量副本，外部修改不影响 lambda，lambda 内部修改也不影响外部（默认情况下）。

### 引用捕获

```cpp
int sum = 0;

auto accumulate = [&sum](int n) {
    sum += n;  // 通过引用修改外部的 sum
};

for (int i = 1; i <= 5; i++) {
    accumulate(i);
}
std::cout << sum << "\n";  // 15
```

引用捕获让 lambda 直接操作外部变量，修改会反映到外部。

### 捕获所有变量

```cpp
int a = 1, b = 2, c = 3;

// [=]：值捕获所有外部变量
auto f1 = [=]() { return a + b + c; };

// [&]：引用捕获所有外部变量
auto f2 = [&]() { a++; b++; c++; };

f2();
std::cout << a << " " << b << " " << c << "\n";  // 2 3 4
```

### 混合捕获

```cpp
int x = 10;
int y = 20;

// 值捕获 x，引用捕获 y
auto f = [x, &y]() {
    // x 是值捕获，不能修改（修改会编译错误，除非用 mutable）
    y = 100;  // y 是引用捕获，可以修改
    return x + y;
};

// [=, &y]：默认值捕获，但 y 用引用捕获
auto f2 = [=, &y]() {
    y = 200;
    return x + y;
};

// [&, x]：默认引用捕获，但 x 用值捕获
auto f3 = [&, x]() {
    y = 300;  // 引用捕获
    return x + y;  // x 是值捕获
};
```

### mutable lambda

值捕获的变量在 lambda 里默认是 `const`，不能修改。用 `mutable` 关键字可以允许修改（但修改的是 lambda 内部的副本，不影响外部）：

```cpp
int count = 0;

// 没有 mutable：不能修改值捕获的变量
auto counter = [count]() mutable {
    count++;  // 修改的是 lambda 内部的 count 副本
    return count;
};

std::cout << counter() << "\n";  // 1
std::cout << counter() << "\n";  // 2
std::cout << count << "\n";      // 0（外部的 count 没变）
```

这和 Taco 的 `makeCounter` 闭包有点像，但区别是：Taco 用引用捕获（通过 `shared_ptr<Environment>`），修改真的会反映到外部。C++ 的 lambda 值捕获是副本，不影响外部。

---

### 捕获 this

在类的成员函数里，lambda 可以捕获 `this` 来访问成员：

```cpp
class Evaluator {
public:
    void process_tokens(std::vector<Token>& tokens) {
        // 捕获 this，可以访问 Evaluator 的成员
        std::for_each(tokens.begin(), tokens.end(),
                      [this](const Token& t) {
                          this->handle_token(t);  // 访问成员函数
                      });

        // 也可以直接写（this 是隐式的）
        std::for_each(tokens.begin(), tokens.end(),
                      [this](const Token& t) {
                          handle_token(t);  // 同上，this 隐式
                      });
    }

private:
    void handle_token(const Token& t) { ... }
};
```

---

## 24.3 Lambda 与算法的配合

Lambda 让算法的可读性大幅提升。来看一些实际的 Taco 项目里的例子：

```cpp
// 过滤掉注释 Token
std::vector<Token> filtered;
std::copy_if(tokens.begin(), tokens.end(),
             std::back_inserter(filtered),
             [](const Token& t) {
                 return t.type != TokenType::EndOfFile;
             });

// 统计每种 Token 类型的数量
std::unordered_map<TokenType, int> counts;
std::for_each(tokens.begin(), tokens.end(),
              [&counts](const Token& t) {
                  counts[t.type]++;
              });

// 找到第一个错误 Token（假设有 Error 类型）
auto error_it = std::find_if(tokens.begin(), tokens.end(),
                              [](const Token& t) {
                                  return t.line == 0;  // 无效行号表示错误
                              });

// 把所有 Token 的值提取到字符串列表
std::vector<std::string> values;
std::transform(tokens.begin(), tokens.end(),
               std::back_inserter(values),
               [](const Token& t) { return t.value; });

// 检查是否所有 Token 都有有效行号
bool all_valid = std::all_of(tokens.begin(), tokens.end(),
                              [](const Token& t) { return t.line > 0; });
```

---

## 24.4 std::function

`std::function<R(Args...)>` 是一个可以存储任何可调用对象（函数、lambda、函数指针、函数对象）的类型。

```cpp
#include <functional>

// 存储函数指针
int add(int a, int b) { return a + b; }
std::function<int(int, int)> f1 = add;

// 存储 lambda
std::function<int(int, int)> f2 = [](int a, int b) { return a * b; };

// 存储成员函数（需要 bind 或 lambda）
struct Calculator {
    int subtract(int a, int b) { return a - b; }
};
Calculator calc;
std::function<int(int, int)> f3 = [&calc](int a, int b) {
    return calc.subtract(a, b);
};

// 调用
std::cout << f1(3, 4) << "\n";  // 7
std::cout << f2(3, 4) << "\n";  // 12
std::cout << f3(10, 3) << "\n"; // 7
```

`std::function` 的主要用途是当需要把函数作为参数传递，而调用者不知道具体是什么类型的可调用对象时：

```cpp
// Taco 内置函数：存储为 std::function
using NativeFunction = std::function<TacoValue(std::vector<TacoValue>)>;

struct TacoNative {
    std::string    name;
    NativeFunction fn;
};

// 注册内置函数
env->define("print", std::make_shared<TacoNative>(TacoNative{
    "print",
    [](std::vector<TacoValue> args) -> TacoValue {
        std::cout << value_to_string(args[0]) << "\n";
        return nullptr;
    }
}));
```

### std::function 的开销

`std::function` 有一定的运行时开销：
- 存储可调用对象需要动态分配（如果对象超过小对象优化的阈值）
- 调用时有虚函数调用的开销（类型擦除的代价）

在性能敏感的热路径上，直接用模板参数传递可调用对象比 `std::function` 快：

```cpp
// 用 std::function：有运行时开销
void process(std::function<void(int)> fn, std::vector<int>& v) {
    for (int n : v) fn(n);
}

// 用模板：零开销，编译时决定
template<typename Fn>
void process(Fn fn, std::vector<int>& v) {
    for (int n : v) fn(n);
}
```

在 Taco 的内置函数里用 `std::function` 是合理的——内置函数调用不是性能瓶颈，而且需要在运行时存储不同类型的函数，类型擦除是必要的。

---

## *More About Lambda*：Lambda 的底层——函数对象

> 第一次读可以跳过。

### Lambda 的本质

Lambda 在底层是一个由编译器生成的**函数对象**（function object，也叫 functor）——一个重载了 `operator()` 的类。

```cpp
int x = 10;
auto add_x = [x](int n) { return x + n; };
```

编译器把这个 lambda 展开成类似这样的类：

```cpp
class __lambda_add_x {
public:
    __lambda_add_x(int x) : m_x(x) {}  // 捕获 x

    int operator()(int n) const {
        return m_x + n;
    }

private:
    int m_x;  // 值捕获的副本
};

// lambda 实际上是这个类的一个实例
__lambda_add_x add_x(x);  // 等价于 auto add_x = [x](int n) { return x + n; };
```

这就解释了：
- 为什么值捕获是副本（构造时把变量值存为成员）
- 为什么引用捕获是引用（成员是引用类型）
- 为什么 lambda 可以作为算法的参数（它是一个可调用对象）
- 为什么 `mutable` 让你能修改值捕获（去掉了 `operator()` 的 `const`）

### 与手写函数对象的对比

在 lambda 之前（C++11 之前），每次需要传一个自定义函数给算法，都要手写一个类：

```cpp
// C++11 之前：手写函数对象
struct IsGreaterThan {
    int threshold;
    IsGreaterThan(int t) : threshold(t) {}
    bool operator()(int n) const { return n > threshold; }
};

std::find_if(v.begin(), v.end(), IsGreaterThan(5));

// C++11 之后：lambda
std::find_if(v.begin(), v.end(), [](int n) { return n > 5; });
```

Lambda 让函数对象的定义和使用在同一个地方，不需要在别处定义类，代码更紧凑，意图更清晰。

---

## 小结

**Lambda 语法**：`[capture](params) -> return_type { body }`。返回类型通常可以省略，无参数时括号可以省略。

**捕获列表**：
- `[x, y]`：值捕获 x 和 y（副本）
- `[&x, &y]`：引用捕获 x 和 y
- `[=]`：值捕获所有外部变量
- `[&]`：引用捕获所有外部变量
- `[=, &y]`：默认值捕获，y 用引用捕获

**mutable**：允许修改值捕获的变量（修改的是 lambda 内部的副本）。

**`std::function<R(Args...)>`**：存储任意可调用对象的包装器，有运行时开销，适合需要类型擦除的场景。

**底层**：Lambda 是编译器生成的函数对象类，捕获的变量存为成员变量，`operator()` 实现函数调用。

---

下一章是第五部分的项目章节：用 STL 容器和 Lambda 实现 Taco 的 array、map 和内置标准库，以及 pipeline 风格（v4）。
# 第二十五章：项目 v4——array、map、enum 与标准库

---

第五部分学完了 STL 容器、迭代器、算法、Lambda。现在用这些知识做 v4 最重要的两件事：

1. **实现 Taco 的 array 和 map 类型**，以及对应的 pipeline 方法（`filter`、`map`、`each`、`reduce`）
2. **实现 Taco 的内置标准库**，包括字符串、文件、系统操作等

v4 结束时，Taco 能运行这样的代码：

```taco
// array 和 pipeline
var nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

nums
    .filter { n in n % 2 == 0 }
    .map { n in n * n }
    .each { n in print(n) };
// 4 16 36 64 100

// array 方法
var arr = [3, 1, 4, 1, 5, 9, 2, 6];
print(arr.sum());         // 31
print(arr.min());         // 1
print(arr.max());         // 9
print(arr.len());         // 8
print(arr.contains(5));   // true
print(arr.getFirst(3));   // [3, 1, 4]
print(arr.getLast(3));    // [2, 6]... 不对，是最后3个 [9, 2, 6]

// map（字典）
var person = {"name": "Miguel", "age": 12};
print(person["name"]);    // Miguel
print(person.name);       // Miguel（点语法）
person["city"] = "Oaxaca";
print(person.getKeys());  // ["name", "age", "city"]

// 字符串方法
var s = "Hello, World!";
print(s.len());           // 13
print(s.upper());         // HELLO, WORLD!
print(s.contains("World")); // true
print(s.split(", "));     // ["Hello", "World!"]
print(s.getChars());      // ["H", "e", "l", ...]

// 文件操作
var content = cat("test.taco");
print(content.getLines().len());

// 系统操作
var files = ls(".");
files.filter { f in f.endsWith(".taco") }.each { f in print(f) };
```

---

## 25.1 用 STL 容器实现 Taco 的 array 和 map

### 更新值类型

```cpp
// value.h（v4 版本）
#pragma once
#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

struct TacoFunction;
struct TacoNative;

// Taco 的 array：vector<TacoValue> 的 shared_ptr 包装
// 用 shared_ptr 是因为 array 是引用类型（赋值是共享，不是拷贝）
struct TacoArray;
struct TacoMap;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>
>;

// 定义在 value.h 里（或 array.h/map.h 里）
struct TacoArray {
    std::vector<TacoValue> elements;

    TacoArray() = default;
    explicit TacoArray(std::vector<TacoValue> elems)
        : elements(std::move(elems)) {}
};

struct TacoMap {
    // 用 vector<pair> 保持插入顺序（unordered_map 不保证顺序）
    std::vector<std::pair<std::string, TacoValue>> entries;

    TacoMap() = default;

    TacoValue get(const std::string& key) const;
    void set(const std::string& key, TacoValue val);
    bool has(const std::string& key) const;
    void remove(const std::string& key);
};
```

为什么 `TacoArray` 和 `TacoMap` 用 `shared_ptr` 包装？

在 Taco 里，array 和 map 是**引用类型**（和 class 一样）：

```taco
var a = [1, 2, 3];
var b = a;        // b 和 a 指向同一个 array
b.push(4);
print(a.len());   // 4，a 也变了
```

用 `shared_ptr` 可以让多个变量指向同一个底层数组，赋值时只拷贝指针，不拷贝数组内容。

---

### TacoMap 的实现

```cpp
// value.cpp（部分）

TacoValue TacoMap::get(const std::string& key) const {
    for (const auto& [k, v] : entries) {
        if (k == key) return v;
    }
    throw std::runtime_error("🌮 Key '" + key + "' not found.");
}

void TacoMap::set(const std::string& key, TacoValue val) {
    for (auto& [k, v] : entries) {
        if (k == key) {
            v = std::move(val);
            return;
        }
    }
    entries.push_back({key, std::move(val)});
}

bool TacoMap::has(const std::string& key) const {
    for (const auto& [k, v] : entries) {
        if (k == key) return true;
    }
    return false;
}

void TacoMap::remove(const std::string& key) {
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
                       [&key](const auto& pair) { return pair.first == key; }),
        entries.end()
    );
}
```

这里用 `vector<pair>` 而不是 `unordered_map`，是因为 Taco 的 map 需要保持插入顺序（和 Python 3.7+ 的 `dict` 类似）。查找是 O(n)，对于小型字典完全够用。如果需要高性能，可以改成 `unordered_map`。

---

## 25.2 实现 enum

Taco 的 enum 是一组命名常量：

```taco
enum Direction {
    North,
    South,
    East,
    West
}

var d = Direction.North;
switch (d) {
    case Direction.North { print("going north"); }
    default { print("other"); }
}
```

在 C++ 里，Taco 的 enum 值用字符串表示：`Direction.North` 就是字符串 `"Direction.North"`。

```cpp
// ast.h 新增
struct EnumDecl : Expr {
    std::string              name;
    std::vector<std::string> variants;  // 枚举值的名字列表

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "enum " + name; }
};
```

```cpp
// ast.cpp
TacoValue EnumDecl::evaluate(Evaluator& eval) const {
    // 把每个变体注册为字符串常量
    // Direction.North = "Direction.North"
    for (const auto& variant : variants) {
        std::string full_name = name + "." + variant;
        eval.current_env()->define(name + "." + variant,
                                   std::string(full_name));
    }
    return nullptr;
}
```

带关联值的 enum：

```taco
enum Result {
    Ok(value),
    Err(message)
}

var r = Result.Ok(42);
```

带关联值的枚举变体是一个函数，调用它返回一个包含类型标签和值的 map：

```cpp
// Result.Ok 是一个函数，调用后返回 {"__type": "Result.Ok", "value": 42}
TacoValue make_enum_variant(const std::string& tag, std::vector<TacoValue> args) {
    auto m = std::make_shared<TacoMap>();
    m->set("__type", std::string(tag));

    // 根据关联值数量绑定到对应的字段名
    // 简化实现：只有一个关联值时用 "value"
    if (!args.empty()) {
        m->set("value", args[0]);
    }
    return m;
}
```

---

## 25.3 实现 pipeline：filter、map、each、reduce

Pipeline 是 Taco 最有特色的功能。`arr.filter { x in x > 3 }` 这样的方法链，每个方法接受一个闭包，返回新的 array。

在 C++ 里，这些方法挂在 `TacoArray` 上（通过方法分派），或者在求值器里作为内置方法处理。

### 方法调用的 AST 节点

```cpp
// ast.h 新增
struct MethodCallExpr : Expr {
    ExprPtr              object;     // 调用方法的对象
    std::string          method;     // 方法名
    std::vector<ExprPtr> args;       // 普通参数
    ExprPtr              closure_arg; // 闭包参数（{ x in ... } 风格）

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return object->to_string() + "." + method + "(...)";
    }
};
```

### 方法调用的求值

```cpp
// ast.cpp
TacoValue MethodCallExpr::evaluate(Evaluator& eval) const {
    TacoValue obj = eval.evaluate(object.get());

    // 求值普通参数
    std::vector<TacoValue> arg_vals;
    for (const auto& arg : args) {
        arg_vals.push_back(eval.evaluate(arg.get()));
    }

    // 求值闭包参数（如果有）
    TacoValue closure_val = nullptr;
    if (closure_arg) {
        closure_val = eval.evaluate(closure_arg.get());
    }

    // 分派到对应类型的方法
    if (std::holds_alternative<std::string>(obj)) {
        return dispatch_string_method(
            std::get<std::string>(obj), method, arg_vals, closure_val, eval
        );
    }

    if (std::holds_alternative<std::shared_ptr<TacoArray>>(obj)) {
        return dispatch_array_method(
            std::get<std::shared_ptr<TacoArray>>(obj), method, arg_vals, closure_val, eval
        );
    }

    if (std::holds_alternative<std::shared_ptr<TacoMap>>(obj)) {
        return dispatch_map_method(
            std::get<std::shared_ptr<TacoMap>>(obj), method, arg_vals, closure_val, eval
        );
    }

    throw std::runtime_error(
        "🌮 '" + value_to_string(obj) + "' has no method '" + method + "'."
    );
}
```

### array 的方法实现

```cpp
// array_methods.cpp

// 调用一个 Taco 函数（闭包）
static TacoValue call_closure(Evaluator& eval, const TacoValue& fn,
                               std::vector<TacoValue> args) {
    if (std::holds_alternative<std::shared_ptr<TacoFunction>>(fn)) {
        auto& f = std::get<std::shared_ptr<TacoFunction>>(fn);
        return call_user_function(eval, f, args, {});
    }
    throw std::runtime_error("🌮 Expected a function.");
}

TacoValue dispatch_array_method(
    std::shared_ptr<TacoArray> arr,
    const std::string& method,
    const std::vector<TacoValue>& args,
    const TacoValue& closure,
    Evaluator& eval)
{
    // ── filter ──────────────────────────────────────────
    if (method == "filter") {
        auto result = std::make_shared<TacoArray>();
        for (const auto& elem : arr->elements) {
            TacoValue keep = call_closure(eval, closure, {elem});
            if (is_truthy(keep)) {
                result->elements.push_back(elem);
            }
        }
        return result;
    }

    // ── map ─────────────────────────────────────────────
    if (method == "map") {
        auto result = std::make_shared<TacoArray>();
        result->elements.reserve(arr->elements.size());
        std::transform(
            arr->elements.begin(), arr->elements.end(),
            std::back_inserter(result->elements),
            [&](const TacoValue& elem) {
                return call_closure(eval, closure, {elem});
            }
        );
        return result;
    }

    // ── each ────────────────────────────────────────────
    if (method == "each") {
        std::for_each(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                call_closure(eval, closure, {elem});
            }
        );
        return nullptr;
    }

    // ── reduce ──────────────────────────────────────────
    if (method == "reduce") {
        if (arr->elements.empty()) {
            return args.empty() ? TacoValue{nullptr} : args[0];
        }

        TacoValue acc = args.empty() ? arr->elements[0] : args[0];
        int start = args.empty() ? 1 : 0;

        for (int i = start; i < static_cast<int>(arr->elements.size()); i++) {
            acc = call_closure(eval, closure, {acc, arr->elements[i]});
        }
        return acc;
    }

    // ── find ────────────────────────────────────────────
    if (method == "find") {
        auto it = std::find_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                return is_truthy(call_closure(eval, closure, {elem}));
            }
        );
        return it != arr->elements.end() ? *it : TacoValue{nullptr};
    }

    // ── sortBy ──────────────────────────────────────────
    if (method == "sortBy") {
        auto result = std::make_shared<TacoArray>(*arr);  // 拷贝
        std::sort(
            result->elements.begin(), result->elements.end(),
            [&](const TacoValue& a, const TacoValue& b) {
                TacoValue ka = call_closure(eval, closure, {a});
                TacoValue kb = call_closure(eval, closure, {b});
                if (std::holds_alternative<double>(ka) &&
                    std::holds_alternative<double>(kb)) {
                    return std::get<double>(ka) < std::get<double>(kb);
                }
                if (std::holds_alternative<std::string>(ka) &&
                    std::holds_alternative<std::string>(kb)) {
                    return std::get<std::string>(ka) < std::get<std::string>(kb);
                }
                return false;
            }
        );
        return result;
    }

    // ── push ────────────────────────────────────────────
    if (method == "push") {
        if (args.empty()) throw std::runtime_error("🌮 push() needs an argument.");
        arr->elements.push_back(args[0]);
        return nullptr;
    }

    // ── pop ─────────────────────────────────────────────
    if (method == "pop") {
        if (arr->elements.empty()) throw std::runtime_error("🌮 pop() on empty array.");
        TacoValue last = arr->elements.back();
        arr->elements.pop_back();
        return last;
    }

    // ── len ─────────────────────────────────────────────
    if (method == "len") {
        return static_cast<double>(arr->elements.size());
    }

    // ── contains ────────────────────────────────────────
    if (method == "contains") {
        if (args.empty()) throw std::runtime_error("🌮 contains() needs an argument.");
        auto it = std::find_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) { return values_equal(elem, args[0]); }
        );
        return it != arr->elements.end();
    }

    // ── sum ─────────────────────────────────────────────
    if (method == "sum") {
        double total = std::accumulate(
            arr->elements.begin(), arr->elements.end(), 0.0,
            [](double acc, const TacoValue& v) {
                if (std::holds_alternative<double>(v))
                    return acc + std::get<double>(v);
                return acc;
            }
        );
        return total;
    }

    // ── avg ─────────────────────────────────────────────
    if (method == "avg") {
        if (arr->elements.empty()) return 0.0;
        TacoValue sum_val = dispatch_array_method(arr, "sum", {}, nullptr, eval);
        return std::get<double>(sum_val) / arr->elements.size();
    }

    // ── min / max ────────────────────────────────────────
    if (method == "min" || method == "max") {
        if (arr->elements.empty())
            throw std::runtime_error("🌮 min()/max() on empty array.");

        auto cmp = [](const TacoValue& a, const TacoValue& b) {
            if (std::holds_alternative<double>(a) &&
                std::holds_alternative<double>(b)) {
                return std::get<double>(a) < std::get<double>(b);
            }
            return false;
        };

        auto it = (method == "min")
            ? std::min_element(arr->elements.begin(), arr->elements.end(), cmp)
            : std::max_element(arr->elements.begin(), arr->elements.end(), cmp);

        return *it;
    }

    // ── getFirst / getLast ───────────────────────────────
    if (method == "getFirst") {
        if (arr->elements.empty()) return nullptr;
        if (args.empty()) return arr->elements.front();

        int n = static_cast<int>(std::get<double>(args[0]));
        int count = std::min(n, static_cast<int>(arr->elements.size()));
        auto result = std::make_shared<TacoArray>();
        result->elements.assign(arr->elements.begin(),
                                arr->elements.begin() + count);
        return result;
    }

    if (method == "getLast") {
        if (arr->elements.empty()) return nullptr;
        if (args.empty()) return arr->elements.back();

        int n = static_cast<int>(std::get<double>(args[0]));
        int count = std::min(n, static_cast<int>(arr->elements.size()));
        auto result = std::make_shared<TacoArray>();
        result->elements.assign(arr->elements.end() - count,
                                arr->elements.end());
        return result;
    }

    // ── findFirst / findLast ─────────────────────────────
    if (method == "findFirst") {
        if (arr->elements.empty()) return nullptr;
        return arr->elements.front();
    }

    if (method == "findLast") {
        if (arr->elements.empty()) return nullptr;
        return arr->elements.back();
    }

    // ── groupBy ──────────────────────────────────────────
    if (method == "groupBy") {
        auto result = std::make_shared<TacoMap>();
        for (const auto& elem : arr->elements) {
            TacoValue key_val = call_closure(eval, closure, {elem});
            std::string key = value_to_string(key_val);

            if (!result->has(key)) {
                result->set(key, std::make_shared<TacoArray>());
            }
            auto& group = std::get<std::shared_ptr<TacoArray>>(result->get(key));
            group->elements.push_back(elem);
        }
        return result;
    }

    // ── countBy ──────────────────────────────────────────
    if (method == "countBy") {
        int count = static_cast<int>(std::count_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                return is_truthy(call_closure(eval, closure, {elem}));
            }
        ));
        return static_cast<double>(count);
    }

    throw std::runtime_error(
        "🌮 Array has no method '" + method + "'."
    );
}
```

---

## 25.4 字符串方法的实现

```cpp
// string_methods.cpp

TacoValue dispatch_string_method(
    const std::string& str,
    const std::string& method,
    const std::vector<TacoValue>& args,
    const TacoValue& closure,
    Evaluator& eval)
{
    if (method == "len") {
        return static_cast<double>(str.size());
    }

    if (method == "upper") {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    if (method == "lower") {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    if (method == "contains") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 contains() needs a string argument.");
        return str.find(std::get<std::string>(args[0])) != std::string::npos;
    }

    if (method == "startsWith") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 startsWith() needs a string argument.");
        const auto& prefix = std::get<std::string>(args[0]);
        return str.size() >= prefix.size() &&
               str.substr(0, prefix.size()) == prefix;
    }

    if (method == "endsWith") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 endsWith() needs a string argument.");
        const auto& suffix = std::get<std::string>(args[0]);
        return str.size() >= suffix.size() &&
               str.substr(str.size() - suffix.size()) == suffix;
    }

    if (method == "trimSpace") {
        std::string result = str;
        // 去掉头部空白
        result.erase(result.begin(),
                     std::find_if(result.begin(), result.end(),
                                  [](char c) { return !std::isspace(c); }));
        // 去掉尾部空白
        result.erase(std::find_if(result.rbegin(), result.rend(),
                                  [](char c) { return !std::isspace(c); }).base(),
                     result.end());
        return result;
    }

    if (method == "split") {
        std::string delimiter = args.empty() ? " "
            : std::get<std::string>(args[0]);

        auto result = std::make_shared<TacoArray>();
        std::string::size_type start = 0;
        std::string::size_type pos;

        while ((pos = str.find(delimiter, start)) != std::string::npos) {
            result->elements.push_back(str.substr(start, pos - start));
            start = pos + delimiter.size();
        }
        result->elements.push_back(str.substr(start));
        return result;
    }

    if (method == "getLines") {
        return dispatch_string_method(str, "split", {std::string("\n")},
                                     nullptr, eval);
    }

    if (method == "getWords") {
        auto result = std::make_shared<TacoArray>();
        std::istringstream iss(str);
        std::string word;
        while (iss >> word) {
            result->elements.push_back(word);
        }
        return result;
    }

    if (method == "getChars") {
        auto result = std::make_shared<TacoArray>();
        for (char c : str) {
            result->elements.push_back(std::string(1, c));
        }
        return result;
    }

    if (method == "replaceStr") {
        if (args.size() < 2) throw std::runtime_error("🌮 replaceStr() needs 2 arguments.");
        const auto& from = std::get<std::string>(args[0]);
        const auto& to   = std::get<std::string>(args[1]);

        std::string result = str;
        std::string::size_type pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
        return result;
    }

    if (method == "findStr") {
        if (args.empty()) throw std::runtime_error("🌮 findStr() needs an argument.");
        auto pos = str.find(std::get<std::string>(args[0]));
        return pos != std::string::npos
            ? TacoValue{static_cast<double>(pos)}
            : TacoValue{-1.0};
    }

    throw std::runtime_error("🌮 String has no method '" + method + "'.");
}
```

---

## 25.5 内置标准库：文件和系统

```cpp
// builtins.cpp（新增部分）

void register_builtins(Environment::Ptr env) {
    // 之前注册的 print, type, input, number, string...

    // ── 文件系统 ────────────────────────────────────────

    // cat：读取文件内容
    env->define("cat", std::make_shared<TacoNative>(TacoNative{
        "cat",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 cat() needs a path.");
            const auto& path = std::get<std::string>(args[0]);
            std::ifstream f(path);
            if (!f) throw std::runtime_error("🌮 Cannot read: " + path);
            std::ostringstream buf;
            buf << f.rdbuf();
            return buf.str();
        }
    }));

    // echo：写文件
    env->define("echo", std::make_shared<TacoNative>(TacoNative{
        "echo",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 echo() needs content and path.");
            const auto& content = value_to_string(args[0]);
            const auto& path    = std::get<std::string>(args[1]);
            std::ofstream f(path);
            if (!f) throw std::runtime_error("🌮 Cannot write: " + path);
            f << content;
            return nullptr;
        }
    }));

    // ls：列出目录
    env->define("ls", std::make_shared<TacoNative>(TacoNative{
        "ls",
        [](std::vector<TacoValue> args) -> TacoValue {
            namespace fs = std::filesystem;
            std::string path = args.empty() ? "." : std::get<std::string>(args[0]);
            auto result = std::make_shared<TacoArray>();
            for (const auto& entry : fs::directory_iterator(path)) {
                result->elements.push_back(entry.path().filename().string());
            }
            return result;
        }
    }));

    // mkdir：创建目录
    env->define("mkdir", std::make_shared<TacoNative>(TacoNative{
        "mkdir",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 mkdir() needs a path.");
            namespace fs = std::filesystem;
            fs::create_directories(std::get<std::string>(args[0]));
            return nullptr;
        }
    }));

    // rm：删除文件
    env->define("rm", std::make_shared<TacoNative>(TacoNative{
        "rm",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 rm() needs a path.");
            namespace fs = std::filesystem;
            fs::remove(std::get<std::string>(args[0]));
            return nullptr;
        }
    }));

    // mv：移动/重命名
    env->define("mv", std::make_shared<TacoNative>(TacoNative{
        "mv",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 mv() needs src and dst.");
            namespace fs = std::filesystem;
            fs::rename(std::get<std::string>(args[0]),
                       std::get<std::string>(args[1]));
            return nullptr;
        }
    }));

    // cp：复制
    env->define("cp", std::make_shared<TacoNative>(TacoNative{
        "cp",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 cp() needs src and dst.");
            namespace fs = std::filesystem;
            fs::copy(std::get<std::string>(args[0]),
                     std::get<std::string>(args[1]));
            return nullptr;
        }
    }));

    // pwd：当前目录
    env->define("pwd", std::make_shared<TacoNative>(TacoNative{
        "pwd",
        [](std::vector<TacoValue>) -> TacoValue {
            return std::filesystem::current_path().string();
        }
    }));

    // exists：文件是否存在
    env->define("exists", std::make_shared<TacoNative>(TacoNative{
        "exists",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 exists() needs a path.");
            return std::filesystem::exists(std::get<std::string>(args[0]));
        }
    }));

    // ── 系统 ────────────────────────────────────────────

    // exec：执行系统命令，返回输出
    env->define("exec", std::make_shared<TacoNative>(TacoNative{
        "exec",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 exec() needs a command.");
            const auto& cmd = std::get<std::string>(args[0]);

            // 用 popen 捕获命令输出
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) throw std::runtime_error("🌮 exec() failed.");

            std::string result;
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                result += buffer;
            }
            pclose(pipe);
            return result;
        }
    }));

    // env：读取环境变量
    env->define("env", std::make_shared<TacoNative>(TacoNative{
        "env",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 env() needs a key.");
            const char* val = std::getenv(std::get<std::string>(args[0]).c_str());
            return val ? std::string(val) : TacoValue{nullptr};
        }
    }));

    // ── 网络（基础版，v7 会完善）──────────────────────────

    env->define("fetchUrl", std::make_shared<TacoNative>(TacoNative{
        "fetchUrl",
        [](std::vector<TacoValue> args) -> TacoValue {
            // 占位：v7 会用 cpp-httplib 实现
            throw std::runtime_error("🌮 fetchUrl() is available in v7.");
            return nullptr;
        }
    }));

    // ── random ──────────────────────────────────────────

    static std::mt19937 rng(std::random_device{}());

    env->define("random.pick", std::make_shared<TacoNative>(TacoNative{
        "random.pick",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty() || !std::holds_alternative<std::shared_ptr<TacoArray>>(args[0]))
                throw std::runtime_error("🌮 random.pick() needs an array.");
            auto& arr = std::get<std::shared_ptr<TacoArray>>(args[0]);
            if (arr->elements.empty()) return nullptr;
            std::uniform_int_distribution<int> dist(0, arr->elements.size() - 1);
            return arr->elements[dist(rng)];
        }
    }));

    env->define("random.flip", std::make_shared<TacoNative>(TacoNative{
        "random.flip",
        [](std::vector<TacoValue>) -> TacoValue {
            std::bernoulli_distribution dist(0.5);
            return dist(rng);
        }
    }));

    env->define("random.int", std::make_shared<TacoNative>(TacoNative{
        "random.int",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 random.int() needs min and max.");
            int lo = static_cast<int>(std::get<double>(args[0]));
            int hi = static_cast<int>(std::get<double>(args[1]));
            std::uniform_int_distribution<int> dist(lo, hi);
            return static_cast<double>(dist(rng));
        }
    }));
}
```

---

## 25.6 测试：运行完整的 Taco 脚本

创建 `test_v4.taco`：

```taco
// pipeline 风格处理数据
var scores = [88, 45, 92, 33, 76, 100, 55, 87];

print("通过的分数：");
scores
    .filter { s in s >= 60 }
    .sortBy { s in s }
    .each { s in print(s) };

print("平均分（通过）：");
var passed = scores.filter { s in s >= 60 };
print(passed.avg());

// 字符串处理
var log = "ERROR: something failed\nINFO: started\nERROR: another error\nINFO: done";
print("错误行：");
log.getLines()
   .filter { line in line.startsWith("ERROR") }
   .each { line in print(line) };

// map 操作
var person = {"name": "Miguel", "age": 12, "city": "Oaxaca"};
print(person.getKeys());
print(person.getValues());

// 文件操作
var files = ls(".");
print("当前目录文件数：" + string(files.len()));
files.filter { f in f.endsWith(".taco") }.each { f in print(f) };

// 系统命令
var git_status = exec("git status --short 2>/dev/null");
if (git_status.len() > 0) {
    print("Git 状态：");
    print(git_status);
}

// groupBy 示例
var words = ["apple", "banana", "avocado", "blueberry", "apricot"];
var grouped = words.groupBy { w in w.getChars()[0] };
print(grouped);
```

### 运行结果

```
通过的分数：
76
87
88
92
100
平均分（通过）：
88.6
错误行：
ERROR: something failed
ERROR: another error
当前目录文件数：5
test_v4.taco
test_v3.taco
...
```

---

## 25.7 这个版本的局限性

**`array` 是引用类型，但字面量每次创建新对象**

```taco
var a = [1, 2, 3];
var b = a;       // 共享
var c = [1, 2, 3];  // 新对象，和 a 不共享
```

这是正确的行为，但在 AST 节点里，数组字面量每次求值都创建新的 `TacoArray`，所以 `c` 和 `a` 不共享，符合预期。

**没有展开运算符 `...`**

```taco
var merged = [...a, ...b];  // 还不支持
```

展开运算符需要在语法分析器里特殊处理，v4 暂时不加。

**可选链 `?.` 和默认值 `??` 还没实现**

```taco
var city = user?.address?.city ?? "unknown";  // 还不支持
```

这些是语法糖，可以在解析器里加入，留给读者作为练习。

**没有 OOP（class、struct）**

class 和 struct 的实现需要在值系统里加入对象类型，以及方法查找机制。这在综合收尾章节里处理。

---

## 小结

v4 是 Taco 最接近"完整脚本语言"的一步。

**array 和 map** 用 STL 容器实现，用 `shared_ptr` 包装成引用类型。所有 pipeline 方法（`filter`、`map`、`each`、`reduce`）在方法分派函数里实现，内部用 STL 算法（`std::find_if`、`std::transform`、`std::sort` 等）。

**字符串方法** 用 STL 算法和标准库字符串操作实现，方法名遵循 v+n 的命名规范（`getLines`、`trimSpace`、`findStr` 等）。

**内置标准库** 包含文件系统操作（`cat`、`ls`、`mkdir`、`rm`、`mv`、`cp`、`pwd`、`exists`）、系统操作（`exec`、`env`）、随机数（`random.pick`、`random.flip`、`random.int`）。每个内置函数都是一个 `TacoNative`（用 `std::function` 包装的 lambda）。

**Lambda 在这里的核心作用**：每个 pipeline 方法里，调用 Taco 闭包的操作是通过 C++ lambda 包装后传给 STL 算法的。C++ lambda 捕获了 `eval` 引用，让 Taco 的 `{ x in x * 2 }` 可以在 C++ 的 `std::transform` 里被调用。

---

第五部分到这里结束。第六部分进入模板，学完之后 v5 会用模板重构解释器内部，让值类型和容器更通用、更高效。
