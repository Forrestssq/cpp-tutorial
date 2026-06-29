# 第十七章：智能指针

---

上一章讲了裸指针的问题和 RAII 的思想。这一章讲 C++ 标准库提供的三种智能指针：`unique_ptr`、`shared_ptr`、`weak_ptr`。

智能指针是 RAII 思想的直接应用——它们是包装了裸指针的类，在析构时自动释放内存。使用智能指针，几乎可以完全避免手动 `new`/`delete`。

---

## 17.1 unique_ptr：独占所有权

`std::unique_ptr<T>` 表示对某块内存的**唯一所有权**：同一时刻只有一个 `unique_ptr` 拥有某块内存，它销毁时内存自动释放。

### 基本用法

```cpp
#include <memory>

// 创建 unique_ptr（推荐用 make_unique）
auto p = std::make_unique<Token>("NUMBER", "42");
// 等价于：std::unique_ptr<Token> p(new Token("NUMBER", "42"));
// 但 make_unique 更安全（异常情况下不会泄漏）

// 使用：和裸指针一样
std::cout << p->value << "\n";   // 用 -> 访问成员
std::cout << (*p).value << "\n"; // 用 * 解引用

// 获取裸指针（不转移所有权）
Token* raw = p.get();

// p 离开作用域时，Token 自动释放
```

### 所有权是唯一的：不能拷贝

`unique_ptr` 的"唯一"体现在：它不能被拷贝，只能被移动：

```cpp
auto p1 = std::make_unique<Token>("VAR", "var");

// 拷贝：被禁止
auto p2 = p1;  // 错误！unique_ptr 不能拷贝

// 移动：转移所有权
auto p2 = std::move(p1);
// 现在 p2 拥有 Token，p1 变成 nullptr
// 访问 p1 是未定义行为！

if (p1 == nullptr) {
    std::cout << "p1 is now empty\n";
}
std::cout << p2->value << "\n";  // var
```

为什么不能拷贝？因为如果允许拷贝，两个 `unique_ptr` 会指向同一块内存，都认为自己是所有者，析构时会 double free。

### 从函数返回 unique_ptr

这是最常见的用法之一——函数创建对象，通过 `unique_ptr` 返回，调用者获得所有权：

```cpp
std::unique_ptr<Expr> make_number(double value) {
    return std::make_unique<NumberLiteral>(value);
}

// 调用：
auto expr = make_number(42.0);  // expr 拥有 NumberLiteral
// expr 离开作用域时，NumberLiteral 自动释放
```

返回局部 `unique_ptr` 时，编译器会自动应用移动语义，不需要显式写 `std::move`。

### 把 unique_ptr 作为函数参数

有三种方式，语义不同：

```cpp
// 1. 传值：转移所有权给函数，调用后调用者不再拥有对象
void take_ownership(std::unique_ptr<Token> t) {
    // 函数拥有 t，函数返回时 t 自动释放
}

auto t = std::make_unique<Token>("VAR", "var");
take_ownership(std::move(t));  // 必须显式 move
// 之后 t 是 nullptr，不能再使用

// 2. 传引用：借用，不转移所有权
void borrow(std::unique_ptr<Token>& t) {
    t->value = "modified";  // 修改对象
    // 函数结束，t 的所有权不变
}

// 3. 传裸指针/引用：只是访问，不涉及所有权
void just_read(const Token* t) {
    std::cout << t->value << "\n";
}

auto t2 = std::make_unique<Token>("STRING", "hello");
just_read(t2.get());  // get() 返回裸指针，不转移所有权
```

**规则**：函数需要使用对象但不拥有它，传 `const T*` 或 `const T&`；函数需要修改对象但不拥有它，传 `T*` 或 `T&`；函数需要拥有对象，传 `unique_ptr<T>`（值传递）。

### unique_ptr 在 Taco AST 里的应用

v1/v2 里已经用了 `unique_ptr` 管理 AST 节点，这就是正确的做法：

```cpp
// ExprPtr 是 unique_ptr<Expr> 的别名
using ExprPtr = std::unique_ptr<Expr>;

struct BinaryExpr : Expr {
    ExprPtr left;   // BinaryExpr 拥有左子节点
    ExprPtr right;  // BinaryExpr 拥有右子节点
    std::string op;

    BinaryExpr(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l))  // 移动，转移所有权
        , op(std::move(o))
        , right(std::move(r))
    {}

    ~BinaryExpr() = default;
    // 不需要手动 delete left 和 right
    // BinaryExpr 析构时，left 和 right 的 unique_ptr 析构函数自动调用
    // 递归释放整棵子树
};
```

整棵 AST 树的所有权是这样的：

```
unique_ptr<Program>
└── vector<ExprPtr>
    ├── unique_ptr<VarDecl>
    │   └── unique_ptr<BinaryExpr>
    │       ├── unique_ptr<NumberLiteral>
    │       └── unique_ptr<NumberLiteral>
    └── unique_ptr<PrintStmt>
        └── unique_ptr<Identifier>
```

`Program` 被一个 `unique_ptr` 拥有。`Program` 里的每个语句被 `vector` 里的 `unique_ptr` 拥有。每个节点拥有它的子节点。当顶层的 `unique_ptr<Program>` 析构时，整棵树递归析构，所有节点自动释放。

---

## 17.2 shared_ptr：共享所有权

`std::shared_ptr<T>` 表示对某块内存的**共享所有权**：可以有多个 `shared_ptr` 指向同一块内存，内部用引用计数追踪有多少个 `shared_ptr` 拥有它，当最后一个 `shared_ptr` 析构时，内存才被释放。

### 基本用法

```cpp
// 创建 shared_ptr（推荐用 make_shared）
auto p1 = std::make_shared<Token>("NUMBER", "42");

// 拷贝：增加引用计数
auto p2 = p1;  // p1 和 p2 共同拥有 Token
               // 引用计数变为 2

{
    auto p3 = p1;  // 引用计数变为 3
}  // p3 析构，引用计数变为 2，Token 不释放

// 查看引用计数（调试用）
std::cout << p1.use_count() << "\n";  // 2

p1.reset();  // p1 放弃所有权，引用计数变为 1
// p2 析构，引用计数变为 0，Token 释放
```

### shared_ptr 的开销

`shared_ptr` 比 `unique_ptr` 有更多开销：

- **空间开销**：引用计数通常存在堆上（控制块），每个 `shared_ptr` 持有两个指针（指向对象的指针和指向控制块的指针）
- **时间开销**：每次拷贝或析构需要原子操作修改引用计数（线程安全的引用计数修改比普通加减慢）

`make_shared` 可以优化空间：它把对象和控制块分配在同一块内存里，只需要一次内存分配：

```cpp
// 两次内存分配（先 new Token，再分配控制块）
std::shared_ptr<Token> p(new Token("VAR", "var"));

// 一次内存分配（对象和控制块在一起）——推荐
auto p = std::make_shared<Token>("VAR", "var");
```

### 什么时候用 shared_ptr

当一块内存真的需要被多个对象共享，且无法确定谁最后一个用完时，用 `shared_ptr`。

在 Taco 解释器里，典型的场景是**环境（Environment）**——闭包需要捕获创建它时的外层作用域。外层函数可能已经返回，但闭包还在使用那个作用域。这时，外层函数和闭包共同拥有那个作用域，用 `shared_ptr` 管理：

```cpp
// 环境：变量名 → 值的映射
class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;

    Environment(Ptr parent = nullptr)
        : m_parent(std::move(parent))
    {}

    // ...变量的查找和定义

private:
    Ptr m_parent;  // 外层作用域
    std::unordered_map<std::string, TacoValue> m_vars;
};

// 创建函数时，捕获当前环境
class Function {
public:
    Environment::Ptr closure_env;  // shared_ptr，和创建时的作用域共享
    // ...
};
```

这样，即使创建函数的作用域已经退出，闭包仍然持有对那个环境的引用，环境不会被释放。v3 实现函数时会看到这个模式。

---

## 17.3 weak_ptr：打破循环引用

`std::weak_ptr<T>` 是 `shared_ptr` 的"弱引用"——它可以观察 `shared_ptr` 指向的对象，但不增加引用计数，也不拥有对象。

### 循环引用问题

`shared_ptr` 有一个著名的问题：循环引用导致内存泄漏。

```cpp
struct Node {
    std::shared_ptr<Node> next;
    int value;
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();

a->next = b;  // a 引用 b，b 的引用计数变为 2
b->next = a;  // b 引用 a，a 的引用计数变为 2

// 函数结束，a 和 b 局部变量析构
// a 的引用计数：2 → 1（b->next 还引用着 a）
// b 的引用计数：2 → 1（a->next 还引用着 b）
// 两者都不会到 0，内存泄漏！
```

两个节点互相引用，形成一个环，引用计数永远不会降到 0。

### weak_ptr 解决循环引用

```cpp
struct Node {
    std::weak_ptr<Node> next;  // 改用 weak_ptr，不增加引用计数
    int value;
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();

a->next = b;  // 不增加 b 的引用计数
b->next = a;  // 不增加 a 的引用计数

// 函数结束，a 和 b 析构
// a 的引用计数：1 → 0，a 释放
// b 的引用计数：1 → 0，b 释放
```

### weak_ptr 的使用

`weak_ptr` 不能直接访问对象，必须先转成 `shared_ptr`（`lock()` 方法），如果对象已经释放，`lock()` 返回空 `shared_ptr`：

```cpp
std::weak_ptr<Token> weak;

{
    auto strong = std::make_shared<Token>("VAR", "var");
    weak = strong;  // weak_ptr 指向 Token，但不增加引用计数

    // 通过 weak_ptr 访问对象
    if (auto p = weak.lock()) {  // lock() 返回 shared_ptr
        std::cout << p->value << "\n";  // var
    }
}  // strong 析构，Token 释放

// 现在 Token 已经释放
if (auto p = weak.lock()) {
    // 不会进入这里
} else {
    std::cout << "Token already released\n";
}
```

---

## 17.4 什么时候用哪个

一个简单的决策流程：

**首先考虑是否需要堆上分配**：很多时候，栈上的对象或标准库容器里的对象就够了，不需要任何智能指针。

**如果需要堆上分配，所有权是唯一的**：用 `unique_ptr`。这是最常见的情况。Taco AST 节点就是这样的——每个节点只有一个父节点，父节点拥有子节点。

**如果所有权真的需要共享**：用 `shared_ptr`。Taco 的闭包环境是这样的——函数和它捕获的外层作用域共同拥有那个环境。

**如果需要引用 `shared_ptr` 管理的对象，但不想拥有它**：用 `weak_ptr`。主要用于打破 `shared_ptr` 的循环引用。

**如果只是暂时访问某个对象，不涉及所有权**：用裸指针或引用。`unique_ptr::get()` 可以获取裸指针，但不要存储它——它可能随时失效。

```cpp
// Taco 里典型的几种情况：

// 1. AST 节点：unique_ptr，所有权清晰
using ExprPtr = std::unique_ptr<Expr>;
ExprPtr node = std::make_unique<NumberLiteral>(42.0);

// 2. 求值时访问节点：裸指针（不拥有）
TacoValue evaluate(const Expr* expr) {  // 不转移所有权
    return expr->evaluate(*this);
}

// 3. 环境（作用域）：shared_ptr，闭包和调用者共享
using EnvPtr = std::shared_ptr<Environment>;
EnvPtr env = std::make_shared<Environment>(parent_env);

// 4. 可能失效的引用：weak_ptr
std::weak_ptr<Environment> weak_env = env;
```

---

## *More About 智能指针*：引用计数的底层实现

> 第一次读可以跳过。

`shared_ptr` 的引用计数是怎么实现的？

### 控制块

`shared_ptr` 内部有一个**控制块**（control block），存放在堆上，包含：

- 引用计数（strong count）：有多少个 `shared_ptr` 指向这个对象
- 弱引用计数（weak count）：有多少个 `weak_ptr` 指向这个对象
- 指向对象的指针（或者对象本身，如果用了 `make_shared`）

```
shared_ptr<Token>:
  [ptr to Token] [ptr to control block]
                        │
                        ▼
                 [strong count: 2]
                 [weak count: 1]
                 [ptr to Token (or Token itself)]
```

每个 `shared_ptr` 持有两个指针：一个指向被管理的对象，一个指向控制块。

### 引用计数的原子操作

引用计数的修改必须是**原子操作**（atomic operation），保证多线程安全——多个线程可以同时持有指向同一对象的 `shared_ptr`，拷贝或析构时修改引用计数必须不冲突。

```cpp
// 伪代码：shared_ptr 拷贝构造
shared_ptr(const shared_ptr& other)
    : m_ptr(other.m_ptr)
    , m_control(other.m_control)
{
    if (m_control) {
        m_control->strong_count.fetch_add(1, std::memory_order_relaxed);
        // fetch_add 是原子操作，线程安全
    }
}

// 伪代码：shared_ptr 析构
~shared_ptr() {
    if (m_control) {
        if (m_control->strong_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // 引用计数降到 0，释放对象
            delete m_ptr;

            if (m_control->weak_count.load() == 0) {
                // 弱引用也没了，释放控制块
                delete m_control;
            }
        }
    }
}
```

原子操作比普通整数操作慢，这是 `shared_ptr` 比 `unique_ptr` 慢的根本原因。在性能敏感的场景，应该优先考虑 `unique_ptr`。

### make_shared 的优化

```cpp
// 两次内存分配
std::shared_ptr<Token> p(new Token("VAR", "var"));
// 1. new Token：分配 Token 的内存
// 2. shared_ptr 构造函数：分配控制块的内存

// 一次内存分配
auto p = std::make_shared<Token>("VAR", "var");
// make_shared 把 Token 和控制块分配在同一块连续内存里
```

`make_shared` 的一次分配不只是少了一次 `malloc` 调用，还有缓存友好性的好处——Token 和控制块在内存里相邻，访问控制块时 Token 很可能也在 CPU 缓存里。

缺点：如果有 `weak_ptr` 指向这个对象，即使 `shared_ptr` 引用计数降到 0（对象应该释放），也要等 `weak_ptr` 引用计数也降到 0，整块内存才能释放（因为 Token 和控制块在一起）。普通 `shared_ptr`（不用 `make_shared`）在强引用计数为 0 时就能释放 Token 的内存，控制块等弱引用也消失后再释放。

---

## 小结

**`unique_ptr`** 表示唯一所有权：不能拷贝，只能移动。销毁时自动释放内存。这是最常用的智能指针，几乎没有额外开销。Taco AST 节点用 `unique_ptr` 管理。

**`shared_ptr`** 表示共享所有权：引用计数管理生命周期，最后一个 `shared_ptr` 析构时释放内存。有引用计数的时间和空间开销。Taco 的闭包环境用 `shared_ptr` 管理。

**`weak_ptr`** 是观察者，不增加引用计数，必须通过 `lock()` 转成 `shared_ptr` 才能访问对象。主要用途是打破 `shared_ptr` 的循环引用。

**选择原则**：优先 `unique_ptr`，需要共享时用 `shared_ptr`，需要打破循环时用 `weak_ptr`。

---

下一章讲移动语义——`std::move` 是什么，右值引用是什么，以及为什么移动比拷贝更高效。
