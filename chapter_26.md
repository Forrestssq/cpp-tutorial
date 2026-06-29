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
