# 第一章：C++ 是什么

---

## 1.1 从 C 到 C++：不只是"加了类的 C"

学过 C 的人第一次看到 C++ 代码，往往会有一种奇怪的感觉：熟悉，又陌生。

熟悉是因为 C++ 几乎完整地继承了 C 的语法。变量声明、指针、控制流、函数——这些东西在 C++ 里看起来和 C 里几乎一样。陌生是因为 C++ 里有很多 C 没有的东西：类、模板、引用、异常、标准库……而且这些东西不是孤立地堆在一起，而是互相配合，构成一套完整的系统。

有一种流传很广的说法：C++ 就是"加了类的 C"。这个说法在历史上有一定道理——C++ 的前身确实叫做"带类的 C"（C with Classes），由 Bjarne Stroustrup 在 1979 年开始设计。但如果真的把 C++ 理解成"C 加上一堆新东西"，就会走很多弯路。

C++ 和 C 的根本区别，不在于它多了什么语法，而在于它背后的**编程思想**不同。

---

### C 的思想：过程式编程

C 是一门过程式语言。写 C 程序，就是在描述计算机**做什么**：先分配内存，再读取数据，然后处理，最后释放内存。程序是一系列指令的集合，数据和操作数据的函数是分开的。

来看一个 C 里典型的例子——管理一个动态数组：

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int* data;
    int  size;
    int  capacity;
} IntArray;

void array_init(IntArray* arr, int capacity) {
    arr->data     = (int*)malloc(capacity * sizeof(int));
    arr->size     = 0;
    arr->capacity = capacity;
}

void array_push(IntArray* arr, int value) {
    if (arr->size >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = (int*)realloc(arr->data, arr->capacity * sizeof(int));
    }
    arr->data[arr->size++] = value;
}

void array_free(IntArray* arr) {
    free(arr->data);
    arr->data = NULL;
}

int main() {
    IntArray arr;
    array_init(&arr, 4);
    array_push(&arr, 10);
    array_push(&arr, 20);
    array_push(&arr, 30);
    printf("%d\n", arr.data[0]);
    array_free(&arr);
    return 0;
}
```

这段代码完全正确，也是标准的 C 风格。但注意几件事：

- `array_init` 和 `array_free` 必须手动配对调用——忘了调用 `array_free` 就会内存泄漏。
- 所有函数都需要显式地把 `IntArray*` 传进去——数据和函数是分离的。
- 如果想在别的地方复用这个数组，需要再传一个指针，稍有不慎就会出现悬空指针。

这些不是 C 的缺点，而是 C 的设计哲学决定的：C 相信程序员，把所有控制权都交给程序员，代价是程序员必须承担所有责任。

---

### C++ 的思想：抽象与零开销

C++ 的核心思想是：**让高层次的抽象和低层次的控制同时存在，而且抽象不能带来额外的性能代价**。

这个思想有一个名字，叫做**零开销抽象**（Zero-overhead abstraction）。Bjarne Stroustrup 对它的定义是：

> 你不使用的东西，不应该让你付出代价。你使用的东西，手写代码不可能比它更快。

来看同样的动态数组，用 C++ 来写：

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> arr;
    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    std::cout << arr[0] << "\n";
    return 0;
}
// arr 在这里自动释放，不需要手动 free
```

代码短了很多，而且不需要手动管理内存。`std::vector` 在内部做了和 C 版本完全一样的事情——动态分配、扩容、释放——但这些细节被封装在类里，使用者不需要关心。

更重要的是：这个封装**不会带来性能损失**。编译器优化后，`std::vector` 生成的机器码和手写的 C 版本几乎完全相同。这就是零开销抽象的含义。

C++ 的很多特性——类、模板、RAII、智能指针——都是围绕这个核心思想设计的。它们提供更高层次的抽象，让代码更安全、更易维护，但不牺牲性能。

---

### 为什么不直接用 C，或者直接用 Python？

这是一个合理的问题。既然 C 已经够快、够底层，为什么不直接用 C？既然 Python 已经够方便，为什么要用更复杂的 C++？

答案在于 C++ 同时解决了两个 C 和 Python 各自的问题：

**C 的问题**：没有语言层面的抽象工具。写大型程序时，所有资源管理、错误处理、数据结构都要手动实现，极易出错，维护困难。

**Python 的问题**：性能。Python 的动态类型、解释执行、垃圾回收，使得它在计算密集型任务上比 C++ 慢几十倍甚至几百倍。

C++ 的定位是：**系统级的性能，应用级的抽象能力**。操作系统、数据库、游戏引擎、编译器、浏览器、嵌入式系统——这些领域需要精确控制硬件，同时又需要管理极其复杂的代码逻辑。C++ 是目前能同时满足这两个需求的最主流的语言。

---

## 1.2 C++ 的设计哲学：零开销抽象

上一节提到了零开销抽象，这一节把它讲得更透彻一些。

---

### 抽象的代价

在大多数高级语言里，抽象是有代价的。

Python 的列表是一个抽象，使用起来很方便，但它比 C 的数组慢很多：每个元素都是一个 Python 对象，对象有引用计数，访问元素需要间接寻址，垃圾回收器需要定期扫描……这些开销在大多数业务场景里不重要，但在性能敏感的场景里就成了瓶颈。

Java 的对象是一个抽象，但在 Java 里，几乎所有东西都必须是对象，哪怕你只想要一个整数。Java 的虚拟机、垃圾回收、装箱/拆箱，都带来了额外开销。

C++ 的抽象设计原则不同：**抽象只有在被使用时才产生代价，而且这个代价应该等于手写同等功能的代码**。

---

### 三个具体的例子

**例子一：内联函数**

在 C 里，如果想让一个函数不产生函数调用开销，需要用宏：

```c
#define SQUARE(x) ((x) * (x))
```

宏很危险，`SQUARE(x++)` 会让 `x` 自增两次。C++ 提供了内联函数：

```cpp
inline int square(int x) {
    return x * x;
}
```

编译器会把 `square(5)` 直接替换成 `5 * 5`，没有函数调用开销，但比宏安全得多。这是零开销抽象的一个简单例子：提供了函数的安全性，保留了宏的性能。

**例子二：模板**

C 里如果想写一个"找数组最大值"的函数，需要为每种类型各写一个：

```c
int   max_int(int* arr, int n);
float max_float(float* arr, int n);
// ...
```

C++ 用模板解决这个问题：

```cpp
template <typename T>
T max_value(T* arr, int n) {
    T result = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > result) result = arr[i];
    }
    return result;
}
```

编译器会为每种类型生成一份专门的代码。使用 `max_value` 的地方，编译后的机器码和手写对应类型的版本完全相同——没有运行时的类型判断，没有额外的间接层。这就是零开销。

**例子三：RAII**

RAII（Resource Acquisition Is Initialization）是 C++ 里一个非常重要的惯用法，第七章会详细讲。这里先看一个直觉：

```cpp
{
    std::vector<int> arr;    // 分配内存
    arr.push_back(1);
    arr.push_back(2);
    // ... 做各种操作
}   // arr 的析构函数在这里自动调用，内存自动释放
```

`arr` 的生命周期和它所在的作用域绑定。离开作用域，内存自动释放。这个机制在语言层面实现，不需要垃圾回收器，没有运行时开销——释放内存的指令在编译时就已经确定了位置。

---

### 零开销的边界

需要说清楚的是，零开销抽象不是魔法。它的含义是：**用 C++ 抽象写出来的代码，性能下限等于用 C 手写同等功能的代码**。不是说 C++ 比 C 快，而是说 C++ 不比 C 慢。

如果滥用某些特性——比如过度使用虚函数、频繁动态分配内存——依然会有性能问题。C++ 给了工具，但怎么用还是取决于人。

---

## 1.3 现代 C++ 与古老 C++ 的区别

C++ 是一门有四十多年历史的语言。在这四十多年里，它经历了好几次重大的演变。如果去翻二十年前的 C++ 教材，会发现很多写法和今天完全不同——有些甚至是今天明确不推荐的做法。

这就是为什么需要区分**现代 C++** 和**古老 C++**。

---

### C++ 的标准版本

C++ 由国际标准化组织（ISO）负责标准化。每隔几年发布一个新标准：

| 标准 | 发布年份 | 俗称 |
|------|----------|------|
| C++98 | 1998 | 第一个正式标准 |
| C++03 | 2003 | 小幅修订 |
| C++11 | 2011 | 现代 C++ 的起点 |
| C++14 | 2014 | C++11 的补充 |
| C++17 | 2017 | 大量实用特性 |
| C++20 | 2020 | 概念、协程、模块 |
| C++23 | 2023 | 最新标准 |

C++11 是一个分水岭。Bjarne Stroustrup 曾说，C++11 感觉像一门全新的语言。在 C++11 之前，写 C++ 需要大量手动管理资源、写很多样板代码；C++11 之后，语言提供了更多工具，代码可以写得更简洁、更安全、更现代。

本书主要使用 **C++17**，并在合适的地方介绍 C++20 的特性。

---

### 古老 C++ 的样子

来看一段古老风格的 C++ 代码——在 C++11 之前，这种写法很常见：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// 函数对象（仿函数），用于比较
struct IsEven {
    bool operator()(int n) const {
        return n % 2 == 0;
    }
};

int main() {
    std::vector<int> nums;
    nums.push_back(1);
    nums.push_back(2);
    nums.push_back(3);
    nums.push_back(4);
    nums.push_back(5);

    // 找第一个偶数
    std::vector<int>::iterator it = std::find_if(nums.begin(), nums.end(), IsEven());
    if (it != nums.end()) {
        std::cout << "Found: " << *it << std::endl;
    }

    return 0;
}
```

这段代码能工作，但有几个让人皱眉的地方：

- `nums.push_back(1); nums.push_back(2); ...` 每次只能加一个元素，初始化很啰嗦。
- `std::vector<int>::iterator` 这个类型名极长，眼睛看着都累。
- 为了传递一个"是否是偶数"的判断，需要单独定义一个 `IsEven` 结构体。

---

### 现代 C++ 的样子

同样的代码，用现代 C++ 风格来写：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5};  // 初始化列表

    // lambda 表达式，直接在调用处定义匿名函数
    auto it = std::find_if(nums.begin(), nums.end(),
        [](int n) { return n % 2 == 0; });

    if (it != nums.end()) {
        std::cout << "Found: " << *it << "\n";
    }

    return 0;
}
```

差别显而易见：

- `{1, 2, 3, 4, 5}` 直接初始化，不需要一个个 `push_back`。
- `auto` 让编译器自动推导类型，不需要写出冗长的迭代器类型名。
- Lambda 表达式 `[](int n) { return n % 2 == 0; }` 直接在调用处定义匿名函数，不需要专门定义一个结构体。

---

### 现代 C++ 的几个核心变化

**1. 类型推导（auto）**

```cpp
// 古老风格
std::map<std::string, std::vector<int>>::iterator it = m.begin();

// 现代风格
auto it = m.begin();
```

`auto` 让编译器自动推导变量类型。类型信息不会丢失——编译器知道 `it` 的确切类型，只是不需要手动写出来。

**2. Lambda 表达式**

```cpp
// 古老风格：需要单独定义函数或函数对象
bool is_even(int n) { return n % 2 == 0; }

// 现代风格：直接在需要的地方定义
auto is_even = [](int n) { return n % 2 == 0; };
```

Lambda 让函数成为"一等公民"，可以像变量一样传递、返回、存储。

**3. 移动语义**

```cpp
std::vector<int> make_vector() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    return v;  // 现代 C++ 会"移动"而不是"拷贝"，零开销
}

auto v = make_vector();  // 没有不必要的数据复制
```

移动语义让对象的所有权可以高效地转移，避免不必要的拷贝。这是 C++11 最重要的特性之一，第十七章会深入讲解。

**4. 智能指针**

```cpp
// 古老风格：裸指针，容易泄漏
int* p = new int(42);
// ... 如果这里抛出异常，delete 永远不会执行
delete p;

// 现代风格：智能指针，自动管理生命周期
auto p = std::make_unique<int>(42);
// p 离开作用域时自动 delete，永远不会泄漏
```

智能指针把内存管理的责任从程序员转移到语言层面，几乎消除了内存泄漏的可能。

**5. 范围 for 循环**

```cpp
std::vector<int> nums = {1, 2, 3, 4, 5};

// 古老风格
for (std::vector<int>::iterator it = nums.begin(); it != nums.end(); ++it) {
    std::cout << *it << "\n";
}

// 现代风格
for (int n : nums) {
    std::cout << n << "\n";
}
```

---

### 抛弃什么，保留什么

学习现代 C++，意味着有些东西要主动不用：

**不再用的东西**：
- 裸指针管理动态内存（用智能指针代替）
- `NULL`（用 `nullptr` 代替）
- C 风格的类型转换如 `(int)x`（用 `static_cast<int>(x)` 代替）
- `printf`/`scanf`（用 `cout`/`cin` 或 `std::format` 代替）
- 手动 `new`/`delete`（用 RAII 和智能指针代替）

**依然重要的东西**：
- 指针的概念和底层内存模型（C 里学到的这些知识完全适用）
- 手动控制内存布局的能力（高性能场景依然需要）
- 对编译、链接过程的理解

有 C 基础的优势，在于对底层的感觉——知道指针是什么、内存是如何工作的、函数调用在栈上发生了什么。这些知识在 C++ 里依然有用，只是不需要时刻手动管理这一切了。

---

## 1.4 第一个程序：Hello, World

理论讲够了。来写第一个 C++ 程序。

---

### 环境准备

在写代码之前，需要一个编译器和一个编辑器。

**编译器**推荐以下两种：

- **GCC**：Linux 上最常见的选择，macOS 上可以通过 Homebrew 安装。
- **Clang**：macOS 上推荐，错误信息比 GCC 更友好，也支持 Linux。

安装完成后，在终端运行：

```bash
g++ --version
# 或者
clang++ --version
```

如果看到版本号，说明安装成功。本书的所有代码都需要支持 C++17，所以编译器版本建议：GCC 7 以上，Clang 5 以上。

**编辑器**推荐 VS Code，配合 C/C++ 插件和 clangd 插件，可以获得语法高亮、代码补全、错误提示等功能。当然，用 Vim、Emacs、CLion 都可以，工具的选择不影响学习。

---

### Hello, World

新建一个文件，命名为 `hello.cpp`：

```cpp
#include <iostream>

int main() {
    std::cout << "Hello, World!\n";
    return 0;
}
```

在终端里编译并运行：

```bash
g++ -std=c++17 hello.cpp -o hello
./hello
```

输出：

```
Hello, World!
```

---

### 逐行解释

```cpp
#include <iostream>
```

`#include` 是预处理指令，告诉编译器把 `iostream` 头文件的内容插入到这里。`iostream` 提供了输入输出的功能——`std::cout` 就来自这里。

注意 C++ 标准库的头文件没有 `.h` 后缀，而 C 的头文件有（如 `stdio.h`）。在 C++ 里用 C 的库，会使用 `<cstdio>` 这样的形式（加 `c` 前缀，去掉 `.h`）。

```cpp
int main() {
```

`main` 函数是程序的入口点，这和 C 一样。返回 `int` 类型——按惯例，返回 0 表示程序正常结束，返回非零表示出错。

```cpp
    std::cout << "Hello, World!\n";
```

`std::cout` 是标准输出流，对应屏幕。`<<` 是流插入运算符，把右边的内容发送到左边的流里。`"Hello, World!\n"` 是一个字符串字面量，`\n` 是换行符。

`std::` 是命名空间前缀。C++ 标准库里的所有东西都在 `std` 命名空间里，使用时需要加上 `std::` 前缀。这是为了避免名字冲突——如果自己定义了一个叫 `cout` 的变量，它不会和 `std::cout` 产生冲突。

```cpp
    return 0;
```

返回 0，表示程序正常结束。在 `main` 函数里，`return 0` 可以省略——编译器会自动在 `main` 末尾加上。但写出来是个好习惯。

---

### using namespace std

很多教材会在 `main` 前面加一行：

```cpp
using namespace std;
```

加了这行之后，可以直接写 `cout` 而不用写 `std::cout`。这样代码看起来更短，但有一个问题：它把 `std` 命名空间里的所有名字都引入了当前作用域，很容易造成名字冲突，在大型项目里是不推荐的做法。

本书的代码统一使用 `std::` 前缀，不使用 `using namespace std`。偶尔会用 `using std::cout;` 这种针对单个名字的引入，这是安全的。

---

### 编译选项

上面的编译命令是：

```bash
g++ -std=c++17 hello.cpp -o hello
```

解释一下各个选项：

- `-std=c++17`：指定使用 C++17 标准。如果不加，编译器可能默认使用更旧的标准。
- `hello.cpp`：源文件名。
- `-o hello`：指定输出文件名。不加 `-o` 的话，默认输出到 `a.out`。

还有几个有用的选项，以后会用到：

- `-Wall`：开启常见警告。强烈建议加上，很多 bug 可以在警告里发现。
- `-Wextra`：开启更多警告。
- `-O2`：开启优化，生成的代码更快。调试时不加，发布时加。
- `-g`：生成调试信息，配合调试器使用。

建议在开发时使用：

```bash
g++ -std=c++17 -Wall -Wextra -g hello.cpp -o hello
```

---

### 一个稍微复杂一点的例子

只有 Hello World 太单调了。来看一个稍微体现 C++ 特色的例子：

```cpp
#include <iostream>
#include <vector>
#include <string>

int main() {
    // vector 是 C++ 标准库里的动态数组
    std::vector<std::string> languages = {"C", "C++", "Python"};

    std::cout << "Languages I know:\n";

    // 范围 for 循环，遍历容器
    for (const std::string& lang : languages) {
        std::cout << "  - " << lang << "\n";
    }

    std::cout << "Total: " << languages.size() << "\n";

    return 0;
}
```

输出：

```
Languages I know:
  - C
  - C++
  - Python
Total: 3
```

这段代码展示了几个 C++ 的特色：

- `std::vector<std::string>`：一个存放字符串的动态数组，不需要指定大小，不需要手动管理内存。
- `{"C", "C++", "Python"}`：初始化列表，直接在声明时初始化容器。
- `for (const std::string& lang : languages)`：范围 for 循环，`const std::string&` 表示"常量引用"，避免不必要的字符串拷贝。
- `languages.size()`：容器的成员函数，返回元素个数。不需要手动维护一个 `count` 变量。

这些特性的细节在后面的章节里都会详细讲到。现在只需要对它们有个直观的印象。

---

### 和 C 的对比

来并排看看同样的功能在 C 和 C++ 里分别怎么写：

**C 版本**：

```c
#include <stdio.h>

int main() {
    const char* languages[] = {"C", "C++", "Python"};
    int count = 3;

    printf("Languages I know:\n");
    for (int i = 0; i < count; i++) {
        printf("  - %s\n", languages[i]);
    }
    printf("Total: %d\n", count);

    return 0;
}
```

**C++ 版本**（上面那个）：

```cpp
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<std::string> languages = {"C", "C++", "Python"};
    // ...
}
```

两个版本都能工作，输出相同。区别在于：

- C 版本需要手动维护 `count`，数组越界是程序员的责任。
- C++ 版本的 `vector` 自动管理大小，`size()` 永远正确。
- C 版本用 `const char*`，是 C 风格的字符串，处理起来容易出错。
- C++ 版本用 `std::string`，是一个完整的字符串类，有丰富的操作。

这个对比在这本书里会反复出现：C 给控制权，C++ 给安全性，而且在现代 C++ 里，这两者不是对立的。

---

## 小结

这一章讲了三件事：

**C++ 不是"加了类的 C"**。它有自己的设计哲学——零开销抽象。在这个哲学下，高层次的抽象和低层次的性能可以同时存在。

**现代 C++ 和古老 C++ 差别很大**。C++11 是分水岭。本书使用现代 C++，会主动避免一些过时的写法。

**C 的底层知识依然有用**。对内存、指针、编译过程的理解，是学习 C++ 的优势。不同的是，C++ 提供了更多工具，让这些底层操作更安全。

---

下一章，开始搭建开发环境，引入构建系统 CMake，并为后面的主线项目——Taco 语言解释器——做准备。
