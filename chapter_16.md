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
