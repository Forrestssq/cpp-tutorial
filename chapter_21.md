# 第二十一章：容器

---

第五部分进入 STL（Standard Template Library，标准模板库）。STL 是 C++ 标准库的核心，提供了一套经过精心设计、高度优化的数据结构和算法。

在 Python 里，列表（`list`）、字典（`dict`）、集合（`set`）是内置类型，用起来很自然。C++ 的 STL 容器提供了同样的功能，但因为是静态类型，有一些不同的使用方式和性能特性。

这一章讲最常用的几种容器，下一章讲迭代器，再下一章讲算法。理解了这三章，再看 v4 的代码会非常清晰。

---

## 21.1 vector：最常用的序列容器

`std::vector<T>` 是动态数组——可以在尾部高效地添加或删除元素，支持随机访问（通过下标 `[]`）。它是 C++ 里最常用的容器，在很多场景下 `vector` 是默认选择。

### 基本用法

```cpp
#include <vector>

// 创建
std::vector<int> nums;               // 空 vector
std::vector<int> nums2(5);           // 5 个元素，初始值 0
std::vector<int> nums3(5, 42);       // 5 个元素，初始值 42
std::vector<int> nums4 = {1, 2, 3, 4, 5};  // 初始化列表

// 添加元素
nums.push_back(10);   // 在尾部添加
nums.push_back(20);
nums.push_back(30);

// 访问元素
int first = nums[0];        // 下标访问，不检查边界
int second = nums.at(1);    // 带边界检查，越界抛 std::out_of_range
int last = nums.back();     // 最后一个元素
int front = nums.front();   // 第一个元素

// 大小
nums.size();     // 元素个数
nums.empty();    // 是否为空
nums.capacity(); // 当前分配的容量（可能大于 size）

// 删除
nums.pop_back();   // 删除最后一个元素
nums.clear();      // 删除所有元素

// 遍历
for (int n : nums) {
    std::cout << n << "\n";
}

// 或者用迭代器（下一章详讲）
for (auto it = nums.begin(); it != nums.end(); ++it) {
    std::cout << *it << "\n";
}
```

### vector 的内存模型

`vector` 在内存里是一块连续的数组，这意味着：
- 随机访问 `O(1)`：`nums[i]` 直接计算内存地址
- 尾部添加 `O(1)` 均摊：大多数时候很快，偶尔需要扩容（分配更大的数组，把旧数据复制过去）
- 中间插入 `O(n)`：需要把后面的元素往后移

```
vector 内部：
  [ptr] [size] [capacity]
    │
    ▼
  [1][2][3][4][5][ ][ ][ ]  ← 连续内存，capacity=8，size=5
```

**扩容策略**：当 `size == capacity` 时，`push_back` 会分配一块更大的内存（通常是当前容量的 2 倍），把旧数据移过去，释放旧内存。这就是为什么说尾部添加是 `O(1)` "均摊"——偶尔有 `O(n)` 的扩容操作，但平均下来还是 `O(1)`。

如果预先知道元素个数，用 `reserve` 避免重复扩容：

```cpp
std::vector<Token> tokens;
tokens.reserve(1000);  // 预分配 1000 个元素的空间
// 之后的 push_back 不会触发扩容（直到超过 1000）
```

### vector 在 Taco 里的应用

`vector` 在 Taco 里到处都是：

```cpp
// Token 列表
std::vector<Token> tokens = lexer.tokenize();

// AST 节点列表（Program 里的语句）
std::vector<ExprPtr> statements;

// 函数参数列表
std::vector<TacoValue> args;

// 作用域链（v2 的实现）
std::vector<std::unordered_map<std::string, TacoValue>> m_scopes;
```

v4 里，Taco 的 array 类型内部就用 `vector<TacoValue>` 实现：

```cpp
// Taco 的 array 值
using TacoArray = std::vector<TacoValue>;
```

---

## 21.2 deque、list：其他序列容器

### deque：双端队列

`std::deque<T>`（double-ended queue）在头部和尾部都能高效地添加/删除元素，但内存不连续（分段存储），随机访问比 `vector` 稍慢。

```cpp
#include <deque>

std::deque<int> dq = {1, 2, 3};

dq.push_back(4);   // 在尾部添加
dq.push_front(0);  // 在头部添加
dq.pop_front();    // 删除头部
dq.pop_back();     // 删除尾部

// 支持下标访问
int n = dq[1];
```

什么时候用 `deque` 而不是 `vector`：需要频繁在**头部**插入/删除时。`vector` 头部插入是 `O(n)`（所有元素要移动），`deque` 头部插入是 `O(1)`。

在 Taco 的词法分析里，REPL 的历史记录可以用 `deque` 管理（新条目加到头部，超过限制就从尾部删除）。

### list：双向链表

`std::list<T>` 是双向链表，每个节点单独分配内存，任意位置的插入/删除都是 `O(1)`（如果有迭代器指向那个位置），但不支持随机访问（不能用 `[]`），遍历也比 `vector` 慢（因为内存不连续，缓存不友好）。

```cpp
#include <list>

std::list<int> lst = {1, 2, 3, 4, 5};

lst.push_front(0);  // 头部添加
lst.push_back(6);   // 尾部添加

// 不支持 lst[2]
// 遍历必须用迭代器
for (int n : lst) {
    std::cout << n << "\n";
}

// 任意位置插入：先找到位置，再插入
auto it = lst.begin();
std::advance(it, 2);  // 移动到第 3 个元素
lst.insert(it, 99);   // 在这个位置插入 99
```

**实际使用**：`list` 在现代 C++ 里用得越来越少。因为 CPU 的缓存效果，即使是需要频繁中间插入的场景，`vector` 实际上往往也比 `list` 快（内存连续，缓存命中率高）。在大多数情况下，先用 `vector`，性能真的不够了再考虑 `list`。

---

## 21.3 map 与 unordered_map

### map：有序关联容器

`std::map<K, V>` 是基于红黑树实现的有序映射，键按照 `<` 运算符排序：

```cpp
#include <map>

std::map<std::string, int> word_count;

// 插入
word_count["hello"] = 1;
word_count["world"] = 2;
word_count.insert({"foo", 3});

// 访问
int n = word_count["hello"];   // 如果不存在，插入默认值！
int m = word_count.at("hello");  // 如果不存在，抛异常

// 查找
auto it = word_count.find("hello");
if (it != word_count.end()) {
    std::cout << it->first << ": " << it->second << "\n";
}

// 遍历（按键排序的顺序）
for (const auto& [key, value] : word_count) {
    std::cout << key << ": " << value << "\n";
}

// 删除
word_count.erase("hello");

// 大小
word_count.size();
word_count.count("hello");  // 存在返回 1，不存在返回 0
word_count.contains("hello");  // C++20，更直观
```

`map` 的特点：
- 键自动排序
- 查找、插入、删除：`O(log n)`
- 内存开销较大（每个节点单独分配，有指针开销）

### unordered_map：哈希映射

`std::unordered_map<K, V>` 基于哈希表，不保证顺序，但平均操作复杂度是 `O(1)`：

```cpp
#include <unordered_map>

std::unordered_map<std::string, int> env;

env["x"] = 10;
env["name"] = 42;

// 用法和 map 基本相同，但遍历顺序不确定
for (const auto& [key, value] : env) {
    std::cout << key << ": " << value << "\n";
}
```

`unordered_map` 的特点：
- 不排序
- 查找、插入、删除：平均 `O(1)`，最坏 `O(n)`（哈希冲突严重时）
- 内存效率比 `map` 稍好

**什么时候用哪个**：

- 需要有序遍历（比如打印所有变量按字母排序）：用 `map`
- 只需要快速查找，不关心顺序：用 `unordered_map`（更快）

Taco 的 `Environment` 用 `unordered_map<string, TacoValue>`——变量查找是热路径，需要尽可能快，不需要排序。

Taco 的关键字表（`Lexer::KEYWORDS`）也用 `unordered_map`——词法分析里频繁查找关键字。

### map 在 Taco 里的应用

Taco 语言的 `map` 类型（字典），内部用 `unordered_map<string, TacoValue>` 实现：

```cpp
// Taco 的 map 值
using TacoMap = std::unordered_map<std::string, TacoValue>;
```

```taco
// Taco 代码
var person = {"name": "Miguel", "age": 12};
print(person["name"]);
```

在 C++ 里对应：

```cpp
auto person = std::make_shared<TacoMap>();
(*person)["name"] = std::string("Miguel");
(*person)["age"] = 12.0;
```

---

## 21.4 set 与 unordered_set

### set：有序集合

`std::set<T>` 是有序集合，不允许重复元素：

```cpp
#include <set>

std::set<int> s = {3, 1, 4, 1, 5, 9, 2, 6};
// 自动去重并排序：{1, 2, 3, 4, 5, 6, 9}

s.insert(7);
s.erase(3);
bool exists = s.count(5) > 0;  // 或 s.contains(5)（C++20）

for (int n : s) {
    std::cout << n << "\n";  // 1 2 4 5 6 7 9
}
```

### unordered_set：哈希集合

`std::unordered_set<T>` 是哈希集合，不排序，平均 `O(1)` 查找：

```cpp
#include <unordered_set>

std::unordered_set<std::string> keywords = {
    "var", "func", "if", "else", "while", "for", "return"
};

bool is_keyword = keywords.count("var") > 0;  // true
bool is_keyword2 = keywords.count("print") > 0;  // false
```

在 Taco 词法分析器里，关键字集合可以用 `unordered_set` 或 `unordered_map`——前者只需要检查是否是关键字，后者同时能获取对应的 `TokenType`。

---

## 21.5 stack、queue、priority_queue

这三种是基于其他容器实现的适配器，提供特定的访问接口。

### stack：栈

`std::stack<T>` 是后进先出（LIFO）的栈，默认用 `deque` 实现：

```cpp
#include <stack>

std::stack<int> s;
s.push(1);
s.push(2);
s.push(3);

int top = s.top();   // 3（查看但不删除）
s.pop();             // 删除顶部元素
s.size();            // 2
s.empty();           // false
```

栈在 Taco 里的一个用途：解析器可以用栈来处理括号匹配，或者在将来实现字节码虚拟机时用作操作数栈。

### queue：队列

`std::queue<T>` 是先进先出（FIFO）的队列：

```cpp
#include <queue>

std::queue<std::string> q;
q.push("first");
q.push("second");
q.push("third");

std::string front = q.front();  // "first"
q.pop();                         // 删除 "first"
std::string next = q.front();   // "second"
```

v6 的 REPL 里，输入线程和求值线程之间可以用 `queue` 传递命令（配合 `mutex` 保护）。

### priority_queue：优先队列

`std::priority_queue<T>` 是堆实现的优先队列，默认是最大堆（最大的元素先出）：

```cpp
#include <queue>

std::priority_queue<int> pq;
pq.push(3);
pq.push(1);
pq.push(4);
pq.push(1);
pq.push(5);

while (!pq.empty()) {
    std::cout << pq.top() << " ";  // 5 4 3 1 1
    pq.pop();
}
```

最小堆：

```cpp
// 用 greater<int> 实现最小堆
std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
min_pq.push(3);
min_pq.push(1);
min_pq.push(4);
// min_pq.top() == 1
```

---

## 21.6 如何选择容器

选择容器的核心问题是：**这个容器最常做什么操作，那个操作的复杂度是多少？**

| 需求 | 推荐容器 | 理由 |
|------|----------|------|
| 按顺序存储，随机访问 | `vector` | O(1) 随机访问，内存连续 |
| 按顺序存储，频繁头部操作 | `deque` | 头尾都是 O(1) |
| 频繁任意位置插入/删除 | `list` | O(1) 插入（但遍历慢） |
| 键值映射，需要排序 | `map` | O(log n) 操作，有序 |
| 键值映射，不需要排序 | `unordered_map` | O(1) 平均，更快 |
| 去重，需要排序 | `set` | O(log n)，有序 |
| 去重，不需要排序 | `unordered_set` | O(1) 平均，更快 |
| 后进先出 | `stack` | 语义明确 |
| 先进先出 | `queue` | 语义明确 |
| 按优先级出队 | `priority_queue` | O(log n) 插入和取出 |

**经验规则**：如果不确定用什么，先用 `vector`。`vector` 在绝大多数场景下都是合理的选择，而且它的内存连续性让它有很好的缓存效果。性能真的不够了，再用性能分析工具找瓶颈，再换容器。

---

## *More About 容器*：底层数据结构与复杂度分析

> 第一次读可以跳过。

### vector 的摊还分析

`vector::push_back` 的平均 `O(1)` 复杂度可以用摊还分析证明。

假设当前容量是 $n$，每次扩容容量翻倍：
- 前 $n/2$ 次 `push_back`：每次 $O(1)$，共 $O(n/2)$
- 第 $n/2 + 1$ 次：触发扩容，复制 $n/2$ 个元素，$O(n/2)$
- 前 $n$ 次 `push_back`：共 $O(n/2) + O(n/2) = O(n)$，平均 $O(1)$

如果每次只扩容固定数量（比如 +100），摊还分析会得到 $O(n)$ 的平均复杂度。所以容量翻倍是最优策略。

### unordered_map 的哈希冲突

`unordered_map` 的 `O(1)` 是平均情况。最坏情况下，所有元素哈希到同一个桶，退化成链表，查找是 `O(n)`。

现代标准库的实现会在负载因子（元素数/桶数）超过阈值时自动扩容（重新哈希），保持平均 `O(1)`。

对于自定义类型，需要提供哈希函数：

```cpp
struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// 自定义哈希
struct PointHash {
    std::size_t operator()(const Point& p) const {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);  // 组合两个哈希
    }
};

std::unordered_map<Point, std::string, PointHash> map;
```

### map 的红黑树

`std::map` 基于红黑树（一种自平衡二叉搜索树）。红黑树保证树的高度是 $O(\log n)$，所以查找、插入、删除都是 $O(\log n)$。

红黑树的每个节点存储：
- 键值对 `(key, value)`
- 左右子节点指针
- 父节点指针
- 颜色（红或黑）

每个节点单独分配，内存不连续，这是 `map` 比 `unordered_map` 慢（对于随机查找）的原因之一——缓存命中率低。

---

## 小结

**`vector`** 是最常用的序列容器，内存连续，随机访问 $O(1)$，尾部添加摊还 $O(1)$。不确定用什么容器时，先用 `vector`。

**`deque`** 头尾添加都是 $O(1)$，适合需要双端操作的场景。

**`list`** 任意位置插入/删除 $O(1)$，但随机访问不支持，遍历也慢，实际上很少用。

**`map`** 有序键值映射，$O(\log n)$ 操作，需要排序时用。**`unordered_map`** 哈希映射，平均 $O(1)$，不需要排序时优先用。

**`set`/`unordered_set`** 分别对应有序和无序的集合。

**`stack`、`queue`、`priority_queue`** 是适配器，语义明确的特殊用途容器。

---

下一章讲迭代器——STL 容器和算法之间的桥梁，也是范围 for 循环的底层机制。
