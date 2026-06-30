# 第八章：构造与析构

---

上一章看到了类的基本结构。这一章深入讲对象的生命周期：它是怎么被创建的，又是怎么被销毁的。

这是 C++ 和 Python 差距最大的地方之一。Python 有垃圾回收，对象什么时候销毁不需要关心。C++ 没有垃圾回收，但它用一套精妙的机制来弥补这个缺失——RAII。

---

## 8.1 构造函数：对象如何诞生

构造函数（constructor）在对象被创建时自动调用，负责初始化对象的状态。

```cpp
class Lexer {
public:
    // 构造函数：和类同名，没有返回类型
    Lexer(std::string source) {
        m_source = std::move(source);
        m_pos    = 0;
        m_line   = 1;
        m_column = 1;
    }

private:
    std::string m_source;
    int m_pos;
    int m_line;
    int m_column;
};

// 创建对象时，构造函数自动调用
Lexer lexer("var x = 10;");
```

**和 Python 的 `__init__` 对比**：

```python
class Lexer:
    def __init__(self, source):
        self.source = source
        self.pos = 0
        self.line = 1
        self.column = 1
```

功能完全相同。区别在于：
- Python 的 `__init__` 是特殊方法名，C++ 的构造函数是和类同名的函数
- Python 需要显式写 `self`，C++ 里是隐式的 `this`
- C++ 的构造函数没有返回类型（连 `void` 都没有）

---

### 默认构造函数

如果没有定义任何构造函数，编译器会自动生成一个**默认构造函数**（default constructor），它什么都不做：

```cpp
class Point {
public:
    double x;
    double y;
    // 没有定义构造函数
};

Point p;   // 使用默认构造函数，x 和 y 的值未定义（是内存里的随机数！）
Point p2{};  // 值初始化，x 和 y 被初始化为 0
```

注意：`Point p` 和 `Point p2{}` 的行为不同。对于内置类型（`int`、`double` 等），`p` 的成员变量值是未定义的（读取它是未定义行为），`p2` 的成员变量被初始化为 0。

一旦定义了任何构造函数，编译器就不再自动生成默认构造函数：

```cpp
class Lexer {
public:
    Lexer(std::string source) { ... }  // 定义了带参数的构造函数
};

Lexer lexer;  // 错误！没有默认构造函数
```

如果既需要带参数的构造函数，又需要默认构造函数，可以显式定义，或者用 `= default`：

```cpp
class Token {
public:
    Token() = default;  // 显式要求编译器生成默认构造函数
    Token(std::string type, std::string value)
        : m_type(std::move(type)), m_value(std::move(value)) {}

private:
    std::string m_type;
    std::string m_value;
};

Token t1;              // 使用默认构造函数
Token t2("NUMBER", "42");  // 使用带参数的构造函数
```

---

### explicit 关键字

构造函数可以触发隐式类型转换，有时候这不是我们想要的：

```cpp
class Lexer {
public:
    Lexer(std::string source) { ... }
};

void run(Lexer lexer) { ... }

run("var x = 10;");  // 可以！"var x = 10;" 隐式转换成 Lexer
```

这里 `"var x = 10;"` 是一个 `const char*`，它先被转换成 `std::string`，再被用来构造 `Lexer`。这个隐式转换可能让代码难以理解。

用 `explicit` 关键字禁止隐式转换：

```cpp
class Lexer {
public:
    explicit Lexer(std::string source) { ... }
};

run("var x = 10;");     // 错误！不能隐式转换
run(Lexer("var x = 10;"));  // 正确：显式构造
```

对于只有一个参数的构造函数，`explicit` 几乎是标配——可以防止意外的隐式转换。

---

## 8.2 析构函数：对象如何消亡

析构函数（destructor）在对象被销毁时自动调用。

```cpp
class FileReader {
public:
    FileReader(const std::string& path) {
        m_file = std::fopen(path.c_str(), "r");
        if (!m_file) {
            throw std::runtime_error("Cannot open file: " + path);
        }
    }

    // 析构函数：~ 加类名，没有参数，没有返回类型
    ~FileReader() {
        if (m_file) {
            std::fclose(m_file);  // 自动关闭文件
            m_file = nullptr;
        }
    }

    // ... 其他成员函数

private:
    FILE* m_file = nullptr;
};
```

析构函数何时被调用？

```cpp
void process_file() {
    FileReader reader("script.taco");  // 构造函数调用，文件打开
    // ... 使用 reader
}   // 函数结束，reader 离开作用域，析构函数自动调用，文件关闭
```

**Python 没有析构函数的直接对应物**。Python 有 `__del__`，但它的调用时机是不确定的（取决于垃圾回收器），不能用于可靠的资源清理。C++ 的析构函数是确定性的——对象离开作用域，析构函数立刻调用。

---

### 什么时候需要析构函数

如果类持有需要手动释放的资源，就需要析构函数：

- 动态分配的内存（`new` 出来的）
- 文件句柄
- 网络连接
- 锁（mutex）
- 任何需要配对操作的资源（open/close、lock/unlock、acquire/release）

如果类只包含普通成员变量（`int`、`std::string`、`std::vector` 等），不需要定义析构函数——编译器自动生成的析构函数会正确销毁所有成员变量。

---

## 8.3 初始化列表

回到 Lexer 的构造函数。上一节（8.1）举例时用的是赋值方式初始化：

```cpp
Lexer::Lexer(std::string source) {
    m_source = std::move(source);  // 赋值
    m_pos    = 0;
    m_line   = 1;
    m_column = 1;
}
```

但第七章 `lexer.cpp` 里实际写的是另一种方式：**初始化列表**（member initializer list），当时留了个坑，现在来填上：

```cpp
Lexer::Lexer(std::string source)
    : m_source(std::move(source))  // 直接初始化
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}
```

冒号 `:` 后面是初始化列表，每个成员变量用括号括起来要初始化的值。

**为什么要用初始化列表？**

有几个原因：

**原因一：效率**

用赋值方式，成员变量先被默认构造（对于 `std::string`，这会创建一个空字符串），然后再被赋值。用初始化列表，成员变量直接用给定的值构造，省去了默认构造这一步。

对于 `int` 这类简单类型，差别不大。但对于 `std::string`、`std::vector` 这类复杂类型，初始化列表更高效。

**原因二：必要性**

有些成员变量必须用初始化列表：

- `const` 成员变量：`const` 变量不能赋值，只能初始化
- 引用成员变量：引用必须在声明时绑定，不能用赋值
- 没有默认构造函数的成员变量

```cpp
class Config {
public:
    Config(int max_tokens, const std::string& filename)
        : m_max_tokens(max_tokens)   // const int 必须用初始化列表
        , m_filename(filename)       // const std::string& 引用
    {}

private:
    const int m_max_tokens;          // const，不能赋值
    const std::string& m_filename;   // 引用，不能重新绑定
};
```

**初始化顺序**

成员变量的初始化顺序取决于它们在类里的**声明顺序**，而不是初始化列表里的顺序。

```cpp
class Example {
    int m_a;
    int m_b;
public:
    Example(int a, int b)
        : m_b(b)   // 虽然 m_b 写在前面
        , m_a(a)   // 但 m_a 先初始化（因为声明在前）
    {}
};
```

为了避免混淆，最好让初始化列表的顺序和声明顺序一致。

---

## 8.4 RAII 初步：资源与对象生命周期绑定

RAII（Resource Acquisition Is Initialization）是 C++ 里最重要的设计模式。名字有点绕，但思想很简单：

> 用对象的生命周期来管理资源。对象构造时获取资源，对象析构时释放资源。

这样，只要对象的生命周期管理好了，资源就自动管理好了——不需要手动释放，也不会忘记释放。

### 没有 RAII 的世界

```cpp
void process_file(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return;

    // 处理文件...
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f)) {
        // ... 处理每一行
        if (some_error_condition) {
            std::fclose(f);  // 如果忘了这行，内存泄漏！
            return;
        }
    }

    std::fclose(f);  // 正常情况下关闭
}
```

每个 `return` 之前都需要手动关闭文件。如果有异常、有多个 `return`，很容易漏掉。这是 C 代码里常见的 bug 来源。

### 有 RAII 的世界

```cpp
class FileGuard {
public:
    explicit FileGuard(const std::string& path)
        : m_file(std::fopen(path.c_str(), "r"))
    {
        if (!m_file) {
            throw std::runtime_error("Cannot open: " + path);
        }
    }

    ~FileGuard() {
        if (m_file) std::fclose(m_file);
    }

    FILE* get() const { return m_file; }

    // 禁止拷贝（文件句柄不应该被复制）
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;

private:
    FILE* m_file;
};

void process_file(const std::string& path) {
    FileGuard guard(path);  // 打开文件

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), guard.get())) {
        if (some_error_condition) {
            return;  // guard 离开作用域，析构函数自动关闭文件
        }
    }
    // 函数结束，guard 析构，文件自动关闭
}
```

不管函数从哪里 `return`，甚至抛出异常，`FileGuard` 的析构函数都会被调用，文件一定被关闭。这就是 RAII 的魔力。

### RAII 无处不在

标准库里的容器都是 RAII 的：

```cpp
{
    std::vector<int> v = {1, 2, 3};  // 构造，分配内存
    v.push_back(4);
    // ... 使用 v
}  // v 析构，内存自动释放

{
    std::string s = "hello";  // 构造，分配内存
    s += " world";
}  // s 析构，内存自动释放
```

`std::mutex` 的 RAII 包装：

```cpp
std::mutex mu;

void safe_increment(int& count) {
    std::lock_guard<std::mutex> lock(mu);  // 加锁
    count++;
    // 函数结束，lock 析构，自动解锁
    // 不需要手动 mu.unlock()
}
```

第十六到十七章讲智能指针时，还会看到更多 RAII 的应用。

---

### 在 Taco 里的应用

Taco 的 `Lexer` 类虽然简单，但已经用到了 RAII 的思想：

```cpp
Lexer::Lexer(std::string source)
    : m_source(std::move(source))
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}
```

`m_source` 是 `std::string`，它自己管理内存。`Lexer` 不需要析构函数——当 `Lexer` 对象销毁时，`m_source` 的析构函数会自动被调用，内存自动释放。

这就是 RAII 的另一面：不只是自己写析构函数，而是尽量用已经实现好了 RAII 的类型，让它们帮你管理资源。

---

## *More About 构造函数*：委托构造与 = delete / = default

> 第一次读可以跳过。

### 委托构造

如果多个构造函数有重复的初始化逻辑，可以用委托构造（delegating constructor）：

```cpp
class Token {
public:
    // 完整构造函数
    Token(std::string type, std::string value, int line, int column)
        : m_type(std::move(type))
        , m_value(std::move(value))
        , m_line(line)
        , m_column(column)
    {}

    // 委托构造：调用完整构造函数，line 和 column 用默认值
    Token(std::string type, std::string value)
        : Token(std::move(type), std::move(value), 0, 0)
    {}

private:
    std::string m_type;
    std::string m_value;
    int m_line;
    int m_column;
};
```

委托构造避免了重复代码，初始化逻辑只写一次。

### = delete 和 = default

`= delete` 显式删除某个函数，让编译器报错而不是生成默认版本：

```cpp
class Lexer {
public:
    explicit Lexer(std::string source);

    // 禁止拷贝：Lexer 不应该被复制
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;

    // 允许移动（第十七章讲）
    Lexer(Lexer&&) = default;
    Lexer& operator=(Lexer&&) = default;
};

Lexer a("var x = 10;");
Lexer b = a;  // 错误！拷贝构造函数被删除
```

`= default` 显式要求编译器生成默认实现：

```cpp
class Token {
public:
    Token() = default;      // 生成默认构造函数
    ~Token() = default;     // 生成默认析构函数
    Token(const Token&) = default;  // 生成默认拷贝构造函数
};
```

这在需要明确表达意图时很有用：显式写 `= default` 比什么都不写更清晰——让读代码的人知道"这不是忘了写，而是故意让编译器生成"。

---

## 小结

这一章讲了对象的生命周期：

**构造函数**在对象创建时自动调用，负责初始化。`explicit` 防止意外的隐式转换，是单参数构造函数的标配。

**析构函数**在对象销毁时自动调用，负责清理资源。如果类持有需要手动释放的资源，就需要析构函数。

**初始化列表**是初始化成员变量的正确方式，比赋值更高效，而且对 `const` 成员和引用成员是唯一选择。

**RAII** 是 C++ 最重要的设计模式：用对象的生命周期管理资源。构造时获取，析构时释放。不需要手动管理，也不会忘记释放。标准库里的 `vector`、`string`、`mutex` 都是 RAII 的。

---

下一章讲拷贝与赋值——当一个对象被复制时，C++ 做了什么，以及什么时候这会变得危险。
