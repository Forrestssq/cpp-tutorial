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
