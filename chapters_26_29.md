# 第二十六章：函数模板

---

第六部分进入**模板**（template）——C++ 最强大也最独特的特性之一。

模板让你写出"与类型无关"的代码：写一次，适用于所有类型。这不是运行时的多态（虚函数），而是编译期的多态——编译器在编译时，根据你实际传入的类型，生成对应的代码。

在 Python 里，函数天然就是"与类型无关"的：

```python
def max_of(a, b):
    return a if a > b else b

max_of(3, 5)        # 整数，工作
max_of(3.14, 2.71)  # 浮点数，工作
max_of("apple", "banana")  # 字符串，工作
```

Python 的动态类型让这件事自然发生。C++ 是静态类型语言，但模板让 C++ 也能做到同样的事——而且是在编译期完成，没有运行时开销。

---

## 26.1 为什么需要模板

先看没有模板时的困境。

假设要写一个"返回两个值中较大者"的函数。对于 `int`，可以这样写：

```cpp
int max_of(int a, int b) {
    return a > b ? a : b;
}
```

但如果需要支持 `double`？再写一个：

```cpp
double max_of(double a, double b) {
    return a > b ? a : b;
}
```

`float`？`long`？`std::string`？每种类型都写一遍，函数体完全一样，只有类型不同。这违反了 DRY 原则（Don't Repeat Yourself），而且每次新增类型都需要修改库代码。

重载确实可以解决问题，但重载是手动的——对每种类型手写一个版本。模板是自动的——写一次，编译器帮你生成所有类型的版本。

更深层的问题：如果用户自定义了一个类型 `Score`，并且 `Score` 支持 `>` 运算，那么 `max_of` 对 `Score` 也应该能工作。但没有模板的话，用户没法扩展你的 `max_of`——除非你为每种可能的类型都写一个版本，这显然不现实。

**模板解决的核心问题是：写出对"满足某种要求的所有类型"都适用的代码。**

---

## 26.2 函数模板的写法

### 基本语法

```cpp
template<typename T>
T max_of(T a, T b) {
    return a > b ? a : b;
}
```

`template<typename T>` 声明这是一个模板，`T` 是**模板参数**（template parameter），代表一个未知的类型。

用法：

```cpp
int    m1 = max_of(3, 5);           // T 推导为 int
double m2 = max_of(3.14, 2.71);     // T 推导为 double
std::string m3 = max_of(std::string("apple"), std::string("banana"));
                                    // T 推导为 std::string
```

编译器看到 `max_of(3, 5)` 时，推导出 `T = int`，然后生成这样一份代码：

```cpp
// 编译器生成的代码（概念上的，实际在二进制里）
int max_of_int(int a, int b) {
    return a > b ? a : b;
}
```

看到 `max_of(3.14, 2.71)` 时，再生成：

```cpp
double max_of_double(double a, double b) {
    return a > b ? a : b;
}
```

这个过程叫**模板实例化**（template instantiation）——编译器用具体类型"填入"模板，生成实际的函数。每种类型只生成一份代码，不会重复。

### 多个模板参数

模板可以有多个类型参数：

```cpp
template<typename T, typename U>
auto add(T a, U b) {
    return a + b;  // 返回类型由编译器推导
}

auto result1 = add(3, 4.5);    // T = int, U = double, 返回 double
auto result2 = add(1, 2);      // T = int, U = int, 返回 int
```

这里用了 `auto` 作为返回类型，让编译器根据 `a + b` 的实际类型推导返回类型。这是 C++14 引入的特性，在 C++11 里需要用 `decltype`（暂时不需要深究）。

### 非类型模板参数

模板参数不只能是类型，也可以是**值**（非类型模板参数）：

```cpp
// N 是一个编译期常量整数
template<int N>
void print_n_times(const std::string& msg) {
    for (int i = 0; i < N; ++i) {
        std::cout << msg << "\n";
    }
}

print_n_times<3>("hello");  // 打印 3 次 hello
print_n_times<5>("taco");   // 打印 5 次 taco
```

`N` 在编译时就确定了，编译器为 `N=3` 和 `N=5` 分别生成不同的函数。循环次数是编译期常量，编译器可以做更激进的优化（甚至展开循环）。

非类型模板参数还常见于固定大小的数组：

```cpp
// 标准库的 std::array<T, N> 就是这样定义的
template<typename T, std::size_t N>
struct Array {
    T data[N];
    std::size_t size() const { return N; }
};

Array<int, 5>  arr1;  // 5 个 int
Array<double, 10> arr2;  // 10 个 double
```

`std::array<int, 5>` 的大小在编译时就固定了，不同于 `std::vector<int>` 的运行时动态大小。

---

## 26.3 模板类型推导

大多数情况下，调用函数模板时不需要显式指定类型——编译器会根据传入的参数自动推导。这叫**模板参数推导**（template argument deduction）。

### 推导的基本规则

```cpp
template<typename T>
void print(T value) {
    std::cout << value << "\n";
}

print(42);          // T 推导为 int
print(3.14);        // T 推导为 double
print("hello");     // T 推导为 const char*（注意不是 std::string！）
print(std::string("hello"));  // T 推导为 std::string
```

`"hello"` 字面量的类型是 `const char*`，不是 `std::string`。如果函数体里用到了 `std::string` 特有的方法，就会编译错误。这是初学模板时常见的坑。

### 引用参数的推导

当模板参数是引用时，推导规则稍复杂：

```cpp
template<typename T>
void modify(T& value) {
    // value 是 T 的引用
}

int x = 42;
modify(x);   // T 推导为 int，value 类型是 int&
modify(42);  // 错误！42 是右值，不能绑定到左值引用
```

```cpp
template<typename T>
void read(const T& value) {
    // value 是 const T 的引用
}

int x = 42;
read(x);    // T 推导为 int，value 类型是 const int&
read(42);   // T 推导为 int，value 类型是 const int&（右值可以绑定到 const 引用）
read(3.14); // T 推导为 double
```

`const T&` 是最通用的参数形式——既接受左值，也接受右值，还避免了拷贝。在模板里，如果只读参数，用 `const T&` 是最佳实践。

### 显式指定类型

当推导有歧义，或者希望强制使用某种类型时，可以显式指定：

```cpp
template<typename T>
T max_of(T a, T b) {
    return a > b ? a : b;
}

// max_of(3, 3.14);  // 错误！T 无法同时推导为 int 和 double

// 显式指定 T = double，3 会隐式转换为 double
double m = max_of<double>(3, 3.14);
```

```cpp
template<typename T>
T zero() {
    return T{};  // 返回 T 类型的默认值
}

// zero() 没有参数，无法推导，必须显式指定
int z1 = zero<int>();      // 0
double z2 = zero<double>(); // 0.0
std::string z3 = zero<std::string>(); // ""
```

### 推导失败的情形

```cpp
template<typename T>
T convert(double d) {
    return static_cast<T>(d);
}

// convert(3.14);  // 错误！T 无法从返回类型推导
int n = convert<int>(3.14);  // 必须显式指定
```

模板参数只能从函数**参数**推导，不能从**返回类型**推导。

---

## 26.4 模板与重载的关系

函数模板和普通重载函数可以共存。当调用一个函数时，编译器按照以下优先级选择：

1. 完全匹配的普通函数（非模板）
2. 模板实例化
3. 需要类型转换的普通函数

```cpp
template<typename T>
void print(T value) {
    std::cout << "[template] " << value << "\n";
}

// 针对 int 的特殊版本
void print(int value) {
    std::cout << "[int overload] " << value << "\n";
}

print(42);    // 选择 print(int)：完全匹配的普通函数优先
print(3.14);  // 选择模板版本：没有匹配的普通函数
print("hi");  // 选择模板版本：T = const char*
```

这个机制让你可以为某些特定类型提供定制化的实现，而对其他类型使用通用的模板版本。

### 模板与重载的实际应用

来看一个 Taco 里真实会用到的场景。Taco 的值类型是一个 `variant`（第二十九章会深入讲），现在先假设它叫 `TacoValue`，可以持有 `double`、`std::string`、`bool` 等。

我们想写一个"安全提取值"的函数：

```cpp
#include <variant>
#include <string>
#include <optional>

using TacoValue = std::variant<double, std::string, bool>;

// 通用模板：尝试提取类型 T 的值
template<typename T>
std::optional<T> get_as(const TacoValue& val) {
    if (auto* p = std::get_if<T>(&val)) {
        return *p;
    }
    return std::nullopt;
}

// 使用
TacoValue v = 3.14;
auto d = get_as<double>(v);   // optional<double>，值为 3.14
auto s = get_as<std::string>(v);  // optional<string>，为空
```

对于 `bool` 类型，可能需要特殊处理（比如允许把数字隐式转换为 bool）：

```cpp
// bool 的特殊版本：数字也能转换
std::optional<bool> get_as_bool(const TacoValue& val) {
    if (auto* b = std::get_if<bool>(&val)) {
        return *b;
    }
    if (auto* d = std::get_if<double>(&val)) {
        return *d != 0.0;  // 非零即为 true
    }
    return std::nullopt;
}
```

这里用了两个不同的函数名（`get_as` 和 `get_as_bool`），但也可以用模板特化（下一章讲）统一名字。现在先建立这个思维：**通用逻辑用模板，特殊逻辑用重载或特化**。

---

### 模板函数的声明与定义位置

普通函数可以在 `.h` 里声明，在 `.cpp` 里定义。模板函数不行——**模板函数的定义必须放在头文件里**（或者和声明放在同一个文件里）。

原因是：编译器在实例化模板时（比如看到 `max_of<int>(3, 5)`），需要看到模板的完整定义，才能生成代码。如果定义在另一个 `.cpp` 文件里，编译器看不到它，就没法实例化。

```cpp
// max_of.h
#pragma once

// 声明和定义必须在同一个头文件里
template<typename T>
T max_of(T a, T b) {
    return a > b ? a : b;
}
```

```cpp
// main.cpp
#include "max_of.h"

int main() {
    auto m = max_of(3, 5);  // 编译器能看到完整定义，可以实例化
}
```

这是模板和普通函数在工程实践上最重要的区别之一。

---

## 一个完整的例子：通用的栈

来写一个比 STL 稍简单但完整的栈（stack），展示函数模板的综合用法：

```cpp
// generic_stack.h
#pragma once
#include <vector>
#include <stdexcept>

template<typename T>
class Stack {
public:
    void push(T value) {
        data_.push_back(std::move(value));
    }

    T pop() {
        if (empty()) {
            throw std::runtime_error("Stack is empty");
        }
        T top = std::move(data_.back());
        data_.pop_back();
        return top;
    }

    const T& peek() const {
        if (empty()) {
            throw std::runtime_error("Stack is empty");
        }
        return data_.back();
    }

    bool empty() const { return data_.empty(); }
    std::size_t size() const { return data_.size(); }

private:
    std::vector<T> data_;
};
```

使用：

```cpp
Stack<int> int_stack;
int_stack.push(1);
int_stack.push(2);
int_stack.push(3);
std::cout << int_stack.pop() << "\n";  // 3
std::cout << int_stack.pop() << "\n";  // 2

Stack<std::string> str_stack;
str_stack.push("hello");
str_stack.push("world");
std::cout << str_stack.peek() << "\n"; // world（不弹出）
```

同一份代码，对 `int` 和 `std::string` 都能工作——这就是模板的价值。

注意这里 `push` 接收的是 `T value`（值），在内部用了 `std::move(value)` 避免多余的拷贝。如果你传一个右值（比如 `push(std::string("hello"))`），直接移动；如果传左值，先拷贝到 `value` 参数，再移动到 `vector`，总共一次拷贝。这是模板函数里处理参数的常见方式。

---

## 小结

**模板的核心思想**：写一次，适用于所有满足条件的类型。编译器在编译期根据实际类型生成具体的代码（模板实例化）。这是编译期多态，没有运行时开销。

**语法**：`template<typename T>` 声明模板，`T` 代表未知类型。多个类型参数用逗号分隔：`template<typename T, typename U>`。非类型参数用具体类型声明：`template<int N>`。

**类型推导**：大多数情况下编译器自动推导 `T`，不需要显式指定。推导不了时（比如返回类型、无参数的函数），需要显式写 `func<Type>(args)`。`const T&` 是最通用的参数形式。

**与重载的关系**：普通函数优先于模板实例化。可以为特定类型提供普通函数重载，覆盖模板的通用行为。

**实践要点**：模板定义必须在头文件里，不能分离到 `.cpp`。

---

下一章讲类模板——把模板用到类上，这是 STL 容器（`vector<T>`、`map<K,V>`）背后的机制。
# 第二十七章：类模板

---

上一章讲了函数模板——用模板参数让函数适用于多种类型。类模板是同样的思想用于类：定义一个"与类型无关"的类，让编译器根据使用时的具体类型生成对应的版本。

`std::vector<int>`、`std::map<std::string, int>`、`std::optional<double>`——STL 里所有的容器和工具类都是类模板。这一章解释它们背后的机制，并动手实现几个。

---

## 27.1 类模板的写法

### 基本语法

```cpp
template<typename T>
class Box {
public:
    Box(T value) : value_(value) {}

    T get() const { return value_; }
    void set(T value) { value_ = value; }

    bool operator==(const Box<T>& other) const {
        return value_ == other.value_;
    }

private:
    T value_;
};
```

使用类模板时，**必须显式指定类型**（C++17 之前；C++17 引入了类模板参数推导，后面会提到）：

```cpp
Box<int> int_box(42);
Box<std::string> str_box("hello");

int n = int_box.get();        // 42
std::string s = str_box.get(); // "hello"

int_box.set(100);

Box<int> another(100);
std::cout << (int_box == another) << "\n";  // 1（true）
```

### 类模板的实例化

和函数模板一样，类模板在编译时实例化。`Box<int>` 和 `Box<std::string>` 是两个完全不同的类，它们之间没有继承关系——只是碰巧从同一个模板生成出来。

```cpp
Box<int> int_box(42);
Box<double> dbl_box(3.14);

// 这两个是不同的类型，不能互相赋值
// int_box = dbl_box;  // 编译错误！
```

### 成员函数的定义

类模板的成员函数如果在类外定义，需要重复写模板参数：

```cpp
template<typename T>
class Box {
public:
    Box(T value);
    T get() const;
    void set(T value);
private:
    T value_;
};

// 类外定义：每个成员函数都要加 template<typename T>
template<typename T>
Box<T>::Box(T value) : value_(value) {}

template<typename T>
T Box<T>::get() const {
    return value_;
}

template<typename T>
void Box<T>::set(T value) {
    value_ = value;
}
```

`Box<T>::get` 表示"类型参数为 `T` 的 `Box` 类的 `get` 函数"。这个写法比较繁琐，所以通常把类模板的成员函数直接写在类体内——这是和普通类不同的习惯。

### C++17 类模板参数推导（CTAD）

C++17 引入了类模板参数推导（Class Template Argument Deduction），允许从构造函数参数推导模板参数：

```cpp
Box<int> b1(42);      // C++17 之前必须这样
Box b2(42);           // C++17 可以这样：编译器推导 T = int
Box b3(std::string("hello"));  // T = std::string
```

`std::vector{1, 2, 3}` 在 C++17 里也可以不写类型，编译器推导为 `std::vector<int>`。

不过显式写出类型参数依然是好习惯，特别是对复杂类型——清晰比省事更重要。

---

## 27.2 模板参数的默认值

和函数参数可以有默认值一样，模板参数也可以有默认值：

```cpp
// 第二个模板参数有默认值
template<typename Key, typename Value = std::string>
class Registry {
public:
    void insert(const Key& key, const Value& value) {
        data_[key] = value;
    }

    std::optional<Value> find(const Key& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::map<Key, Value> data_;
};

// 使用默认值：Value = std::string
Registry<std::string> r1;
r1.insert("name", "Miguel");
r1.insert("city", "Oaxaca");

// 显式指定两个参数
Registry<std::string, int> r2;
r2.insert("age", 12);
r2.insert("score", 100);
```

STL 里有很多模板参数默认值的例子：

```cpp
// std::vector 的真实定义（简化版）
template<typename T, typename Allocator = std::allocator<T>>
class vector { ... };

// std::map 的真实定义（简化版）
template<
    typename Key,
    typename Value,
    typename Compare = std::less<Key>,         // 默认升序比较
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class map { ... };
```

所以 `std::map<std::string, int>` 用了两个显式参数，Compare 和 Allocator 都用默认值。如果需要自定义排序：

```cpp
// 自定义比较：按字符串长度排序
auto cmp = [](const std::string& a, const std::string& b) {
    return a.size() < b.size();
};

std::map<std::string, int, decltype(cmp)> sorted_by_len(cmp);
sorted_by_len["hello"] = 1;
sorted_by_len["hi"] = 2;
sorted_by_len["hey"] = 3;
// 迭代顺序：hi, hey, hello（按长度升序）
```

---

## 27.3 成员函数模板

类模板的成员函数可以有**自己独立的**模板参数，和类的模板参数相互独立。

```cpp
template<typename T>
class Box {
public:
    Box(T value) : value_(value) {}
    T get() const { return value_; }

    // 成员函数模板：接受任意类型 U，转换成 Box<U>
    template<typename U>
    Box<U> convert_to() const {
        return Box<U>(static_cast<U>(value_));
    }

private:
    T value_;
};

Box<int> int_box(42);
Box<double> dbl_box = int_box.convert_to<double>();
Box<std::string> str_box = Box<double>(3.14).convert_to<std::string>();
// 错误！double 不能 static_cast 到 string
// 模板会编译失败——要求类型之间能互相转换
```

成员函数模板的另一个常见用途是**从不同类型构造**：

```cpp
template<typename T>
class SmartBox {
public:
    SmartBox(T value) : value_(value) {}

    // 允许从 Box<U> 构造 SmartBox<T>，只要 U 可以转换为 T
    template<typename U>
    SmartBox(const SmartBox<U>& other) : value_(other.get()) {}

    T get() const { return value_; }

private:
    T value_;
};

SmartBox<int> ibox(42);
SmartBox<double> dbox(ibox);  // int → double，OK
SmartBox<int> ibox2(dbox);    // double → int，精度损失，但编译通过
```

这种"转换构造函数模板"在智能指针里很常见：`std::shared_ptr<Base>` 可以从 `std::shared_ptr<Derived>` 构造（只要 `Derived` 继承自 `Base`）。

---

## 27.4 模板与继承

类模板可以继承，继承可以涉及模板参数，这里几种情况都要搞清楚。

### 普通类继承自类模板的实例

```cpp
template<typename T>
class Container {
public:
    virtual void add(T item) = 0;
    virtual T get(int index) const = 0;
    virtual int size() const = 0;
    virtual ~Container() = default;
};

// 普通类继承自具体实例
class IntList : public Container<int> {
public:
    void add(int item) override { data_.push_back(item); }
    int get(int index) const override { return data_[index]; }
    int size() const override { return data_.size(); }

private:
    std::vector<int> data_;
};

IntList list;
list.add(1);
list.add(2);
list.add(3);
std::cout << list.size() << "\n";  // 3
```

### 类模板继承自类模板

```cpp
template<typename T>
class Stack : public Container<T> {
public:
    void add(T item) override {
        data_.push_back(std::move(item));
    }

    T get(int index) const override {
        return data_[index];
    }

    int size() const override {
        return data_.size();
    }

    T pop() {
        T top = std::move(data_.back());
        data_.pop_back();
        return top;
    }

private:
    std::vector<T> data_;
};

Stack<std::string> str_stack;
str_stack.add("hello");
str_stack.add("world");
std::cout << str_stack.pop() << "\n";  // world

// 多态：通过 Container<std::string>* 操作 Stack<std::string>
Container<std::string>* c = &str_stack;
c->add("taco");
std::cout << c->size() << "\n";  // 2（因为 pop 了一个）
```

### 奇特的递归模板模式（CRTP）——了解即可

有一种叫 CRTP（Curiously Recurring Template Pattern）的高级技巧，让子类作为模板参数传给父类：

```cpp
// 父类以子类为模板参数
template<typename Derived>
class Printable {
public:
    void print() const {
        // static_cast 到子类，调用子类的方法
        static_cast<const Derived*>(this)->do_print();
    }
};

class MyClass : public Printable<MyClass> {
public:
    void do_print() const {
        std::cout << "I am MyClass\n";
    }
};

MyClass obj;
obj.print();  // 输出：I am MyClass
```

CRTP 让父类在不用虚函数的前提下调用子类的方法——是编译期多态的一种实现方式，没有虚函数表的开销。在性能敏感的场合（如数值库、嵌入式系统）会用到。这里只是让你知道有这个东西，Taco 项目里不会用到。

---

## 综合例子：Result 类型

Taco 在 v3 里用 `std::optional` 处理"可能没有值"的情况，但有时候需要区分"失败"和"为空"，需要一个携带错误信息的类型。来实现一个简化版的 `Result<T, E>`：

```cpp
// result.h
#pragma once
#include <variant>
#include <stdexcept>
#include <string>

template<typename T, typename E = std::string>
class Result {
public:
    // 工厂函数：构造成功的结果
    static Result ok(T value) {
        Result r;
        r.data_ = std::move(value);
        return r;
    }

    // 工厂函数：构造失败的结果
    static Result err(E error) {
        Result r;
        r.data_ = std::move(error);
        return r;
    }

    bool is_ok() const {
        return std::holds_alternative<T>(data_);
    }

    bool is_err() const {
        return std::holds_alternative<E>(data_);
    }

    // 提取值（调用前必须确认 is_ok()）
    T& value() {
        if (!is_ok()) throw std::runtime_error("Result is not ok");
        return std::get<T>(data_);
    }

    const T& value() const {
        if (!is_ok()) throw std::runtime_error("Result is not ok");
        return std::get<T>(data_);
    }

    // 提取错误（调用前必须确认 is_err()）
    E& error() {
        if (!is_err()) throw std::runtime_error("Result is not err");
        return std::get<E>(data_);
    }

    // 带默认值的提取：失败时返回默认值
    T value_or(T default_value) const {
        if (is_ok()) return std::get<T>(data_);
        return default_value;
    }

    // 链式操作：如果成功，对值应用函数；否则传播错误
    template<typename F>
    auto and_then(F func) const {
        using ReturnType = decltype(func(std::get<T>(data_)));
        if (is_ok()) {
            return func(std::get<T>(data_));
        }
        return ReturnType::err(std::get<E>(data_));
    }

private:
    std::variant<T, E> data_;
    Result() = default;
};
```

在 Taco 的求值器里，可以用这个类型来处理运行时错误：

```cpp
// 用 Result 处理除法（可能除以零）
Result<double> safe_divide(double a, double b) {
    if (b == 0.0) {
        return Result<double>::err("Division by zero");
    }
    return Result<double>::ok(a / b);
}

auto r1 = safe_divide(10.0, 2.0);
if (r1.is_ok()) {
    std::cout << r1.value() << "\n";  // 5
}

auto r2 = safe_divide(10.0, 0.0);
if (r2.is_err()) {
    std::cout << "Error: " << r2.error() << "\n";  // Error: Division by zero
}

// 带默认值
double result = safe_divide(10.0, 0.0).value_or(0.0);  // 0.0
```

这就是现代 C++ 里处理"可能失败的操作"的惯用方式——不用异常，不用裸裸的错误码，而是用类型系统编码成功或失败两种情况。Rust 语言的 `Result<T, E>` 是这个思想的更彻底实践。

---

## 小结

**类模板的语法**和函数模板一样，用 `template<typename T>` 前缀。类的成员变量、成员函数都可以用 `T`。成员函数如果在类外定义，每个都要加 `template<typename T>`，这很繁琐，所以通常把实现写在类体内。

**类模板的实例化**是显式的——必须写 `Box<int>` 而不是 `Box`（C++17 的 CTAD 可以推导，但显式写更清晰）。`Box<int>` 和 `Box<double>` 是完全不同的类型。

**模板参数默认值**：`template<typename K, typename V = std::string>` 让某些参数可以省略。STL 容器大量使用这个特性（`vector` 的 Allocator，`map` 的 Compare）。

**成员函数模板**：类模板的成员函数可以有独立的模板参数，与类的模板参数无关。常用于"从兼容类型转换"的构造函数。

**模板与继承**：普通类可以继承类模板的具体实例，类模板也可以继承另一个类模板。虚函数在模板类里照常工作，组合使用可以同时获得泛型和多态。

---

下一章讲模板进阶：特化、可变参数模板、`if constexpr`、`constexpr` 编译期计算——这些是理解 v5 项目代码的基础。
# 第二十八章：模板进阶

---

前两章的模板是"基础模板"：一个模板，适用于所有类型，编译器按需生成代码。但有时候需要对特定类型做特殊处理，或者处理数量不定的参数，或者在编译期做条件判断——这一章讲这些进阶用法。

---

## 28.1 模板特化：全特化与偏特化

**模板特化**（template specialization）允许为特定的类型参数提供不同的实现。

### 全特化

全特化（full specialization）为某个具体类型提供完整的替代实现：

```cpp
// 通用模板：打印任意类型
template<typename T>
void print_type(const T& value) {
    std::cout << "value: " << value << "\n";
}

// 全特化：bool 类型要打印 true/false，而不是 1/0
template<>
void print_type<bool>(const bool& value) {
    std::cout << "bool: " << (value ? "true" : "false") << "\n";
}

// 全特化：vector 要打印所有元素
template<>
void print_type<std::vector<int>>(const std::vector<int>& value) {
    std::cout << "vector[";
    for (int i = 0; i < value.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << value[i];
    }
    std::cout << "]\n";
}

print_type(42);       // value: 42（通用模板）
print_type(true);     // bool: true（bool 特化）
print_type(std::vector<int>{1, 2, 3});  // vector[1, 2, 3]（vector 特化）
```

`template<>` 是全特化的语法——模板参数列表是空的，因为具体类型已经写在函数名后面了。

类模板也可以全特化：

```cpp
// 通用模板：用数组实现的存储
template<typename T>
class Storage {
public:
    Storage() : data_(nullptr), size_(0) {}
    void store(const T& val) { /* ... */ }
    T retrieve() const { /* ... */ }
private:
    T* data_;
    int size_;
};

// 针对 bool 的全特化：用位图存储，节省内存
template<>
class Storage<bool> {
public:
    Storage() : bits_(0) {}
    void store(bool val) { bits_ = val ? 1 : 0; }
    bool retrieve() const { return bits_ != 0; }
private:
    unsigned char bits_;  // 用一个字节存 bool，而不是通用版本的指针
};
```

`std::vector<bool>` 就是标准库里臭名昭著的特化例子——为了节省内存，它用位图存 bool，导致行为和其他 `vector<T>` 不一致（比如 `v[0]` 返回的不是 `bool&` 而是一个代理对象）。这是过度优化导致破坏一致性的经典反例。

### 偏特化

偏特化（partial specialization）只针对部分模板参数做特化，剩余参数仍然是通用的。**偏特化只能用于类模板，不能用于函数模板。**

```cpp
// 通用模板：两个类型参数
template<typename T, typename U>
class Pair {
public:
    Pair(T first, U second) : first_(first), second_(second) {}
    void print() const {
        std::cout << "(" << first_ << ", " << second_ << ")\n";
    }
private:
    T first_;
    U second_;
};

// 偏特化：当两个类型相同时
template<typename T>
class Pair<T, T> {
public:
    Pair(T first, T second) : first_(first), second_(second) {}
    void print() const {
        std::cout << "same-type pair: (" << first_ << ", " << second_ << ")\n";
    }
    T sum() const { return first_ + second_; }  // 同类型才有 sum
private:
    T first_;
    T second_;
};

// 偏特化：当第二个类型是指针时
template<typename T, typename U>
class Pair<T, U*> {
public:
    Pair(T first, U* second) : first_(first), second_(second) {}
    void print() const {
        std::cout << "ptr pair: (" << first_ << ", " << *second_ << ")\n";
    }
private:
    T first_;
    U* second_;  // 第二个是指针
};

Pair<int, double> p1(1, 2.5);
p1.print();  // (1, 2.5)

Pair<int, int> p2(3, 4);
p2.print();  // same-type pair: (3, 4)
std::cout << p2.sum() << "\n";  // 7

int val = 42;
Pair<std::string, int> p3("answer", &val);
p3.print();  // ptr pair: (answer, 42)
```

偏特化让类模板可以根据类型的"形状"（是否是指针、是否两个参数相同等）采用不同的实现。标准库里 `std::is_pointer<T>`、`std::is_same<T, U>` 等类型特征（type traits）就是通过偏特化实现的。

### 类型特征（Type Traits）

`<type_traits>` 头文件提供了大量编译期查询类型属性的工具，它们都是用模板特化实现的：

```cpp
#include <type_traits>

// std::is_integral<T>::value：T 是否是整数类型
static_assert(std::is_integral<int>::value);         // 通过
static_assert(std::is_integral<double>::value == false); // 通过
static_assert(std::is_integral<bool>::value);         // 通过，bool 是整数类型

// C++17 的 _v 后缀简写
static_assert(std::is_integral_v<int>);
static_assert(!std::is_floating_point_v<int>);
static_assert(std::is_same_v<int, int>);
static_assert(!std::is_same_v<int, double>);

// std::remove_const<T>::type：去掉 const
using T = std::remove_const<const int>::type;  // T 是 int
using U = std::remove_reference<int&>::type;   // U 是 int
```

这些类型特征在模板编程里很有用——可以在编译期检查类型属性，然后用 `if constexpr` 做分支（下面就讲到）。

---

## 28.2 可变参数模板

**可变参数模板**（variadic template）让模板接受任意数量的类型参数。这是 C++11 引入的特性，解决了"函数参数数量不定"的问题。

### 基本用法

```cpp
// ... 表示"零个或多个"参数
template<typename... Args>
void print_all(Args... args) {
    // sizeof...(args) 返回参数数量
    std::cout << "Count: " << sizeof...(args) << "\n";
}

print_all();            // Count: 0
print_all(1);           // Count: 1
print_all(1, 2.5, "hi"); // Count: 3
```

但只知道参数数量没什么用——怎么访问每个参数？可变参数模板通常配合**递归展开**使用：

```cpp
// 递归终止条件：没有参数时打印换行
void print_all() {
    std::cout << "\n";
}

// 递归展开：打印第一个，然后递归处理剩余的
template<typename First, typename... Rest>
void print_all(First first, Rest... rest) {
    std::cout << first;
    if constexpr (sizeof...(rest) > 0) {
        std::cout << " ";
    }
    print_all(rest...);  // 递归，去掉第一个参数
}

print_all(1, 2.5, "hello", true);
// 输出：1 2.5 hello 1
```

每次递归调用，`First` 接收当前第一个参数，`Rest...` 是剩余参数。直到 `rest` 为空，调用无参数版本终止。

### 折叠表达式（C++17）

C++17 引入了折叠表达式（fold expression），避免了递归展开的繁琐：

```cpp
// 求和：对所有参数应用 + 运算符
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);  // 折叠表达式：a + (b + (c + ...))
}

std::cout << sum(1, 2, 3, 4, 5) << "\n";  // 15
std::cout << sum(1.5, 2.5, 3.0) << "\n";  // 7

// 打印所有参数（用逗号运算符展开）
template<typename... Args>
void print_all(Args... args) {
    ((std::cout << args << " "), ...);  // 对每个 arg 执行 cout << arg << " "
    std::cout << "\n";
}

print_all(1, 2.5, "hello");  // 1 2.5 hello
```

折叠表达式的语法：`(pack op ...)` 是右折叠，`(... op pack)` 是左折叠，`(pack op ... op init)` 是带初始值的右折叠。

四种折叠形式：

```cpp
template<typename... Args>
auto right_fold(Args... args) { return (args + ...); }   // a + (b + c)

template<typename... Args>
auto left_fold(Args... args) { return (... + args); }    // (a + b) + c

template<typename... Args>
auto right_fold_init(Args... args) { return (args + ... + 0); }  // a + (b + (c + 0))

template<typename... Args>
auto left_fold_init(Args... args) { return (0 + ... + args); }   // ((0 + a) + b) + c
```

对加法，结果一样；对减法，左折叠和右折叠结果不同：

```cpp
// 对 1, 2, 3
// 左折叠：(1 - 2) - 3 = -4
// 右折叠：1 - (2 - 3) = 2
```

### 可变参数模板的实际应用

可变参数模板最重要的应用是 `std::make_unique` 和 `std::make_shared`：

```cpp
// make_unique 的简化实现
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 使用
auto p = std::make_unique<NumberLiteralNode>(42.0, 1, 5);
// 把 42.0, 1, 5 完美转发给 NumberLiteralNode 的构造函数
```

`std::forward<Args>(args)...` 展开 `args` 参数包，对每个参数调用 `std::forward`，保持原始的左值/右值属性（完美转发，第十八章讲过）。

---

## 28.3 if constexpr：编译期条件

`if constexpr` 是 C++17 引入的，允许在模板里做**编译期**的条件判断。和普通 `if` 不同，`if constexpr` 的分支在编译时决定——没有被选中的分支根本不会编译。

### 为什么需要 if constexpr

看一个普通 `if` 解决不了的问题：

```cpp
template<typename T>
void process(T value) {
    if (std::is_integral_v<T>) {
        // 整数类型的处理
        std::cout << value % 2 << "\n";  // 对 double 这行会编译错误！
    } else {
        // 浮点数类型的处理
        std::cout << value * 1.5 << "\n";
    }
}

process(42);    // T = int，但 value * 1.5 分支仍然被编译（虽然不执行）
process(3.14);  // T = double，但 value % 2 分支仍然被编译，double 不支持 %
```

普通 `if` 的两个分支都会被编译，即使运行时只执行其中一个。对于模板，这可能导致编译错误：`double % 2` 在 C++ 里非法。

用 `if constexpr` 解决：

```cpp
template<typename T>
void process(T value) {
    if constexpr (std::is_integral_v<T>) {
        // 只有 T 是整数时，这个分支才会被编译
        std::cout << value % 2 << "\n";
    } else {
        // 只有 T 不是整数时，这个分支才会被编译
        std::cout << value * 1.5 << "\n";
    }
}

process(42);    // 只编译 % 分支，输出：0
process(3.14);  // 只编译 * 分支，输出：4.71
```

### if constexpr 在可变参数模板里的应用

前面写 `print_all` 时用了递归，现在改用 `if constexpr`：

```cpp
template<typename First, typename... Rest>
void print_all(First first, Rest... rest) {
    std::cout << first;
    if constexpr (sizeof...(rest) > 0) {
        std::cout << " ";
        print_all(rest...);  // 只有 rest 不空时才递归
    } else {
        std::cout << "\n";
    }
}

print_all(1, 2.5, "hello");
// 输出：1 2.5 hello
```

不再需要单独的无参数重载——`if constexpr` 在编译期检查 `rest` 是否为空，决定是否展开递归。

### if constexpr 的常见用法

```cpp
// 根据类型选择不同的序列化方式
template<typename T>
std::string serialize(const T& value) {
    if constexpr (std::is_same_v<T, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_integral_v<T>) {
        return std::to_string(value);
    } else if constexpr (std::is_floating_point_v<T>) {
        return std::to_string(value);
    } else if constexpr (std::is_same_v<T, std::string>) {
        return "\"" + value + "\"";
    } else {
        // 对不支持的类型，在编译时给出清晰的错误信息
        static_assert(sizeof(T) == 0, "Unsupported type for serialize");
        return "";
    }
}

std::cout << serialize(true) << "\n";          // true
std::cout << serialize(42) << "\n";            // 42
std::cout << serialize(3.14) << "\n";          // 3.140000
std::cout << serialize(std::string("hi")) << "\n";  // "hi"
```

`static_assert(sizeof(T) == 0, ...)` 是一个编译时断言——`sizeof(T)` 永远不会是 0，所以这个断言总是失败，会在编译时打印指定的错误信息。这是模板编程里给出清晰编译错误的惯用方式。

---

## 28.4 编译期计算：constexpr

`constexpr` 修饰的函数可以在编译期求值——如果所有参数在编译期已知，编译器直接把结果替换进去，不产生函数调用的开销。

### constexpr 函数

```cpp
constexpr int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// 编译期求值：5! 在编译时就计算好了
constexpr int f5 = factorial(5);  // 编译器计算，结果 120 直接嵌入代码

// 运行期求值：n 在运行时才知道
int n;
std::cin >> n;
int fn = factorial(n);  // 运行时调用
```

`constexpr` 函数可以同时用于编译期和运行期——取决于参数是否在编译期已知。

### constexpr 与模板的结合

```cpp
// 编译期计算斐波那契数列
template<int N>
struct Fib {
    static constexpr int value = Fib<N-1>::value + Fib<N-2>::value;
};

template<>
struct Fib<0> {
    static constexpr int value = 0;
};

template<>
struct Fib<1> {
    static constexpr int value = 1;
};

// Fib<10>::value 在编译时计算，不产生运行时开销
static_assert(Fib<10>::value == 55);
std::cout << Fib<20>::value << "\n";  // 6765，编译时已算好
```

这种技巧叫**模板元编程**（Template Metaprogramming, TMP）——在编译期用模板做计算。在 C++11 之前，这是实现编译期计算的主要方式，非常繁琐。C++11 的 `constexpr` 让编译期计算容易得多。

### constexpr 在 Taco 里的应用

Taco 的哈希函数是一个很好的 `constexpr` 应用场景。关键字查找是词法分析器里的热路径，如果能用编译期生成的完美哈希表，性能会更好：

```cpp
// 编译期字符串哈希（FNV-1a 算法）
constexpr uint32_t hash_str(const char* str, uint32_t hash = 2166136261u) {
    return (*str == '\0')
        ? hash
        : hash_str(str + 1, (hash ^ (uint8_t)*str) * 16777619u);
}

// 编译期常量：每个关键字的哈希值在编译时就算好
constexpr uint32_t HASH_VAR    = hash_str("var");
constexpr uint32_t HASH_FUNC   = hash_str("func");
constexpr uint32_t HASH_IF     = hash_str("if");
constexpr uint32_t HASH_WHILE  = hash_str("while");
constexpr uint32_t HASH_RETURN = hash_str("return");

// 词法分析时，用运行时哈希和编译期常量比较
// 比 std::unordered_map 的查找快，因为常量表是编译期内嵌的
TokenType lookup_keyword(const std::string& word) {
    uint32_t h = hash_str(word.c_str());
    if (h == HASH_VAR    && word == "var")    return TokenType::Var;
    if (h == HASH_FUNC   && word == "func")   return TokenType::Func;
    if (h == HASH_IF     && word == "if")     return TokenType::If;
    if (h == HASH_WHILE  && word == "while")  return TokenType::While;
    if (h == HASH_RETURN && word == "return") return TokenType::Return;
    return TokenType::Identifier;
}
```

哈希碰撞后还是要比较字符串，但大多数情况下一次哈希匹配就能确定。

---

## *More About 模板*：SFINAE 初步与 concepts（C++20）

> 第一次读可以跳过。

### SFINAE

SFINAE 是"Substitution Failure Is Not An Error"（替换失败不是错误）的缩写。当编译器尝试用某个类型实例化模板，但替换失败时，不会报错，而是把这个候选排除，继续寻找其他重载。

```cpp
// enable_if：T 是整数类型时，函数才存在
template<typename T>
typename std::enable_if<std::is_integral_v<T>, T>::type
double_it(T value) {
    return value * 2;
}

double_it(42);    // T = int，整数，OK
// double_it(3.14);  // T = double，不是整数，这个重载被排除，没有其他候选，编译错误
```

`std::enable_if<Condition, T>::type`：当 `Condition` 为 `true` 时，`type` 是 `T`；当 `Condition` 为 `false` 时，`type` 不存在，替换失败，这个函数被排除。

SFINAE 写法非常晦涩，C++20 的 `concepts` 提供了更清晰的替代方案。

### Concepts（C++20）

`concepts` 是 C++20 引入的特性，允许在模板参数上附加约束，让模板只对满足条件的类型工作：

```cpp
#include <concepts>

// 定义一个 concept：T 必须支持 + 和 == 运算，以及可以流输出
template<typename T>
concept Printable = requires(T t) {
    { std::cout << t } -> std::same_as<std::ostream&>;
};

template<typename T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
};

// 用 concept 约束模板参数
template<Addable T>
T sum(T a, T b) {
    return a + b;
}

template<Printable T>
void print(const T& value) {
    std::cout << value << "\n";
}

sum(1, 2);           // T = int，满足 Addable，OK
sum(1.5, 2.5);       // T = double，满足 Addable，OK
// sum("a", "b");    // const char* 不满足 Addable，清晰的编译错误

print(42);           // int 满足 Printable，OK
print("hello");      // const char* 满足 Printable，OK
```

违反 concept 时，编译器会给出清晰的错误信息，说明哪个约束没有满足——比 SFINAE 的报错好懂得多。

标准库也提供了很多内置的 concept：`std::integral<T>`、`std::floating_point<T>`、`std::copyable<T>`、`std::sortable<Range>` 等。

本书主要覆盖 C++17，concepts 是 C++20 的特性，这里只做了解。在 v5 项目里不会用到，但知道它的存在有助于理解现代 C++ 的方向。

---

## 小结

**全特化**：为特定类型提供完全不同的模板实现，语法是 `template<>` 加具体类型。函数模板和类模板都可以全特化。

**偏特化**：只针对部分参数做特化，剩余参数仍然通用。只有类模板可以偏特化（函数模板不行，用重载代替）。类型特征（`is_integral`、`is_same` 等）是偏特化的典型应用。

**可变参数模板**：`template<typename... Args>` 接受任意数量的类型参数。C++17 的折叠表达式 `(args op ...)` 简化了参数展开，避免了递归。

**if constexpr**：编译期条件分支，没有被选中的分支不会编译。解决了模板里"两个分支只有一个能编译"的问题。常与 `std::is_xxx_v` 类型特征配合使用。

**constexpr**：修饰函数或变量，允许编译期求值。编译期的值和运算可以内嵌到代码里，减少运行时开销。`static_assert` 用于编译期检查，在模板里提供清晰的错误信息。

---

下一章是第六部分的项目章节：用学到的模板知识对 Taco 解释器做内部重构（v5）。
# 第二十九章：项目 v5——解释器内部模板化

---

第六部分学了三章模板：函数模板、类模板、特化与 `if constexpr`。现在用这些知识对 Taco 解释器做一次内部重构——v5 不增加新的语言功能，而是改进内部实现。

v5 做三件事：

1. **用 `std::variant` 重新设计 TacoValue**：v1 里已经用过 `variant`，但随着值类型增多，需要系统地重新设计
2. **用模板重构环境（Environment）**：让符号表可以存储不同类型的映射
3. **用模板实现通用的 AST 节点访问器（Visitor）**：用模板取代 `dynamic_cast` 链，实现干净的 Visitor 模式

重构之后，代码更干净，扩展更容易，性能略有提升。

---

## 29.1 用 std::variant 实现 Taco 值类型

### v4 的 TacoValue 回顾

v4 的 `TacoValue` 是这样的：

```cpp
// v4 的实现——已经可以工作，但有几个问题
struct TacoArray;
struct TacoMap;
struct TacoFunction;
struct TacoNative;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>
>;
```

这个设计工作得很好，但有几个摩擦点：
- 辅助函数（`is_truthy`、`value_to_string`、运算符）全部堆在 `value.cpp` 里，越来越难维护
- `std::get_if<T>` 的使用散落在各处，同样的模式重复很多次
- 需要一个统一的"访问"（visit）接口，对任意类型的值都能做某种操作

`std::visit` 是 `variant` 的标准配套工具，配合模板可以写出非常整洁的值处理代码。

### std::visit 的用法

`std::visit` 接收一个**访问者**（visitor）和一个 `variant`，用 `variant` 当前持有的实际类型调用访问者：

```cpp
#include <variant>

using Value = std::variant<int, double, std::string>;

Value v = 42;

// 访问者：一个有 operator() 的对象
struct Printer {
    void operator()(int n)                { std::cout << "int: " << n << "\n"; }
    void operator()(double d)             { std::cout << "double: " << d << "\n"; }
    void operator()(const std::string& s) { std::cout << "string: " << s << "\n"; }
};

std::visit(Printer{}, v);  // 输出：int: 42

v = 3.14;
std::visit(Printer{}, v);  // 输出：double: 3.14

v = std::string("hello");
std::visit(Printer{}, v);  // 输出：string: hello
```

访问者可以用一个重载了多个 `operator()` 的结构体，但写起来有点繁琐。C++17 里有一个更简洁的技巧：**overloaded**：

```cpp
// 把多个 lambda 合并成一个重载集合
template<typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;  // 把所有基类的 operator() 引入当前作用域
};

// C++17 的推导指南：让编译器推导模板参数
template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// 使用
Value v = 42;
std::visit(overloaded{
    [](int n)                { std::cout << "int: " << n << "\n"; },
    [](double d)             { std::cout << "double: " << d << "\n"; },
    [](const std::string& s) { std::cout << "string: " << s << "\n"; },
}, v);
```

`overloaded` 继承自所有传入的 lambda 类型，把它们的 `operator()` 全都引入，形成一个重载集。这是模板继承的实际应用。

### 重新设计 value.h

把 `overloaded` 工具和 TacoValue 的辅助函数整合进来：

```cpp
// value.h（v5 版本）
#pragma once
#include <variant>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <stdexcept>

// ── 前向声明 ──────────────────────────────────────────────────
struct TacoArray;
struct TacoMap;
struct TacoFunction;
struct TacoNative;
class  Environment;

// ── TacoValue：所有 Taco 值类型的 variant ─────────────────────
using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>
>;

// ── overloaded 工具 ───────────────────────────────────────────
// 把多个 lambda 合并成一个可以重载的访问者
template<typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// ── 辅助函数 ─────────────────────────────────────────────────
std::string  taco_to_string(const TacoValue& v);
bool         taco_is_truthy(const TacoValue& v);
bool         taco_equal(const TacoValue& a, const TacoValue& b);
TacoValue    taco_add(const TacoValue& a, const TacoValue& b);  // + 运算，支持数字和字符串拼接

// 类型检查辅助
template<typename T>
bool taco_is(const TacoValue& v) {
    return std::holds_alternative<T>(v);
}

// 安全提取（提取失败抛运行时错误）
template<typename T>
const T& taco_get(const TacoValue& v, const std::string& context = "") {
    if (auto* p = std::get_if<T>(&v)) {
        return *p;
    }
    throw std::runtime_error(
        context.empty()
            ? "Type error"
            : "Type error in " + context
    );
}

template<typename T>
T& taco_get_mut(TacoValue& v, const std::string& context = "") {
    if (auto* p = std::get_if<T>(&v)) {
        return *p;
    }
    throw std::runtime_error(
        context.empty()
            ? "Type error"
            : "Type error in " + context
    );
}
```

模板函数 `taco_is<T>` 和 `taco_get<T>` 比直接写 `holds_alternative` 和 `get_if` 清晰得多，而且集中了错误信息的格式。

### 辅助函数的实现

```cpp
// value.cpp（v5 版本）
#include "value.h"
#include "taco_array.h"
#include "taco_map.h"
#include "taco_function.h"

std::string taco_to_string(const TacoValue& v) {
    return std::visit(overloaded{
        [](double d) -> std::string {
            // 整数时不显示小数点
            if (d == static_cast<long long>(d) && !std::isinf(d))
                return std::to_string(static_cast<long long>(d));
            return std::to_string(d);
        },
        [](const std::string& s) -> std::string {
            return s;
        },
        [](bool b) -> std::string {
            return b ? "true" : "false";
        },
        [](std::nullptr_t) -> std::string {
            return "nil";
        },
        [](const std::shared_ptr<TacoArray>& arr) -> std::string {
            return arr->to_string();
        },
        [](const std::shared_ptr<TacoMap>& map) -> std::string {
            return map->to_string();
        },
        [](const std::shared_ptr<TacoFunction>& fn) -> std::string {
            return "<func " + fn->name + ">";
        },
        [](const std::shared_ptr<TacoNative>&) -> std::string {
            return "<native>";
        },
    }, v);
}

bool taco_is_truthy(const TacoValue& v) {
    return std::visit(overloaded{
        [](double d)                              { return d != 0.0; },
        [](const std::string& s)                 { return !s.empty(); },
        [](bool b)                               { return b; },
        [](std::nullptr_t)                       { return false; },
        [](const std::shared_ptr<TacoArray>& a)  { return true; },
        [](const std::shared_ptr<TacoMap>& m)    { return true; },
        [](const std::shared_ptr<TacoFunction>&) { return true; },
        [](const std::shared_ptr<TacoNative>&)   { return true; },
    }, v);
}
```

这种写法的好处是：**所有类型必须有对应的处理分支**。如果将来 `TacoValue` 加了新的类型，但 `overloaded` 里没有新分支，编译器会直接报错——强制你不能忘记处理新类型。这是 `variant` + `visit` 相比用继承 + 虚函数的一个优势：你不能漏掉某个类型。

---

## 29.2 用模板重构符号表与环境

### v4 的 Environment

v4 的环境是一个 `unordered_map<string, TacoValue>` 加上父环境的 `shared_ptr`：

```cpp
class Environment {
public:
    void define(const std::string& name, TacoValue value);
    TacoValue get(const std::string& name);
    void assign(const std::string& name, TacoValue value);

    std::shared_ptr<Environment> parent;

private:
    std::unordered_map<std::string, TacoValue> vars_;
};
```

这个设计是够用的，但有一个问题：`get` 找不到变量时怎么办？要么抛异常，要么返回 nil——两种方案都有缺点（抛异常开销大，返回 nil 会掩盖错误）。

v5 用模板改进 `get`，返回 `std::optional<TacoValue>`：

```cpp
// environment.h（v5 版本）
#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include "value.h"

class Environment {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr)
        : parent_(std::move(parent)) {}

    // 在当前作用域定义新变量
    void define(const std::string& name, TacoValue value) {
        vars_[name] = std::move(value);
    }

    // 查找变量：当前作用域找不到就往上找，找到返回值，找不到返回空
    std::optional<TacoValue> lookup(const std::string& name) const {
        auto it = vars_.find(name);
        if (it != vars_.end()) {
            return it->second;
        }
        if (parent_) {
            return parent_->lookup(name);
        }
        return std::nullopt;
    }

    // 获取变量（找不到抛错误）
    TacoValue get(const std::string& name) const {
        auto result = lookup(name);
        if (!result) {
            throw std::runtime_error("Undefined variable: " + name);
        }
        return *result;
    }

    // 赋值：找到变量的作用域，在那里修改
    bool assign(const std::string& name, TacoValue value) {
        auto it = vars_.find(name);
        if (it != vars_.end()) {
            it->second = std::move(value);
            return true;
        }
        if (parent_) {
            return parent_->assign(name, std::move(value));
        }
        return false;  // 变量不存在
    }

    // 创建子作用域
    std::shared_ptr<Environment> make_child() {
        return std::make_shared<Environment>(shared_from_this());
    }

    std::shared_ptr<Environment> parent() const { return parent_; }

private:
    std::unordered_map<std::string, TacoValue> vars_;
    std::shared_ptr<Environment> parent_;
};
```

`make_child()` 的实现用到了 `shared_from_this()`，这要求 `Environment` 继承自 `std::enable_shared_from_this<Environment>`：

```cpp
class Environment : public std::enable_shared_from_this<Environment> {
    // ... 同上
};
```

`enable_shared_from_this` 让对象可以安全地获取指向自己的 `shared_ptr`，不会创建第二套引用计数。这是在对象内部需要 `shared_ptr<this>` 时的标准做法。

### 模板化的注册接口

内置函数注册也可以用模板简化。v4 里每次注册都要手动包装 lambda：

```cpp
// v4 的手动注册方式
env->define("print", TacoValue{std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args) -> TacoValue {
        // ...
        return nullptr;
    }
)});
```

用模板写一个辅助函数，让注册更简洁：

```cpp
// 注册内置函数的辅助模板
template<typename Fn>
void register_builtin(Environment& env, const std::string& name, Fn fn) {
    env.define(name, TacoValue{
        std::make_shared<TacoNative>(std::function<TacoValue(std::vector<TacoValue>)>(fn))
    });
}

// 使用
void register_builtins(Environment& env) {
    register_builtin(env, "print", [](std::vector<TacoValue> args) -> TacoValue {
        for (auto& a : args) std::cout << taco_to_string(a);
        std::cout << "\n";
        return nullptr;
    });

    register_builtin(env, "type", [](std::vector<TacoValue> args) -> TacoValue {
        if (args.empty()) return std::string("nil");
        return std::visit(overloaded{
            [](double)                            -> TacoValue { return std::string("number"); },
            [](const std::string&)                -> TacoValue { return std::string("string"); },
            [](bool)                              -> TacoValue { return std::string("bool"); },
            [](std::nullptr_t)                    -> TacoValue { return std::string("nil"); },
            [](const std::shared_ptr<TacoArray>&) -> TacoValue { return std::string("array"); },
            [](const std::shared_ptr<TacoMap>&)   -> TacoValue { return std::string("map"); },
            [](const std::shared_ptr<TacoFunction>&) -> TacoValue { return std::string("func"); },
            [](const std::shared_ptr<TacoNative>&)   -> TacoValue { return std::string("func"); },
        }, args[0]);
    });

    register_builtin(env, "len", [](std::vector<TacoValue> args) -> TacoValue {
        if (args.empty()) throw std::runtime_error("len() requires an argument");
        return std::visit(overloaded{
            [](const std::string& s)              -> TacoValue { return (double)s.size(); },
            [](const std::shared_ptr<TacoArray>& a) -> TacoValue { return (double)a->elements.size(); },
            [](const std::shared_ptr<TacoMap>& m)   -> TacoValue { return (double)m->entries.size(); },
            [](const auto&) -> TacoValue {
                throw std::runtime_error("len() only works on string, array, and map");
                return nullptr;
            },
        }, args[0]);
    });
}
```

`register_builtin` 用 `Fn` 接收任意可调用类型（lambda、函数指针等），内部转换成 `std::function`。这比每次都手写 `make_shared<TacoNative>` 简洁很多。

---

## 29.3 用模板实现通用的 AST 节点访问器

### 访问者模式（Visitor Pattern）

v2 里用了一个简化版：让每个 AST 节点有 `evaluate()` 虚函数。这个设计有个问题：如果需要对同一棵 AST 做多种操作（求值、打印、类型检查、代码生成……），就要在每个节点类里加很多虚函数，节点类会越来越臃肿。

标准的访问者模式把操作从节点里分离出来：

- **节点类**只负责存储数据和调用 `accept(visitor)`
- **访问者类**实现对每种节点的具体操作

```
AST 节点：NumberLiteral, BinaryExpr, IfStmt, ...
  每个节点：accept(Visitor&)
  
访问者：Evaluator, Printer, TypeChecker, ...
  每个访问者：对每种节点实现 visit() 方法
```

这样，新增操作只需要写一个新的访问者类，不需要修改任何节点类。

### 用模板实现访问者基类

在不用模板的情况下，访问者模式需要手动列出所有节点类型：

```cpp
// 传统方式：为每种节点类型写一个 visit 函数
struct Visitor {
    virtual TacoValue visit(const NumberLiteralNode&) = 0;
    virtual TacoValue visit(const StringLiteralNode&) = 0;
    virtual TacoValue visit(const BinaryExprNode&) = 0;
    virtual TacoValue visit(const IfStmtNode&) = 0;
    // ... 每增加一种节点，这里就要加一行
};
```

用模板可以稍微改进——用 `if constexpr` 配合类型特征，让节点的 `accept` 函数更通用：

```cpp
// ast.h（v5 的新方式）
#pragma once
#include <memory>
#include <string>
#include "value.h"

// 前向声明访问者
class Evaluator;
class ASTPrinter;

// ── 基类 ──────────────────────────────────────────────────────
struct ASTNode {
    int line = 0;    // 出错时的行号
    virtual ~ASTNode() = default;

    // 求值：由子类实现
    virtual TacoValue evaluate(Evaluator& eval) const = 0;

    // 打印 AST（调试用）
    virtual std::string dump(int indent = 0) const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

// ── 表达式基类 ────────────────────────────────────────────────
struct Expr : ASTNode {};
using ExprPtr = std::unique_ptr<Expr>;

// ── 语句基类 ─────────────────────────────────────────────────
struct Stmt : ASTNode {};
using StmtPtr = std::unique_ptr<Stmt>;

// ── 字面量节点 ────────────────────────────────────────────────

struct NumberLiteralNode : Expr {
    double value;
    explicit NumberLiteralNode(double v, int ln = 0) : value(v) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Number(" + std::to_string(value) + ")";
    }
};

struct StringLiteralNode : Expr {
    std::string value;
    explicit StringLiteralNode(std::string v, int ln = 0) : value(std::move(v)) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "String(\"" + value + "\")";
    }
};

struct BoolLiteralNode : Expr {
    bool value;
    explicit BoolLiteralNode(bool v, int ln = 0) : value(v) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + std::string(value ? "True" : "False");
    }
};

struct NilLiteralNode : Expr {
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Nil";
    }
};

// ── 变量节点 ─────────────────────────────────────────────────

struct VariableNode : Expr {
    std::string name;
    explicit VariableNode(std::string n, int ln = 0) : name(std::move(n)) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Var(" + name + ")";
    }
};

// ── 二元表达式 ────────────────────────────────────────────────

struct BinaryExprNode : Expr {
    ExprPtr left;
    std::string op;
    ExprPtr right;

    BinaryExprNode(ExprPtr l, std::string o, ExprPtr r, int ln = 0)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) { line = ln; }

    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Binary(" + op + ")\n"
             + left->dump(indent + 2) + "\n"
             + right->dump(indent + 2);
    }
};
```

### 用 if constexpr 实现通用的 dump

为了演示 `if constexpr` 的实际应用，来写一个通用的缩进打印函数：

```cpp
// 通用的 AST 打印辅助：根据节点类型选择不同的打印方式
template<typename Node>
std::string dump_node(const Node& node, int indent) {
    std::string prefix(indent, ' ');

    if constexpr (std::is_same_v<Node, NumberLiteralNode>) {
        return prefix + "Number(" + std::to_string(node.value) + ")";
    } else if constexpr (std::is_same_v<Node, StringLiteralNode>) {
        return prefix + "String(\"" + node.value + "\")";
    } else if constexpr (std::is_same_v<Node, BinaryExprNode>) {
        return prefix + "Binary(" + node.op + ")\n"
             + node.left->dump(indent + 2) + "\n"
             + node.right->dump(indent + 2);
    } else {
        // 兜底：只打印类型名
        return prefix + "[Unknown Node]";
    }
}
```

这里 `if constexpr` 根据 `Node` 的类型在编译期决定打印格式——对 `NumberLiteralNode` 访问 `.value`，对 `BinaryExprNode` 访问 `.op`、`.left`、`.right`。如果用普通 `if`，所有分支都会被编译，但 `NumberLiteralNode` 没有 `.op` 成员，会编译失败。

---

## 29.4 模板在这里解决了什么问题

回顾一下 v5 里模板的三个应用，以及它们各自解决了什么问题：

**`overloaded` + `std::visit`**

解决了 `variant` 处理代码重复的问题。每次处理 `TacoValue` 都要 `if (holds_alternative<double>)...` 一长串，`visit` + `overloaded` 把所有类型的处理集中在一个地方，而且如果漏掉某种类型，编译器会报错。

**`taco_is<T>` 和 `taco_get<T>` 模板函数**

把散落在求值器各处的 `get_if`/`holds_alternative` 调用统一成一个接口，错误信息也更清晰。`context` 参数让错误定位更精准。

**`register_builtin<Fn>` 模板函数**

把每次注册内置函数都要写的 `make_shared<TacoNative>(function<...>(lambda))` 封装起来，调用端只需要写内置函数的名字和实现 lambda。新增内置函数从十行代码变成三行。

**`if constexpr` 在 `dump_node` 里**

让一个函数可以根据节点类型采用不同的打印逻辑，而且每个分支只访问该类型实际存在的成员，不需要基类预先声明所有可能用到的成员。

---

## 29.5 性能对比：重构前后的差异

v5 的重构对性能的影响主要体现在两个地方：

### 1. `std::visit` vs `dynamic_cast` 链

v1 里用过 `dynamic_cast` 来识别节点类型：

```cpp
// v1 的方式：O(n)，每次要试每种类型直到匹配
TacoValue evaluate(const Expr* e) {
    if (auto* n = dynamic_cast<const NumberLiteralNode*>(e)) return eval_number(n);
    if (auto* n = dynamic_cast<const BinaryExprNode*>(e))   return eval_binary(n);
    // ...
}
```

v2 改成了虚函数（查 vtable 一次，`O(1)`）。`std::variant` + `std::visit` 也是 `O(1)` 的——`variant` 内部存了一个标记（通常是整数），`visit` 根据这个标记做一次跳转（类似 switch，但更安全）。

两者性能相近，但 `variant` 的好处是**不需要基类**：`TacoValue` 里的 `double`、`std::string` 不是类，不需要继承任何东西，内存布局也更紧凑。

### 2. 内存布局

用继承实现的多态值类型，每个值都需要：
- 堆上一个对象（`new`）
- 一个 vtable 指针（通常 8 字节）
- 引用计数（如果用 `shared_ptr`）

`std::variant` 的内存布局：
- 一块栈上的内存，大小等于最大成员类型的大小
- 一个标记整数（通常 1-4 字节）
- 没有堆分配，没有 vtable，没有引用计数

对于像 `double` 和 `std::string` 这样的基本类型，`variant` 直接存在栈上，比 `shared_ptr` 包装的堆对象快很多（省去了分配和引用计数的开销）。

用 `<chrono>` 做一个简单的对比测试：

```cpp
#include <chrono>
#include <iostream>
#include <variant>
#include <string>

using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;

// 求和：用 variant 存数字
double sum_variant(int n) {
    TacoValue acc = 0.0;
    for (int i = 0; i < n; ++i) {
        std::get<double>(acc) += i;
    }
    return std::get<double>(acc);
}

int main() {
    const int N = 10'000'000;

    auto t1 = std::chrono::high_resolution_clock::now();
    double result = sum_variant(N);
    auto t2 = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "Sum: " << result << "\n";
    std::cout << "Time: " << ms << " ms\n";

    return 0;
}
```

在 Release 模式（`-O2`）下，`variant` 里的 `double` 几乎和裸 `double` 一样快——编译器能优化掉大部分 `variant` 的开销。

### 真正的性能瓶颈在哪里

测量之后会发现，Taco v5 的性能瓶颈不在 `variant`，而在：

1. **字符串操作**：每次字符串拼接都创建新的 `std::string`，对于 `"Hello, " + name + "!"` 这类插值，中间有多次临时字符串
2. **`unordered_map` 的变量查找**：每次变量读写都要做哈希和比较
3. **AST 节点的虚函数调用**：虚函数调用有 vtable 查找和间接跳转的开销

这些才是真正值得优化的地方。字节码虚拟机（把 AST 编译成指令数组，然后解释执行）可以消除虚函数调用的开销，但那是更高级的话题，留给第三十七章讨论。

---

## 测试 v5

用一段稍复杂的 Taco 脚本测试 v5 的核心功能——重点验证类型处理和错误信息：

```taco
// test_v5.taco

// 基本类型和 type()
var n = 42;
var s = "hello";
var b = true;
var arr = [1, 2, 3];
var m = {"key": "value"};

print(type(n));    // number
print(type(s));    // string
print(type(b));    // bool
print(type(arr));  // array
print(type(m));    // map

// 类型安全的运算
func safe_add(a, b) {
    if (type(a) != type(b)) {
        return "type mismatch";
    }
    return a + b;
}

print(safe_add(1, 2));       // 3
print(safe_add("a", "b"));   // ab
print(safe_add(1, "b"));     // type mismatch

// len() 对不同类型的行为
print(len("hello"));         // 5
print(len([1, 2, 3, 4]));    // 4
print(len({"a": 1, "b": 2})); // 2

// variant 值的完整生命周期：创建、传递、返回
func make_pair(k, v) {
    return {"key": k, "value": v};
}

var pair = make_pair("name", "Miguel");
print(pair["key"]);    // name
print(pair["value"]);  // Miguel

// 闭包 + 类型
func counter(start) {
    var count = start;
    return {
        inc:  func() { count = count + 1; return count; },
        dec:  func() { count = count - 1; return count; },
        get:  func() { return count; },
    };
}

var c = counter(10);
print(c["inc"]());  // 11
print(c["inc"]());  // 12
print(c["dec"]());  // 11
print(c["get"]());  // 11
```

### 预期输出

```
number
string
bool
array
map
3
ab
type mismatch
5
4
2
name
Miguel
11
12
11
11
```

---

## 小结

v5 是一次内部重构，没有新的语言特性，但代码质量有明显提升。

**`overloaded` + `std::visit`** 是 `std::variant` 的标准配套用法。`overloaded` 本身只有几行代码（模板继承 + using），但它让 `visit` 可以直接接收 lambda 列表，不需要手写结构体。所有类型必须有对应的处理分支，漏掉会在编译时报错——比虚函数更安全。

**`taco_is<T>` 和 `taco_get<T>`** 把类型检查和提取操作封装成模板函数，错误信息集中，调用端干净。

**`register_builtin<Fn>`** 用模板接收任意可调用类型，让注册内置函数的代码从繁琐的样板代码变成一行。

**`if constexpr`** 在需要对不同类型做不同操作、但又不想为每种类型写单独函数的场合非常有用。它是模板里的"条件编译"，没有被选中的分支根本不会编译。

**性能上**，`variant` 里的基本类型（`double`、`bool`）直接存在栈上，没有堆分配，在 Release 模式下编译器能充分优化。真正的性能瓶颈在字符串操作和变量查找，不在类型分发本身。

---

第六部分到这里结束。第七部分进入多线程，学完之后 v6 会用线程实现 REPL，让 Taco 有一个交互式的游乐场。
