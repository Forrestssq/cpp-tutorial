# 第十五章：项目 v2——求值器与控制流

---

第三部分学完了继承、多态、虚函数、类型转换。现在用这些知识做两件事：

1. **重构求值器**：把 v1 里用 `dynamic_cast` 判断节点类型的方式，换成用虚函数让节点自己求值
2. **加入控制流**：`if/elseif/else`、`while`、C 风格 `for`、`range` 风格 `for`、`switch`

v2 结束时，Taco 能运行这样的代码：

```taco
var score = 85;

if (score >= 90) {
    print("A");
} elseif (score >= 80) {
    print("B");
} else {
    print("C");
}

var sum = 0;
for (var i = 1; i <= 10; i++) {
    sum = sum + i;
}
print(sum);

for i in range(0, 5) {
    print(i);
}

var x = 10;
while (x > 0) {
    x = x - 1;
}
print(x);

switch (score) {
    case 100 { print("perfect"); }
    default  { print("not perfect"); }
}
```

---

## 15.1 用继承重新设计 AST 节点体系

v1 的 AST 节点是简单的数据结构，求值逻辑完全在 `Evaluator` 里。v2 改成让每个节点自己知道怎么求值——通过虚函数 `evaluate()`。

这个设计叫**访问者模式**（Visitor Pattern）的简化版。完整的访问者模式会在第二十九章讲到，现在先用更直接的方式。

### 为什么这样更好

v1 的求值器：

```cpp
// 每次调用都要做多次 dynamic_cast，直到找到匹配的类型
TacoValue Evaluator::evaluate(const Expr* expr) {
    if (auto* node = dynamic_cast<const NumberLiteral*>(expr))
        return eval_number(node);
    if (auto* node = dynamic_cast<const BinaryExpr*>(expr))
        return eval_binary(node);
    // ... 每增加一种节点，就要在这里加一行
}
```

v2 的求值器：

```cpp
// 直接调用虚函数，让节点自己处理
TacoValue Evaluator::evaluate(const Expr* expr) {
    return expr->evaluate(*this);
}
```

v2 的方式：
- **性能更好**：一次虚函数调用，不需要多次 `dynamic_cast`
- **扩展更方便**：加新节点类型只需要在新类里实现 `evaluate()`，不需要修改求值器
- **结构更清晰**：每种节点的求值逻辑和节点定义在一起

---

### 新的 AST 设计

前向声明 `Evaluator`，因为 `Expr::evaluate()` 需要接收它：

```cpp
// ast.h
#pragma once
#include <string>
#include <vector>
#include <memory>

// 前向声明
class Evaluator;

// TacoValue 的前向声明（实际定义在 value.h）
#include "value.h"

using ExprPtr = std::unique_ptr<struct Expr>;

// ────────────────────────────────
// 基类：纯虚，不能直接实例化
// ────────────────────────────────

struct Expr {
    virtual ~Expr() = default;

    // 核心：每个节点自己实现求值
    virtual TacoValue evaluate(Evaluator& eval) const = 0;

    // 调试用
    virtual std::string to_string() const = 0;
};
```

把 Taco 的值类型单独放到一个文件里：

```cpp
// value.h
#pragma once
#include <string>
#include <variant>

// Taco 的值：数字、字符串、布尔、nil
using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;

std::string value_to_string(const TacoValue& val);
bool is_truthy(const TacoValue& val);
bool values_equal(const TacoValue& a, const TacoValue& b);
```

```cpp
// value.cpp
#include "value.h"
#include <stdexcept>

std::string value_to_string(const TacoValue& val) {
    if (std::holds_alternative<double>(val)) {
        double d = std::get<double>(val);
        if (d == static_cast<long long>(d)) {
            return std::to_string(static_cast<long long>(d));
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

bool is_truthy(const TacoValue& val) {
    if (std::holds_alternative<bool>(val))
        return std::get<bool>(val);
    if (std::holds_alternative<std::nullptr_t>(val))
        return false;
    return true;
}

bool values_equal(const TacoValue& a, const TacoValue& b) {
    return a == b;
}
```

---

## 15.2 完整的 AST 节点定义

现在把所有节点的 `evaluate()` 都加上。`Evaluator` 类会在 evaluator.h 里定义，这里只需要前向声明。

```cpp
// ast.h（完整版）
#pragma once
#include "value.h"
#include <string>
#include <vector>
#include <memory>

class Evaluator;
using ExprPtr = std::unique_ptr<struct Expr>;

// ────────────────────────────────
// 基类
// ────────────────────────────────

struct Expr {
    virtual ~Expr() = default;
    virtual TacoValue evaluate(Evaluator& eval) const = 0;
    virtual std::string to_string() const = 0;
};

// ────────────────────────────────
// 字面量
// ────────────────────────────────

struct NumberLiteral : Expr {
    double value;
    explicit NumberLiteral(double v) : value(v) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override;
};

struct StringLiteral : Expr {
    std::string value;
    explicit StringLiteral(std::string v) : value(std::move(v)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "\"" + value + "\""; }
};

struct BoolLiteral : Expr {
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return value ? "true" : "false"; }
};

struct NilLiteral : Expr {
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "nil"; }
};

// ────────────────────────────────
// 变量引用
// ────────────────────────────────

struct Identifier : Expr {
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return name; }
};

// ────────────────────────────────
// 赋值
// ────────────────────────────────

struct AssignExpr : Expr {
    std::string name;
    ExprPtr     value;
    AssignExpr(std::string n, ExprPtr v)
        : name(std::move(n)), value(std::move(v)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return name + " = " + value->to_string();
    }
};

// ────────────────────────────────
// 运算
// ────────────────────────────────

struct BinaryExpr : Expr {
    ExprPtr     left;
    std::string op;
    ExprPtr     right;
    BinaryExpr(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
    }
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr     operand;
    UnaryExpr(std::string o, ExprPtr e)
        : op(std::move(o)), operand(std::move(e)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "(" + op + operand->to_string() + ")";
    }
};

// ────────────────────────────────
// 变量声明
// ────────────────────────────────

struct VarDecl : Expr {
    std::string name;
    ExprPtr     value;
    VarDecl(std::string n, ExprPtr v)
        : name(std::move(n)), value(std::move(v)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "var " + name + " = " + value->to_string();
    }
};

// ────────────────────────────────
// 控制流
// ────────────────────────────────

// if/elseif/else
struct IfStmt : Expr {
    // 每个分支是一个 (条件, 语句块) 对
    // branches[0] = if 分支
    // branches[1..n-1] = elseif 分支
    struct Branch {
        ExprPtr condition;
        std::vector<ExprPtr> body;
    };
    std::vector<Branch>      branches;
    std::vector<ExprPtr>     else_body;  // else 分支（可能为空）

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "if(...)"; }
};

// while 循环
struct WhileStmt : Expr {
    ExprPtr              condition;
    std::vector<ExprPtr> body;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "while(...)"; }
};

// C 风格 for 循环：for (init; condition; update) { body }
struct ForStmt : Expr {
    ExprPtr              init;       // var i = 0
    ExprPtr              condition;  // i < 10
    ExprPtr              update;     // i++（实现为 i = i + 1）
    std::vector<ExprPtr> body;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "for(...)"; }
};

// range 风格 for 循环：for i in range(0, 10)
struct ForRangeStmt : Expr {
    std::string          var_name;
    ExprPtr              start;
    ExprPtr              end;
    std::vector<ExprPtr> body;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "for i in range(...)"; }
};

// switch 语句
struct SwitchStmt : Expr {
    struct Case {
        ExprPtr              value;  // nullptr 表示 default
        std::vector<ExprPtr> body;
    };
    ExprPtr       subject;
    std::vector<Case> cases;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "switch(...)"; }
};

// ────────────────────────────────
// 内置函数调用（暂时只有 print）
// ────────────────────────────────

struct CallExpr : Expr {
    std::string              callee;
    std::vector<ExprPtr>     arguments;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return callee + "(...)"; }
};

// ────────────────────────────────
// return 语句（用异常实现控制流）
// ────────────────────────────────

struct ReturnStmt : Expr {
    ExprPtr value;  // 可以是 nullptr（return;）
    explicit ReturnStmt(ExprPtr v) : value(std::move(v)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "return ..."; }
};

// ────────────────────────────────
// 程序：所有顶层语句
// ────────────────────────────────

struct Program : Expr {
    std::vector<ExprPtr> statements;
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        std::string r;
        for (const auto& s : statements) r += s->to_string() + "\n";
        return r;
    }
};
```

---

## 15.3 实现求值器

求值器现在简洁很多——它只需要管理环境（变量），具体的求值逻辑在每个节点里。

```cpp
// evaluator.h
#pragma once
#include "value.h"
#include "ast.h"
#include <unordered_map>
#include <string>
#include <stdexcept>

// 用异常实现 return 语句的控制流跳转
struct ReturnException {
    TacoValue value;
};

class Evaluator {
public:
    Evaluator();

    // 主入口
    TacoValue evaluate(const Expr* expr);

    // 变量环境操作（供 AST 节点调用）
    TacoValue get_var(const std::string& name) const;
    void      set_var(const std::string& name, TacoValue val);
    void      define_var(const std::string& name, TacoValue val);

    // 作用域管理
    void push_scope();
    void pop_scope();

private:
    // 作用域链：栈顶是当前作用域
    std::vector<std::unordered_map<std::string, TacoValue>> m_scopes;
};
```

```cpp
// evaluator.cpp
#include "evaluator.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

Evaluator::Evaluator() {
    // 推入全局作用域
    m_scopes.push_back({});
}

TacoValue Evaluator::evaluate(const Expr* expr) {
    return expr->evaluate(*this);
}

TacoValue Evaluator::get_var(const std::string& name) const {
    // 从内向外查找变量
    for (int i = static_cast<int>(m_scopes.size()) - 1; i >= 0; i--) {
        auto it = m_scopes[i].find(name);
        if (it != m_scopes[i].end()) {
            return it->second;
        }
    }
    throw std::runtime_error("🌮 '" + name + "' is not defined.");
}

void Evaluator::set_var(const std::string& name, TacoValue val) {
    // 从内向外找到变量，修改它
    for (int i = static_cast<int>(m_scopes.size()) - 1; i >= 0; i--) {
        auto it = m_scopes[i].find(name);
        if (it != m_scopes[i].end()) {
            it->second = std::move(val);
            return;
        }
    }
    throw std::runtime_error("🌮 '" + name + "' is not defined.");
}

void Evaluator::define_var(const std::string& name, TacoValue val) {
    // 在当前作用域定义新变量
    m_scopes.back()[name] = std::move(val);
}

void Evaluator::push_scope() {
    m_scopes.push_back({});
}

void Evaluator::pop_scope() {
    if (m_scopes.size() > 1) {
        m_scopes.pop_back();
    }
}
```

---

## 15.4 各节点的 evaluate() 实现

把各个节点的求值实现单独放在 `ast.cpp` 里：

```cpp
// ast.cpp
#include "ast.h"
#include "evaluator.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

// ────────────────────────────────
// 字面量
// ────────────────────────────────

TacoValue NumberLiteral::evaluate(Evaluator& eval) const {
    return value;
}

std::string NumberLiteral::to_string() const {
    if (value == static_cast<long long>(value))
        return std::to_string(static_cast<long long>(value));
    return std::to_string(value);
}

TacoValue StringLiteral::evaluate(Evaluator& eval) const {
    return value;
}

TacoValue BoolLiteral::evaluate(Evaluator& eval) const {
    return value;
}

TacoValue NilLiteral::evaluate(Evaluator& eval) const {
    return nullptr;
}

// ────────────────────────────────
// 变量引用和赋值
// ────────────────────────────────

TacoValue Identifier::evaluate(Evaluator& eval) const {
    return eval.get_var(name);
}

TacoValue AssignExpr::evaluate(Evaluator& eval) const {
    TacoValue val = eval.evaluate(value.get());
    eval.set_var(name, val);
    return val;
}

// ────────────────────────────────
// 运算
// ────────────────────────────────

TacoValue BinaryExpr::evaluate(Evaluator& eval) const {
    // 短路求值：&& 和 || 先求左边
    if (op == "&&") {
        TacoValue l = eval.evaluate(left.get());
        if (!is_truthy(l)) return false;
        return is_truthy(eval.evaluate(right.get()));
    }
    if (op == "||") {
        TacoValue l = eval.evaluate(left.get());
        if (is_truthy(l)) return true;
        return is_truthy(eval.evaluate(right.get()));
    }

    TacoValue l = eval.evaluate(left.get());
    TacoValue r = eval.evaluate(right.get());

    // 字符串拼接
    if (op == "+" &&
        std::holds_alternative<std::string>(l) &&
        std::holds_alternative<std::string>(r)) {
        return std::get<std::string>(l) + std::get<std::string>(r);
    }

    // 算术运算
    if (op == "+" || op == "-" || op == "*" ||
        op == "/" || op == "%" || op == "^") {
        if (!std::holds_alternative<double>(l) ||
            !std::holds_alternative<double>(r)) {
            throw std::runtime_error(
                "🌮 Operator '" + op + "' requires numbers."
            );
        }
        double lv = std::get<double>(l);
        double rv = std::get<double>(r);

        if (op == "+") return lv + rv;
        if (op == "-") return lv - rv;
        if (op == "*") return lv * rv;
        if (op == "/") {
            if (rv == 0)
                throw std::runtime_error("🌮 Division by zero. Nobody can.");
            return lv / rv;
        }
        if (op == "%") return std::fmod(lv, rv);
        if (op == "^") return std::pow(lv, rv);
    }

    // 比较运算
    if (op == "==") return values_equal(l, r);
    if (op == "!=") return !values_equal(l, r);

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!std::holds_alternative<double>(l) ||
            !std::holds_alternative<double>(r)) {
            throw std::runtime_error(
                "🌮 Operator '" + op + "' requires numbers."
            );
        }
        double lv = std::get<double>(l);
        double rv = std::get<double>(r);
        if (op == "<")  return lv < rv;
        if (op == ">")  return lv > rv;
        if (op == "<=") return lv <= rv;
        if (op == ">=") return lv >= rv;
    }

    throw std::runtime_error("🌮 Unknown operator: '" + op + "'.");
}

TacoValue UnaryExpr::evaluate(Evaluator& eval) const {
    TacoValue val = eval.evaluate(operand.get());
    if (op == "!") return !is_truthy(val);
    if (op == "-") {
        if (!std::holds_alternative<double>(val))
            throw std::runtime_error("🌮 Unary '-' requires a number.");
        return -std::get<double>(val);
    }
    throw std::runtime_error("🌮 Unknown unary operator: '" + op + "'.");
}

// ────────────────────────────────
// 变量声明
// ────────────────────────────────

TacoValue VarDecl::evaluate(Evaluator& eval) const {
    TacoValue val = eval.evaluate(value.get());
    eval.define_var(name, val);
    return val;
}

// ────────────────────────────────
// 控制流
// ────────────────────────────────

TacoValue IfStmt::evaluate(Evaluator& eval) const {
    for (const auto& branch : branches) {
        TacoValue cond = eval.evaluate(branch.condition.get());
        if (is_truthy(cond)) {
            eval.push_scope();
            TacoValue result = nullptr;
            for (const auto& stmt : branch.body) {
                result = eval.evaluate(stmt.get());
            }
            eval.pop_scope();
            return result;
        }
    }
    // else 分支
    if (!else_body.empty()) {
        eval.push_scope();
        TacoValue result = nullptr;
        for (const auto& stmt : else_body) {
            result = eval.evaluate(stmt.get());
        }
        eval.pop_scope();
        return result;
    }
    return nullptr;
}

TacoValue WhileStmt::evaluate(Evaluator& eval) const {
    TacoValue result = nullptr;
    while (is_truthy(eval.evaluate(condition.get()))) {
        eval.push_scope();
        for (const auto& stmt : body) {
            result = eval.evaluate(stmt.get());
        }
        eval.pop_scope();
    }
    return result;
}

TacoValue ForStmt::evaluate(Evaluator& eval) const {
    eval.push_scope();  // for 循环的作用域

    // 初始化
    if (init) eval.evaluate(init.get());

    TacoValue result = nullptr;
    while (true) {
        // 检查条件
        if (condition) {
            TacoValue cond = eval.evaluate(condition.get());
            if (!is_truthy(cond)) break;
        }

        // 执行循环体
        eval.push_scope();
        for (const auto& stmt : body) {
            result = eval.evaluate(stmt.get());
        }
        eval.pop_scope();

        // 更新
        if (update) eval.evaluate(update.get());
    }

    eval.pop_scope();
    return result;
}

TacoValue ForRangeStmt::evaluate(Evaluator& eval) const {
    TacoValue start_val = eval.evaluate(start.get());
    TacoValue end_val   = eval.evaluate(end.get());

    if (!std::holds_alternative<double>(start_val) ||
        !std::holds_alternative<double>(end_val)) {
        throw std::runtime_error("🌮 range() requires numbers.");
    }

    double from = std::get<double>(start_val);
    double to   = std::get<double>(end_val);

    TacoValue result = nullptr;
    for (double i = from; i < to; i++) {
        eval.push_scope();
        eval.define_var(var_name, i);
        for (const auto& stmt : body) {
            result = eval.evaluate(stmt.get());
        }
        eval.pop_scope();
    }
    return result;
}

TacoValue SwitchStmt::evaluate(Evaluator& eval) const {
    TacoValue subject_val = eval.evaluate(subject.get());

    for (const auto& c : cases) {
        bool matches = false;

        if (c.value == nullptr) {
            // default 分支：总是匹配（如果没有其他分支匹配）
            matches = true;
        } else {
            TacoValue case_val = eval.evaluate(c.value.get());
            matches = values_equal(subject_val, case_val);
        }

        if (matches) {
            eval.push_scope();
            TacoValue result = nullptr;
            for (const auto& stmt : c.body) {
                result = eval.evaluate(stmt.get());
            }
            eval.pop_scope();
            return result;  // 每个 case 默认不穿透
        }
    }

    return nullptr;
}

// ────────────────────────────────
// 函数调用（暂时只有内置函数）
// ────────────────────────────────

TacoValue CallExpr::evaluate(Evaluator& eval) const {
    // 目前只支持内置函数，v3 会加用户定义函数
    if (callee == "print") {
        if (arguments.size() != 1) {
            throw std::runtime_error("🌮 print() takes exactly 1 argument.");
        }
        TacoValue val = eval.evaluate(arguments[0].get());
        std::cout << value_to_string(val) << "\n";
        return nullptr;
    }

    if (callee == "range") {
        // range 在 ForRangeStmt 里直接处理，这里不应该被调用
        throw std::runtime_error("🌮 range() can only be used in for loops.");
    }

    throw std::runtime_error("🌮 '" + callee + "' is not defined.");
}

// ────────────────────────────────
// return 语句
// ────────────────────────────────

TacoValue ReturnStmt::evaluate(Evaluator& eval) const {
    TacoValue val = value ? eval.evaluate(value.get()) : TacoValue{nullptr};
    throw ReturnException{val};  // 用异常实现 return 的控制流跳转
}

// ────────────────────────────────
// 程序
// ────────────────────────────────

TacoValue Program::evaluate(Evaluator& eval) const {
    TacoValue last = nullptr;
    for (const auto& stmt : statements) {
        last = eval.evaluate(stmt.get());
    }
    return last;
}
```

---

## 15.5 更新语法分析器

语法分析器需要支持新的语法结构：if、while、for、switch。

完整的 parser.cpp 太长，这里展示新增的控制流解析部分：

```cpp
// parser.cpp（新增部分）

ExprPtr Parser::parse_statement() {
    switch (current().type) {
        case TokenType::Var:    return parse_var_decl();
        case TokenType::If:     return parse_if_stmt();
        case TokenType::While:  return parse_while_stmt();
        case TokenType::For:    return parse_for_stmt();
        case TokenType::Switch: return parse_switch_stmt();
        case TokenType::Return: return parse_return_stmt();
        default: break;
    }

    // 表达式语句
    auto expr = parse_expression();
    expect(TokenType::Semicolon, "Expected ';' after expression.");
    return expr;
}

// if/elseif/else
ExprPtr Parser::parse_if_stmt() {
    auto node = std::make_unique<IfStmt>();

    // 解析 if 分支
    expect(TokenType::If, "Expected 'if'.");
    expect(TokenType::LeftParen, "Expected '(' after 'if'.");

    IfStmt::Branch if_branch;
    if_branch.condition = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after condition.");
    expect(TokenType::LeftBrace, "Expected '{' after condition.");
    if_branch.body = parse_block();
    node->branches.push_back(std::move(if_branch));

    // 解析 elseif 分支
    while (current().type == TokenType::Elseif) {
        advance();
        expect(TokenType::LeftParen, "Expected '(' after 'elseif'.");

        IfStmt::Branch elseif_branch;
        elseif_branch.condition = parse_expression();
        expect(TokenType::RightParen, "Expected ')' after condition.");
        expect(TokenType::LeftBrace, "Expected '{'.");
        elseif_branch.body = parse_block();
        node->branches.push_back(std::move(elseif_branch));
    }

    // 解析 else 分支
    if (current().type == TokenType::Else) {
        advance();
        expect(TokenType::LeftBrace, "Expected '{' after 'else'.");
        node->else_body = parse_block();
    }

    return node;
}

// while 循环
ExprPtr Parser::parse_while_stmt() {
    expect(TokenType::While, "Expected 'while'.");
    expect(TokenType::LeftParen, "Expected '(' after 'while'.");

    auto node = std::make_unique<WhileStmt>();
    node->condition = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after condition.");
    expect(TokenType::LeftBrace, "Expected '{'.");
    node->body = parse_block();

    return node;
}

// for 循环（C 风格或 range 风格）
ExprPtr Parser::parse_for_stmt() {
    expect(TokenType::For, "Expected 'for'.");

    // range 风格：for i in range(0, 10)
    if (current().type == TokenType::Identifier &&
        peek().type == TokenType::In) {
        return parse_for_range_stmt();
    }

    // C 风格：for (var i = 0; i < 10; i++)
    expect(TokenType::LeftParen, "Expected '(' after 'for'.");

    auto node = std::make_unique<ForStmt>();

    // 初始化
    if (current().type != TokenType::Semicolon) {
        node->init = parse_statement_no_brace();
    } else {
        advance();  // 跳过空的初始化部分的分号
    }

    // 条件
    if (current().type != TokenType::Semicolon) {
        node->condition = parse_expression();
    }
    expect(TokenType::Semicolon, "Expected ';' after for condition.");

    // 更新
    if (current().type != TokenType::RightParen) {
        node->update = parse_expression();
    }
    expect(TokenType::RightParen, "Expected ')' after for clauses.");
    expect(TokenType::LeftBrace, "Expected '{'.");
    node->body = parse_block();

    return node;
}

ExprPtr Parser::parse_for_range_stmt() {
    auto node = std::make_unique<ForRangeStmt>();

    // 变量名
    node->var_name = expect(TokenType::Identifier,
                            "Expected variable name.").value;
    expect(TokenType::In, "Expected 'in'.");

    // range(start, end)
    expect(TokenType::Identifier, "Expected 'range'.");  // 消费 "range"
    expect(TokenType::LeftParen, "Expected '(' after 'range'.");
    node->start = parse_expression();
    expect(TokenType::Comma, "Expected ',' in range.");
    node->end = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after range.");
    expect(TokenType::LeftBrace, "Expected '{'.");
    node->body = parse_block();

    return node;
}

// switch 语句
ExprPtr Parser::parse_switch_stmt() {
    expect(TokenType::Switch, "Expected 'switch'.");
    expect(TokenType::LeftParen, "Expected '(' after 'switch'.");

    auto node = std::make_unique<SwitchStmt>();
    node->subject = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after switch expression.");
    expect(TokenType::LeftBrace, "Expected '{'.");

    while (current().type != TokenType::RightBrace && !is_at_end()) {
        SwitchStmt::Case c;

        if (current().type == TokenType::Case) {
            advance();
            c.value = parse_expression();
        } else if (current().type == TokenType::Default) {
            advance();
            c.value = nullptr;  // nullptr 表示 default
        } else {
            error("Expected 'case' or 'default'.");
        }

        expect(TokenType::LeftBrace, "Expected '{' after case.");
        c.body = parse_block();
        node->cases.push_back(std::move(c));
    }

    expect(TokenType::RightBrace, "Expected '}' after switch.");
    return node;
}

// return 语句
ExprPtr Parser::parse_return_stmt() {
    expect(TokenType::Return, "Expected 'return'.");

    ExprPtr value = nullptr;
    if (current().type != TokenType::Semicolon) {
        value = parse_expression();
    }
    expect(TokenType::Semicolon, "Expected ';' after return.");

    return std::make_unique<ReturnStmt>(std::move(value));
}

// 解析 { ... } 块里的语句列表
std::vector<ExprPtr> Parser::parse_block() {
    std::vector<ExprPtr> stmts;
    while (current().type != TokenType::RightBrace && !is_at_end()) {
        stmts.push_back(parse_statement());
    }
    expect(TokenType::RightBrace, "Expected '}' after block.");
    return stmts;
}
```

---

## 15.6 字符串插值的实现

Taco 支持字符串插值：`"Hola, {name}!"`。在词法分析阶段，字符串被整体读取，插值表达式没有被识别。

处理插值需要在字符串求值时，扫描 `{...}` 并对其中的表达式求值：

```cpp
// 在 StringLiteral::evaluate 里处理插值
TacoValue StringLiteral::evaluate(Evaluator& eval) const {
    std::string result;
    int i = 0;

    while (i < static_cast<int>(value.size())) {
        if (value[i] == '{') {
            // 找到插值表达式的结尾
            int j = i + 1;
            while (j < static_cast<int>(value.size()) && value[j] != '}') {
                j++;
            }
            if (j >= static_cast<int>(value.size())) {
                result += '{';  // 没有配对的 }，当普通字符处理
                i++;
                continue;
            }

            // 提取 {} 里的表达式
            std::string expr_src = value.substr(i + 1, j - i - 1);

            // 对这个表达式单独做词法分析和语法分析
            try {
                Lexer lexer(expr_src);
                auto tokens = lexer.tokenize();
                Parser parser(std::move(tokens));
                // 解析单个表达式
                auto expr = parser.parse_expression_only();
                TacoValue val = eval.evaluate(expr.get());
                result += value_to_string(val);
            } catch (...) {
                // 解析失败，当普通文本处理
                result += value.substr(i, j - i + 1);
            }

            i = j + 1;
        } else {
            result += value[i++];
        }
    }

    return result;
}
```

---

## 15.7 测试：运行第一个真实的 Taco 程序

创建 `test_v2.taco`：

```taco
// 基础计算
var x = 100;
var y = x / 4 + 5;
print(y);  // 30

// 字符串插值
var name = "Miguel";
print("Hola, {name}!");  // Hola, Miguel!

// if/elseif/else
var score = 85;
if (score >= 90) {
    print("A");
} elseif (score >= 80) {
    print("B");
} else {
    print("C");
}

// while 循环
var n = 5;
var factorial = 1;
while (n > 0) {
    factorial = factorial * n;
    n = n - 1;
}
print(factorial);  // 120

// C 风格 for 循环（求和）
var sum = 0;
for (var i = 1; i <= 10; i = i + 1) {
    sum = sum + i;
}
print(sum);  // 55

// range 风格 for 循环
for i in range(0, 5) {
    print(i);
}

// switch 语句
var day = 3;
switch (day) {
    case 1 { print("Monday"); }
    case 2 { print("Tuesday"); }
    case 3 { print("Wednesday"); }
    default { print("Other"); }
}
```

### 运行结果

```
30
Hola, Miguel!
B
120
55
0
1
2
3
4
Wednesday
```

---

## 15.8 这个版本的局限性

**没有用户定义函数**

`func greet(name) { ... }` 还不支持。函数需要闭包和作用域的完整实现，v3 会加。

**`i++` 还不支持**

for 循环的更新部分现在要写成 `i = i + 1`，因为后缀自增运算符还没实现。这是一个小的语法糖，可以在解析器里加一个特殊处理，这里先留着。

**没有 break 和 continue**

循环里没有 `break` 和 `continue`。实现方式和 `return` 类似——用特殊的异常来中断控制流。

**字符串插值是"嵌套解析"**

目前的字符串插值实现方式是：对 `{}` 里的内容再做一次完整的词法分析和语法分析。这在工程上不是最干净的做法，完整的实现应该在词法分析阶段就把插值字符串拆开。这留给以后的版本。

---

## 小结

v2 完成了两件大事：

**重构求值器**：从 `dynamic_cast` 链改成虚函数。每个 AST 节点自己实现 `evaluate()`，求值器只需要调用 `expr->evaluate(*this)`。代码更简洁，性能更好，扩展更容易。

**加入控制流**：`if/elseif/else`、`while`、C 风格 `for`、range 风格 `for`、`switch`。每种控制结构都有对应的 AST 节点，求值逻辑在节点的 `evaluate()` 里。

**作用域**用作用域链实现：一个 `vector` 存储多层 `map`，查找变量从内向外遍历。进入新的代码块时 `push_scope()`，退出时 `pop_scope()`。

**`return` 语句**用异常实现控制流跳转——这是一种常见的解释器实现技巧，让 `return` 可以从任意深度的嵌套中跳出。

---

第三部分到这里结束。第四部分进入现代 C++ 核心：所有权、智能指针、移动语义。学完之后 v3 会用智能指针管理 AST 节点，并加入用户定义函数和闭包。
