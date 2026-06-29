# 第二十四章：Lambda 表达式

---

Lambda 表达式（lambda expression）是 C++11 引入的特性，让你可以在需要的地方直接定义匿名函数，而不需要单独命名一个函数或定义一个函数对象类。Lambda 和算法库是绝配——传给 `sort`、`find_if`、`transform` 的比较器和谓词，用 lambda 写最自然。

Lambda 也是理解 Taco 闭包语法的基础——Taco 的 `{ x in x * 2 }` 和 C++ 的 `[](int x) { return x * 2; }` 在设计思想上完全一致。

---

## 24.1 Lambda 的语法

Lambda 的完整语法：

```cpp
[capture_list](parameter_list) -> return_type {
    function_body
}
```

各个部分：
- **`[capture_list]`**：捕获列表，声明从外部作用域捕获哪些变量
- **`(parameter_list)`**：参数列表，和普通函数一样
- **`-> return_type`**：返回类型，大多数情况下可以省略（编译器自动推导）
- **`{ function_body }`**：函数体

---

### 最简单的 lambda

```cpp
// 没有参数，没有返回值
auto greet = []() {
    std::cout << "Hello!\n";
};
greet();  // Hello!

// 有参数
auto add = [](int a, int b) {
    return a + b;
};
int sum = add(3, 4);  // 7

// 省略参数列表的括号（无参数时可省）
auto say_hi = [] {
    std::cout << "Hi!\n";
};
say_hi();
```

### 立即调用

Lambda 可以在定义后立刻调用：

```cpp
int result = [](int a, int b) {
    return a + b;
}(3, 4);  // 立刻调用，传入 3 和 4
// result == 7
```

---

### 与算法配合

这是 lambda 最常见的用途：

```cpp
std::vector<int> v = {5, 3, 8, 1, 9, 2, 7, 4, 6};

// sort：自定义比较器
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a > b;  // 降序
});

// find_if：自定义条件
auto it = std::find_if(v.begin(), v.end(), [](int n) {
    return n > 5;  // 找第一个大于 5 的
});

// transform：自定义映射
std::vector<int> squares(v.size());
std::transform(v.begin(), v.end(), squares.begin(), [](int n) {
    return n * n;
});

// count_if：自定义计数条件
int count = std::count_if(v.begin(), v.end(), [](int n) {
    return n % 2 == 0;  // 偶数个数
});
```

---

## 24.2 捕获列表：值捕获与引用捕获

Lambda 可以"捕获"外部作用域里的变量——这就是为什么 lambda 叫做"闭包"（closure）。

### 值捕获

```cpp
int x = 10;
int y = 20;

// 值捕获：捕获 x 和 y 的当前值（拷贝）
auto add_xy = [x, y](int z) {
    return x + y + z;
};

x = 100;  // 修改 x，但 lambda 捕获的是原来的值
y = 200;

int result = add_xy(5);
// result == 10 + 20 + 5 == 35（不是 100 + 200 + 5）
// 因为 lambda 捕获的是定义时的 x=10, y=20
```

值捕获是拷贝，lambda 内部有自己的变量副本，外部修改不影响 lambda，lambda 内部修改也不影响外部（默认情况下）。

### 引用捕获

```cpp
int sum = 0;

auto accumulate = [&sum](int n) {
    sum += n;  // 通过引用修改外部的 sum
};

for (int i = 1; i <= 5; i++) {
    accumulate(i);
}
std::cout << sum << "\n";  // 15
```

引用捕获让 lambda 直接操作外部变量，修改会反映到外部。

### 捕获所有变量

```cpp
int a = 1, b = 2, c = 3;

// [=]：值捕获所有外部变量
auto f1 = [=]() { return a + b + c; };

// [&]：引用捕获所有外部变量
auto f2 = [&]() { a++; b++; c++; };

f2();
std::cout << a << " " << b << " " << c << "\n";  // 2 3 4
```

### 混合捕获

```cpp
int x = 10;
int y = 20;

// 值捕获 x，引用捕获 y
auto f = [x, &y]() {
    // x 是值捕获，不能修改（修改会编译错误，除非用 mutable）
    y = 100;  // y 是引用捕获，可以修改
    return x + y;
};

// [=, &y]：默认值捕获，但 y 用引用捕获
auto f2 = [=, &y]() {
    y = 200;
    return x + y;
};

// [&, x]：默认引用捕获，但 x 用值捕获
auto f3 = [&, x]() {
    y = 300;  // 引用捕获
    return x + y;  // x 是值捕获
};
```

### mutable lambda

值捕获的变量在 lambda 里默认是 `const`，不能修改。用 `mutable` 关键字可以允许修改（但修改的是 lambda 内部的副本，不影响外部）：

```cpp
int count = 0;

// 没有 mutable：不能修改值捕获的变量
auto counter = [count]() mutable {
    count++;  // 修改的是 lambda 内部的 count 副本
    return count;
};

std::cout << counter() << "\n";  // 1
std::cout << counter() << "\n";  // 2
std::cout << count << "\n";      // 0（外部的 count 没变）
```

这和 Taco 的 `makeCounter` 闭包有点像，但区别是：Taco 用引用捕获（通过 `shared_ptr<Environment>`），修改真的会反映到外部。C++ 的 lambda 值捕获是副本，不影响外部。

---

### 捕获 this

在类的成员函数里，lambda 可以捕获 `this` 来访问成员：

```cpp
class Evaluator {
public:
    void process_tokens(std::vector<Token>& tokens) {
        // 捕获 this，可以访问 Evaluator 的成员
        std::for_each(tokens.begin(), tokens.end(),
                      [this](const Token& t) {
                          this->handle_token(t);  // 访问成员函数
                      });

        // 也可以直接写（this 是隐式的）
        std::for_each(tokens.begin(), tokens.end(),
                      [this](const Token& t) {
                          handle_token(t);  // 同上，this 隐式
                      });
    }

private:
    void handle_token(const Token& t) { ... }
};
```

---

## 24.3 Lambda 与算法的配合

Lambda 让算法的可读性大幅提升。来看一些实际的 Taco 项目里的例子：

```cpp
// 过滤掉注释 Token
std::vector<Token> filtered;
std::copy_if(tokens.begin(), tokens.end(),
             std::back_inserter(filtered),
             [](const Token& t) {
                 return t.type != TokenType::EndOfFile;
             });

// 统计每种 Token 类型的数量
std::unordered_map<TokenType, int> counts;
std::for_each(tokens.begin(), tokens.end(),
              [&counts](const Token& t) {
                  counts[t.type]++;
              });

// 找到第一个错误 Token（假设有 Error 类型）
auto error_it = std::find_if(tokens.begin(), tokens.end(),
                              [](const Token& t) {
                                  return t.line == 0;  // 无效行号表示错误
                              });

// 把所有 Token 的值提取到字符串列表
std::vector<std::string> values;
std::transform(tokens.begin(), tokens.end(),
               std::back_inserter(values),
               [](const Token& t) { return t.value; });

// 检查是否所有 Token 都有有效行号
bool all_valid = std::all_of(tokens.begin(), tokens.end(),
                              [](const Token& t) { return t.line > 0; });
```

---

## 24.4 std::function

`std::function<R(Args...)>` 是一个可以存储任何可调用对象（函数、lambda、函数指针、函数对象）的类型。

```cpp
#include <functional>

// 存储函数指针
int add(int a, int b) { return a + b; }
std::function<int(int, int)> f1 = add;

// 存储 lambda
std::function<int(int, int)> f2 = [](int a, int b) { return a * b; };

// 存储成员函数（需要 bind 或 lambda）
struct Calculator {
    int subtract(int a, int b) { return a - b; }
};
Calculator calc;
std::function<int(int, int)> f3 = [&calc](int a, int b) {
    return calc.subtract(a, b);
};

// 调用
std::cout << f1(3, 4) << "\n";  // 7
std::cout << f2(3, 4) << "\n";  // 12
std::cout << f3(10, 3) << "\n"; // 7
```

`std::function` 的主要用途是当需要把函数作为参数传递，而调用者不知道具体是什么类型的可调用对象时：

```cpp
// Taco 内置函数：存储为 std::function
using NativeFunction = std::function<TacoValue(std::vector<TacoValue>)>;

struct TacoNative {
    std::string    name;
    NativeFunction fn;
};

// 注册内置函数
env->define("print", std::make_shared<TacoNative>(TacoNative{
    "print",
    [](std::vector<TacoValue> args) -> TacoValue {
        std::cout << value_to_string(args[0]) << "\n";
        return nullptr;
    }
}));
```

### std::function 的开销

`std::function` 有一定的运行时开销：
- 存储可调用对象需要动态分配（如果对象超过小对象优化的阈值）
- 调用时有虚函数调用的开销（类型擦除的代价）

在性能敏感的热路径上，直接用模板参数传递可调用对象比 `std::function` 快：

```cpp
// 用 std::function：有运行时开销
void process(std::function<void(int)> fn, std::vector<int>& v) {
    for (int n : v) fn(n);
}

// 用模板：零开销，编译时决定
template<typename Fn>
void process(Fn fn, std::vector<int>& v) {
    for (int n : v) fn(n);
}
```

在 Taco 的内置函数里用 `std::function` 是合理的——内置函数调用不是性能瓶颈，而且需要在运行时存储不同类型的函数，类型擦除是必要的。

---

## *More About Lambda*：Lambda 的底层——函数对象

> 第一次读可以跳过。

### Lambda 的本质

Lambda 在底层是一个由编译器生成的**函数对象**（function object，也叫 functor）——一个重载了 `operator()` 的类。

```cpp
int x = 10;
auto add_x = [x](int n) { return x + n; };
```

编译器把这个 lambda 展开成类似这样的类：

```cpp
class __lambda_add_x {
public:
    __lambda_add_x(int x) : m_x(x) {}  // 捕获 x

    int operator()(int n) const {
        return m_x + n;
    }

private:
    int m_x;  // 值捕获的副本
};

// lambda 实际上是这个类的一个实例
__lambda_add_x add_x(x);  // 等价于 auto add_x = [x](int n) { return x + n; };
```

这就解释了：
- 为什么值捕获是副本（构造时把变量值存为成员）
- 为什么引用捕获是引用（成员是引用类型）
- 为什么 lambda 可以作为算法的参数（它是一个可调用对象）
- 为什么 `mutable` 让你能修改值捕获（去掉了 `operator()` 的 `const`）

### 与手写函数对象的对比

在 lambda 之前（C++11 之前），每次需要传一个自定义函数给算法，都要手写一个类：

```cpp
// C++11 之前：手写函数对象
struct IsGreaterThan {
    int threshold;
    IsGreaterThan(int t) : threshold(t) {}
    bool operator()(int n) const { return n > threshold; }
};

std::find_if(v.begin(), v.end(), IsGreaterThan(5));

// C++11 之后：lambda
std::find_if(v.begin(), v.end(), [](int n) { return n > 5; });
```

Lambda 让函数对象的定义和使用在同一个地方，不需要在别处定义类，代码更紧凑，意图更清晰。

---

## 小结

**Lambda 语法**：`[capture](params) -> return_type { body }`。返回类型通常可以省略，无参数时括号可以省略。

**捕获列表**：
- `[x, y]`：值捕获 x 和 y（副本）
- `[&x, &y]`：引用捕获 x 和 y
- `[=]`：值捕获所有外部变量
- `[&]`：引用捕获所有外部变量
- `[=, &y]`：默认值捕获，y 用引用捕获

**mutable**：允许修改值捕获的变量（修改的是 lambda 内部的副本）。

**`std::function<R(Args...)>`**：存储任意可调用对象的包装器，有运行时开销，适合需要类型擦除的场景。

**底层**：Lambda 是编译器生成的函数对象类，捕获的变量存为成员变量，`operator()` 实现函数调用。

---

下一章是第五部分的项目章节：用 STL 容器和 Lambda 实现 Taco 的 array、map 和内置标准库，以及 pipeline 风格（v4）。
