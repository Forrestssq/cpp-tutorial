# 第七章：类的基础

---

第一部分讲的是 C++ 的"升级版 C"——那些让 C 写起来更舒服的特性。从这一章开始进入第二部分：类与对象。

类是 C++ 最核心的特性之一，也是和 C 差距最大的地方。Python 里已经有类的概念，所以这里不从零解释"什么是封装"，而是直接对比：C++ 的类和 Python 的类有什么相同，有什么不同，为什么会有这些不同。

---

## 7.1 从 Python 的类到 C++ 的类：对比切入

先来看同一个概念在两种语言里的写法。

**Python 版本**：

```python
class Token:
    def __init__(self, type, value):
        self.type = type
        self.value = value

    def to_string(self):
        return f"[{self.type}] \"{self.value}\""

t = Token("NUMBER", "42")
print(t.to_string())  # [NUMBER] "42"
```

**C++ 版本**：

```cpp
#include <string>
#include <iostream>

class Token {
public:
    std::string type;
    std::string value;

    Token(std::string type, std::string value)
        : type(std::move(type)), value(std::move(value))
    {}

    std::string to_string() const {
        return "[" + type + "] \"" + value + "\"";
    }
};

int main() {
    Token t("NUMBER", "42");
    std::cout << t.to_string() << "\n";  // [NUMBER] "42"
}
```

表面上很相似：都有数据成员、构造函数、成员函数。但有几个关键区别：

**区别一：必须声明类型**

Python 里 `self.type = type` 就直接创建了成员变量，类型是运行时决定的。C++ 里必须在类定义里声明每个成员变量的类型：`std::string type;`。

**区别二：访问控制**

C++ 有 `public`、`private`、`protected` 关键字，明确控制哪些东西外部可以访问。Python 里只有约定（以 `_` 开头表示"私有"），没有强制性的访问控制。

**区别三：构造函数的写法**

Python 的构造函数叫 `__init__`，C++ 的构造函数和类同名（`Token`）。C++ 还有初始化列表（`: type(...), value(...)`），这是 C++ 特有的语法，第八章会详细讲。

**区别四：`const` 成员函数**

C++ 的 `to_string()` 后面有一个 `const`，表示这个函数不会修改对象的状态。Python 没有这个概念——任何函数都可以修改 `self`。

**区别五：对象的创建方式**

Python 里 `t = Token("NUMBER", "42")` 返回的是一个对象引用，对象本身在堆上。C++ 里 `Token t("NUMBER", "42")` 默认在栈上创建对象，`Token* t = new Token("NUMBER", "42")` 才是在堆上。这个区别非常重要，第十五章讲智能指针时会深入讨论。

---

## 7.2 成员变量与成员函数

### 成员变量

成员变量（member variable）是类的数据部分，描述对象的状态：

```cpp
class Lexer {
    std::string m_source;  // 源代码
    int         m_pos;     // 当前位置
    int         m_line;    // 当前行号
};
```

命名约定：很多 C++ 项目用 `m_` 前缀表示成员变量（member），区别于局部变量和参数。这不是语言要求，但是一个有用的习惯——读代码时一眼就能看出哪些是成员变量。

成员变量可以在声明时给默认值（C++11 起）：

```cpp
class Lexer {
    std::string m_source;
    int m_pos    = 0;     // 默认值
    int m_line   = 1;     // 默认值
    int m_column = 1;     // 默认值
};
```

---

### 成员函数

成员函数（member function）是类的行为部分，定义对象能做什么：

```cpp
class Lexer {
public:
    // 成员函数：可以访问 m_source、m_pos 等成员变量
    char current() const {
        if (is_at_end()) return '\0';
        return m_source[m_pos];
    }

    bool is_at_end() const {
        return m_pos >= static_cast<int>(m_source.size());
    }
};
```

成员函数可以直接访问同一个类的成员变量，不需要传参数。这就是"封装"的核心——数据和操作数据的函数绑定在一起。

**在类外定义成员函数**

成员函数可以在类定义里声明，在类外实现：

```cpp
// lexer.h：只声明
class Lexer {
public:
    char current() const;
    bool is_at_end() const;
private:
    std::string m_source;
    int m_pos = 0;
};

// lexer.cpp：实现，用 Lexer:: 前缀表明属于 Lexer 类
char Lexer::current() const {
    if (is_at_end()) return '\0';
    return m_source[m_pos];
}

bool Lexer::is_at_end() const {
    return m_pos >= static_cast<int>(m_source.size());
}
```

这是大型项目里的标准做法：头文件只放接口（声明），源文件放实现。

---

## 7.3 访问控制：public、private、protected

C++ 用三个关键字控制成员的访问权限：

- `public`：任何地方都可以访问
- `private`：只有类自己的成员函数可以访问
- `protected`：类自己和子类可以访问（第十二章讲继承时再展开）

```cpp
class BankAccount {
public:
    // 公开接口：外部可以调用
    void deposit(double amount) {
        if (amount > 0) m_balance += amount;
    }

    double get_balance() const {
        return m_balance;
    }

private:
    // 私有数据：外部无法直接访问和修改
    double m_balance = 0.0;
    std::string m_owner;
};

BankAccount account;
account.deposit(100.0);          // 可以，deposit 是 public
double b = account.get_balance(); // 可以，get_balance 是 public
account.m_balance = 9999.0;      // 错误！m_balance 是 private
```

**为什么要 private？**

把数据设为 private，强迫外部代码通过公开接口操作对象，而不是直接修改数据。这样有几个好处：

- 可以在接口里做检查（`if (amount > 0)`），防止非法操作
- 内部实现可以改变，只要接口不变，外部代码不需要修改
- 读代码时，public 部分就是类的"说明书"，private 是实现细节

**struct 和 class 的区别**

C++ 里 `struct` 和 `class` 几乎完全相同，只有一个区别：`struct` 的成员默认是 `public`，`class` 的成员默认是 `private`。

```cpp
struct Point {
    double x;  // 默认 public
    double y;  // 默认 public
};

class Token {
    // 这里是 private！
    std::string value;
public:
    // 这里才是 public
    std::string get_value() const { return value; }
};
```

惯例上，`struct` 用于简单的数据容器（不需要封装），`class` 用于有复杂行为的对象（需要封装）。在 Taco 项目里，`Token` 用 `struct`（只是数据），`Lexer` 用 `class`（有复杂的内部逻辑）。

---

## 7.4 this 指针

在成员函数里，有一个隐式的指针叫 `this`，指向调用这个函数的对象本身：

```cpp
class Counter {
public:
    void increment() {
        this->count++;  // 等价于 count++
    }

    // 返回对象自身的引用，可以链式调用
    Counter& add(int n) {
        this->count += n;
        return *this;  // 解引用 this，返回对象本身
    }

    int get() const {
        return this->count;  // 等价于 count
    }

private:
    int count = 0;
};

Counter c;
c.add(5).add(3).add(2);  // 链式调用
std::cout << c.get();    // 10
```

大多数情况下不需要显式写 `this->`，编译器知道成员函数里的 `count` 就是 `this->count`。但有两种情况必须用 `this`：

**情况一：参数名和成员变量同名**

```cpp
class Token {
public:
    Token(std::string value) {
        this->value = value;  // this->value 是成员变量，value 是参数
    }
private:
    std::string value;
};
```

不过更好的做法是用初始化列表（下一章讲），或者给成员变量加 `m_` 前缀来避免歧义。

**情况二：返回对象自身**

```cpp
Counter& add(int n) {
    count += n;
    return *this;  // 必须用 this 才能返回对象自身的引用
}
```

---

### Taco 里的应用

在 Taco 项目里，`Lexer` 类用 `this` 的地方主要是构造函数。当前版本的 Lexer 用了成员变量初始化列表（下一章详细讲），所以 `this` 用得不多。但理解 `this` 的存在，有助于理解成员函数和普通函数的本质区别：

成员函数在底层其实就是一个普通函数，只不过编译器帮你把"调用这个函数的对象"作为第一个参数（`this`）传进去。这就是为什么 Python 里需要显式写 `self`，而 C++ 里 `this` 是隐式的。

```python
# Python：self 是显式参数
def to_string(self):
    return self.value
```

```cpp
// C++：this 是隐式指针
std::string to_string() const {
    return value;  // 等价于 this->value
}
```

---

## 小结

这一章介绍了 C++ 类的基础：

**成员变量**描述对象的状态，**成员函数**描述对象的行为。两者绑定在一起，这就是封装。

**访问控制**（`public`/`private`）让类可以暴露稳定的接口，隐藏实现细节。`struct` 默认 `public`，`class` 默认 `private`——根据是否需要封装来选择。

**`this` 指针**是每个成员函数里隐式存在的，指向当前对象。大多数时候不需要显式写，但返回对象自身时必须用 `*this`。

---

下一章讲构造函数和析构函数——对象是怎么诞生和消亡的，以及 C++ 里最重要的设计模式之一：RAII。
