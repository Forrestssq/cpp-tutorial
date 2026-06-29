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
# 第二章：构建系统与第三方库

---

上一章写的 `hello.cpp` 只有一个文件，直接用 `g++` 编译很方便。但真实的项目不是这样的——Taco 解释器最终会有几十个文件，还要依赖第三方库。这时候，手动敲编译命令就变得不现实了。

这一章解决一个实际问题：**怎么管理一个真实的 C++ 项目**。

---

## 2.1 为什么需要构建系统

先来感受一下"没有构建系统"是什么感觉。

假设项目有三个文件：

```
src/
  main.cpp
  lexer.cpp
  lexer.h
```

手动编译：

```bash
g++ -std=c++17 src/main.cpp src/lexer.cpp -o taco
```

还好，两个文件。但如果有二十个文件：

```bash
g++ -std=c++17 src/main.cpp src/lexer.cpp src/parser.cpp \
    src/evaluator.cpp src/environment.cpp src/value.cpp \
    src/ast.cpp src/token.cpp src/error.cpp src/builtin.cpp \
    ... -o taco
```

每次修改一个文件，都要重新编译所有文件。如果项目有五十个文件，编译一次要等很长时间，哪怕只改了一行代码。

构建系统解决的核心问题是：**只重新编译发生变化的文件**。改了 `lexer.cpp`，只重新编译 `lexer.cpp`，然后重新链接，而不是把所有文件都编译一遍。

除此之外，构建系统还解决：

- 不同平台（Mac、Linux、Windows）上编译命令不同的问题
- 管理第三方库的依赖关系
- 配置编译选项（Debug 模式、Release 模式）
- 生成 IDE 的项目文件

C++ 世界里有很多构建系统：Make、CMake、Bazel、Meson……其中 CMake 是目前最主流的选择，几乎所有主流的 C++ 开源项目都用它。

---

## 2.2 CMake 基础：最小可用配置

CMake 本身不是编译器，而是一个**元构建系统**：它读取项目的描述文件，生成对应平台的构建文件（在 Linux/Mac 上通常生成 Makefile，在 Windows 上可以生成 Visual Studio 项目），然后由那个构建文件驱动实际的编译。

整个流程是：

```
CMakeLists.txt → CMake → Makefile → Make → 编译器 → 可执行文件
```

---

### 安装 CMake

**macOS**：

```bash
brew install cmake
```

**Ubuntu/Debian**：

```bash
sudo apt install cmake
```

安装完成后验证：

```bash
cmake --version
# cmake version 3.x.x
```

本书需要 CMake 3.14 以上版本。

---

### 第一个 CMakeLists.txt

CMake 的配置文件叫 `CMakeLists.txt`，放在项目根目录。

创建如下项目结构：

```
taco/
  CMakeLists.txt
  src/
    main.cpp
```

`src/main.cpp`：

```cpp
#include <iostream>

int main() {
    std::cout << "Taco 0.1.0\n";
    return 0;
}
```

`CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(taco src/main.cpp)
```

逐行解释：

```cmake
cmake_minimum_required(VERSION 3.14)
```

声明这个项目需要至少 3.14 版本的 CMake。这是防止用户用太旧的 CMake 构建项目时出现莫名其妙的错误。

```cmake
project(taco VERSION 0.1.0 LANGUAGES CXX)
```

声明项目名称是 `taco`，版本是 `0.1.0`，使用的语言是 C++（CMake 里 C++ 写作 `CXX`）。

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

设置 C++ 标准为 C++17，并且要求编译器必须支持这个标准（如果编译器不支持，CMake 会报错而不是静默降级）。

```cmake
add_executable(taco src/main.cpp)
```

告诉 CMake：用 `src/main.cpp` 构建一个名为 `taco` 的可执行文件。

---

### 构建项目

CMake 的标准构建流程是**外部构建**（out-of-source build）：把构建产物放在一个单独的目录里，不污染源代码目录。

```bash
cd taco           # 进入项目根目录
mkdir build       # 创建构建目录
cd build          # 进入构建目录
cmake ..          # 运行 CMake，.. 表示源代码在上一级目录
make              # 执行实际的编译
./taco            # 运行程序
```

或者用更现代的方式，不需要进入 build 目录：

```bash
cmake -B build    # 在 build 目录里生成构建文件
cmake --build build  # 编译
./build/taco      # 运行
```

第一次运行 `cmake ..` 会看到很多输出，CMake 在检测系统环境、编译器版本等信息。之后再运行就快很多了。

---

### 多文件项目

现在把项目拆成多个文件：

```
taco/
  CMakeLists.txt
  src/
    main.cpp
    lexer.cpp
    lexer.h
```

`src/lexer.h`：

```cpp
#pragma once
#include <string>
#include <vector>

enum class TokenType {
    Number,
    String,
    Identifier,
    Plus,
    EndOfFile,
};

struct Token {
    TokenType type;
    std::string value;
};

std::vector<Token> tokenize(const std::string& source);
```

`src/lexer.cpp`：

```cpp
#include "lexer.h"

std::vector<Token> tokenize(const std::string& source) {
    // 暂时返回空列表，第六章会实现完整版本
    return {};
}
```

`src/main.cpp`：

```cpp
#include <iostream>
#include "lexer.h"

int main() {
    auto tokens = tokenize("var x = 10;");
    std::cout << "Taco 0.1.0\n";
    std::cout << "Tokens: " << tokens.size() << "\n";
    return 0;
}
```

更新 `CMakeLists.txt`，添加新文件：

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(taco
    src/main.cpp
    src/lexer.cpp
)

target_include_directories(taco PRIVATE src)
```

新增的 `target_include_directories` 告诉编译器去 `src` 目录里找头文件，这样 `#include "lexer.h"` 才能找到正确的文件。

重新构建：

```bash
cmake --build build
./build/taco
```

---

### Debug 和 Release 模式

CMake 支持不同的构建类型：

```bash
# Debug 模式：包含调试信息，不优化
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release 模式：开启优化，去除调试信息
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

开发时用 Debug，发布时用 Release。两者编译出来的程序在性能上可能差几倍。

---

## 2.3 引入第三方库：FetchContent 与 vcpkg

真实项目几乎都会用到第三方库。C++ 的包管理历来是一个痛点——没有像 Python 的 `pip` 或 JavaScript 的 `npm` 那样的统一工具。不过近年来情况有所改善，主要有两种方案：**CMake 内置的 FetchContent** 和 **vcpkg**。

---

### FetchContent：直接从源码构建

FetchContent 是 CMake 3.11 引入的功能，可以在构建时自动下载并编译第三方库的源码。

假设要引入 `nlohmann/json`（一个流行的 JSON 库），在 `CMakeLists.txt` 里：

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引入 FetchContent 模块
include(FetchContent)

# 声明要下载的库
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.2
)

# 下载并使其可用
FetchContent_MakeAvailable(nlohmann_json)

add_executable(taco src/main.cpp)

# 链接库
target_link_libraries(taco PRIVATE nlohmann_json::nlohmann_json)
```

第一次构建时，CMake 会自动从 GitHub 下载这个库并编译。之后的构建会使用缓存，不需要再次下载。

在代码里就可以直接使用了：

```cpp
#include <iostream>
#include <nlohmann/json.hpp>

int main() {
    nlohmann::json j = {{"name", "Miguel"}, {"age", 12}};
    std::cout << j.dump(2) << "\n";
    return 0;
}
```

FetchContent 的优点是简单，不需要额外安装工具，缺点是每个项目单独下载和编译依赖，如果依赖很多，构建时间会很长。

---

### vcpkg：集中管理的包管理器

vcpkg 是 Microsoft 开发的 C++ 包管理器，可以集中管理所有项目的依赖，避免重复下载和编译。

**安装 vcpkg**：

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh   # macOS/Linux
```

**安装库**：

```bash
./vcpkg install nlohmann-json
./vcpkg install fmt
./vcpkg install catch2
```

**在 CMake 里使用**：

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

然后在 `CMakeLists.txt` 里正常使用 `find_package` 即可：

```cmake
find_package(nlohmann_json REQUIRED)
target_link_libraries(taco PRIVATE nlohmann_json::nlohmann_json)
```

vcpkg 的优点是库版本统一，依赖关系清晰，适合有多个项目的情况。缺点是需要单独安装和维护。

**本书的选择**：Taco 解释器使用 FetchContent，因为它不需要额外安装工具，对于单个项目足够用。读者在自己的项目里可以根据需要选择 vcpkg。

---

## 2.4 静态库与动态库的区别

在引入第三方库之前，需要理解库的两种基本形态：**静态库**和**动态库**。这个区别会影响到最终程序的大小、启动速度和依赖关系。

---

### 静态库

静态库（Static Library）在链接时被直接嵌入到可执行文件里。

文件扩展名：
- Linux/macOS：`.a`（archive）
- Windows：`.lib`

编译生成静态库：

```bash
# 先编译成目标文件
g++ -c lexer.cpp -o lexer.o

# 打包成静态库
ar rcs liblexer.a lexer.o
```

链接时使用静态库：

```bash
g++ main.cpp -L. -llexer -o taco
```

`-L.` 告诉链接器在当前目录找库，`-llexer` 表示链接名为 `liblexer.a` 的库（去掉 `lib` 前缀和 `.a` 后缀）。

**静态链接的结果**：`taco` 这个可执行文件包含了 `liblexer.a` 里的所有代码。把 `taco` 复制到另一台机器上，不需要任何其他文件就能运行。

---

### 动态库

动态库（Dynamic Library / Shared Library）在程序运行时才被加载，不嵌入到可执行文件里。

文件扩展名：
- Linux：`.so`（shared object）
- macOS：`.dylib`（dynamic library）
- Windows：`.dll`（dynamic link library）

动态链接的结果：`taco` 可执行文件本身很小，但运行时需要动态库文件存在于系统的某个固定位置。如果动态库不存在或版本不对，程序会在启动时报错：

```
error while loading shared libraries: libxxx.so.1: cannot open shared object file
```

这就是"DLL Hell"问题的根源——同一个库的不同版本之间不兼容，程序需要特定版本的库。

---

### 对比

| | 静态库 | 动态库 |
|--|--------|--------|
| 可执行文件大小 | 大 | 小 |
| 运行时依赖 | 无 | 需要库文件 |
| 内存占用 | 每个进程一份 | 多个进程共享 |
| 更新库 | 需要重新编译程序 | 替换库文件即可 |
| 部署 | 简单，单文件 | 复杂，需要管理依赖 |

**Taco 使用静态库**。对于一个命令行工具来说，单文件部署更方便，用户下载一个可执行文件就能运行，不需要安装任何依赖。

在 CMake 里，FetchContent 下载的库默认构建为静态库。可以通过设置选项改变：

```cmake
set(BUILD_SHARED_LIBS OFF)  # 强制使用静态库
```

---

## 2.5 头文件与链接：底层发生了什么

很多人学了很久 C/C++ 还是对编译过程感到困惑：头文件是什么？为什么要 `#include`？链接器是做什么的？为什么有时候会出现"undefined reference"错误？

这一节把这个过程讲清楚。

---

### 编译的四个阶段

一个 `.cpp` 文件变成可执行程序，要经过四个阶段：

```
源文件(.cpp)
    ↓ 预处理器
预处理后的源文件
    ↓ 编译器
汇编文件(.s)
    ↓ 汇编器
目标文件(.o)
    ↓ 链接器
可执行文件
```

**阶段一：预处理**

预处理器处理所有以 `#` 开头的指令：

- `#include <iostream>`：把 `iostream` 头文件的内容原封不动地插入到这里
- `#define MAX 100`：把后面所有的 `MAX` 替换成 `100`
- `#ifdef`/`#endif`：条件编译

预处理是纯文本操作，不理解 C++ 语法。`#include` 就是简单的文本拼接。

可以用 `-E` 选项看预处理后的结果：

```bash
g++ -E main.cpp -o main.i
```

会发现 `main.i` 比 `main.cpp` 大很多——因为 `<iostream>` 被展开进去了，展开后可能有几万行。

**阶段二：编译**

编译器把预处理后的 C++ 代码翻译成汇编代码。这一步会做语法检查、类型检查、语义分析，然后生成汇编指令。

**阶段三：汇编**

汇编器把汇编代码转换成机器码，生成目标文件（`.o` 文件）。目标文件里包含机器码，但还不是可执行文件——因为它可能引用了其他文件里定义的函数，这些引用还没有被解析。

**阶段四：链接**

链接器把多个目标文件和库文件合并成一个可执行文件。它的核心工作是**解析符号引用**：如果 `main.o` 里调用了 `tokenize` 函数，链接器就去其他目标文件里找 `tokenize` 的定义，找到了就把调用地址填进去。

如果找不到，就会出现：

```
undefined reference to 'tokenize(std::string const&)'
```

这是最常见的链接错误。

---

### 头文件的作用

头文件解决的是**跨文件引用**的问题。

假设 `lexer.cpp` 里定义了 `tokenize` 函数，`main.cpp` 想调用它。在调用之前，编译器需要知道 `tokenize` 的**声明**——函数的名字、参数类型、返回类型。有了声明，编译器才能检查调用是否正确。

声明和定义是不同的：

```cpp
// 声明：告诉编译器"这个函数存在，长这样"
std::vector<Token> tokenize(const std::string& source);

// 定义：函数的实际实现
std::vector<Token> tokenize(const std::string& source) {
    // ... 实际代码
}
```

头文件就是存放**声明**的地方。`main.cpp` 通过 `#include "lexer.h"` 把声明插入进来，编译器就知道 `tokenize` 的存在了。实际的定义在 `lexer.cpp` 里，链接阶段才会被找到并连接。

---

### 为什么不能在头文件里放定义？

这是一个常见的疑问。答案是：可以放，但大多数情况下不应该放。

如果在头文件里放了函数定义，每个 `#include` 这个头文件的 `.cpp` 文件都会有一份这个函数的拷贝。链接时，链接器发现同一个函数有多个定义，就会报错：

```
multiple definition of 'tokenize'
```

有两个例外：

- **内联函数**（`inline` 关键字）：允许在头文件里定义，每个编译单元有一份，链接器会处理重复。
- **模板函数/类**：模板必须在头文件里定义，因为编译器需要在实例化时看到完整的定义。

---

### 实际的项目结构

对于 Taco 项目，推荐的结构是：

```
taco/
  CMakeLists.txt
  src/
    main.cpp        # 程序入口
    lexer.h         # Lexer 的声明
    lexer.cpp       # Lexer 的实现
    parser.h        # Parser 的声明
    parser.cpp      # Parser 的实现
    ...
  tests/
    test_lexer.cpp  # 测试文件
```

每个模块一对 `.h` 和 `.cpp` 文件。头文件放声明，源文件放实现。

头文件里的 `#pragma once` 是防止重复包含的保护措施：

```cpp
#pragma once  // 如果这个文件已经被包含过，就忽略后面的内容

#include <string>
#include <vector>

// 声明...
```

它等价于传统的 include guard：

```cpp
#ifndef LEXER_H
#define LEXER_H
// 内容...
#endif
```

`#pragma once` 更简洁，现代编译器都支持，推荐使用。

---

## 2.6 引入 ftxui：把它跑起来

现在把所有内容结合起来，为 Taco 项目引入第一个真正的第三方库——ftxui。

ftxui（Functional Terminal UI）是一个用 C++17 写的终端 UI 库，声明式风格，和现代 C++ 配合很自然。Taco 的 REPL 界面就用它来构建。

---

### 更新 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用静态库
set(BUILD_SHARED_LIBS OFF)

include(FetchContent)

# 引入 ftxui
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v5.0.0
)
FetchContent_MakeAvailable(ftxui)

add_executable(taco src/main.cpp)

target_include_directories(taco PRIVATE src)

target_link_libraries(taco
    PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
)
```

ftxui 有三个组件：
- `ftxui::screen`：底层屏幕渲染
- `ftxui::dom`：声明式 UI 元素
- `ftxui::component`：交互式组件（输入框、按钮等）

---

### 第一个 ftxui 程序

`src/main.cpp`：

```cpp
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>

int main() {
    using namespace ftxui;

    // 声明式地描述界面
    auto document = vbox({
        text("🌮 Taco 0.1.0") | bold,
        separator(),
        text("It works on my machine.") | dim,
    });

    // 创建屏幕并渲染
    auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
    Render(screen, document);
    screen.Print();

    return 0;
}
```

构建并运行：

```bash
cmake -B build
cmake --build build
./build/taco
```

第一次构建会比较慢，因为 CMake 需要下载并编译 ftxui。之后的构建只会编译修改过的文件。

输出大概是：

```
🌮 Taco 0.1.0
──────────────
It works on my machine.
```

---

### 理解 ftxui 的声明式风格

ftxui 的核心思想是：描述界面**应该是什么**，而不是**怎么画**。

```cpp
auto document = vbox({          // 垂直排列
    text("Hello") | bold,       // 加粗文字
    hbox({                      // 水平排列
        text("Left"),
        filler(),               // 填充空间
        text("Right"),
    }),
    separator(),                // 分隔线
    gauge(0.7),                 // 进度条，70%
});
```

每个 `text`、`separator`、`gauge` 都是一个 UI 元素，可以用 `|` 操作符添加修饰（`bold`、`dim`、`color(...)`），用 `vbox`/`hbox` 组合。

这和 SwiftUI 的声明式思想非常相似——描述结构，而不是命令式地调用绘图函数。

---

### 添加交互

ftxui 不只能渲染静态界面，还支持交互：

```cpp
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>

int main() {
    using namespace ftxui;

    std::string input_text;
    std::string output_text = "Type something and press Enter.";

    auto input = Input(&input_text, "input...");

    auto component = Renderer(input, [&] {
        return vbox({
            text("🌮 Taco REPL") | bold,
            separator(),
            text(output_text),
            separator(),
            hbox({text("> "), input->Render()}),
        });
    });

    // 处理 Enter 键
    component = CatchEvent(component, [&](Event event) {
        if (event == Event::Return) {
            output_text = "You typed: " + input_text;
            input_text.clear();
            return true;
        }
        return false;
    });

    auto screen = ScreenInteractive::TerminalOutput();
    screen.Loop(component);

    return 0;
}
```

这个程序会显示一个交互式输入框，输入文字按 Enter，上方显示刚才输入的内容。这就是 Taco REPL 的雏形。

---

### 构建时间太长怎么办

ftxui 第一次构建确实需要一些时间（通常几分钟）。有几个办法可以加快：

**使用 ccache**：ccache 缓存编译结果，下次构建时如果文件没变，直接用缓存：

```bash
# 安装
brew install ccache    # macOS
sudo apt install ccache  # Ubuntu

# 在 CMakeLists.txt 里启用
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()
```

**只构建当前目标**：

```bash
cmake --build build --target taco
```

只构建 `taco`，不构建其他目标。

**并行构建**：

```bash
cmake --build build --parallel 4  # 用 4 个线程并行编译
```

---

## 小结

这一章解决了一个实际问题：怎么管理真实的 C++ 项目。

**构建系统**解决了手动编译的低效问题。CMake 是目前 C++ 世界里的主流选择，它不是编译器，而是生成构建文件的工具。

**第三方库**的引入有两种主流方式：FetchContent（简单，适合单项目）和 vcpkg（集中管理，适合多项目）。Taco 使用 FetchContent。

**头文件和链接**的底层逻辑：头文件放声明，源文件放定义，链接器负责把声明和定义连接起来。理解这个机制，就能看懂"undefined reference"这类错误。

**ftxui** 是 Taco 的 UI 层，声明式风格，和现代 C++ 配合自然。这一章把它跑起来了。

---

下一章开始讲 C++ 的基础语法升级——那些 C 里没有的、让 C++ 写起来更舒服的东西。
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

在类里，`const` 可以用来修饰成员函数，表示这个函数不会修改对象的状态：

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
# 第四章：字符串与输入输出

---

Taco 解释器的核心工作之一，是读取用户输入的代码（字符串），处理它，然后输出结果。这一章讲 C++ 里处理字符串和输入输出的工具。

C++ 在这个领域提供了两套东西：一套是从 C 继承来的（`char*`、`printf`、`scanf`），另一套是 C++ 自己的（`std::string`、`iostream`）。本书优先使用 C++ 的方式，但也会解释 C 的遗产，因为真实代码里两者都会遇到。

---

## 4.1 std::string vs C 字符串

### C 字符串的问题

C 里的字符串是 `char` 数组，以空字符 `'\0'` 结尾：

```c
char name[] = "Miguel";
// 实际存储：['M', 'i', 'g', 'u', 'e', 'l', '\0']
```

C 字符串的问题很多：

```c
// 拼接需要手动计算长度
char result[100];
strcpy(result, "Hello, ");
strcat(result, "World");  // 如果 result 太小，缓冲区溢出！

// 比较不能用 ==
char a[] = "hello";
char b[] = "hello";
if (a == b) { ... }  // 比较的是指针地址，不是内容！
if (strcmp(a, b) == 0) { ... }  // 正确方式，但很别扭

// 获取长度需要遍历
int len = strlen(name);  // O(n) 操作
```

缓冲区溢出（buffer overflow）是 C 程序里最常见的安全漏洞之一——往一个太小的数组里写入太多数据，会覆盖相邻的内存，导致程序崩溃或被攻击者利用。

---

### std::string：真正的字符串类

`std::string` 解决了 C 字符串的所有主要问题：

```cpp
#include <string>

std::string name = "Miguel";

// 拼接用 +
std::string greeting = "Hello, " + name + "!";

// 比较用 ==
if (name == "Miguel") { ... }  // 比较内容，符合直觉

// 长度是 O(1)
std::cout << name.size() << "\n";  // 6

// 不会溢出，自动管理内存
name += " Rivera";  // 自动扩展
```

---

### std::string 的常用操作

```cpp
std::string s = "Hello, World!";

// 长度
s.size();      // 13
s.length();    // 同上，两个函数等价
s.empty();     // false

// 访问字符
s[0];          // 'H'，不做边界检查
s.at(0);       // 'H'，做边界检查，越界抛异常

// 子串
s.substr(7, 5);      // "World"，从下标 7 开始，取 5 个字符
s.substr(7);         // "World!"，从下标 7 到结尾

// 查找
s.find("World");     // 7，返回第一次出现的位置
s.find("world");     // std::string::npos，找不到

// 是否包含（C++23 才有 contains，C++17 用 find）
s.find("World") != std::string::npos;  // true

// 替换
s.replace(7, 5, "Taco");  // "Hello, Taco!"

// 首尾操作
s.front();     // 'H'
s.back();      // '!'

// 插入和删除
s.insert(7, "Beautiful ");  // 在下标 7 处插入
s.erase(7, 10);             // 从下标 7 删除 10 个字符

// 转换大小写（标准库没有直接提供，需要借助算法）
#include <algorithm>
#include <cctype>
std::transform(s.begin(), s.end(), s.begin(), ::toupper);
```

---

### 字符串与数字的转换

```cpp
// 数字转字符串
std::string s = std::to_string(42);      // "42"
std::string s2 = std::to_string(3.14);   // "3.140000"

// 字符串转数字
int n = std::stoi("42");        // 42
double d = std::stod("3.14");   // 3.14
long l = std::stol("1000000");  // 1000000

// 非法输入会抛异常
try {
    int bad = std::stoi("hello");  // 抛出 std::invalid_argument
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
}
```

---

### 字符串字面量的类型

有几种不同的字符串字面量：

```cpp
"hello"           // const char*，C 风格字符串
"hello"s          // std::string（需要 using namespace std::string_literals）
R"(hello\nworld)" // 原始字符串字面量，\n 不会被转义
u8"你好"          // UTF-8 编码的字符串
```

原始字符串字面量在写正则表达式或 JSON 时很有用：

```cpp
// 普通字符串，需要大量转义
std::string json = "{ \"name\": \"Miguel\", \"age\": 12 }";

// 原始字符串，不需要转义
std::string json = R"({ "name": "Miguel", "age": 12 })";
```

---

### C 字符串和 std::string 的互转

实际项目里经常需要在两者之间转换：

```cpp
// std::string → const char*
std::string s = "hello";
const char* cstr = s.c_str();  // 返回 C 风格字符串指针

// const char* → std::string
const char* cstr2 = "world";
std::string s2 = cstr2;  // 隐式转换，直接赋值即可
std::string s3(cstr2);   // 或者用构造函数
```

注意：`c_str()` 返回的指针指向 `std::string` 内部的数据，`std::string` 对象销毁后，这个指针就变成悬空指针，不能再使用。

---

## 4.2 iostream：cin 与 cout

### cout：标准输出

`std::cout` 是标准输出流，通常对应终端屏幕：

```cpp
#include <iostream>

std::cout << "Hello\n";           // 输出字符串
std::cout << 42 << "\n";         // 输出整数
std::cout << 3.14 << "\n";       // 输出浮点数
std::cout << true << "\n";       // 输出 1（bool 默认输出 0/1）

// 链式输出
std::cout << "Name: " << "Miguel" << ", Age: " << 12 << "\n";
```

`"\n"` 和 `std::endl` 的区别：

```cpp
std::cout << "Hello\n";        // 换行
std::cout << "Hello" << std::endl;  // 换行 + 刷新缓冲区
```

`std::endl` 会刷新输出缓冲区，确保内容立刻显示在屏幕上。在大多数情况下，`"\n"` 就够了，而且更快——频繁刷新缓冲区会降低性能。只在确实需要立刻看到输出时（比如打印调试信息后程序崩溃了）才用 `std::endl`。

---

### cin：标准输入

`std::cin` 读取用户输入：

```cpp
int n;
std::cin >> n;  // 读取一个整数

std::string name;
std::cin >> name;  // 读取一个单词（遇到空格停止）

std::string line;
std::getline(std::cin, line);  // 读取一整行（包括空格）
```

`>>` 运算符会跳过前导空白（空格、换行、制表符），然后读取对应类型的数据。`std::getline` 读取一整行，直到遇到换行符（换行符本身被丢弃）。

一个常见的陷阱：混合使用 `>>` 和 `getline` 时，`>>` 不会消耗换行符，导致 `getline` 读到一个空行：

```cpp
int n;
std::cin >> n;           // 读取数字，但换行符留在缓冲区里

std::string line;
std::getline(std::cin, line);  // 读到空行！因为缓冲区里还有换行符
```

解决方法：在用 `getline` 之前，先忽略残留的换行符：

```cpp
std::cin >> n;
std::cin.ignore();  // 忽略一个字符（换行符）
std::getline(std::cin, line);  // 现在才能正确读到下一行
```

---

### cerr 和 clog：错误输出

除了 `cout`，标准库还提供了 `cerr` 和 `clog`：

```cpp
std::cerr << "Error: file not found\n";  // 标准错误输出，不缓冲
std::clog << "Info: starting...\n";      // 标准错误输出，有缓冲
```

`cerr` 和 `clog` 输出到标准错误流（stderr），而 `cout` 输出到标准输出流（stdout）。终端里两者看起来一样，但可以分别重定向：

```bash
./taco 2>/dev/null    # 丢弃错误输出，保留正常输出
./taco > output.txt   # 把正常输出写到文件，错误输出仍然显示
```

在 Taco 解释器里，错误信息用 `cerr` 输出，正常结果用 `cout` 输出。

---

## 4.3 格式化输出

### printf 风格（C 遗产）

C 的 `printf` 依然可以在 C++ 里用：

```cpp
#include <cstdio>

printf("Name: %s, Age: %d\n", "Miguel", 12);
printf("Pi: %.2f\n", 3.14159);  // 保留 2 位小数
```

`printf` 的格式符：
- `%d`：整数
- `%f`：浮点数
- `%s`：字符串（C 风格）
- `%c`：字符
- `%x`：十六进制整数

`printf` 的问题：类型不安全。如果格式符和参数类型不匹配，会产生未定义行为（undefined behavior）——程序可能输出乱码，可能崩溃，也可能看起来正常但实际上在悄悄损坏内存。编译器会警告这类错误，但不会报错。

---

### iostream 风格

`cout <<` 是类型安全的——编译器知道每个参数的类型，选择正确的输出方式：

```cpp
std::cout << "Name: " << "Miguel" << ", Age: " << 12 << "\n";
```

但 `cout` 的格式控制比较麻烦，需要用操纵符（manipulator）：

```cpp
#include <iomanip>

// 保留 2 位小数
std::cout << std::fixed << std::setprecision(2) << 3.14159 << "\n";

// 十六进制
std::cout << std::hex << 255 << "\n";  // ff

// 宽度和填充
std::cout << std::setw(10) << std::setfill('0') << 42 << "\n";  // 0000000042
```

这些操纵符写起来比 `printf` 格式符啰嗦很多。

---

### std::format（C++20）和 fmtlib（第三方库）

C++20 引入了 `std::format`，结合了 `printf` 的简洁和 `cout` 的类型安全：

```cpp
#include <format>  // C++20

std::string s = std::format("Name: {}, Age: {}", "Miguel", 12);
std::cout << s << "\n";

// 格式控制
std::cout << std::format("Pi: {:.2f}\n", 3.14159);  // Pi: 3.14
std::cout << std::format("{:>10}\n", "right");       // 右对齐，宽度 10
std::cout << std::format("{:0>5}\n", 42);            // 00042
```

如果编译器不支持 C++20，可以用 `fmtlib`——`std::format` 的原型就是从 fmtlib 来的，API 几乎完全相同：

```cmake
# CMakeLists.txt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.1.1
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(taco PRIVATE fmt::fmt)
```

```cpp
#include <fmt/format.h>

fmt::print("Name: {}, Age: {}\n", "Miguel", 12);
std::string s = fmt::format("Pi: {:.2f}", 3.14159);
```

本书使用 `std::format`（C++17 的编译器用 fmtlib 代替），它是目前格式化输出的最佳选择。

---

## 4.4 文件 I/O：fstream

### 读写文件

C++ 用 `fstream` 头文件里的类来处理文件：

```cpp
#include <fstream>
#include <string>
#include <iostream>
```

**写文件**：

```cpp
std::ofstream file("output.txt");  // 创建并打开文件
if (!file) {
    std::cerr << "Failed to open file\n";
    return 1;
}

file << "Hello, Taco!\n";
file << 42 << "\n";
// file 析构时自动关闭，不需要手动 close()
```

**读文件**：

```cpp
std::ifstream file("input.txt");
if (!file) {
    std::cerr << "File not found\n";
    return 1;
}

// 逐行读取
std::string line;
while (std::getline(file, line)) {
    std::cout << line << "\n";
}
```

**读取整个文件内容**：

```cpp
std::ifstream file("script.taco");
std::string content(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>()
);
// content 现在包含文件的全部内容
```

这种写法有点奇怪，但是很常用的惯用法。也可以用更直观的方式：

```cpp
std::ifstream file("script.taco");
std::ostringstream buffer;
buffer << file.rdbuf();
std::string content = buffer.str();
```

---

### 文件打开模式

`ofstream` 默认会覆盖已有文件。如果想追加内容：

```cpp
std::ofstream file("log.txt", std::ios::app);  // 追加模式
file << "New log entry\n";
```

常用的打开模式：

```cpp
std::ios::in      // 读（ifstream 默认）
std::ios::out     // 写（ofstream 默认，覆盖）
std::ios::app     // 追加
std::ios::binary  // 二进制模式
std::ios::trunc   // 清空文件（out 默认包含）
```

可以组合使用：

```cpp
// 读写都支持，二进制模式
std::fstream file("data.bin", std::ios::in | std::ios::out | std::ios::binary);
```

---

### Taco 的文件读取函数

在 Taco 里，需要读取 `.taco` 脚本文件。封装一个辅助函数：

```cpp
// src/utils.h
#pragma once
#include <string>
#include <optional>

// 读取文件内容，失败返回空 optional
std::optional<std::string> read_file(const std::string& path);
```

```cpp
// src/utils.cpp
#include "utils.h"
#include <fstream>
#include <sstream>

std::optional<std::string> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;  // 文件打开失败
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
```

使用：

```cpp
auto content = read_file("script.taco");
if (!content) {
    std::cerr << "Error: cannot open file\n";
    return 1;
}
std::cout << *content;  // 解引用 optional 获取字符串
```

`std::optional` 是 C++17 的特性，表示一个"可能有值，也可能没有值"的类型。第十九章会详细介绍。这里只需要知道：`std::nullopt` 表示没有值，`*content` 获取值。

---

## 4.5 filesystem：现代文件系统操作

C++17 引入了 `<filesystem>` 头文件，提供了跨平台的文件系统操作：

```cpp
#include <filesystem>
namespace fs = std::filesystem;  // 别名，简化书写
```

---

### 常用操作

```cpp
// 检查文件是否存在
fs::path p = "script.taco";
if (fs::exists(p)) {
    std::cout << "File exists\n";
}

// 获取文件信息
std::cout << fs::file_size(p) << " bytes\n";
std::cout << p.extension() << "\n";    // ".taco"
std::cout << p.filename() << "\n";     // "script.taco"
std::cout << p.stem() << "\n";         // "script"（不含扩展名）
std::cout << p.parent_path() << "\n";  // 父目录

// 列出目录内容
for (const auto& entry : fs::directory_iterator(".")) {
    std::cout << entry.path().filename() << "\n";
}

// 递归列出
for (const auto& entry : fs::recursive_directory_iterator(".")) {
    if (entry.path().extension() == ".taco") {
        std::cout << entry.path() << "\n";
    }
}

// 创建目录
fs::create_directory("output");
fs::create_directories("a/b/c");  // 递归创建

// 复制、移动、删除
fs::copy("src.taco", "dst.taco");
fs::rename("old.taco", "new.taco");
fs::remove("file.taco");
fs::remove_all("directory");  // 递归删除

// 获取当前目录
fs::path cwd = fs::current_path();
std::cout << cwd << "\n";
```

---

### 路径操作

`fs::path` 是跨平台的路径类，自动处理不同系统的路径分隔符（`/` 和 `\`）：

```cpp
fs::path p = "src";
p /= "lexer.cpp";   // 用 /= 拼接路径
// p 现在是 "src/lexer.cpp"（Linux/Mac）或 "src\lexer.cpp"（Windows）

fs::path absolute = fs::absolute(p);  // 转换为绝对路径
fs::path canonical = fs::canonical(p); // 解析符号链接，转换为真实路径
```

---

### 在 Taco 里的应用

Taco 解释器运行脚本时，需要处理文件路径：

```cpp
int main(int argc, char* argv[]) {
    if (argc < 2) {
        // 没有参数，启动 REPL
        start_repl();
        return 0;
    }

    fs::path script_path = argv[1];

    // 检查文件存在且是 .taco 文件
    if (!fs::exists(script_path)) {
        std::cerr << "🌮 Error: file not found: " << script_path << "\n";
        return 1;
    }

    if (script_path.extension() != ".taco") {
        std::cerr << "🌮 Warning: expected .taco file\n";
    }

    auto content = read_file(script_path.string());
    if (!content) {
        std::cerr << "🌮 Error: cannot read file\n";
        return 1;
    }

    run(*content);
    return 0;
}
```

---

## 小结

这一章讲了 C++ 的字符串和输入输出：

**std::string** 是 C 字符串的现代替代品，自动管理内存，支持 `+` 拼接和 `==` 比较，内置丰富的操作方法。

**iostream** 提供了类型安全的输入输出。`cout` 用于正常输出，`cerr` 用于错误输出。`"\n"` 比 `std::endl` 更高效。

**std::format**（或 fmtlib）是格式化字符串的最佳方式，兼具 `printf` 的简洁和 `cout` 的类型安全。

**fstream** 处理文件读写，结合 RAII，文件会在对象析构时自动关闭。

**filesystem**（C++17）提供跨平台的文件系统操作，`fs::path` 自动处理不同系统的路径差异。

---

下一章是第一部分的收尾：介绍解释器的架构，定义 Taco 语言的设计，并完成第一个版本（v0）——一个能把 Taco 代码切成 Token 的词法分析器。
# 第五章：解释器是什么

---

前四章讲的都是工具——C++ 的语法、构建系统、字符串处理。从这一章开始，进入本书的核心：**构建 Taco 解释器**。

在写代码之前，先把"解释器是什么"、"Taco 长什么样"、"这本书要做什么"这三件事讲清楚。

---

## 5.1 语言、编译器、解释器的关系

### 什么是编程语言

编程语言是一种形式化的符号系统，让人类能够以一种精确的方式描述计算过程。

"形式化"意味着语言有严格的规则——哪些写法是合法的，哪些不是；合法的写法代表什么含义。自然语言（中文、英文）有歧义，而编程语言不允许歧义。

---

### 编译器 vs 解释器

一段程序代码，要想让计算机执行，需要把它翻译成计算机能理解的机器码。翻译的方式主要有两种：

**编译器**（Compiler）：把整个源文件翻译成机器码，生成一个独立的可执行文件。翻译发生在程序运行之前。

```
源代码 → 编译器 → 可执行文件 → 运行
```

C 和 C++ 就是编译型语言。`g++ main.cpp -o main` 这个命令完成的就是编译，生成的 `main` 文件可以直接运行，不再需要 C++ 环境。

**解释器**（Interpreter）：直接读取源代码，边理解边执行，不生成独立的可执行文件。执行和翻译同时发生。

```
源代码 → 解释器 → 执行
```

Python 就是典型的解释型语言。运行 `python script.py` 时，Python 解释器读取 `script.py`，理解每一行的含义，然后执行对应的操作。

**混合方式**：很多现代语言介于两者之间。Python 实际上会先把源代码编译成字节码（`.pyc` 文件），再由虚拟机解释执行字节码。Java 也类似——先编译成字节码，再由 JVM 执行。这种方式结合了两者的优点：比纯解释快，比编译到机器码更便于跨平台。

Taco 使用**纯解释**方式：直接读取源代码，构建语法树，然后执行语法树。这是最简单的实现方式，适合这本书的教学目标。如果以后想让 Taco 更快，可以加入字节码编译——那是进阶的话题。

---

### 解释器内部发生了什么

解释器不是黑盒，它的内部有清晰的结构。以执行这行 Taco 代码为例：

```taco
var x = 10 + 20;
```

解释器内部经历以下步骤：

**第一步：词法分析（Lexical Analysis）**

把源代码字符串切成一个个有意义的单元，叫做 **Token**（词法单元）：

```
"var x = 10 + 20;" 
→ [var] [x] [=] [10] [+] [20] [;]
```

每个 Token 有类型（关键字、标识符、数字、运算符、分号……）和值。负责这个工作的模块叫**词法分析器**（Lexer）或**扫描器**（Scanner）。

**第二步：语法分析（Parsing）**

把 Token 列表组织成有层次结构的**抽象语法树**（Abstract Syntax Tree，AST）：

```
VarDeclaration
├── name: "x"
└── value: BinaryExpression
    ├── left: NumberLiteral(10)
    ├── operator: "+"
    └── right: NumberLiteral(20)
```

AST 反映的是代码的语法结构。负责这个工作的模块叫**语法分析器**（Parser）。

**第三步：求值（Evaluation）**

遍历 AST，执行每个节点代表的操作：

```
执行 VarDeclaration：
  求值 BinaryExpression：
    求值 NumberLiteral(10) → 10
    求值 NumberLiteral(20) → 20
    计算 10 + 20 → 30
  把 30 存储到变量 x
```

负责这个工作的模块叫**求值器**（Evaluator）或**解释器**（Interpreter）——虽然有时候整个系统也叫解释器。

---

整个流程：

```
源代码字符串
    ↓ 词法分析器（Lexer）
Token 列表
    ↓ 语法分析器（Parser）
抽象语法树（AST）
    ↓ 求值器（Evaluator）
结果
```

这三层结构是绝大多数解释器和编译器的基础架构。理解了这个架构，就理解了接下来每一章在做什么。

---

## 5.2 Taco 语言设计：我们要实现什么

在开始写解释器之前，需要先定义 Taco 是什么样的语言。

### Taco 的定位

Taco 是一门**动态类型的脚本语言**，定位是：

> 住在终端里的语言。不用 import，不用项目，打开就写，写完就跑。语法是人话，数据处理像写英语，系统操作像呼吸一样自然。

Taco 的竞争对手是 bash 脚本和 Python 一次性脚本：
- 比 bash 可读性强，语法是真正的编程语言
- 比 Python 更轻量，shell 操作开箱即用，不需要 `import os`

---

### Taco 的语法

**变量声明**：

```taco
var x = 10;
var name = "Miguel";
var flag = true;
var nothing = nil;
```

**基本数据类型**：
- `number`：整数和浮点数统一（`10`、`3.14`）
- `string`：字符串（`"hello"`）
- `bool`：布尔值（`true`、`false`）
- `nil`：空值

**运算符**：

```taco
// 算术
x + y;  x - y;  x * y;  x / y;  x % y;  x ^ y;

// 比较
x == y;  x != y;  x > y;  x < y;  x >= y;  x <= y;

// 逻辑
x && y;  x || y;  !x;

// 字符串拼接
"Hello, " + name + "!";

// 字符串插值
"Hello, {name}!";

// 三元
x > 0 ? "positive" : "negative";
```

**控制流**：

```taco
// if/elseif/else
if (x > 90) {
    print("A");
} elseif (x > 80) {
    print("B");
} else {
    print("C");
}

// while
while (x > 0) {
    x = x - 1;
}

// C 风格 for
for (var i = 0; i < 10; i++) {
    print(i);
}

// range 风格
for i in range(0, 10) {
    print(i);
}

// switch（默认不穿透）
switch x {
    case 1 { print("one"); }
    case 2 { print("two"); }
    default { print("other"); }
}
```

**函数**：

```taco
func greet(name, greeting: "Hola") {
    print("{greeting}, {name}!");
}

greet("Miguel");              // Hola, Miguel!
greet("Dante", greeting: "Hey");  // Hey, Dante!

// 闭包
var double = { x in x * 2 };

// 多返回值
func minmax(arr) {
    return arr.getFirst(), arr.getLast();
}
var min, max = minmax([3, 1, 4, 1, 5]);
```

**数据结构**：

```taco
// array（从 0 开始）
var arr = [1, 2, 3, 4, 5];
var first = arr[0];
var merged = [...arr, 6, 7];

// map
var person = {"name": "Miguel", "age": 12};
var name = person["name"];
var name2 = person.name;  // 点语法

// pipeline
arr
    .filter { x in x % 2 == 0 }
    .map { x in x * 2 }
    .each { x in print(x) };
```

**OOP**：

```taco
// class：引用类型，支持继承
class Animal {
    var name;
    func init(name) { self.name = name; }
    func speak() { print("{self.name} makes a sound"); }
}

class Dog extends Animal {
    func speak() {
        super.speak();
        print("{self.name} also barks");
    }
}

// struct：值类型，不支持继承，自动 init
struct Point {
    var x;
    var y;
    func distance() { return (self.x ^ 2 + self.y ^ 2) ^ 0.5; }
}

// enum
enum Direction { North, South, East, West }
var d = Direction.North;
```

**并发**：

```taco
var ch = channel();
var t = thread {
    ch.send(42);
};
var val = ch.receive();
t.join();
```

**模块**：

```taco
import utils;
from io import readFile;
import taco.net;
```

**彩蛋**：

```taco
🌮 "Will my code compile?";  // Magic 8 Ball
🌮🌮 "Same question";        // 魔法海螺
🌮🌮🌮;                      // 随机笑话
🌮🌮🌮🌮;                    // Unix 时间戳
```

---

### 我们会实现哪些部分

Taco 的完整特性集很丰富，但这本书的重点是**学 C++**，不是构建一个完整的生产级语言。所以 Taco 的实现是**渐进的**：

| 版本 | 实现的特性 | 对应章节 |
|------|------------|----------|
| v0 | 词法分析器，能识别 Token | 第六章 |
| v1 | 语法分析器，能构建 AST，能求值基本表达式 | 第十一章 |
| v2 | 控制流：if/while/for/switch | 第十五章 |
| v3 | 函数、闭包、作用域 | 第二十章 |
| v4 | array、map、enum、内置标准库 | 第二十五章 |
| v5 | 解释器内部模板化，值类型用 variant | 第二十九章 |
| v6 | REPL、多线程 | 第三十三章 |
| v7 | 网络支持，fetch() | 第三十六章 |

OOP（class、struct）、模块系统、VSCode 插件等高级特性放在综合收尾部分。

每次项目进化，都有具体的动机：学到新的 C++ 特性，发现旧版本的不足，用新特性来改进。

---

## 5.3 解释器的三层架构：词法、语法、求值

现在把架构具体化，对应到 Taco 项目的代码结构。

### 项目结构

```
taco/
  CMakeLists.txt
  src/
    main.cpp          # 程序入口，解析命令行参数
    token.h           # Token 的定义
    token.cpp         # Token 相关工具函数
    lexer.h           # 词法分析器接口
    lexer.cpp         # 词法分析器实现
    ast.h             # AST 节点定义
    parser.h          # 语法分析器接口
    parser.cpp        # 语法分析器实现
    evaluator.h       # 求值器接口
    evaluator.cpp     # 求值器实现
    environment.h     # 变量环境（作用域）
    environment.cpp
    value.h           # Taco 值类型
    value.cpp
    error.h           # 错误处理
    error.cpp
    builtin.h         # 内置函数
    builtin.cpp
  tests/
    test_lexer.cpp
    test_parser.cpp
    test_evaluator.cpp
```

---

### 第一层：Token

Token 是词法分析的输出，语法分析的输入。一个 Token 包含两件事：

- **类型**（TokenType）：这个词是什么——关键字、数字、字符串、运算符……
- **值**（value）：这个词的原始文本内容

```cpp
// token.h
#pragma once
#include <string>

enum class TokenType {
    // 字面量
    Number,       // 42, 3.14
    String,       // "hello"
    True,         // true
    False,        // false
    Nil,          // nil

    // 标识符和关键字
    Identifier,   // x, name, greet
    Var,          // var
    Func,         // func
    If,           // if
    Elseif,       // elseif
    Else,         // else
    While,        // while
    For,          // for
    In,           // in
    Return,       // return
    Class,        // class
    Struct,       // struct
    Enum,         // enum
    Extends,      // extends
    Self,         // self
    Super,        // super
    Switch,       // switch
    Case,         // case
    Default,      // default
    Import,       // import
    From,         // from
    Thread,       // thread
    Channel,      // channel

    // 运算符
    Plus,         // +
    Minus,        // -
    Star,         // *
    Slash,        // /
    Percent,      // %
    Caret,        // ^
    Equal,        // ==
    NotEqual,     // !=
    Greater,      // >
    Less,         // <
    GreaterEqual, // >=
    LessEqual,    // <=
    And,          // &&
    Or,           // ||
    Not,          // !
    Assign,       // =
    Arrow,        // =>（箭头函数，暂时保留）
    Question,     // ?
    Colon,        // :
    Dot,          // .
    DotDot,       // ..（range）
    Ellipsis,     // ...（展开）
    Pipe,         // |（pipeline，如需要）

    // 分隔符
    LeftParen,    // (
    RightParen,   // )
    LeftBrace,    // {
    RightBrace,   // }
    LeftBracket,  // [
    RightBracket, // ]
    Semicolon,    // ;
    Comma,        // ,

    // 特殊
    Taco,         // 🌮
    EndOfFile,    // 文件结束
};

struct Token {
    TokenType type;
    std::string value;  // 原始文本
    int line;           // 行号，用于错误报告
    int column;         // 列号，用于错误报告
};
```

---

### 第二层：AST 节点

AST 节点表示代码的语法结构。每种语法结构对应一种节点类型：

```
var x = 10 + 20;
→
VarDecl {
    name: "x",
    value: BinaryExpr {
        left: NumberExpr { value: 10 },
        op: "+",
        right: NumberExpr { value: 20 }
    }
}
```

AST 是一棵树：节点可以包含其他节点。树的根节点代表整个程序，叶节点代表最基本的元素（数字、字符串、变量名）。

在 C++ 里，AST 节点天然适合用继承来实现——所有节点共享一个基类，不同的节点类型是子类。这是第十一章引入类和继承之后要做的事情。

---

### 第三层：求值器

求值器遍历 AST，对每个节点求值：

```
BinaryExpr { left: 10, op: "+", right: 20 }
→ 求值 left → 10
→ 求值 right → 20
→ 计算 10 + 20 → 30
```

求值器需要一个**环境**（Environment）来存储变量：

```
var x = 10;  → 环境: {x: 10}
var y = 20;  → 环境: {x: 10, y: 20}
print(x + y); → 从环境里取 x=10, y=20, 求值 10+20=30
```

当进入一个新的作用域（函数体、if 块），创建一个新的子环境；离开时销毁子环境。

---

## 5.4 整本书的项目路线图

现在把整本书的主线梳理一遍，建立全局视角。

### 七次进化

Taco 从一个只能切 Token 的程序，一步步进化成一个完整的解释器：

**v0：词法分析器**（第一部分）

```
输入：var x = 10 + 20;
输出：[VAR][x][=][10][+][20][;]
```

涉及 C++ 知识：结构体、枚举、字符串处理、基本函数。

**v1：语法分析器 + 基础求值**（第二部分）

```
输入：var x = 10 + 20;
输出：30（x 被赋值并打印）
```

涉及 C++ 知识：类、构造函数、析构函数、运算符重载。

**v2：控制流**（第三部分）

```taco
// 能运行这样的代码
var score = 85;
if (score >= 90) { print("A"); }
elseif (score >= 80) { print("B"); }
else { print("C"); }
```

涉及 C++ 知识：继承、多态、虚函数、vtable。

**v3：函数、闭包、作用域**（第四部分）

```taco
// 能运行这样的代码
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n-1) + fib(n-2);
}
print(fib(10));
```

涉及 C++ 知识：智能指针、RAII、移动语义。

**v4：array、map、内置库**（第五部分）

```taco
// 能运行这样的代码
var nums = [1, 2, 3, 4, 5];
nums.filter { x in x % 2 == 0 }.each { x in print(x); };
```

涉及 C++ 知识：STL 容器、迭代器、算法、Lambda。

**v5：模板化重构**（第六部分）

把解释器内部的值类型和容器用模板重构，代码更通用，性能略有提升。

涉及 C++ 知识：函数模板、类模板、模板特化、`std::variant`。

**v6：REPL + 多线程**（第七部分）

```
🌮 Taco 0.1.0
   It works on my machine.
> var x = 10;
> print(x + 20);
30
>
```

涉及 C++ 知识：`std::thread`、`std::mutex`、`std::channel`、`std::atomic`。

**v7：网络支持**（第八部分）

```taco
var res = fetchUrl("https://api.github.com/users/torvalds");
print(res);
```

涉及 C++ 知识：socket、Asio、cpp-httplib。

---

### 每一章的结构

本书每一章的结构是一致的：

1. **概念介绍**：这个 C++ 特性是什么，为什么需要它
2. **详细讲解**：语法、用法、底层原理
3. **More About**（部分章节）：更深入的细节，第一次读可以跳过
4. **项目章节**：用刚学到的特性改进 Taco

项目章节会展示完整的代码，并解释"为什么这样做，而不是那样做"。每次改进都有真实的动机，不是为了用新特性而用新特性。

---

## 小结

这一章建立了全书的框架：

**解释器的三层架构**：词法分析（源码 → Token）、语法分析（Token → AST）、求值（AST → 结果）。接下来每一个项目章节都在这个框架里。

**Taco 的设计**：动态类型、终端脚本、pipeline 风格、带有个性的彩蛋。

**七次进化**：从只能切 Token 的 v0，到能运行脚本、支持 REPL 和网络请求的 v7。每次进化对应一部分新学的 C++ 知识。

---

下一章开始动手：实现 Taco 的词法分析器（v0）。
