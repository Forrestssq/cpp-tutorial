# 第十九章：常用标准库补充

---

这一章介绍几个 C++17 标准库里的实用工具：`<chrono>`、`<optional>`、`<variant>`、`<random>`。这些不是新的语法特性，而是标准库提供的工具类，在 Taco 项目里都会用到。

---

## 19.1 chrono：时间与性能测量

`<chrono>` 提供了类型安全的时间处理工具。在 Taco 里，`🌮🌮🌮🌮` 彩蛋需要输出 Unix 时间戳，就用 `chrono` 实现。

### 时间点与时间段

`chrono` 有三个核心概念：
- **时钟**（clock）：时间的来源，比如系统时钟、高精度时钟
- **时间点**（time_point）：某个时钟上的一个具体时刻
- **时间段**（duration）：两个时间点之间的间隔

```cpp
#include <chrono>

// 获取当前时间点
auto now = std::chrono::system_clock::now();

// 转换成 Unix 时间戳（秒数）
auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
    now.time_since_epoch()
).count();

std::cout << seconds << "\n";  // 比如：1719556800
```

这就是 `🌮🌮🌮🌮` 彩蛋的实现：

```cpp
// 四个 🌮：输出 Unix 时间戳
case 4: {
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    std::cout << ts << "\n";
    break;
}
```

### 性能测量

`chrono` 也是测量代码执行时间的标准工具：

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

// 要测量的代码
for (int i = 0; i < 1000000; i++) {
    // ...
}

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    end - start
).count();

std::cout << "Elapsed: " << duration << "ms\n";
```

`high_resolution_clock` 比 `system_clock` 精度更高，适合性能测量。`system_clock` 适合获取"真实时间"（日历时间）。

### 常用的时间单位

```cpp
using namespace std::chrono_literals;  // 让时间字面量可用

auto one_second   = 1s;      // std::chrono::seconds(1)
auto half_second  = 500ms;   // std::chrono::milliseconds(500)
auto one_minute   = 1min;    // std::chrono::minutes(1)
auto micro        = 100us;   // std::chrono::microseconds(100)

// 在线程里休眠
#include <thread>
std::this_thread::sleep_for(100ms);  // 休眠 100 毫秒
```

---

## 19.2 optional：可能有值，也可能没有值

`std::optional<T>` 表示一个可能存在、也可能不存在的值。这比用 `-1`、`nullptr`、或者特殊值来表示"没有"更安全、更清晰。

### 基本用法

```cpp
#include <optional>

// 有值的 optional
std::optional<int> a = 42;
std::optional<std::string> b = "hello";

// 没有值的 optional
std::optional<int> c;          // 默认构造，没有值
std::optional<int> d = std::nullopt;  // 显式表示没有值

// 检查是否有值
if (a.has_value()) { ... }  // 或者直接 if (a)
if (!c) { ... }             // c 没有值

// 获取值
int val = *a;            // 解引用，如果没有值，未定义行为
int val2 = a.value();    // 如果没有值，抛出 std::bad_optional_access
int val3 = a.value_or(0); // 有值返回值，没有值返回 0
```

### 在 Taco 里的应用

第四章已经用了 `optional` 来表示文件读取结果：

```cpp
std::optional<std::string> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;  // 文件打开失败
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// 使用
auto content = read_file("script.taco");
if (!content) {
    std::cerr << "🌮 Cannot read file\n";
    return 1;
}
run(*content);
```

`optional` 比"用空字符串表示失败"更安全——调用者必须检查是否有值，不能直接使用，编译器会强制这种正确性。

### optional 和 Python 的对比

Python 里通常用 `None` 来表示"没有值"：

```python
def read_file(path):
    try:
        with open(path) as f:
            return f.read()
    except:
        return None

content = read_file("script.taco")
if content is not None:
    run(content)
```

C++ 的 `optional<string>` 和 Python 的 `Optional[str]`（类型提示）思路完全相同。不同的是 C++ 在编译时强制检查，Python 只是类型提示（运行时不强制）。

---

## 19.3 variant：类型安全的联合体

`std::variant<T1, T2, T3>` 表示一个可以是多种类型之一的值，但在同一时刻只有一种类型。这是 C 语言 `union` 的类型安全升级版。

Taco 的值类型就是 `variant`：

```cpp
using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;
```

一个 `TacoValue` 可以是数字、字符串、布尔值或 nil 中的任意一种，但同一时刻只能是一种。

### 基本用法

```cpp
#include <variant>

std::variant<int, std::string, double> v;

// 赋值
v = 42;          // 现在 v 是 int
v = "hello";     // 现在 v 是 string
v = 3.14;        // 现在 v 是 double

// 检查类型
if (std::holds_alternative<int>(v)) {
    std::cout << "It's an int\n";
}

// 获取值
int i = std::get<int>(v);       // 如果 v 不是 int，抛出 std::bad_variant_access
int* p = std::get_if<int>(&v);  // 如果 v 是 int，返回指针；否则返回 nullptr

// 访问（推荐方式）：visit 模式
std::visit([](auto&& val) {
    std::cout << val << "\n";
}, v);
```

### std::visit 的用法

`std::visit` 接受一个"访问者"（通常是 lambda 或函数对象）和一个 `variant`，根据 `variant` 当前的类型调用访问者：

```cpp
TacoValue value = 42.0;

// 根据类型做不同的事
std::visit([](auto&& v) {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, double>) {
        std::cout << "Number: " << v << "\n";
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "String: " << v << "\n";
    } else if constexpr (std::is_same_v<T, bool>) {
        std::cout << "Bool: " << (v ? "true" : "false") << "\n";
    } else {
        std::cout << "nil\n";
    }
}, value);
```

这里用了 `if constexpr`——编译期的条件分支，根据类型在编译时选择代码路径。`std::is_same_v<T, double>` 检查 `T` 是否等于 `double`，这是编译期计算，没有运行时开销。

### Taco 里的 variant 使用

在 `value.cpp` 里，很多函数都用 `holds_alternative` 来检查类型：

```cpp
std::string value_to_string(const TacoValue& val) {
    if (std::holds_alternative<double>(val)) {
        double d = std::get<double>(val);
        // ...
        return std::to_string(d);
    }
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    }
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? "true" : "false";
    }
    return "nil";  // std::nullptr_t
}
```

### variant vs union

C 的 `union` 把同一块内存解释成不同类型，没有类型检查：

```c
union Value {
    int   i;
    float f;
    char* s;
};

union Value v;
v.i = 42;
printf("%f\n", v.f);  // 未定义行为！但编译器不会报错
```

`variant` 追踪当前存储的是哪种类型，访问错误类型会抛出异常：

```cpp
std::variant<int, float, std::string> v = 42;
std::get<float>(v);  // 抛出 std::bad_variant_access，不是静默的未定义行为
```

---

## 19.4 random：随机数

`<random>` 是 C++11 引入的随机数库，比 C 的 `rand()` 更灵活、更统一、质量更好。

在 Taco 里，`🌮` 彩蛋需要随机选择 Magic 8 Ball 的答案，`🌮🌮` 需要随机选择魔法海螺的回答，`🌮🌮🌮` 需要随机选择程序员笑话——都用 `<random>`。

### 基本用法

`<random>` 的设计分两层：**随机数引擎**（产生随机位序列）和**分布**（把随机位序列转成特定分布的数）：

```cpp
#include <random>

// 随机数引擎：Mersenne Twister 是最常用的
std::mt19937 engine(std::random_device{}());
// std::random_device 用硬件随机数（真随机）作为种子
// std::mt19937 是伪随机数生成器，种子固定时序列可重现

// 均匀整数分布：[0, 19] 之间的随机整数
std::uniform_int_distribution<int> dist(0, 19);

int random_index = dist(engine);
```

### 在 Taco 彩蛋里的应用

```cpp
// 全局随机数引擎（只初始化一次）
static std::mt19937 rng(std::random_device{}());

// Magic 8 Ball：20 个回答里随机选一个
std::string magic_8_ball() {
    static const std::vector<std::string> answers = {
        "It is certain.",
        "Without a doubt.",
        "Yes, definitely.",
        "You may rely on it.",
        "Most likely.",
        "Outlook good.",
        "Yes.",
        "Signs point to yes.",
        "As I see it, yes.",
        "It is decidedly so.",
        "Reply hazy, try again.",
        "Ask again later.",
        "Cannot predict now.",
        "Concentrate and ask again.",
        "Better not tell you now.",
        "Don't count on it.",
        "My reply is no.",
        "Very doubtful.",
        "Outlook not so good.",
        "My sources say no.",
    };

    std::uniform_int_distribution<int> dist(0, answers.size() - 1);
    return "🎱 " + answers[dist(rng)];
}

// 魔法海螺：9 个回答里随机选一个
std::string magic_conch() {
    static const std::vector<std::string> answers = {
        "No.",
        "I don't think so.",
        "No, no, no, no, no.",
        "Maybe someday.",
        "Yes.",
        "Try asking again.",
        "Neither.",
        "Mmm, I don't think so.",
        "No, definitely not.",
    };

    std::uniform_int_distribution<int> dist(0, answers.size() - 1);
    return answers[dist(rng)];
}
```

### 为什么不用 rand()

C 的 `rand()` 有几个问题：

```c
srand(time(NULL));       // 种子只有秒级精度，同一秒内总是相同
int r = rand() % 20;    // 取模会使分布不均匀（如果 RAND_MAX 不是 20 的整倍数）
                         // rand() 的质量通常很差，周期短
```

`<random>` 的设计解决了这些问题：`random_device` 提供高质量的种子，`mt19937` 有极长的周期（2^19937 - 1），分布类（`uniform_int_distribution`）保证均匀分布。

---

### 常用的分布类

```cpp
std::mt19937 rng(std::random_device{}());

// 均匀整数：[a, b] 闭区间
std::uniform_int_distribution<int> int_dist(1, 100);
int n = int_dist(rng);  // 1 到 100 之间的整数

// 均匀浮点：[a, b) 半开区间
std::uniform_real_distribution<double> real_dist(0.0, 1.0);
double d = real_dist(rng);  // 0.0 到 1.0 之间的浮点数

// 正态分布（高斯分布）
std::normal_distribution<double> normal_dist(0.0, 1.0);  // 均值 0，标准差 1
double g = normal_dist(rng);

// 伯努利分布（抛硬币）
std::bernoulli_distribution coin(0.5);  // 0.5 概率为 true
bool heads = coin(rng);
```

---

## 小结

**`chrono`** 提供类型安全的时间处理，`system_clock::now()` 获取当前时间，`high_resolution_clock` 用于性能测量，`duration_cast` 在时间单位之间转换。

**`optional<T>`** 表示可能存在或不存在的值，比用特殊值（`-1`、`nullptr`）表示"没有"更安全、更清晰。用 `if (opt)` 检查是否有值，用 `*opt` 或 `opt.value()` 获取值。

**`variant<T1, T2, ...>`** 是类型安全的联合体，同一时刻只能是其中一种类型。用 `holds_alternative<T>` 检查类型，`get<T>` 或 `get_if<T>` 获取值，`visit` 遍历所有可能的类型。Taco 的值类型 `TacoValue` 就是一个 `variant`。

**`random`** 提供高质量的随机数，`mt19937` + `random_device` 是标准组合，各种分布类（`uniform_int_distribution` 等）保证正确的统计分布。

---

下一章是第四部分的项目章节：用智能指针和移动语义重构 Taco，实现函数、闭包和作用域（v3）。
