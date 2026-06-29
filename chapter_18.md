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
