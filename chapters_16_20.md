# 第十六章：所有权与生命周期

---

第四部分进入现代 C++ 最核心的领域：内存管理。这是 C++ 和几乎所有其他语言差距最大的地方，也是 C++ 程序员必须真正理解的东西。

Python 用垃圾回收，内存管理对程序员透明。C 把内存管理完全交给程序员，`malloc`/`free` 要配对。C++ 走了第三条路：**用语言机制（RAII + 智能指针）让内存管理既安全又高效，既不需要垃圾回收，也不需要手动 `free`**。

这一章先把基础讲清楚：栈和堆的区别，裸指针的问题，所有权的概念。

---

## 16.1 C++ 的内存模型：栈与堆

程序运行时使用的内存分为两个主要区域：**栈**（stack）和**堆**（heap）。理解这两者的区别，是理解 C++ 内存管理的基础。

### 栈

栈内存由编译器自动管理。函数调用时，局部变量被压入栈；函数返回时，这些变量被自动弹出。

```cpp
void foo() {
    int x = 10;      // x 在栈上
    double y = 3.14; // y 在栈上
    // ...
}  // foo 返回，x 和 y 自动销毁
```

栈的特点：
- **自动管理**：变量离开作用域自动销毁，不需要手动释放
- **速度极快**：分配只是移动栈指针，几乎没有开销
- **大小有限**：通常是几 MB，存太多数据会栈溢出（stack overflow）
- **生命周期固定**：绑定到函数调用，不能超出函数范围

### 堆

堆内存需要手动管理（或通过智能指针间接管理）。用 `new` 在堆上分配，用 `delete` 释放：

```cpp
int* p = new int(42);  // 在堆上分配一个 int，值为 42
// p 本身在栈上，指向堆上的内存

*p = 100;   // 通过指针修改堆上的值
delete p;   // 手动释放堆上的内存
p = nullptr; // 好习惯：释放后置空
```

堆的特点：
- **手动管理**（或通过智能指针）：需要显式释放
- **大小灵活**：可以用几个 GB
- **生命周期灵活**：可以跨函数存在
- **速度相对慢**：分配需要向操作系统申请内存，有一定开销

### 一个直观的对比

```cpp
void stack_example() {
    int arr[1000];  // 在栈上，4000 字节，自动管理
}  // 函数返回，arr 消失

void heap_example() {
    int* arr = new int[1000];  // 在堆上，4000 字节
    // 可以把 arr 传出去，继续使用
    delete[] arr;  // 必须手动释放
}
```

在 Taco 解释器里，AST 节点需要跨越多个函数调用存在（从解析一直到求值），所以必须放在堆上。`std::unique_ptr` 管理它们的生命周期，不需要手动 `delete`。

---

### 对象的存储位置

C++ 里对象的存储位置取决于怎么创建它：

```cpp
class Token { ... };

// 在栈上创建
Token t1("NUMBER", "42");  // t1 在栈上，函数返回自动销毁

// 在堆上创建（裸指针，不推荐）
Token* t2 = new Token("STRING", "hello");  // 在堆上，需要手动 delete
delete t2;

// 在堆上创建（unique_ptr，推荐）
auto t3 = std::make_unique<Token>("IDENTIFIER", "x");  // 在堆上，自动管理

// 在容器里（容器管理内存）
std::vector<Token> tokens;
tokens.push_back(Token("PLUS", "+"));  // Token 在 vector 内部的堆内存里
```

---

## 16.2 裸指针的问题

裸指针（raw pointer）就是 C 里的普通指针：`int*`、`Token*`、`Expr*`。裸指针本身没有问题，问题在于**手动管理生命周期非常容易出错**。

### 内存泄漏

```cpp
void process(const std::string& path) {
    Token* t = new Token("STRING", path);

    if (path.empty()) {
        return;  // 忘了 delete t！内存泄漏
    }

    // 使用 t...
    delete t;
}
```

函数提前返回时，`delete t` 被跳过，分配的内存永远不会被释放。程序运行时间越长，泄漏越多，最终可能导致内存耗尽崩溃。

### 悬空指针

```cpp
Token* t = new Token("NUMBER", "42");
delete t;    // 释放内存
t->value;    // 未定义行为！t 是悬空指针
             // 可能崩溃，可能输出乱码，可能"正常运行"（最危险的情况）
```

### Double Free

```cpp
Token* t = new Token("NUMBER", "42");
delete t;
delete t;  // 未定义行为！同一块内存释放两次
           // 通常导致程序崩溃
```

### 所有权不明确

```cpp
Token* create_token() {
    return new Token("VAR", "var");
}

void use_token() {
    Token* t = create_token();
    // 问题来了：谁负责 delete t？
    // 调用者？create_token 函数内部？
    // 文档里没写，代码里也看不出来
}
```

裸指针传来传去，所有权不清晰，很难判断什么时候、谁来释放内存。

---

## 16.3 所有权是什么

**所有权**（ownership）是 C++ 的核心概念：**某个对象"拥有"某块内存，负责在适当的时候释放它**。

所有权有几个关键性质：

**唯一性**：一块内存最好只有一个所有者，否则很容易 double free。

**可转移**：所有权可以从一个对象转移到另一个对象（移动语义，第十八章讲）。

**可借用**：可以在不转移所有权的情况下，临时使用某块内存（引用/指针）。

所有权模型用一句话总结：**谁创建，谁负责销毁；或者把所有权交给另一个对象，让它负责。**

---

### Rust 的所有权 vs C++ 的所有权

Rust 把所有权做成了语言的核心特性，用编译器强制检查所有权规则，保证内存安全。这是 Rust 最大的创新。

C++ 没有语言层面的所有权检查，但通过以下工具在实践中实现所有权管理：

- `std::unique_ptr`：独占所有权
- `std::shared_ptr`：共享所有权
- RAII：把所有权绑定到对象生命周期
- 移动语义：高效转移所有权

这些工具不强制使用（裸指针也可以），所以 C++ 需要程序员有更强的纪律性——但换来的是更大的灵活性和更低的运行时开销（`unique_ptr` 没有引用计数，`shared_ptr` 才有）。

---

## 16.4 RAII 深讲

RAII（Resource Acquisition Is Initialization）在第八章提过，这里深入讲一遍。

RAII 的精髓是：**把资源的获取和释放，绑定到对象的构造和析构**。

```
对象构造 → 资源获取（打开文件、分配内存、加锁）
对象析构 → 资源释放（关闭文件、释放内存、解锁）
```

这样，只要对象的生命周期是确定的（栈上对象离开作用域自动析构），资源的生命周期也是确定的。

### RAII 的经典例子

**文件**：

```cpp
class File {
public:
    explicit File(const std::string& path, const std::string& mode = "r")
        : m_file(std::fopen(path.c_str(), mode.c_str()))
    {
        if (!m_file) {
            throw std::runtime_error("Cannot open: " + path);
        }
    }

    ~File() {
        if (m_file) std::fclose(m_file);
    }

    // 禁止拷贝（文件句柄不能复制）
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    // 允许移动
    File(File&& other) noexcept : m_file(other.m_file) {
        other.m_file = nullptr;
    }

    FILE* get() const { return m_file; }

private:
    FILE* m_file;
};

void read_taco_file(const std::string& path) {
    File f(path);          // 打开文件
    // 使用 f.get()...
}  // f 析构，文件自动关闭——不管函数从哪里返回
```

**互斥锁**：

```cpp
std::mutex m;

void thread_safe_increment(int& count) {
    std::lock_guard<std::mutex> lock(m);  // 构造时加锁
    count++;
    // lock 析构时自动解锁，不管函数怎么返回
}
```

**内存**（这就是 `unique_ptr` 做的事）：

```cpp
// RAII 包装裸指针的概念演示（实际用 unique_ptr）
template<typename T>
class UniqueOwner {
public:
    explicit UniqueOwner(T* ptr) : m_ptr(ptr) {}

    ~UniqueOwner() {
        delete m_ptr;  // 析构时释放内存
    }

    T* get() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    T* operator->() const { return m_ptr; }

    // 禁止拷贝（所有权是唯一的）
    UniqueOwner(const UniqueOwner&) = delete;
    UniqueOwner& operator=(const UniqueOwner&) = delete;

private:
    T* m_ptr;
};

void example() {
    UniqueOwner<Token> t(new Token("VAR", "var"));
    // 使用 t->value，*t 等
}  // t 析构，Token 自动释放
```

这个 `UniqueOwner` 就是 `std::unique_ptr` 的简化版。下一章讲 `unique_ptr` 的完整用法。

---

### RAII 的层次

RAII 可以层层嵌套——RAII 对象管理 RAII 对象，最终形成一个资源管理的层次结构：

```cpp
class Evaluator {
    // m_scopes 是 vector of unordered_map
    // vector 管理 unordered_map 的内存
    // unordered_map 管理 string 和 TacoValue 的内存
    // TacoValue（variant）管理内部值的内存
    // 所有这些都是 RAII，Evaluator 析构时全部自动释放
    std::vector<std::unordered_map<std::string, TacoValue>> m_scopes;
};
```

```cpp
class Parser {
    // tokens 是 vector<Token>
    // 每个 Token 里的 string 是 RAII
    std::vector<Token> m_tokens;

    // AST 节点用 unique_ptr 管理
    // parse() 返回的 unique_ptr<Program> 拥有整棵树
    // 树里每个节点用 unique_ptr 管理子节点
};
```

在 Taco 解释器里，几乎所有内存都由标准库容器或 `unique_ptr` 管理，不需要手动写任何析构函数。

---

### 异常安全

RAII 的另一个重要好处是**异常安全**（exception safety）。

没有 RAII 时，异常可能导致资源泄漏：

```cpp
void unsafe_function() {
    FILE* f = std::fopen("data.taco", "r");
    Token* t = new Token("VAR", "var");

    if (some_condition()) {
        throw std::runtime_error("Error!");  // 异常！
        // f 和 t 都泄漏了，下面的 delete 和 fclose 不会执行
    }

    delete t;
    std::fclose(f);
}
```

用 RAII 时，异常触发析构函数，资源自动释放：

```cpp
void safe_function() {
    File f("data.taco");                    // RAII
    auto t = std::make_unique<Token>("VAR", "var");  // RAII

    if (some_condition()) {
        throw std::runtime_error("Error!");
        // 异常！但 f 和 t 的析构函数仍然会被调用
        // 文件自动关闭，Token 自动释放
    }
}
```

C++ 保证：当异常被抛出时，已经构造完成的局部对象的析构函数会被调用。这叫**栈展开**（stack unwinding）。RAII 利用这个保证来实现异常安全。

---

## 小结

**栈**自动管理，速度快，大小有限，生命周期绑定函数调用。**堆**需要手动管理（或通过智能指针），大小灵活，生命周期自由。

**裸指针**的三大问题：内存泄漏、悬空指针、double free——都源于手动管理生命周期的不可靠性。

**所有权**是解决这些问题的核心概念：明确谁拥有某块内存，谁负责释放。C++ 用 `unique_ptr`（唯一所有权）和 `shared_ptr`（共享所有权）来表达和管理所有权。

**RAII** 把资源管理绑定到对象生命周期：构造时获取，析构时释放。RAII 让异常安全变得自然，因为析构函数在任何情况下（包括异常）都会被调用。

---

下一章讲 `unique_ptr` 和 `shared_ptr`——这是现代 C++ 内存管理的核心工具，也是 v3 重构 AST 所有权的基础。
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
# 第十八章：移动语义

---

移动语义（move semantics）是 C++11 最重要的特性之一。它解决了一个长期存在的问题：当对象被"转移"时（从函数返回、放入容器、传给另一个对象），C++ 之前总是做拷贝，即使那个对象马上就要被销毁。移动语义让"转移"变得高效——不是拷贝数据，而是直接把资源的所有权转过去。

---

## 18.1 左值与右值

理解移动语义，首先要理解左值（lvalue）和右值（rvalue）。

**左值**（lvalue）：有名字、有持久地址的值，可以出现在赋值号左边：

```cpp
int x = 10;   // x 是左值
x = 20;       // 可以赋值给 x

std::string name = "Miguel";  // name 是左值
name = "Dante";               // 可以赋值
```

**右值**（rvalue）：临时的、没有名字的值，不能出现在赋值号左边：

```cpp
10 = x;       // 错误！10 是右值，不能赋值给它
"Miguel" = name;  // 错误！字符串字面量是右值

int x = 10 + 20;  // 10 + 20 是右值（计算结果是临时的）
std::string s = std::string("hello") + " world";
// std::string("hello") 是右值（临时对象）
// std::string("hello") + " world" 也是右值
```

一个简单的判断方法：能不能取地址——能取地址的是左值（`&x` 合法），不能的是右值（`&10` 非法）。

---

### 右值引用

C++11 引入了**右值引用**（rvalue reference），用 `&&` 表示：

```cpp
int x = 10;
int& lref = x;   // 左值引用：只能绑定到左值
int&& rref = 10; // 右值引用：只能绑定到右值
int&& rref2 = x + 5;  // x + 5 是右值，可以绑定

// int&& rref3 = x;  // 错误！x 是左值，不能绑定到右值引用
```

右值引用的存在，让编译器能区分"临时对象"和"有名字的对象"，从而在处理临时对象时选择"移动"而不是"拷贝"。

---

## 18.2 移动构造函数与移动赋值运算符

来看一个具体的例子：一个管理动态数组的类（简化版的 `vector`）：

```cpp
class DynArray {
public:
    DynArray(int size)
        : m_data(new int[size])
        , m_size(size)
    {}

    ~DynArray() {
        delete[] m_data;
    }

    // 拷贝构造：深拷贝，分配新内存
    DynArray(const DynArray& other)
        : m_data(new int[other.m_size])
        , m_size(other.m_size)
    {
        std::copy(other.m_data, other.m_data + m_size, m_data);
        std::cout << "Copy constructed (" << m_size << " ints)\n";
    }

    // 移动构造：偷走资源，不分配新内存
    DynArray(DynArray&& other) noexcept
        : m_data(other.m_data)  // 直接拿走指针
        , m_size(other.m_size)
    {
        other.m_data = nullptr;  // 让 other 放弃对内存的所有权
        other.m_size = 0;
        std::cout << "Move constructed (" << m_size << " ints)\n";
    }

    // 移动赋值运算符
    DynArray& operator=(DynArray&& other) noexcept {
        if (this != &other) {
            delete[] m_data;        // 释放自己的旧内存
            m_data = other.m_data;  // 拿走 other 的内存
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

private:
    int* m_data;
    int  m_size;
};
```

移动构造函数做的事情：
1. 把 `other` 的指针直接"偷"过来，不分配新内存
2. 把 `other` 的指针置为 `nullptr`，防止 `other` 析构时 double free

拷贝构造：分配新内存，复制所有数据，O(n) 时间。
移动构造：只复制指针，O(1) 时间。对于大数组，差距可以非常显著。

---

### noexcept 关键字

移动构造函数通常标记为 `noexcept`——声明这个函数不会抛出异常：

```cpp
DynArray(DynArray&& other) noexcept { ... }
```

这不只是一个文档说明，`noexcept` 让标准库容器（如 `std::vector`）在扩容时能安全地使用移动语义。如果移动构造函数没有 `noexcept`，`vector` 扩容时会用拷贝而不是移动，以保证异常安全。

移动操作通常不应该失败（只是在内存里移动指针），所以标记 `noexcept` 是正确的。

---

## 18.3 std::move 的本质

`std::move` 并不真的"移动"任何东西——它只是把一个左值强制转换成右值引用，告诉编译器"我允许对这个对象使用移动语义"：

```cpp
std::string a = "hello";
std::string b = std::move(a);  // 把 a 转成右值引用，触发移动构造
// 现在 b 是 "hello"，a 是空字符串（或某种有效的空状态）
```

`std::move` 的实现非常简单：

```cpp
// std::move 的简化版实现
template<typename T>
typename std::remove_reference<T>::type&& move(T&& t) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(t);
}
```

本质就是一个 `static_cast` 到右值引用类型。

**重要**：`std::move` 之后，原来的对象处于"有效但未指定的状态"——可以安全析构，但不能假设它有任何特定的值。

```cpp
std::string a = "hello";
std::string b = std::move(a);

// a 可以安全析构（不会 double free）
// 但不要再使用 a 的值！
std::cout << a;  // 可能输出空字符串，也可能是其他值
a = "world";     // 重新赋值后可以正常使用
```

---

### 什么时候用 std::move

**在函数参数传递时**：

```cpp
// 如果确定不再需要 source，用 move 避免拷贝
void process(std::string source) { ... }

std::string s = "hello";
process(std::move(s));  // 移动，不拷贝
// s 现在是空的，不要再使用
```

**在构造函数的初始化列表里**：

```cpp
class Lexer {
public:
    Lexer(std::string source)
        : m_source(std::move(source))  // 移动，不拷贝
    {}
private:
    std::string m_source;
};
```

**从函数返回时（通常不需要）**：

```cpp
std::string make_greeting(const std::string& name) {
    std::string result = "Hello, " + name + "!";
    return result;  // 不需要 std::move，NRVO 会自动优化
}
```

函数返回局部变量时，编译器会自动应用**命名返回值优化**（NRVO，Named Return Value Optimization），直接在调用者的内存里构造返回值，不需要拷贝或移动。显式写 `return std::move(result)` 反而可能阻止 NRVO。

---

## 18.4 Rule of Five

第九章讲了 Rule of Three：如果需要自定义析构函数、拷贝构造函数、拷贝赋值运算符中的任何一个，通常需要自定义全部三个。

C++11 引入移动语义后，这个规则扩展成了 **Rule of Five**：

> 如果需要自定义以下五个中的任何一个，通常需要自定义全部五个：
> 1. 析构函数
> 2. 拷贝构造函数
> 3. 拷贝赋值运算符
> 4. 移动构造函数
> 5. 移动赋值运算符

```cpp
class Resource {
public:
    Resource(int size) : m_data(new int[size]), m_size(size) {}

    // 1. 析构函数
    ~Resource() { delete[] m_data; }

    // 2. 拷贝构造函数
    Resource(const Resource& other)
        : m_data(new int[other.m_size])
        , m_size(other.m_size)
    {
        std::copy(other.m_data, other.m_data + m_size, m_data);
    }

    // 3. 拷贝赋值运算符
    Resource& operator=(const Resource& other) {
        if (this != &other) {
            delete[] m_data;
            m_size = other.m_size;
            m_data = new int[m_size];
            std::copy(other.m_data, other.m_data + m_size, m_data);
        }
        return *this;
    }

    // 4. 移动构造函数
    Resource(Resource&& other) noexcept
        : m_data(other.m_data)
        , m_size(other.m_size)
    {
        other.m_data = nullptr;
        other.m_size = 0;
    }

    // 5. 移动赋值运算符
    Resource& operator=(Resource&& other) noexcept {
        if (this != &other) {
            delete[] m_data;
            m_data = other.m_data;
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

private:
    int* m_data;
    int  m_size;
};
```

**现代 C++ 的建议**：如果能用标准库类型（`std::vector`、`std::string`、`std::unique_ptr`）管理资源，就不需要手动实现这五个函数。标准库类型已经正确实现了所有五个。

在 Taco 项目里，`Lexer`、`Parser`、`Evaluator` 的成员都是标准库类型或智能指针，不需要手动实现任何一个。

---

### 移动语义在 Taco 里的应用

移动语义在 Taco 里随处可见，但大多数时候是隐式发生的：

```cpp
// Parser 构造：移动 tokens 进去，避免拷贝整个 vector
Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))  // 移动
    , m_pos(0)
{}

// parse() 返回时：移动 unique_ptr，不拷贝
std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    // ...
    return program;  // NRVO 或隐式移动
}

// 调用者：
auto program = parser.parse();  // program 拥有 Program，没有拷贝

// Evaluator 里移动 TacoValue
TacoValue Evaluator::evaluate(const Expr* expr) {
    return expr->evaluate(*this);  // 返回时移动，不拷贝
}
```

`std::string` 的移动操作只是交换内部指针，O(1)。`std::vector` 的移动操作只是交换内部指针和大小，O(1)。`std::unique_ptr` 的移动操作只是转移指针，O(1)。

这就是现代 C++ 的性能优势：大量的"拷贝"在底层都变成了移动，代码简洁的同时性能不受影响。

---

## *More About 移动语义*：完美转发与 std::forward

> 第一次读可以跳过。

### 转发问题

考虑这样一个包装函数：

```cpp
template<typename T>
void wrapper(T arg) {
    // 想把 arg 原封不动地传给 process
    process(arg);
}
```

问题来了：如果调用者传的是右值：

```cpp
wrapper(std::string("hello"));
```

`wrapper` 的参数 `arg` 是一个有名字的局部变量（左值），把它传给 `process` 会触发拷贝，而不是移动——即使原来的调用者传的是右值。

### 完美转发

**完美转发**（perfect forwarding）解决这个问题：保持参数的值类别（左值/右值）原封不动地传递。

```cpp
template<typename T>
void wrapper(T&& arg) {  // 这里的 && 是"转发引用"，不是右值引用
    process(std::forward<T>(arg));  // 完美转发
}
```

`std::forward<T>(arg)` 的作用：
- 如果 `T` 是左值引用类型（调用者传了左值），返回左值引用
- 如果 `T` 是普通类型（调用者传了右值），返回右值引用

这样 `process` 收到的参数和调用者传给 `wrapper` 的参数有相同的值类别。

### 在 Taco 里的应用

完美转发主要用于模板代码和工厂函数。`std::make_unique` 内部就用了完美转发：

```cpp
// make_unique 的简化实现
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 调用时，参数原封不动地传给 T 的构造函数
auto p = std::make_unique<NumberLiteral>(42.0);
// 42.0 是右值，通过 forward 传给 NumberLiteral 的构造函数时仍然是右值
```

完美转发和可变参数模板（`Args&&...`）是模板编程里的标准组合，第二十八章讲模板时会再深入。

---

## 小结

**左值**是有名字、有地址的值；**右值**是临时的、没有名字的值。

**右值引用**（`T&&`）让函数可以区分临时对象和普通对象，从而对临时对象使用移动而不是拷贝。

**移动构造函数和移动赋值运算符**：从 `other` "偷"资源（交换指针），而不是深拷贝。移动是 O(1) 的，拷贝是 O(n) 的。移动后 `other` 处于有效但未指定的状态。

**`std::move`**：把左值强制转成右值引用，触发移动语义。`std::move` 本身不移动任何东西，只是一个 `static_cast`。

**Rule of Five**：如果需要自定义析构函数，通常也需要自定义拷贝构造、拷贝赋值、移动构造、移动赋值。用标准库类型管理资源可以避免手动实现这五个。

---

下一章讲常用标准库补充：`<chrono>`、`<optional>`、`<variant>`、`<random>`，这些都是 v3 会用到的工具。
# 第十九章：常用标准库补充

---

这一章介绍几个 C++17 标准库里的实用工具：`<chrono>`、`<optional>`、`<variant>`、`<random>`。这些不是新的语法特性，而是标准库提供的工具类，在 Taco 项目里都会用到。

---

## 19.1 chrono：时间与性能测量

`<chrono>` 提供了类型安全的时间处理工具。在 Taco 里，`🌮🌮🌮🌮` 彩蛋需要输出 Unix 时间戳，就用 `chrono` 实现。

### 时间点与时间段

`chrono` 有三个核心概念：
- **时钟**（clock）：时间的来源，比如系统时钟、高精度时钟
- **时间点**（time_point）：某个时钟上的一个具体时刻
- **时间段**（duration）：两个时间点之间的间隔

```cpp
#include <chrono>

// 获取当前时间点
auto now = std::chrono::system_clock::now();

// 转换成 Unix 时间戳（秒数）
auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
    now.time_since_epoch()
).count();

std::cout << seconds << "\n";  // 比如：1719556800
```

这就是 `🌮🌮🌮🌮` 彩蛋的实现：

```cpp
// 四个 🌮：输出 Unix 时间戳
case 4: {
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    std::cout << ts << "\n";
    break;
}
```

### 性能测量

`chrono` 也是测量代码执行时间的标准工具：

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

// 要测量的代码
for (int i = 0; i < 1000000; i++) {
    // ...
}

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    end - start
).count();

std::cout << "Elapsed: " << duration << "ms\n";
```

`high_resolution_clock` 比 `system_clock` 精度更高，适合性能测量。`system_clock` 适合获取"真实时间"（日历时间）。

### 常用的时间单位

```cpp
using namespace std::chrono_literals;  // 让时间字面量可用

auto one_second   = 1s;      // std::chrono::seconds(1)
auto half_second  = 500ms;   // std::chrono::milliseconds(500)
auto one_minute   = 1min;    // std::chrono::minutes(1)
auto micro        = 100us;   // std::chrono::microseconds(100)

// 在线程里休眠
#include <thread>
std::this_thread::sleep_for(100ms);  // 休眠 100 毫秒
```

---

## 19.2 optional：可能有值，也可能没有值

`std::optional<T>` 表示一个可能存在、也可能不存在的值。这比用 `-1`、`nullptr`、或者特殊值来表示"没有"更安全、更清晰。

### 基本用法

```cpp
#include <optional>

// 有值的 optional
std::optional<int> a = 42;
std::optional<std::string> b = "hello";

// 没有值的 optional
std::optional<int> c;          // 默认构造，没有值
std::optional<int> d = std::nullopt;  // 显式表示没有值

// 检查是否有值
if (a.has_value()) { ... }  // 或者直接 if (a)
if (!c) { ... }             // c 没有值

// 获取值
int val = *a;            // 解引用，如果没有值，未定义行为
int val2 = a.value();    // 如果没有值，抛出 std::bad_optional_access
int val3 = a.value_or(0); // 有值返回值，没有值返回 0
```

### 在 Taco 里的应用

第四章已经用了 `optional` 来表示文件读取结果：

```cpp
std::optional<std::string> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;  // 文件打开失败
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// 使用
auto content = read_file("script.taco");
if (!content) {
    std::cerr << "🌮 Cannot read file\n";
    return 1;
}
run(*content);
```

`optional` 比"用空字符串表示失败"更安全——调用者必须检查是否有值，不能直接使用，编译器会强制这种正确性。

### optional 和 Python 的对比

Python 里通常用 `None` 来表示"没有值"：

```python
def read_file(path):
    try:
        with open(path) as f:
            return f.read()
    except:
        return None

content = read_file("script.taco")
if content is not None:
    run(content)
```

C++ 的 `optional<string>` 和 Python 的 `Optional[str]`（类型提示）思路完全相同。不同的是 C++ 在编译时强制检查，Python 只是类型提示（运行时不强制）。

---

## 19.3 variant：类型安全的联合体

`std::variant<T1, T2, T3>` 表示一个可以是多种类型之一的值，但在同一时刻只有一种类型。这是 C 语言 `union` 的类型安全升级版。

Taco 的值类型就是 `variant`：

```cpp
using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;
```

一个 `TacoValue` 可以是数字、字符串、布尔值或 nil 中的任意一种，但同一时刻只能是一种。

### 基本用法

```cpp
#include <variant>

std::variant<int, std::string, double> v;

// 赋值
v = 42;          // 现在 v 是 int
v = "hello";     // 现在 v 是 string
v = 3.14;        // 现在 v 是 double

// 检查类型
if (std::holds_alternative<int>(v)) {
    std::cout << "It's an int\n";
}

// 获取值
int i = std::get<int>(v);       // 如果 v 不是 int，抛出 std::bad_variant_access
int* p = std::get_if<int>(&v);  // 如果 v 是 int，返回指针；否则返回 nullptr

// 访问（推荐方式）：visit 模式
std::visit([](auto&& val) {
    std::cout << val << "\n";
}, v);
```

### std::visit 的用法

`std::visit` 接受一个"访问者"（通常是 lambda 或函数对象）和一个 `variant`，根据 `variant` 当前的类型调用访问者：

```cpp
TacoValue value = 42.0;

// 根据类型做不同的事
std::visit([](auto&& v) {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, double>) {
        std::cout << "Number: " << v << "\n";
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "String: " << v << "\n";
    } else if constexpr (std::is_same_v<T, bool>) {
        std::cout << "Bool: " << (v ? "true" : "false") << "\n";
    } else {
        std::cout << "nil\n";
    }
}, value);
```

这里用了 `if constexpr`——编译期的条件分支，根据类型在编译时选择代码路径。`std::is_same_v<T, double>` 检查 `T` 是否等于 `double`，这是编译期计算，没有运行时开销。

### Taco 里的 variant 使用

在 `value.cpp` 里，很多函数都用 `holds_alternative` 来检查类型：

```cpp
std::string value_to_string(const TacoValue& val) {
    if (std::holds_alternative<double>(val)) {
        double d = std::get<double>(val);
        // ...
        return std::to_string(d);
    }
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    }
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? "true" : "false";
    }
    return "nil";  // std::nullptr_t
}
```

### variant vs union

C 的 `union` 把同一块内存解释成不同类型，没有类型检查：

```c
union Value {
    int   i;
    float f;
    char* s;
};

union Value v;
v.i = 42;
printf("%f\n", v.f);  // 未定义行为！但编译器不会报错
```

`variant` 追踪当前存储的是哪种类型，访问错误类型会抛出异常：

```cpp
std::variant<int, float, std::string> v = 42;
std::get<float>(v);  // 抛出 std::bad_variant_access，不是静默的未定义行为
```

---

## 19.4 random：随机数

`<random>` 是 C++11 引入的随机数库，比 C 的 `rand()` 更灵活、更统一、质量更好。

在 Taco 里，`🌮` 彩蛋需要随机选择 Magic 8 Ball 的答案，`🌮🌮` 需要随机选择魔法海螺的回答，`🌮🌮🌮` 需要随机选择程序员笑话——都用 `<random>`。

### 基本用法

`<random>` 的设计分两层：**随机数引擎**（产生随机位序列）和**分布**（把随机位序列转成特定分布的数）：

```cpp
#include <random>

// 随机数引擎：Mersenne Twister 是最常用的
std::mt19937 engine(std::random_device{}());
// std::random_device 用硬件随机数（真随机）作为种子
// std::mt19937 是伪随机数生成器，种子固定时序列可重现

// 均匀整数分布：[0, 19] 之间的随机整数
std::uniform_int_distribution<int> dist(0, 19);

int random_index = dist(engine);
```

### 在 Taco 彩蛋里的应用

```cpp
// 全局随机数引擎（只初始化一次）
static std::mt19937 rng(std::random_device{}());

// Magic 8 Ball：20 个回答里随机选一个
std::string magic_8_ball() {
    static const std::vector<std::string> answers = {
        "It is certain.",
        "Without a doubt.",
        "Yes, definitely.",
        "You may rely on it.",
        "Most likely.",
        "Outlook good.",
        "Yes.",
        "Signs point to yes.",
        "As I see it, yes.",
        "It is decidedly so.",
        "Reply hazy, try again.",
        "Ask again later.",
        "Cannot predict now.",
        "Concentrate and ask again.",
        "Better not tell you now.",
        "Don't count on it.",
        "My reply is no.",
        "Very doubtful.",
        "Outlook not so good.",
        "My sources say no.",
    };

    std::uniform_int_distribution<int> dist(0, answers.size() - 1);
    return "🎱 " + answers[dist(rng)];
}

// 魔法海螺：9 个回答里随机选一个
std::string magic_conch() {
    static const std::vector<std::string> answers = {
        "No.",
        "I don't think so.",
        "No, no, no, no, no.",
        "Maybe someday.",
        "Yes.",
        "Try asking again.",
        "Neither.",
        "Mmm, I don't think so.",
        "No, definitely not.",
    };

    std::uniform_int_distribution<int> dist(0, answers.size() - 1);
    return answers[dist(rng)];
}
```

### 为什么不用 rand()

C 的 `rand()` 有几个问题：

```c
srand(time(NULL));       // 种子只有秒级精度，同一秒内总是相同
int r = rand() % 20;    // 取模会使分布不均匀（如果 RAND_MAX 不是 20 的整倍数）
                         // rand() 的质量通常很差，周期短
```

`<random>` 的设计解决了这些问题：`random_device` 提供高质量的种子，`mt19937` 有极长的周期（2^19937 - 1），分布类（`uniform_int_distribution`）保证均匀分布。

---

### 常用的分布类

```cpp
std::mt19937 rng(std::random_device{}());

// 均匀整数：[a, b] 闭区间
std::uniform_int_distribution<int> int_dist(1, 100);
int n = int_dist(rng);  // 1 到 100 之间的整数

// 均匀浮点：[a, b) 半开区间
std::uniform_real_distribution<double> real_dist(0.0, 1.0);
double d = real_dist(rng);  // 0.0 到 1.0 之间的浮点数

// 正态分布（高斯分布）
std::normal_distribution<double> normal_dist(0.0, 1.0);  // 均值 0，标准差 1
double g = normal_dist(rng);

// 伯努利分布（抛硬币）
std::bernoulli_distribution coin(0.5);  // 0.5 概率为 true
bool heads = coin(rng);
```

---

## 小结

**`chrono`** 提供类型安全的时间处理，`system_clock::now()` 获取当前时间，`high_resolution_clock` 用于性能测量，`duration_cast` 在时间单位之间转换。

**`optional<T>`** 表示可能存在或不存在的值，比用特殊值（`-1`、`nullptr`）表示"没有"更安全、更清晰。用 `if (opt)` 检查是否有值，用 `*opt` 或 `opt.value()` 获取值。

**`variant<T1, T2, ...>`** 是类型安全的联合体，同一时刻只能是其中一种类型。用 `holds_alternative<T>` 检查类型，`get<T>` 或 `get_if<T>` 获取值，`visit` 遍历所有可能的类型。Taco 的值类型 `TacoValue` 就是一个 `variant`。

**`random`** 提供高质量的随机数，`mt19937` + `random_device` 是标准组合，各种分布类（`uniform_int_distribution` 等）保证正确的统计分布。

---

下一章是第四部分的项目章节：用智能指针和移动语义重构 Taco，实现函数、闭包和作用域（v3）。
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
