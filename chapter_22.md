# 第二十二章：迭代器

---

迭代器（iterator）是 STL 的核心设计之一，它是容器和算法之间的桥梁。理解迭代器，就理解了为什么 STL 的算法可以对任何容器工作，以及范围 for 循环的底层是什么。

---

## 22.1 迭代器是什么

迭代器是一种对象，它"指向"容器里的某个元素，可以通过它访问那个元素，也可以移动到下一个元素。

从行为上看，迭代器非常像指针：

```cpp
std::vector<int> v = {10, 20, 30, 40, 50};

// 用迭代器遍历，和用指针遍历数组很像
for (auto it = v.begin(); it != v.end(); ++it) {
    std::cout << *it << "\n";  // * 解引用，获取元素
}

// 用裸指针遍历数组（对比）
int arr[] = {10, 20, 30, 40, 50};
for (int* p = arr; p != arr + 5; ++p) {
    std::cout << *p << "\n";
}
```

`begin()` 返回指向第一个元素的迭代器，`end()` 返回指向最后一个元素**之后**位置的迭代器（一个哨兵，不能解引用）。

```
v:    [10][20][30][40][50][ ]
       ↑                   ↑
     begin()             end()
```

这种"左闭右开"区间 `[begin, end)` 是 STL 的标准约定：包含 `begin`，不包含 `end`。

---

### 迭代器的基本操作

```cpp
std::vector<int> v = {10, 20, 30, 40, 50};

auto it = v.begin();

*it;        // 解引用：10
++it;       // 前进一步，it 现在指向 20
*it;        // 20
--it;       // 后退一步，it 现在指向 10（随机访问迭代器支持）
it += 3;    // 向前 3 步，it 现在指向 40（随机访问迭代器支持）
it - v.begin();  // 计算距离：3（随机访问迭代器支持）

// 比较
it == v.end();   // false
it != v.end();   // true
```

---

## 22.2 迭代器的种类

不同的容器提供不同能力的迭代器，按能力从弱到强分五种：

### 输入迭代器（Input Iterator）

只能单向前进，只能读取，不能修改，每个位置只能访问一次：

```cpp
// 典型例子：istream_iterator，从输入流读取
std::istream_iterator<int> it(std::cin);
std::istream_iterator<int> end;  // 默认构造表示结束

while (it != end) {
    std::cout << *it << "\n";
    ++it;
}
```

### 输出迭代器（Output Iterator）

只能单向前进，只能写入，每个位置只能写一次：

```cpp
// 典型例子：ostream_iterator，向输出流写入
std::vector<int> v = {1, 2, 3, 4, 5};
std::copy(v.begin(), v.end(),
          std::ostream_iterator<int>(std::cout, " "));
// 输出：1 2 3 4 5
```

### 前向迭代器（Forward Iterator）

可以单向前进，可以读写，可以多次访问同一位置。`std::forward_list`（单链表）的迭代器是前向迭代器：

```cpp
std::forward_list<int> fl = {1, 2, 3};
for (auto it = fl.begin(); it != fl.end(); ++it) {
    *it *= 2;  // 可以修改
}
// fl: {2, 4, 6}
```

### 双向迭代器（Bidirectional Iterator）

可以双向前进和后退。`std::list`、`std::map`、`std::set` 的迭代器是双向迭代器：

```cpp
std::list<int> lst = {1, 2, 3, 4, 5};
auto it = lst.end();
--it;   // 后退到最后一个元素
*it;    // 5
--it;   // 后退到倒数第二个
*it;    // 4
```

### 随机访问迭代器（Random Access Iterator）

支持随机访问（像数组下标一样）、加减操作、比较大小。`std::vector`、`std::deque`、裸指针的迭代器是随机访问迭代器：

```cpp
std::vector<int> v = {10, 20, 30, 40, 50};
auto it = v.begin();

it += 3;    // 跳到第 4 个元素
*it;        // 40

it - v.begin();  // 3（计算距离）

v.begin() < v.end();  // true（可以比较大小）
```

**为什么要分这么多种？**

STL 算法根据需要的迭代器类型选择最高效的实现。`std::advance` 把迭代器向前移动 n 步：对随机访问迭代器直接 `it += n`（`O(1)`），对双向/前向迭代器循环 `++it` n 次（`O(n)`）。这种差异让算法对不同容器自动使用最优策略。

---

## 22.3 范围 for 循环的本质

范围 for 循环：

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
for (int n : v) {
    std::cout << n << "\n";
}
```

实际上是以下代码的语法糖：

```cpp
{
    auto __begin = v.begin();
    auto __end   = v.end();
    for (; __begin != __end; ++__begin) {
        int n = *__begin;
        std::cout << n << "\n";
    }
}
```

编译器会把范围 for 展开成这种形式。只要一个类型提供了 `begin()` 和 `end()` 函数（或者有对应的全局函数 `begin(v)` 和 `end(v)`），就可以用范围 for 遍历它。

这意味着**任何自定义类型都可以支持范围 for**，只要实现了对应的迭代器接口。

---

### 为自定义类实现迭代器支持

假设要为 Taco 的 `Environment` 添加迭代支持，可以遍历所有变量：

```cpp
class Environment {
public:
    // 内部迭代器类型：直接暴露 unordered_map 的迭代器
    using iterator = std::unordered_map<std::string, TacoValue>::iterator;
    using const_iterator = std::unordered_map<std::string, TacoValue>::const_iterator;

    iterator begin() { return m_vars.begin(); }
    iterator end()   { return m_vars.end(); }
    const_iterator begin() const { return m_vars.begin(); }
    const_iterator end()   const { return m_vars.end(); }

private:
    std::unordered_map<std::string, TacoValue> m_vars;
    // ...
};

// 使用
Environment env;
env.define("x", 10.0);
env.define("name", std::string("Miguel"));

for (const auto& [key, value] : env) {
    std::cout << key << " = " << value_to_string(value) << "\n";
}
```

---

### const 迭代器

每种容器通常提供两套迭代器：普通迭代器（可以修改元素）和 `const_iterator`（只能读取）：

```cpp
std::vector<int> v = {1, 2, 3};

// 普通迭代器：可以修改
for (auto it = v.begin(); it != v.end(); ++it) {
    *it *= 2;  // 可以修改
}

// const_iterator：只能读
const std::vector<int>& cv = v;
for (auto it = cv.begin(); it != cv.end(); ++it) {
    // *it *= 2;  // 错误！不能修改
    std::cout << *it << "\n";
}

// 明确使用 cbegin/cend 获取 const_iterator
for (auto it = v.cbegin(); it != v.cend(); ++it) {
    // 即使 v 不是 const，这里也不能修改
}
```

**经验规则**：如果不需要修改元素，用 `const auto&` 在范围 for 里遍历，或者用 `cbegin()/cend()`，这会触发 `const_iterator`，让代码意图更清晰，也防止意外修改。

---

### 反向迭代器

很多容器还提供反向迭代器，从尾部向头部遍历：

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// rbegin 指向最后一个元素，rend 指向第一个元素之前
for (auto it = v.rbegin(); it != v.rend(); ++it) {
    std::cout << *it << "\n";  // 5 4 3 2 1
}

// 或者范围 for 配合 std::ranges::reverse（C++20）
```

---

## *More About 迭代器*：迭代器设计哲学与 ranges

> 第一次读可以跳过。

### 为什么迭代器设计成"指针风格"

STL 的设计者 Alexander Stepanov 把迭代器设计成像指针一样，是刻意的——他想让算法既能用于容器，也能用于裸数组：

```cpp
int arr[] = {5, 3, 1, 4, 2};

// std::sort 可以接受裸指针（它们是随机访问迭代器）
std::sort(arr, arr + 5);

// 也可以接受 vector 的迭代器
std::vector<int> v = {5, 3, 1, 4, 2};
std::sort(v.begin(), v.end());
```

裸指针满足随机访问迭代器的所有要求（支持 `*p`、`++p`、`p + n`、`p1 - p2` 等），所以直接可以用。

### C++20 的 ranges

传统 STL 算法用一对迭代器 `[begin, end)` 表示范围，使用起来有点冗长，而且容易传错（`sort(v.begin(), v.end())` 而不是 `sort(v.begin(), v2.end())`）。

C++20 引入了 `std::ranges`，让算法直接接受容器：

```cpp
#include <ranges>
#include <algorithm>

std::vector<int> v = {5, 3, 1, 4, 2};

// 传统：传一对迭代器
std::sort(v.begin(), v.end());

// C++20 ranges：直接传容器
std::ranges::sort(v);

// ranges 还支持惰性管道（类似 Taco 的 pipeline）
auto result = v
    | std::views::filter([](int n) { return n % 2 == 0; })
    | std::views::transform([](int n) { return n * 2; });

for (int n : result) {
    std::cout << n << "\n";
}
```

这和 Taco 的 pipeline 风格非常相似：

```taco
v.filter { n in n % 2 == 0 }.map { n in n * 2 }.each { n in print(n); };
```

Taco 的 pipeline 实现（v4）在精神上和 `std::views` 类似，只不过 Taco 是动态类型的，不需要模板。

---

## 小结

**迭代器**是容器和算法之间的桥梁，行为类似指针：`*it` 解引用，`++it` 前进，`it != end()` 判断是否到达末尾。

**五种迭代器**从弱到强：输入、输出、前向、双向、随机访问。容器提供对应能力的迭代器，算法根据需要的能力类型选择最优实现。

**范围 for 循环**是一对 `begin()/end()` 调用加上循环的语法糖。任何提供了 `begin()/end()` 的类型都可以用范围 for 遍历。

**`const_iterator`** 用于只读遍历，通过 `cbegin()/cend()` 或 `const` 容器的 `begin()/end()` 获取。

---

下一章讲算法库——STL 提供的几十个通用算法，用迭代器对容器进行各种操作，是"避免手写循环"的核心工具。
