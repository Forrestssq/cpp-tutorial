# 第九章：拷贝与赋值

---

上一章讲了对象的诞生（构造）和消亡（析构）。这一章讲另一件事：对象的复制。

在 Python 里，赋值几乎总是创建一个引用，指向同一个对象：

```python
a = [1, 2, 3]
b = a        # b 和 a 指向同一个列表
b.append(4)
print(a)     # [1, 2, 3, 4]，a 也变了
```

C++ 里则不同。默认情况下，赋值会创建一个真正的副本。但当对象里有指针时，这个"默认"行为会带来大麻烦。

---

## 9.1 拷贝构造函数

当一个对象被用来初始化另一个同类型对象时，**拷贝构造函数**（copy constructor）会被调用：

```cpp
Token t1("NUMBER", "42");
Token t2 = t1;   // 拷贝构造：用 t1 初始化 t2
Token t3(t1);    // 同上，另一种写法

void print_token(Token t) { ... }  // 传值时也会触发拷贝构造
print_token(t1);
```

如果没有定义拷贝构造函数，编译器会自动生成一个——它会逐个拷贝所有成员变量。对于 `Token` 这样的简单类，这个默认行为完全够用：

```cpp
class Token {
public:
    std::string type;
    std::string value;
    int line;
    int column;
};

Token t1{"NUMBER", "42", 1, 5};
Token t2 = t1;
// t2.type == "NUMBER"，t2.value == "42"，t2.line == 1，t2.column == 5
// t2 是 t1 的完整副本，修改 t2 不影响 t1
```

---

## 9.2 拷贝赋值运算符

拷贝赋值运算符（copy assignment operator）在对象已经存在的情况下被赋值时调用：

```cpp
Token t1{"NUMBER", "42", 1, 5};
Token t2{"STRING", "hello", 2, 3};

t2 = t1;   // 拷贝赋值：t2 已存在，把 t1 的内容复制给 t2
```

编译器同样会自动生成一个拷贝赋值运算符，逐个拷贝成员变量。

拷贝构造函数和拷贝赋值运算符的区别：

```cpp
Token t2 = t1;  // 拷贝构造（t2 是新创建的）
t2 = t1;        // 拷贝赋值（t2 已存在）
```

---

## 9.3 Rule of Three

这里有一个重要的规则：**Rule of Three**（三法则）。

> 如果一个类需要自定义以下三个中的任何一个，那么它通常需要自定义全部三个：
> 1. 析构函数
> 2. 拷贝构造函数
> 3. 拷贝赋值运算符

为什么？因为需要自定义析构函数，通常意味着类里有需要手动管理的资源（比如裸指针）。如果有裸指针，默认的拷贝行为就会出问题——它只拷贝指针的值（地址），而不是指针指向的数据。

---

## 9.4 为什么拷贝有时候很危险

来看一个典型的例子，一个简单的字符串类（不用 `std::string`，手动管理内存）：

```cpp
class MyString {
public:
    MyString(const char* str) {
        m_length = std::strlen(str);
        m_data = new char[m_length + 1];  // 在堆上分配内存
        std::strcpy(m_data, str);
    }

    ~MyString() {
        delete[] m_data;  // 释放内存
    }

    void print() const {
        std::cout << m_data << "\n";
    }

private:
    char* m_data;
    int   m_length;
};
```

这个类有析构函数，但没有自定义拷贝构造函数和拷贝赋值运算符。会发生什么？

```cpp
MyString a("hello");
MyString b = a;   // 使用编译器生成的默认拷贝构造函数
```

默认拷贝构造函数做的事情：把 `a.m_data`（一个指针地址）复制给 `b.m_data`，把 `a.m_length` 复制给 `b.m_length`。

现在 `a.m_data` 和 `b.m_data` 指向**同一块内存**：

```
a.m_data ──┐
           ▼
         [h][e][l][l][o][\0]
           ▲
b.m_data ──┘
```

这就是**浅拷贝**（shallow copy）。问题来了：

```cpp
{
    MyString b = a;
}  // b 析构，delete[] b.m_data，释放了那块内存

a.print();  // 未定义行为！a.m_data 指向已经被释放的内存
```

`b` 析构时，`delete[] b.m_data` 释放了那块内存。但 `a.m_data` 还指向那里。之后访问 `a`，就是访问已释放的内存，程序可能崩溃，也可能输出乱码，这是一种严重的 bug。

更糟糕的是，当 `a` 自己析构时，`delete[] a.m_data` 会**再次释放同一块内存**（double free），这是未定义行为，通常会导致崩溃。

---

### 正确的做法：深拷贝

解决方法是实现**深拷贝**（deep copy）——不只复制指针，而是复制指针指向的数据：

```cpp
class MyString {
public:
    MyString(const char* str) {
        m_length = std::strlen(str);
        m_data = new char[m_length + 1];
        std::strcpy(m_data, str);
    }

    // 拷贝构造函数：深拷贝
    MyString(const MyString& other) {
        m_length = other.m_length;
        m_data = new char[m_length + 1];  // 分配新内存
        std::strcpy(m_data, other.m_data); // 复制数据
    }

    // 拷贝赋值运算符：深拷贝
    MyString& operator=(const MyString& other) {
        if (this == &other) return *this;  // 自我赋值检查

        delete[] m_data;  // 释放旧内存

        m_length = other.m_length;
        m_data = new char[m_length + 1];
        std::strcpy(m_data, other.m_data);

        return *this;
    }

    ~MyString() {
        delete[] m_data;
    }

private:
    char* m_data;
    int   m_length;
};
```

现在 `MyString b = a` 会分配新的内存，复制数据，两个对象互相独立：

```
a.m_data ──→ [h][e][l][l][o][\0]

b.m_data ──→ [h][e][l][l][o][\0]  （独立的副本）
```

注意拷贝赋值运算符里的**自我赋值检查**：`if (this == &other) return *this;`。如果写了 `a = a`，没有这个检查，`delete[] m_data` 会先释放自己的数据，然后再试图复制已经释放的数据——又是未定义行为。

---

### 现代 C++ 的建议

手动管理内存、手动实现深拷贝，这是 C++ 很容易出错的地方。现代 C++ 的建议是：**尽量不要手动管理内存**。

用 `std::string` 而不是 `char*`，用 `std::vector` 而不是裸数组，用 `std::unique_ptr` 而不是裸指针。这些标准库类都已经正确实现了深拷贝，用它们就不需要自己写拷贝构造函数和拷贝赋值运算符。

在 Taco 项目里，`Token` 的成员都是 `std::string` 和 `int`，`Lexer` 的成员是 `std::string` 和 `int`——都不涉及裸指针，所以不需要手动实现拷贝。

Rule of Three 在现代 C++ 里通常这样理解：**如果你觉得需要自定义析构函数，先想想能不能换成用标准库类来管理资源，从而完全避免手动实现拷贝**。

---

## *More About 拷贝*：深拷贝 vs 浅拷贝的底层图景

> 第一次读可以跳过。

### 浅拷贝的完整图景

浅拷贝只复制对象表面的内容：

```
对象 a：
  [m_data: 0x1234] [m_length: 5]
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存

浅拷贝后对象 b：
  [m_data: 0x1234] [m_length: 5]  ← m_data 和 a 相同
         │
         ▼（和 a 指向同一块内存）
       [h][e][l][l][o][\0]
```

浅拷贝之后：
- 修改 `b.m_data[0] = 'H'` 会同时影响 `a`
- `b` 析构后，`a.m_data` 变成悬空指针
- `a` 析构时，double free

### 深拷贝的完整图景

深拷贝复制对象的全部内容，包括指针指向的数据：

```
对象 a：
  [m_data: 0x1234] [m_length: 5]
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存 A

深拷贝后对象 b：
  [m_data: 0x5678] [m_length: 5]  ← m_data 指向不同位置
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存 B（全新分配，内容相同）
```

深拷贝之后：
- 修改 `b` 不影响 `a`
- `b` 析构时，只释放内存 B
- `a` 析构时，只释放内存 A

### std::string 怎么做的

`std::string` 内部通常包含一个指向字符数据的指针（或者对短字符串用内联存储优化）。它实现了正确的深拷贝：

```cpp
std::string a = "hello";
std::string b = a;  // 深拷贝：b 有自己独立的字符数组

b[0] = 'H';
std::cout << a;  // "hello"，a 不受影响
std::cout << b;  // "Hello"
```

这就是为什么用 `std::string` 而不是 `char*` 可以避免很多问题——`std::string` 已经帮你处理好了所有的内存管理细节。

---

## 小结

这一章讲了 C++ 的拷贝机制：

**拷贝构造函数**在用一个对象初始化另一个对象时调用，**拷贝赋值运算符**在对已存在的对象赋值时调用。

**Rule of Three**：如果需要自定义析构函数，通常也需要自定义拷贝构造函数和拷贝赋值运算符。

**浅拷贝**只复制指针值，导致两个对象共享同一块内存，会产生 double free 和悬空指针等严重 bug。**深拷贝**分配新内存，复制数据，两个对象独立。

**现代 C++ 的建议**：用标准库类（`std::string`、`std::vector`、智能指针）代替裸指针，从根本上避免手动实现深拷贝。

---

下一章讲运算符重载——让自定义类型支持 `+`、`==`、`<<` 等运算符，让代码读起来更自然。
