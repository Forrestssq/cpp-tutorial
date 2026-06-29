# 第三章：基础语法升级

---

C++ 继承了 C 的大部分语法，但在很多地方做了改进和扩展。这一章讲的不是全新的概念，而是那些让 C++ 写起来比 C 更舒服、更安全的语法特性。

有 C 基础的读者会发现，这些特性大多数在 Python 里也有对应的概念，只是形式不同。

---

## 3.1 auto 与类型推导

C++ 是静态类型语言——每个变量都有固定的类型，在编译时确定。但这不意味着每次都要手动写出类型名。

`auto` 关键字让编译器根据右边的表达式自动推导变量的类型：

```cpp
auto x = 42;          // int
auto y = 3.14;        // double
auto s = "hello";     // const char*
auto v = std::vector<int>{1, 2, 3};  // std::vector<int>
```

类型依然存在，只是不需要手动写出来。

---

### auto 真正有用的地方

对于简单类型，`auto` 意义不大——写 `int x = 42` 也没多少负担。`auto` 真正的价值在于类型名很长的时候：

```cpp
// 没有 auto：痛苦
std::map<std::string, std::vector<int>>::iterator it = myMap.begin();

// 有 auto：清晰
auto it = myMap.begin();
```

或者类型很难写出来的时候——比如 lambda 表达式的类型：

```cpp
// lambda 的类型是编译器内部生成的，没有办法手动写出来
auto is_even = [](int n) { return n % 2 == 0; };
```

在遍历容器时，`auto` 也很常用：

```cpp
std::vector<std::string> names = {"Miguel", "Dante", "Imelda"};

for (auto& name : names) {
    std::cout << name << "\n";
}
```

---

### auto 不是 Python 的动态类型

需要澄清一个常见的误解：`auto` 不是让 C++ 变成动态类型语言。

```cpp
auto x = 42;   // x 的类型是 int，编译时确定
x = "hello";   // 错误！不能把字符串赋给 int 类型的变量
```

`auto` 只是让编译器帮你写出类型名，类型本身依然是固定的、静态的。这和 Python 的 `x = 42; x = "hello"` 完全不同——Python 里 `x` 可以在运行时变成任意类型，C++ 里不行。

---

### auto 与引用、const

`auto` 推导类型时，默认不保留引用和 `const`：

```cpp
const int x = 42;
auto y = x;   // y 的类型是 int，不是 const int

int a = 10;
int& ref = a;
auto b = ref;   // b 的类型是 int，不是 int&，b 是 a 的拷贝
```

如果想保留引用或 `const`，需要显式写出来：

```cpp
auto& b = ref;         // b 是引用
const auto& c = x;    // c 是 const 引用
```

在遍历容器时，这个区别很重要：

```cpp
std::vector<std::string> names = {"Miguel", "Dante"};

for (auto name : names) {     // 每次拷贝一个字符串，有开销
    std::cout << name << "\n";
}

for (auto& name : names) {    // 引用，不拷贝，高效
    std::cout << name << "\n";
}

for (const auto& name : names) {  // 常量引用，不拷贝，不能修改
    std::cout << name << "\n";
}
```

遍历容器时，如果不需要修改元素，推荐用 `const auto&`——既不拷贝，又表达了"不会修改"的意图。

---

## 3.2 引用：比指针更安全的别名

引用（reference）是 C++ 对 C 的一个重要扩展，C 里没有引用这个概念。

引用是一个已有变量的**别名**——对引用的操作，就是对原变量的操作：

```cpp
int a = 10;
int& ref = a;   // ref 是 a 的别名

ref = 20;       // 修改 ref，就是修改 a
std::cout << a; // 输出 20

std::cout << &a;    // a 的地址
std::cout << &ref;  // 和 a 的地址相同！
```

---

### 引用和指针的对比

引用和指针有很多相似之处，但有几个关键区别：

```cpp
int a = 10;
int b = 20;

// 指针
int* p = &a;
p = &b;       // 指针可以重新指向另一个变量
*p = 30;      // 需要解引用才能访问值

// 引用
int& ref = a;
// ref = b;   // 这不是让 ref 指向 b，而是把 b 的值赋给 a！
ref = 30;     // 直接操作，不需要解引用
```

引用的几个特性：

- **必须初始化**：`int& ref;` 是错误的，引用必须在声明时绑定到某个变量。
- **不能重新绑定**：引用一旦绑定，就永远指向那个变量，不能再指向别的。
- **没有空引用**：不能有 `int& ref = nullptr;` 这样的东西（而指针可以是 `nullptr`）。
- **不需要解引用**：操作引用就像操作原变量，没有 `*` 运算符。

---

### 引用的主要用途：函数参数

引用最常见的用途是函数参数，用来避免不必要的拷贝：

```cpp
// 传值：会拷贝整个字符串，有开销
void print_value(std::string s) {
    std::cout << s << "\n";
}

// 传常量引用：不拷贝，也不能修改
void print_ref(const std::string& s) {
    std::cout << s << "\n";
}

// 传引用：不拷贝，可以修改原变量
void to_upper(std::string& s) {
    for (char& c : s) {
        c = toupper(c);
    }
}

std::string name = "miguel";
to_upper(name);
std::cout << name;  // MIGUEL
```

一个简单的原则：
- 如果函数需要修改参数，传引用（`T&`）。
- 如果函数不修改参数，传常量引用（`const T&`）。
- 对于 `int`、`double`、`char` 这类小类型，传值即可，拷贝开销可以忽略。

---

### 引用与 Python 的对比

Python 里的变量传递本质上都是"传引用"（更准确地说是"传对象引用"），所以在 Python 里修改一个列表会影响原列表：

```python
def append_item(lst):
    lst.append(99)

my_list = [1, 2, 3]
append_item(my_list)
print(my_list)  # [1, 2, 3, 99]，原列表被修改了
```

C++ 里需要显式选择传值还是传引用。传值是拷贝，传引用才会影响原变量。这给了程序员更精确的控制，代价是需要多想一步。

---

## 3.3 const 的正确用法

`const` 在 C 里已经存在，但在 C++ 里用得更多、更细致。

---

### const 变量

```cpp
const int MAX_SIZE = 100;
MAX_SIZE = 200;  // 错误，不能修改 const 变量
```

`const` 变量必须在声明时初始化，之后不能修改。这和 C 里的 `const` 一样。

---

### 指针与 const 的组合

C 里指针和 `const` 的组合很容易让人困惑，C++ 里也一样。有两种不同的含义：

```cpp
int a = 10;
int b = 20;

// const int*：指向常量的指针
// 指针可以改变，但不能通过这个指针修改值
const int* p1 = &a;
p1 = &b;      // 可以，改变了指针指向
*p1 = 30;     // 错误，不能通过 p1 修改值

// int* const：常量指针
// 指针不能改变，但可以通过这个指针修改值
int* const p2 = &a;
p2 = &b;      // 错误，不能改变指针指向
*p2 = 30;     // 可以，修改 a 的值

// const int* const：指向常量的常量指针
// 指针不能改变，也不能通过它修改值
const int* const p3 = &a;
```

一个记忆方法：`const` 修饰的是它左边的东西。`const int*` 里，`const` 在 `int` 左边，修饰 `int`，所以值不能改。`int* const` 里，`const` 在 `*` 左边，修饰指针本身，所以指针不能改。

---

### const 成员函数

在类里，`const` 可以用来修饰成员函数，表示这个函数不会修改对象的状态，用法就是在写完函数声明后，在函数的 `{}` 前面加一个 `const` ：

```cpp
class Token {
public:
    std::string value;

    // const 成员函数：承诺不修改 *this
    std::string getValue() const {
        return value;
    }

    // 非 const 成员函数：可以修改 *this
    void setValue(const std::string& v) {
        value = v;
    }
};
```

`const` 对象只能调用 `const` 成员函数：

```cpp
const Token t{"hello"};
t.getValue();   // 可以，getValue 是 const 函数
t.setValue("world");  // 错误，setValue 不是 const 函数
```

这是一个很重要的设计工具：通过 `const` 函数，可以明确告诉使用者"调用这个函数不会产生副作用"。

---

### const 的最佳实践

一个原则：**默认 const，有需要再去掉**。

```cpp
// 不好的习惯
std::string name = "Miguel";  // name 不会被修改，为什么不加 const？

// 好的习惯
const std::string name = "Miguel";
```

```cpp
// 函数参数默认用 const&
void process(const std::string& input) {
    // ...
}
```

主动使用 `const` 有几个好处：
- 告诉读者这个变量不会被修改，减少理解代码的负担。
- 编译器会在意外修改时报错，减少 bug。
- 允许编译器做更多优化。

---

## 3.4 函数重载与默认参数

### 函数重载

C++ 允许同名函数有多个版本，只要参数列表不同：

```cpp
void print(int n) {
    std::cout << "int: " << n << "\n";
}

void print(double d) {
    std::cout << "double: " << d << "\n";
}

void print(const std::string& s) {
    std::cout << "string: " << s << "\n";
}

print(42);        // 调用 print(int)
print(3.14);      // 调用 print(double)
print("hello");   // 调用 print(const std::string&)
```

编译器根据调用时传入的参数类型，选择最匹配的版本。这个过程叫**重载解析**（overload resolution）。

C 里没有函数重载，不同类型的版本必须有不同的名字（如 `printf` 的 `%d`、`%f`、`%s` 格式符就是在运行时区分的，而不是编译时）。

---

### 重载解析的规则

重载解析的规则很复杂，但大多数情况下符合直觉：编译器优先选择参数类型完全匹配的版本，其次是需要隐式转换的版本：

```cpp
void foo(int n) { std::cout << "int\n"; }
void foo(double d) { std::cout << "double\n"; }

foo(42);     // int → 完全匹配 foo(int)
foo(3.14);   // double → 完全匹配 foo(double)
foo(42L);    // long → 需要转换，选 foo(int) 或 foo(double)？
             // 这里两个都需要转换，编译器会报错：ambiguous
```

遇到歧义时，编译器会报错，而不是悄悄选一个——这是好事。

---

### 默认参数

函数参数可以有默认值，调用时可以省略：

```cpp
void greet(const std::string& name, const std::string& greeting = "Hola") {
    std::cout << greeting << ", " << name << "!\n";
}

greet("Miguel");              // Hola, Miguel!
greet("Dante", "Hey");        // Hey, Dante!
```

默认参数必须从右边开始：

```cpp
// 正确：有默认值的参数在右边
void foo(int a, int b = 10, int c = 20);

// 错误：中间的参数有默认值，但右边的没有
void bar(int a, int b = 10, int c);
```

---

### 和 Python 的对比

Python 里的默认参数：

```python
def greet(name, greeting="Hola"):
    print(f"{greeting}, {name}!")
```

和 C++ 几乎一样，只是 C++ 需要指定类型。

Python 还支持关键字参数（keyword argument），C++ 没有原生支持：

```python
greet(greeting="Hey", name="Dante")  # Python 可以这样
```

C++ 里参数必须按顺序传，或者用默认值省略。

---

## 3.5 namespace 与作用域

### 为什么需要命名空间

大型项目里，不同的代码模块可能定义了同名的函数或类。比如你写的 `sort` 函数和标准库的 `std::sort`，或者两个库都有一个叫 `Error` 的类。

命名空间（namespace）解决这个问题，把名字隔离在不同的空间里：

```cpp
namespace lexer {
    struct Token {
        std::string value;
    };

    std::vector<Token> tokenize(const std::string& source);
}

namespace parser {
    struct Token {  // 和 lexer::Token 同名，但不冲突
        std::string value;
        int line;
    };
}

lexer::Token t1;    // lexer 里的 Token
parser::Token t2;   // parser 里的 Token
```

---

### std 命名空间

C++ 标准库的所有内容都在 `std` 命名空间里：

```cpp
std::cout        // 标准输出
std::string      // 字符串类
std::vector<T>   // 动态数组
std::sort(...)   // 排序算法
```

这就是为什么标准库的东西都要加 `std::` 前缀。

---

### using 声明

每次都写 `std::` 有点繁琐。有两种方式可以简化：

**using 声明**：把单个名字引入当前作用域：

```cpp
using std::cout;
using std::string;

cout << "Hello\n";  // 不需要 std::cout
string s = "world"; // 不需要 std::string
```

**using namespace 指令**：把整个命名空间引入：

```cpp
using namespace std;

cout << "Hello\n";
string s = "world";
sort(v.begin(), v.end());
```

`using namespace std` 在头文件里**绝对不能用**——它会污染所有包含这个头文件的文件，可能造成名字冲突。在 `.cpp` 文件里可以用，但也不推荐在全局作用域使用。

本书的代码统一使用 `std::` 前缀，偶尔对频繁使用的名字用 `using std::xxx;`。

---

### 嵌套命名空间

命名空间可以嵌套：

```cpp
namespace taco {
    namespace interpreter {
        class Evaluator { ... };
    }
}

// 使用
taco::interpreter::Evaluator eval;
```

C++17 提供了更简洁的嵌套写法：

```cpp
namespace taco::interpreter {
    class Evaluator { ... };
}
```

---

### 匿名命名空间

匿名命名空间（unnamed namespace）里的内容只在当前文件可见，相当于 C 里的 `static` 函数：

```cpp
// 在 lexer.cpp 里
namespace {
    // 这个函数只在 lexer.cpp 里可见
    bool is_digit(char c) {
        return c >= '0' && c <= '9';
    }
}

// 外部文件无法访问 is_digit
```

这是一种好习惯：把只在当前文件使用的辅助函数放在匿名命名空间里，避免名字污染。

---

### 作用域与生命周期

C++ 的作用域规则和 C 基本相同，但有一些细节值得注意。

变量的作用域从声明处开始，到包含它的 `{}` 结束：

```cpp
int x = 10;   // x 从这里开始

{
    int y = 20;   // y 从这里开始
    std::cout << x << " " << y << "\n";  // 都可访问
}   // y 在这里销毁

std::cout << x << "\n";  // 可以，x 还在作用域里
// std::cout << y;  // 错误，y 已经销毁
```

C++ 允许在任何地方声明变量（不像 C89 要求在函数开头声明）：

```cpp
for (int i = 0; i < 10; i++) {  // i 在循环里声明
    // i 只在循环体里可见
}
// std::cout << i;  // 错误，i 的作用域在循环里
```

在 `for` 循环里声明循环变量是强烈推荐的做法——变量的作用域越小，代码越容易理解。

---

## 小结

这一章讲了五个 C++ 的语法特性：

**auto**：让编译器推导类型，减少冗长的类型名，但不改变 C++ 静态类型的本质。

**引用**：变量的别名，比指针更安全，主要用于函数参数传递，避免不必要的拷贝。

**const**：声明"不可修改"，默认使用 const 是好习惯，能减少 bug，让代码意图更清晰。

**函数重载和默认参数**：同名函数可以有不同版本，参数可以有默认值，让 API 设计更灵活。

**namespace**：把名字隔离在不同空间里，避免大型项目里的名字冲突。

这些特性在后面的章节里会反复用到，现在有个印象就够了。

---

下一章讲字符串和输入输出——C++ 的 `std::string` 和 `iostream`，以及文件操作。
