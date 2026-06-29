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
