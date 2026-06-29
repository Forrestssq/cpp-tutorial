# 第二十三章：算法库

---

STL 算法库（`<algorithm>`、`<numeric>`）提供了几十个通用算法，可以对任何支持迭代器的容器操作。这些算法是 C++ 里"避免手写循环"的核心工具——大多数循环都能用一个算法调用替代，代码更短、意图更清晰、而且算法本身经过高度优化。

这一章系统地介绍常用算法，按功能分类。

---

## 23.1 不修改序列的算法

这类算法只读取容器里的元素，不改变它们。

### find 和 find_if

```cpp
#include <algorithm>
#include <vector>

std::vector<int> v = {10, 20, 30, 40, 50};

// find：查找特定值，返回迭代器
auto it = std::find(v.begin(), v.end(), 30);
if (it != v.end()) {
    std::cout << "Found: " << *it << " at index " << (it - v.begin()) << "\n";
    // Found: 30 at index 2
}

// find_if：查找满足条件的第一个元素
auto it2 = std::find_if(v.begin(), v.end(), [](int n) {
    return n > 25;
});
// *it2 == 30

// find_if_not：查找第一个不满足条件的元素
auto it3 = std::find_if_not(v.begin(), v.end(), [](int n) {
    return n < 40;
});
// *it3 == 10（第一个不小于 40 的）... 其实是第一个不小于 40 的，即 10 不满足 < 40
// 其实 10 < 40 是 true，find_if_not 找第一个让条件为 false 的
// 这里 40 < 40 是 false，所以 *it3 == 40
```

### count 和 count_if

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};

// count：计算某个值出现的次数
int n = std::count(v.begin(), v.end(), 2);  // 3

// count_if：计算满足条件的元素个数
int evens = std::count_if(v.begin(), v.end(), [](int n) {
    return n % 2 == 0;
});  // 3（2, 2, 4, 2 里有 4 个偶数... 实际是 4 个：2, 2, 4, 2）
```

### all_of、any_of、none_of

```cpp
std::vector<int> v = {2, 4, 6, 8, 10};

// all_of：所有元素都满足条件？
bool all_even = std::all_of(v.begin(), v.end(), [](int n) {
    return n % 2 == 0;
});  // true

// any_of：至少有一个元素满足条件？
bool any_gt5 = std::any_of(v.begin(), v.end(), [](int n) {
    return n > 5;
});  // true

// none_of：没有元素满足条件？
bool none_negative = std::none_of(v.begin(), v.end(), [](int n) {
    return n < 0;
});  // true
```

这三个算法非常直观，而且有短路求值——`any_of` 一找到就停，`all_of` 一发现不满足就停。

### for_each

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// for_each：对每个元素执行操作（和范围 for 类似，但可以传给其他函数）
std::for_each(v.begin(), v.end(), [](int n) {
    std::cout << n << " ";
});
// 1 2 3 4 5

// for_each 的返回值是传入的函数对象（可以用来积累状态）
int sum = 0;
std::for_each(v.begin(), v.end(), [&sum](int n) {
    sum += n;
});
// sum == 15
```

大多数情况下范围 for 更简洁，`for_each` 的优势是可以把函数对象传递给其他函数，或者在某些需要迭代器的接口里使用。

### min_element 和 max_element

```cpp
std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

auto min_it = std::min_element(v.begin(), v.end());
auto max_it = std::max_element(v.begin(), v.end());

std::cout << *min_it << "\n";  // 1
std::cout << *max_it << "\n";  // 9

// 获取下标
int min_idx = min_it - v.begin();  // 1（第一个 1 的位置）
```

### equal 和 mismatch

```cpp
std::vector<int> a = {1, 2, 3, 4, 5};
std::vector<int> b = {1, 2, 3, 4, 5};
std::vector<int> c = {1, 2, 99, 4, 5};

// equal：两个序列是否完全相同
bool same = std::equal(a.begin(), a.end(), b.begin());  // true
bool diff = std::equal(a.begin(), a.end(), c.begin());  // false

// mismatch：找到第一个不同的位置
auto [it_a, it_c] = std::mismatch(a.begin(), a.end(), c.begin());
std::cout << *it_a << " vs " << *it_c << "\n";  // 3 vs 99
```

---

## 23.2 修改序列的算法

这类算法会改变容器的内容（但通常不改变容器的大小）。

### transform

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
std::vector<int> result(v.size());

// transform：对每个元素应用函数，结果存入另一个容器
std::transform(v.begin(), v.end(), result.begin(), [](int n) {
    return n * n;
});
// result: {1, 4, 9, 16, 25}

// 也可以原地修改
std::transform(v.begin(), v.end(), v.begin(), [](int n) {
    return n * 2;
});
// v: {2, 4, 6, 8, 10}

// 两个序列合并：transform 的二元版本
std::vector<int> a = {1, 2, 3};
std::vector<int> b = {10, 20, 30};
std::vector<int> sum(3);
std::transform(a.begin(), a.end(), b.begin(), sum.begin(),
               [](int x, int y) { return x + y; });
// sum: {11, 22, 33}
```

### copy 和 copy_if

```cpp
std::vector<int> src = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
std::vector<int> dst;
dst.resize(src.size());

// copy：复制整个序列
std::copy(src.begin(), src.end(), dst.begin());

// copy_if：只复制满足条件的元素
std::vector<int> evens;
std::copy_if(src.begin(), src.end(), std::back_inserter(evens),
             [](int n) { return n % 2 == 0; });
// evens: {2, 4, 6, 8, 10}
```

`std::back_inserter` 是一个输出迭代器适配器，每次"写入"都调用 `push_back`，让目标容器自动扩展大小。这在不知道结果大小时非常有用。

### replace 和 replace_if

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2};

// replace：把所有等于某值的元素换成新值
std::replace(v.begin(), v.end(), 2, 99);
// v: {1, 99, 3, 99, 4, 99}

// replace_if：把满足条件的元素换成新值
std::replace_if(v.begin(), v.end(), [](int n) { return n > 50; }, 0);
// v: {1, 0, 3, 0, 4, 0}
```

### remove 和 remove_if

`remove` 和 `remove_if` 比较特殊——它们不真正删除元素，而是把不需要删除的元素移到容器前面，然后返回一个迭代器，指向"逻辑末尾"：

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};

// remove：把等于 2 的元素移走
auto new_end = std::remove(v.begin(), v.end(), 2);
// v 的内容变成：{1, 3, 4, 5, ?, ?, ?}（? 是垃圾值）
// new_end 指向第 5 个位置

// 要真正删除，配合 erase
v.erase(new_end, v.end());
// v: {1, 3, 4, 5}

// 更常见的写法：erase-remove 惯用法
v = {1, 2, 3, 2, 4, 2, 5};
v.erase(std::remove(v.begin(), v.end(), 2), v.end());
// v: {1, 3, 4, 5}
```

这个"erase-remove 惯用法"（erase-remove idiom）是 C++ 里删除容器元素的标准方式，看起来有点奇怪但很高效。

### fill 和 generate

```cpp
std::vector<int> v(10);

// fill：用指定值填充
std::fill(v.begin(), v.end(), 42);
// v: {42, 42, 42, ..., 42}

// fill_n：填充 n 个元素
std::fill_n(v.begin(), 5, 99);
// v: {99, 99, 99, 99, 99, 42, 42, 42, 42, 42}

// generate：用函数生成值
int counter = 0;
std::generate(v.begin(), v.end(), [&counter]() { return counter++; });
// v: {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
```

---

## 23.3 排序相关算法

### sort

```cpp
std::vector<int> v = {5, 3, 1, 4, 2};

// 默认：升序排序
std::sort(v.begin(), v.end());
// v: {1, 2, 3, 4, 5}

// 自定义比较器：降序
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
// v: {5, 4, 3, 2, 1}

// 对结构体排序
struct Person {
    std::string name;
    int age;
};

std::vector<Person> people = {
    {"Miguel", 12}, {"Dante", 3}, {"Imelda", 70}
};

// 按年龄排序
std::sort(people.begin(), people.end(), [](const Person& a, const Person& b) {
    return a.age < b.age;
});
// Dante(3), Miguel(12), Imelda(70)

// 按姓名排序
std::sort(people.begin(), people.end(), [](const Person& a, const Person& b) {
    return a.name < b.name;
});
// Dante, Imelda, Miguel
```

`std::sort` 的平均复杂度是 $O(n \log n)$，标准库实现通常是 introsort（快排 + 堆排 + 插排的混合）。

### stable_sort

```cpp
// stable_sort：排序时保持相等元素的相对顺序（稳定排序）
std::vector<Person> people = {
    {"Alice", 25}, {"Bob", 25}, {"Charlie", 30}
};

// Alice 和 Bob 年龄相同
std::stable_sort(people.begin(), people.end(), [](const Person& a, const Person& b) {
    return a.age < b.age;
});
// Alice 仍然在 Bob 前面（相对顺序保持）
```

`stable_sort` 比 `sort` 稍慢（$O(n \log^2 n)$ 或 $O(n \log n)$ 取决于内存），但在需要稳定性时是正确的选择。

### partial_sort 和 nth_element

```cpp
std::vector<int> v = {5, 3, 1, 4, 2};

// partial_sort：只排序前 k 个元素（找最小的 k 个）
std::partial_sort(v.begin(), v.begin() + 3, v.end());
// v 的前 3 个是排好序的最小值：{1, 2, 3, ?, ?}（? 是剩余元素，顺序未定）

// nth_element：让第 n 个位置的元素就是排好序后应该在的那个元素
// 比 sort 快：O(n) 而不是 O(n log n)
v = {5, 3, 1, 4, 2};
std::nth_element(v.begin(), v.begin() + 2, v.end());
// v[2] 是排好序后的第 3 小的元素（1-indexed 是第 3 个）
// 即 v[2] == 3
// v[0], v[1] 都比 3 小（不保证排序），v[3], v[4] 都比 3 大（不保证排序）
```

`nth_element` 常用于"找第 k 小的元素"这类问题，比完整排序快得多。

### binary_search、lower_bound、upper_bound

这三个用于**已排序**的序列：

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// binary_search：序列里是否存在某个值（O(log n)）
bool found = std::binary_search(v.begin(), v.end(), 5);  // true
bool found2 = std::binary_search(v.begin(), v.end(), 11); // false

// lower_bound：第一个不小于目标值的位置
auto lb = std::lower_bound(v.begin(), v.end(), 5);
// *lb == 5，lb - v.begin() == 4

// upper_bound：第一个大于目标值的位置
auto ub = std::upper_bound(v.begin(), v.end(), 5);
// *ub == 6，ub - v.begin() == 5

// 找所有等于 5 的元素的范围
auto range = std::equal_range(v.begin(), v.end(), 5);
// range.first == lower_bound，range.second == upper_bound
```

---

## 23.4 数值算法

`<numeric>` 头文件里的数值算法：

```cpp
#include <numeric>

std::vector<int> v = {1, 2, 3, 4, 5};

// accumulate：累积计算（求和、求积等）
int sum = std::accumulate(v.begin(), v.end(), 0);     // 15（从 0 开始累加）
int product = std::accumulate(v.begin(), v.end(), 1,
                              [](int acc, int n) { return acc * n; }); // 120

// reduce（C++17）：和 accumulate 类似，但可以并行
int sum2 = std::reduce(v.begin(), v.end());  // 15，默认从 0 开始

// inner_product：内积（点积）
std::vector<int> a = {1, 2, 3};
std::vector<int> b = {4, 5, 6};
int dot = std::inner_product(a.begin(), a.end(), b.begin(), 0);
// 1*4 + 2*5 + 3*6 = 32

// partial_sum：前缀和
std::vector<int> prefix(v.size());
std::partial_sum(v.begin(), v.end(), prefix.begin());
// prefix: {1, 3, 6, 10, 15}

// iota：填充连续整数
std::vector<int> nums(10);
std::iota(nums.begin(), nums.end(), 0);  // 从 0 开始
// nums: {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}

std::iota(nums.begin(), nums.end(), 1);  // 从 1 开始
// nums: {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
```

---

## 23.5 其他常用算法

### rotate

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// rotate：将 [first, middle) 旋转到末尾
std::rotate(v.begin(), v.begin() + 2, v.end());
// 原来 [1,2] 移到末尾：{3, 4, 5, 1, 2}
```

### partition

```cpp
std::vector<int> v = {1, 5, 2, 8, 3, 7, 4, 6};

// partition：把满足条件的元素移到前面，不满足的移到后面
auto mid = std::partition(v.begin(), v.end(), [](int n) {
    return n % 2 == 0;  // 偶数在前
});
// v 的前半段是偶数，后半段是奇数（顺序不保证）
// mid 指向后半段的开始

// stable_partition：保持相对顺序的 partition
```

### unique

```cpp
std::vector<int> v = {1, 1, 2, 2, 3, 3, 2, 2};

// unique：移除相邻的重复元素（只处理相邻重复！）
auto new_end = std::unique(v.begin(), v.end());
v.erase(new_end, v.end());
// v: {1, 2, 3, 2}（注意 2 出现两次，因为不是相邻的）

// 要去掉所有重复，先排序
v = {3, 1, 4, 1, 5, 9, 2, 6, 5};
std::sort(v.begin(), v.end());
v.erase(std::unique(v.begin(), v.end()), v.end());
// v: {1, 2, 3, 4, 5, 6, 9}
```

### merge

```cpp
std::vector<int> a = {1, 3, 5, 7};
std::vector<int> b = {2, 4, 6, 8};
std::vector<int> merged(a.size() + b.size());

// merge：合并两个已排序的序列
std::merge(a.begin(), a.end(), b.begin(), b.end(), merged.begin());
// merged: {1, 2, 3, 4, 5, 6, 7, 8}
```

### shuffle

```cpp
#include <random>

std::vector<int> v = {1, 2, 3, 4, 5};
std::mt19937 rng(std::random_device{}());

// shuffle：随机打乱
std::shuffle(v.begin(), v.end(), rng);
// v 的顺序被随机打乱
```

---

## 23.6 避免手写循环的思维方式

用算法库的好处不只是代码更短——它让代码的**意图更清晰**。

来看一个例子：统计 Taco Token 列表里各种类型的数量。

**手写循环版本**：

```cpp
std::unordered_map<TokenType, int> type_counts;
for (const auto& token : tokens) {
    type_counts[token.type]++;
}
```

**算法版本**：

```cpp
// 用 for_each 加 lambda
std::unordered_map<TokenType, int> type_counts;
std::for_each(tokens.begin(), tokens.end(), [&type_counts](const Token& t) {
    type_counts[t.type]++;
});
```

这两个版本差不多，手写循环其实更清晰。但有些场景算法版本明显更好：

**找到所有标识符**：

```cpp
// 手写循环
std::vector<std::string> identifiers;
for (const auto& t : tokens) {
    if (t.type == TokenType::Identifier) {
        identifiers.push_back(t.value);
    }
}

// 算法版本：意图一眼就懂
std::vector<std::string> identifiers;
std::transform(
    tokens.begin(), tokens.end(),
    std::back_inserter(identifiers),
    // 但这里不对，transform 不能做过滤...
    // 需要先 copy_if 再 transform，或者用 ranges
);

// 其实这个场景手写循环更好
// ranges 版本（C++20）更清晰：
auto idents = tokens
    | std::views::filter([](const Token& t) {
          return t.type == TokenType::Identifier;
      })
    | std::views::transform([](const Token& t) {
          return t.value;
      });
```

**对 Token 值排序**：

```cpp
std::vector<std::string> values;
for (const auto& t : tokens) {
    values.push_back(t.value);
}
std::sort(values.begin(), values.end());

// 用算法组合（虽然也可以，但未必更简洁）
```

**结论**：不是所有情况都适合用算法替代手写循环，要看哪种更清晰。但了解这些算法，在合适的时候使用它们，是 C++ 代码质量的标志之一。

---

## 小结

**不修改序列的算法**：`find`/`find_if`、`count`/`count_if`、`all_of`/`any_of`/`none_of`、`for_each`、`min_element`/`max_element`。

**修改序列的算法**：`transform`（映射）、`copy_if`（过滤复制）、`replace`/`replace_if`、`remove`/`remove_if`（配合 `erase` 使用）、`fill`/`generate`。

**排序算法**：`sort`（不稳定）、`stable_sort`（稳定）、`partial_sort`（部分排序）、`nth_element`（第 k 小）、`binary_search`/`lower_bound`/`upper_bound`（已排序序列查找）。

**数值算法**：`accumulate`/`reduce`（累积）、`inner_product`（内积）、`partial_sum`（前缀和）、`iota`（连续填充）。

**其他**：`rotate`（旋转）、`partition`（分区）、`unique`（去重相邻）、`merge`（合并有序序列）、`shuffle`（随机打乱）。

---

下一章讲 Lambda 表达式——它是算法的最佳搭档，也是理解 Taco 闭包语法的 C++ 基础。
