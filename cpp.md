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
# 第六章：项目 v0——词法分析器

---

前五章打下了基础：C++ 的语法、构建系统、字符串处理，以及解释器的整体架构。现在开始动手。

v0 的目标很简单：把一段 Taco 源代码字符串，切成一个个有意义的 Token。不求值，不解析语法树，只做词法分析。

```
输入："var x = 10 + 20;"
输出：[VAR "var"] [IDENTIFIER "x"] [ASSIGN "="] [NUMBER "10"] [PLUS "+"] [NUMBER "20"] [SEMICOLON ";"] [EOF]
```

---

## 6.1 什么是 Token

词法分析器（Lexer）做的事情，类似人类读代码时的直觉：

看到 `var`，知道这是一个关键字。
看到 `x`，知道这是一个标识符。
看到 `10`，知道这是一个数字。
看到 `+`，知道这是一个运算符。

每一个这样的"有意义的单元"，就是一个 Token。

Token 有两个基本属性：
- **类型**（TokenType）：这个词是什么种类
- **值**（value）：这个词的原始文本

```
源代码：var x = 10 + 20;

Token 列表：
  Token { type: VAR,        value: "var" }
  Token { type: IDENTIFIER, value: "x"   }
  Token { type: ASSIGN,     value: "="   }
  Token { type: NUMBER,     value: "10"  }
  Token { type: PLUS,       value: "+"   }
  Token { type: NUMBER,     value: "20"  }
  Token { type: SEMICOLON,  value: ";"   }
  Token { type: EOF,        value: ""    }
```

注意最后有一个 `EOF`（End of File）Token。这是一个哨兵值，告诉后续的语法分析器"输入结束了"，避免在处理 Token 列表时越界。

---

## 6.2 用结构体和枚举实现 Token

先定义 Token 的数据结构。这一章用的都是 C++ 的基础特性——结构体和枚举——还没有涉及类和继承。

### token.h

```cpp
#pragma once
#include <string>

// TokenType 枚举：用 enum class 而不是普通 enum
// enum class 更安全：不会隐式转换成 int，不同枚举的值不会互相冲突
enum class TokenType {
    // 字面量
    Number,       // 42, 3.14, 1_000_000
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
    Question,     // ?
    Colon,        // :
    Dot,          // .
    Ellipsis,     // ...
    PipeArrow,    // |>

    // 分隔符
    LeftParen,    // (
    RightParen,   // )
    LeftBrace,    // {
    RightBrace,   // }
    LeftBracket,  // [
    RightBracket, // ]
    Semicolon,    // ;
    Comma,        // ,

    // 彩蛋
    Taco,         // 🌮

    // 特殊
    EndOfFile,
};

// Token 结构体
struct Token {
    TokenType   type;
    std::string value;   // 原始文本
    int         line;    // 行号（从 1 开始）
    int         column;  // 列号（从 1 开始）
};

// 把 TokenType 转成字符串，方便调试
std::string token_type_to_string(TokenType type);
```

为什么用 `enum class` 而不是普通的 `enum`？

普通 `enum` 有两个问题：

```cpp
// 普通 enum：会隐式转换成 int，不同枚举的值可能冲突
enum Color { Red, Green, Blue };
enum Direction { North, South, East, West };

int c = Red;   // 可以，但这通常不是你想要的
if (Red == North) { ... }  // 比较两个不相关的枚举，编译器不报错！
```

`enum class` 解决了这两个问题：

```cpp
// enum class：类型安全
enum class Color { Red, Green, Blue };
enum class Direction { North, South, East, West };

int c = Color::Red;            // 错误！不能隐式转换
if (Color::Red == Direction::North) { ... }  // 错误！类型不匹配
```

用 `enum class` 写 TokenType，编译器会帮我们检查：如果不小心把 `TokenType::Number` 当 `int` 用，会直接报错，而不是悄悄产生 bug。

---

### token.cpp

```cpp
#include "token.h"

std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::Number:       return "NUMBER";
        case TokenType::String:       return "STRING";
        case TokenType::True:         return "TRUE";
        case TokenType::False:        return "FALSE";
        case TokenType::Nil:          return "NIL";
        case TokenType::Identifier:   return "IDENTIFIER";
        case TokenType::Var:          return "VAR";
        case TokenType::Func:         return "FUNC";
        case TokenType::If:           return "IF";
        case TokenType::Elseif:       return "ELSEIF";
        case TokenType::Else:         return "ELSE";
        case TokenType::While:        return "WHILE";
        case TokenType::For:          return "FOR";
        case TokenType::In:           return "IN";
        case TokenType::Return:       return "RETURN";
        case TokenType::Class:        return "CLASS";
        case TokenType::Struct:       return "STRUCT";
        case TokenType::Enum:         return "ENUM";
        case TokenType::Extends:      return "EXTENDS";
        case TokenType::Self:         return "SELF";
        case TokenType::Super:        return "SUPER";
        case TokenType::Switch:       return "SWITCH";
        case TokenType::Case:         return "CASE";
        case TokenType::Default:      return "DEFAULT";
        case TokenType::Import:       return "IMPORT";
        case TokenType::From:         return "FROM";
        case TokenType::Thread:       return "THREAD";
        case TokenType::Channel:      return "CHANNEL";
        case TokenType::Plus:         return "PLUS";
        case TokenType::Minus:        return "MINUS";
        case TokenType::Star:         return "STAR";
        case TokenType::Slash:        return "SLASH";
        case TokenType::Percent:      return "PERCENT";
        case TokenType::Caret:        return "CARET";
        case TokenType::Equal:        return "EQUAL";
        case TokenType::NotEqual:     return "NOT_EQUAL";
        case TokenType::Greater:      return "GREATER";
        case TokenType::Less:         return "LESS";
        case TokenType::GreaterEqual: return "GREATER_EQUAL";
        case TokenType::LessEqual:    return "LESS_EQUAL";
        case TokenType::And:          return "AND";
        case TokenType::Or:           return "OR";
        case TokenType::Not:          return "NOT";
        case TokenType::Assign:       return "ASSIGN";
        case TokenType::Question:     return "QUESTION";
        case TokenType::Colon:        return "COLON";
        case TokenType::Dot:          return "DOT";
        case TokenType::Ellipsis:     return "ELLIPSIS";
        case TokenType::PipeArrow:    return "PIPE_ARROW";
        case TokenType::LeftParen:    return "LEFT_PAREN";
        case TokenType::RightParen:   return "RIGHT_PAREN";
        case TokenType::LeftBrace:    return "LEFT_BRACE";
        case TokenType::RightBrace:   return "RIGHT_BRACE";
        case TokenType::LeftBracket:  return "LEFT_BRACKET";
        case TokenType::RightBracket: return "RIGHT_BRACKET";
        case TokenType::Semicolon:    return "SEMICOLON";
        case TokenType::Comma:        return "COMMA";
        case TokenType::Taco:         return "TACO";
        case TokenType::EndOfFile:    return "EOF";
    }
    return "UNKNOWN";
}
```

---

## 6.3 实现词法分析器

### lexer.h

```cpp
#pragma once
#include "token.h"
#include <string>
#include <vector>
#include <unordered_map>

class Lexer {
public:
    // 构造函数：接收源代码字符串
    explicit Lexer(std::string source);

    // 核心接口：把源代码切成 Token 列表
    std::vector<Token> tokenize();

private:
    std::string m_source;   // 源代码
    int         m_pos;      // 当前读取位置
    int         m_line;     // 当前行号
    int         m_column;   // 当前列号

    // 关键字表：字符串 → TokenType
    static const std::unordered_map<std::string, TokenType> KEYWORDS;

    // 辅助函数
    char current() const;           // 当前字符
    char peek(int offset = 1) const;// 向前看 offset 个字符
    char advance();                 // 消费当前字符，前进一步
    bool is_at_end() const;         // 是否到达末尾
    bool match(char expected);      // 如果下一个字符匹配，消费并返回 true

    // 创建 Token
    Token make_token(TokenType type, const std::string& value) const;

    // 跳过空白和注释
    void skip_whitespace_and_comments();

    // 各类 Token 的读取函数
    Token read_number();
    Token read_string();
    Token read_identifier_or_keyword();
    Token read_taco_emoji();

    // 读取下一个 Token
    Token next_token();
};
```

---

### lexer.cpp

```cpp
#include "lexer.h"
#include <stdexcept>
#include <sstream>

// 关键字表：static 成员，所有 Lexer 实例共享一份
const std::unordered_map<std::string, TokenType> Lexer::KEYWORDS = {
    {"var",     TokenType::Var},
    {"func",    TokenType::Func},
    {"if",      TokenType::If},
    {"elseif",  TokenType::Elseif},
    {"else",    TokenType::Else},
    {"while",   TokenType::While},
    {"for",     TokenType::For},
    {"in",      TokenType::In},
    {"return",  TokenType::Return},
    {"true",    TokenType::True},
    {"false",   TokenType::False},
    {"nil",     TokenType::Nil},
    {"class",   TokenType::Class},
    {"struct",  TokenType::Struct},
    {"enum",    TokenType::Enum},
    {"extends", TokenType::Extends},
    {"self",    TokenType::Self},
    {"super",   TokenType::Super},
    {"switch",  TokenType::Switch},
    {"case",    TokenType::Case},
    {"default", TokenType::Default},
    {"import",  TokenType::Import},
    {"from",    TokenType::From},
    {"thread",  TokenType::Thread},
    {"channel", TokenType::Channel},
};

Lexer::Lexer(std::string source)
    : m_source(std::move(source))  // move 避免拷贝
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}

// ────────────────────────────────
// 辅助函数
// ────────────────────────────────

char Lexer::current() const {
    if (is_at_end()) return '\0';
    return m_source[m_pos];
}

char Lexer::peek(int offset) const {
    int idx = m_pos + offset;
    if (idx < 0 || idx >= static_cast<int>(m_source.size())) return '\0';
    return m_source[idx];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    return c;
}

bool Lexer::is_at_end() const {
    return m_pos >= static_cast<int>(m_source.size());
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (m_source[m_pos] != expected) return false;
    advance();
    return true;
}

Token Lexer::make_token(TokenType type, const std::string& value) const {
    return Token{type, value, m_line, m_column};
}

// ────────────────────────────────
// 跳过空白和注释
// ────────────────────────────────

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = current();

        // 跳过空白字符
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
            continue;
        }

        // 跳过单行注释 //
        if (c == '/' && peek() == '/') {
            while (!is_at_end() && current() != '\n') {
                advance();
            }
            continue;
        }

        break;
    }
}

// ────────────────────────────────
// 读取数字
// ────────────────────────────────

Token Lexer::read_number() {
    int start_line   = m_line;
    int start_column = m_column;
    std::string value;

    while (!is_at_end() && (std::isdigit(current()) || current() == '_')) {
        if (current() != '_') {   // 下划线只用于分隔，不加入值
            value += current();
        }
        advance();
    }

    // 小数部分
    if (current() == '.' && std::isdigit(peek())) {
        value += '.';
        advance();
        while (!is_at_end() && std::isdigit(current())) {
            value += advance();
        }
    }

    return Token{TokenType::Number, value, start_line, start_column};
}

// ────────────────────────────────
// 读取字符串
// ────────────────────────────────

Token Lexer::read_string() {
    int start_line   = m_line;
    int start_column = m_column;

    advance();  // 跳过开头的 "

    std::string value;
    while (!is_at_end() && current() != '"') {
        if (current() == '\\') {
            advance();  // 跳过反斜杠
            switch (current()) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                default:
                    value += '\\';
                    value += current();
            }
        } else {
            value += current();
        }
        advance();
    }

    if (is_at_end()) {
        // 字符串没有闭合
        throw std::runtime_error(
            "🌮 line " + std::to_string(start_line) +
            ": Unterminated string. Did you forget the closing \"?"
        );
    }

    advance();  // 跳过结尾的 "
    return Token{TokenType::String, value, start_line, start_column};
}

// ────────────────────────────────
// 读取标识符或关键字
// ────────────────────────────────

Token Lexer::read_identifier_or_keyword() {
    int start_line   = m_line;
    int start_column = m_column;
    std::string value;

    while (!is_at_end() && (std::isalnum(current()) || current() == '_')) {
        value += advance();
    }

    // 查关键字表，不在就是标识符
    auto it = KEYWORDS.find(value);
    TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::Identifier;

    return Token{type, value, start_line, start_column};
}

// ────────────────────────────────
// 读取 🌮 彩蛋
// ────────────────────────────────

Token Lexer::read_taco_emoji() {
    // 🌮 是 UTF-8 四字节序列：0xF0 0x9F 0x8C 0xAE
    // 调用时已经确认第一个字节是 0xF0，这里消费剩余三个字节
    int start_line   = m_line;
    int start_column = m_column;

    std::string value;
    value += advance();  // 0xF0
    value += advance();  // 0x9F
    value += advance();  // 0x8C
    value += advance();  // 0xAE

    return Token{TokenType::Taco, value, start_line, start_column};
}

// ────────────────────────────────
// 读取下一个 Token
// ────────────────────────────────

Token Lexer::next_token() {
    skip_whitespace_and_comments();

    if (is_at_end()) {
        return make_token(TokenType::EndOfFile, "");
    }

    int start_line   = m_line;
    int start_column = m_column;
    char c = current();

    // 数字
    if (std::isdigit(c)) {
        return read_number();
    }

    // 字符串
    if (c == '"') {
        return read_string();
    }

    // 标识符或关键字
    if (std::isalpha(c) || c == '_') {
        return read_identifier_or_keyword();
    }

    // 🌮 彩蛋（UTF-8 四字节，第一个字节是 0xF0）
    if (static_cast<unsigned char>(c) == 0xF0) {
        // 检查接下来三个字节是否匹配 🌮
        if (static_cast<unsigned char>(peek(1)) == 0x9F &&
            static_cast<unsigned char>(peek(2)) == 0x8C &&
            static_cast<unsigned char>(peek(3)) == 0xAE) {
            return read_taco_emoji();
        }
    }

    // 消费当前字符，处理单字符和双字符运算符
    advance();

    switch (c) {
        case '+': return Token{TokenType::Plus,         "+", start_line, start_column};
        case '-': return Token{TokenType::Minus,        "-", start_line, start_column};
        case '*': return Token{TokenType::Star,         "*", start_line, start_column};
        case '/': return Token{TokenType::Slash,        "/", start_line, start_column};
        case '%': return Token{TokenType::Percent,      "%", start_line, start_column};
        case '^': return Token{TokenType::Caret,        "^", start_line, start_column};
        case '?': return Token{TokenType::Question,     "?", start_line, start_column};
        case ':': return Token{TokenType::Colon,        ":", start_line, start_column};
        case ';': return Token{TokenType::Semicolon,    ";", start_line, start_column};
        case ',': return Token{TokenType::Comma,        ",", start_line, start_column};
        case '(': return Token{TokenType::LeftParen,    "(", start_line, start_column};
        case ')': return Token{TokenType::RightParen,   ")", start_line, start_column};
        case '{': return Token{TokenType::LeftBrace,    "{", start_line, start_column};
        case '}': return Token{TokenType::RightBrace,   "}", start_line, start_column};
        case '[': return Token{TokenType::LeftBracket,  "[", start_line, start_column};
        case ']': return Token{TokenType::RightBracket, "]", start_line, start_column};

        case '!':
            if (match('=')) return Token{TokenType::NotEqual,     "!=", start_line, start_column};
            return Token{TokenType::Not, "!", start_line, start_column};

        case '=':
            if (match('=')) return Token{TokenType::Equal,  "==", start_line, start_column};
            return Token{TokenType::Assign, "=", start_line, start_column};

        case '<':
            if (match('=')) return Token{TokenType::LessEqual,    "<=", start_line, start_column};
            return Token{TokenType::Less, "<", start_line, start_column};

        case '>':
            if (match('=')) return Token{TokenType::GreaterEqual, ">=", start_line, start_column};
            return Token{TokenType::Greater, ">", start_line, start_column};

        case '&':
            if (match('&')) return Token{TokenType::And, "&&", start_line, start_column};
            break;

        case '|':
            if (match('>')) return Token{TokenType::PipeArrow, "|>", start_line, start_column};
            break;

        case '.':
            if (current() == '.' && peek() == '.') {
                advance(); advance();
                return Token{TokenType::Ellipsis, "...", start_line, start_column};
            }
            return Token{TokenType::Dot, ".", start_line, start_column};
    }

    // 遇到无法识别的字符，报错
    throw std::runtime_error(
        "🌮 line " + std::to_string(start_line) +
        ": Unexpected character '" + c + "'."
    );
}

// ────────────────────────────────
// 核心接口
// ────────────────────────────────

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        Token t = next_token();
        tokens.push_back(t);
        if (t.type == TokenType::EndOfFile) break;
    }

    return tokens;
}
```

---

## 6.4 测试：把 `var x = 10;` 切成 Token 列表

### main.cpp

```cpp
#include <iostream>
#include "lexer.h"
#include "token.h"

int main() {
    std::string source = R"(
var x = 10 + 20;
var name = "Miguel";
var flag = true;

func greet(name) {
    print("Hola, " + name + "!");
}
)";

    Lexer lexer(source);

    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenize();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    // 打印每个 Token
    for (const auto& token : tokens) {
        std::cout
            << "[" << token_type_to_string(token.type) << "] "
            << "\"" << token.value << "\""
            << " (line " << token.line << ")\n";
    }

    return 0;
}
```

### 更新 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_executable(taco
    src/main.cpp
    src/token.cpp
    src/lexer.cpp
)

target_include_directories(taco PRIVATE src)
```

### 编译运行

```bash
cmake -B build
cmake --build build
./build/taco
```

### 输出

```
[VAR] "var" (line 2)
[IDENTIFIER] "x" (line 2)
[ASSIGN] "=" (line 2)
[NUMBER] "10" (line 2)
[PLUS] "+" (line 2)
[NUMBER] "20" (line 2)
[SEMICOLON] ";" (line 2)
[VAR] "var" (line 3)
[IDENTIFIER] "name" (line 3)
[ASSIGN] "=" (line 3)
[STRING] "Miguel" (line 3)
[SEMICOLON] ";" (line 3)
[VAR] "var" (line 4)
[IDENTIFIER] "flag" (line 4)
[ASSIGN] "=" (line 4)
[TRUE] "true" (line 4)
[SEMICOLON] ";" (line 4)
[FUNC] "func" (line 6)
[IDENTIFIER] "greet" (line 6)
[LEFT_PAREN] "(" (line 6)
[IDENTIFIER] "name" (line 6)
[RIGHT_PAREN] ")" (line 6)
[LEFT_BRACE] "{" (line 6)
[IDENTIFIER] "print" (line 7)
[LEFT_PAREN] "(" (line 7)
[STRING] "Hola, " (line 7)
[PLUS] "+" (line 7)
[IDENTIFIER] "name" (line 7)
[PLUS] "+" (line 7)
[STRING] "!" (line 7)
[RIGHT_PAREN] ")" (line 7)
[SEMICOLON] ";" (line 7)
[RIGHT_BRACE] "}" (line 8)
[EOF] "" (line 9)
```

词法分析器正确地把源代码切成了 Token 列表。关键字 `var`、`func`、`true` 被识别为对应的类型，而不是标识符。字符串的内容（`"Miguel"`）去掉了引号，只保留内容本身。

---

## 6.5 这个版本的局限性

v0 能正确识别 Token，但有几个明显的局限性：

**不支持字符串插值**

```taco
var greeting = "Hola, {name}!";
```

`"Hola, {name}!"` 现在被当成一个普通字符串，不会把 `{name}` 识别成插值表达式。字符串插值需要在读取字符串时，把 `{...}` 里的内容单独切出来，作为一个表达式处理。这需要解析器的配合，v1 再处理。

**没有行列号精确追踪**

当前的行号追踪是粗糙的——只在遇到 `\n` 时更新行号。列号的追踪也不够准确。v1 引入 AST 之后，会用行列号来生成精确的错误信息。

**没有从文件读取**

现在源代码是硬编码在 `main.cpp` 里的字符串。下一步要从 `.taco` 文件读取，并处理命令行参数。

**没有彩蛋逻辑**

🌮 Token 现在只是被识别出来，但没有任何特殊行为。彩蛋逻辑（Magic 8 Ball、魔法海螺等）会在 v6 的 REPL 里实现——在那之前，🌮 只是一个普通的 Token。

---

## 小结

v0 完成了解释器的第一层：词法分析器。

**Token** 是词法分析的输出单元，包含类型和原始文本。`enum class` 比普通 `enum` 更安全，是定义 TokenType 的正确方式。

**Lexer** 的核心逻辑是 `next_token()`：跳过空白和注释，然后根据当前字符决定读哪种 Token。关键字通过查表（`unordered_map`）来识别，避免了一大堆 `if/else`。

**错误处理** 现在很简单：遇到无法识别的字符就抛出异常，打印带行号的错误信息。这符合 Taco 的设计——出错直接崩溃，信息要清晰。

---

下一章开始进入第二部分：类与对象。学完类和继承之后，第十一章会用这些知识构建 AST，并实现语法分析器（v1）。
# 第七章：类的基础

---

第一部分讲的是 C++ 的"升级版 C"——那些让 C 写起来更舒服的特性。从这一章开始进入第二部分：类与对象。

类是 C++ 最核心的特性之一，也是和 C 差距最大的地方。Python 里已经有类的概念，所以这里不从零解释"什么是封装"，而是直接对比：C++ 的类和 Python 的类有什么相同，有什么不同，为什么会有这些不同。

---

## 7.1 从 Python 的类到 C++ 的类：对比切入

先来看同一个概念在两种语言里的写法。

**Python 版本**：

```python
class Token:
    def __init__(self, type, value):
        self.type = type
        self.value = value

    def to_string(self):
        return f"[{self.type}] \"{self.value}\""

t = Token("NUMBER", "42")
print(t.to_string())  # [NUMBER] "42"
```

**C++ 版本**：

```cpp
#include <string>
#include <iostream>

class Token {
public:
    std::string type;
    std::string value;

    Token(std::string type, std::string value)
        : type(std::move(type)), value(std::move(value))
    {}

    std::string to_string() const {
        return "[" + type + "] \"" + value + "\"";
    }
};

int main() {
    Token t("NUMBER", "42");
    std::cout << t.to_string() << "\n";  // [NUMBER] "42"
}
```

表面上很相似：都有数据成员、构造函数、成员函数。但有几个关键区别：

**区别一：必须声明类型**

Python 里 `self.type = type` 就直接创建了成员变量，类型是运行时决定的。C++ 里必须在类定义里声明每个成员变量的类型：`std::string type;`。

**区别二：访问控制**

C++ 有 `public`、`private`、`protected` 关键字，明确控制哪些东西外部可以访问。Python 里只有约定（以 `_` 开头表示"私有"），没有强制性的访问控制。

**区别三：构造函数的写法**

Python 的构造函数叫 `__init__`，C++ 的构造函数和类同名（`Token`）。C++ 还有初始化列表（`: type(...), value(...)`），这是 C++ 特有的语法，第八章会详细讲。

**区别四：`const` 成员函数**

C++ 的 `to_string()` 后面有一个 `const`，表示这个函数不会修改对象的状态。Python 没有这个概念——任何函数都可以修改 `self`。

**区别五：对象的创建方式**

Python 里 `t = Token("NUMBER", "42")` 返回的是一个对象引用，对象本身在堆上。C++ 里 `Token t("NUMBER", "42")` 默认在栈上创建对象，`Token* t = new Token("NUMBER", "42")` 才是在堆上。这个区别非常重要，第十五章讲智能指针时会深入讨论。

---

## 7.2 成员变量与成员函数

### 成员变量

成员变量（member variable）是类的数据部分，描述对象的状态：

```cpp
class Lexer {
    std::string m_source;  // 源代码
    int         m_pos;     // 当前位置
    int         m_line;    // 当前行号
};
```

命名约定：很多 C++ 项目用 `m_` 前缀表示成员变量（member），区别于局部变量和参数。这不是语言要求，但是一个有用的习惯——读代码时一眼就能看出哪些是成员变量。

成员变量可以在声明时给默认值（C++11 起）：

```cpp
class Lexer {
    std::string m_source;
    int m_pos    = 0;     // 默认值
    int m_line   = 1;     // 默认值
    int m_column = 1;     // 默认值
};
```

---

### 成员函数

成员函数（member function）是类的行为部分，定义对象能做什么：

```cpp
class Lexer {
public:
    // 成员函数：可以访问 m_source、m_pos 等成员变量
    char current() const {
        if (is_at_end()) return '\0';
        return m_source[m_pos];
    }

    bool is_at_end() const {
        return m_pos >= static_cast<int>(m_source.size());
    }
};
```

成员函数可以直接访问同一个类的成员变量，不需要传参数。这就是"封装"的核心——数据和操作数据的函数绑定在一起。

**在类外定义成员函数**

成员函数可以在类定义里声明，在类外实现：

```cpp
// lexer.h：只声明
class Lexer {
public:
    char current() const;
    bool is_at_end() const;
private:
    std::string m_source;
    int m_pos = 0;
};

// lexer.cpp：实现，用 Lexer:: 前缀表明属于 Lexer 类
char Lexer::current() const {
    if (is_at_end()) return '\0';
    return m_source[m_pos];
}

bool Lexer::is_at_end() const {
    return m_pos >= static_cast<int>(m_source.size());
}
```

这是大型项目里的标准做法：头文件只放接口（声明），源文件放实现。

---

## 7.3 访问控制：public、private、protected

C++ 用三个关键字控制成员的访问权限：

- `public`：任何地方都可以访问
- `private`：只有类自己的成员函数可以访问
- `protected`：类自己和子类可以访问（第十二章讲继承时再展开）

```cpp
class BankAccount {
public:
    // 公开接口：外部可以调用
    void deposit(double amount) {
        if (amount > 0) m_balance += amount;
    }

    double get_balance() const {
        return m_balance;
    }

private:
    // 私有数据：外部无法直接访问和修改
    double m_balance = 0.0;
    std::string m_owner;
};

BankAccount account;
account.deposit(100.0);          // 可以，deposit 是 public
double b = account.get_balance(); // 可以，get_balance 是 public
account.m_balance = 9999.0;      // 错误！m_balance 是 private
```

**为什么要 private？**

把数据设为 private，强迫外部代码通过公开接口操作对象，而不是直接修改数据。这样有几个好处：

- 可以在接口里做检查（`if (amount > 0)`），防止非法操作
- 内部实现可以改变，只要接口不变，外部代码不需要修改
- 读代码时，public 部分就是类的"说明书"，private 是实现细节

**struct 和 class 的区别**

C++ 里 `struct` 和 `class` 几乎完全相同，只有一个区别：`struct` 的成员默认是 `public`，`class` 的成员默认是 `private`。

```cpp
struct Point {
    double x;  // 默认 public
    double y;  // 默认 public
};

class Token {
    // 这里是 private！
    std::string value;
public:
    // 这里才是 public
    std::string get_value() const { return value; }
};
```

惯例上，`struct` 用于简单的数据容器（不需要封装），`class` 用于有复杂行为的对象（需要封装）。在 Taco 项目里，`Token` 用 `struct`（只是数据），`Lexer` 用 `class`（有复杂的内部逻辑）。

---

## 7.4 this 指针

在成员函数里，有一个隐式的指针叫 `this`，指向调用这个函数的对象本身：

```cpp
class Counter {
public:
    void increment() {
        this->count++;  // 等价于 count++
    }

    // 返回对象自身的引用，可以链式调用
    Counter& add(int n) {
        this->count += n;
        return *this;  // 解引用 this，返回对象本身
    }

    int get() const {
        return this->count;  // 等价于 count
    }

private:
    int count = 0;
};

Counter c;
c.add(5).add(3).add(2);  // 链式调用
std::cout << c.get();    // 10
```

大多数情况下不需要显式写 `this->`，编译器知道成员函数里的 `count` 就是 `this->count`。但有两种情况必须用 `this`：

**情况一：参数名和成员变量同名**

```cpp
class Token {
public:
    Token(std::string value) {
        this->value = value;  // this->value 是成员变量，value 是参数
    }
private:
    std::string value;
};
```

不过更好的做法是用初始化列表（下一章讲），或者给成员变量加 `m_` 前缀来避免歧义。

**情况二：返回对象自身**

```cpp
Counter& add(int n) {
    count += n;
    return *this;  // 必须用 this 才能返回对象自身的引用
}
```

---

### Taco 里的应用

在 Taco 项目里，`Lexer` 类用 `this` 的地方主要是构造函数。当前版本的 Lexer 用了成员变量初始化列表（下一章详细讲），所以 `this` 用得不多。但理解 `this` 的存在，有助于理解成员函数和普通函数的本质区别：

成员函数在底层其实就是一个普通函数，只不过编译器帮你把"调用这个函数的对象"作为第一个参数（`this`）传进去。这就是为什么 Python 里需要显式写 `self`，而 C++ 里 `this` 是隐式的。

```python
# Python：self 是显式参数
def to_string(self):
    return self.value
```

```cpp
// C++：this 是隐式指针
std::string to_string() const {
    return value;  // 等价于 this->value
}
```

---

## 小结

这一章介绍了 C++ 类的基础：

**成员变量**描述对象的状态，**成员函数**描述对象的行为。两者绑定在一起，这就是封装。

**访问控制**（`public`/`private`）让类可以暴露稳定的接口，隐藏实现细节。`struct` 默认 `public`，`class` 默认 `private`——根据是否需要封装来选择。

**`this` 指针**是每个成员函数里隐式存在的，指向当前对象。大多数时候不需要显式写，但返回对象自身时必须用 `*this`。

---

下一章讲构造函数和析构函数——对象是怎么诞生和消亡的，以及 C++ 里最重要的设计模式之一：RAII。
# 第八章：构造与析构

---

上一章看到了类的基本结构。这一章深入讲对象的生命周期：它是怎么被创建的，又是怎么被销毁的。

这是 C++ 和 Python 差距最大的地方之一。Python 有垃圾回收，对象什么时候销毁不需要关心。C++ 没有垃圾回收，但它用一套精妙的机制来弥补这个缺失——RAII。

---

## 8.1 构造函数：对象如何诞生

构造函数（constructor）在对象被创建时自动调用，负责初始化对象的状态。

```cpp
class Lexer {
public:
    // 构造函数：和类同名，没有返回类型
    Lexer(std::string source) {
        m_source = std::move(source);
        m_pos    = 0;
        m_line   = 1;
        m_column = 1;
    }

private:
    std::string m_source;
    int m_pos;
    int m_line;
    int m_column;
};

// 创建对象时，构造函数自动调用
Lexer lexer("var x = 10;");
```

**和 Python 的 `__init__` 对比**：

```python
class Lexer:
    def __init__(self, source):
        self.source = source
        self.pos = 0
        self.line = 1
        self.column = 1
```

功能完全相同。区别在于：
- Python 的 `__init__` 是特殊方法名，C++ 的构造函数是和类同名的函数
- Python 需要显式写 `self`，C++ 里是隐式的 `this`
- C++ 的构造函数没有返回类型（连 `void` 都没有）

---

### 默认构造函数

如果没有定义任何构造函数，编译器会自动生成一个**默认构造函数**（default constructor），它什么都不做：

```cpp
class Point {
public:
    double x;
    double y;
    // 没有定义构造函数
};

Point p;   // 使用默认构造函数，x 和 y 的值未定义（是内存里的随机数！）
Point p2{};  // 值初始化，x 和 y 被初始化为 0
```

注意：`Point p` 和 `Point p2{}` 的行为不同。对于内置类型（`int`、`double` 等），`p` 的成员变量值是未定义的（读取它是未定义行为），`p2` 的成员变量被初始化为 0。

一旦定义了任何构造函数，编译器就不再自动生成默认构造函数：

```cpp
class Lexer {
public:
    Lexer(std::string source) { ... }  // 定义了带参数的构造函数
};

Lexer lexer;  // 错误！没有默认构造函数
```

如果既需要带参数的构造函数，又需要默认构造函数，可以显式定义，或者用 `= default`：

```cpp
class Token {
public:
    Token() = default;  // 显式要求编译器生成默认构造函数
    Token(std::string type, std::string value)
        : m_type(std::move(type)), m_value(std::move(value)) {}

private:
    std::string m_type;
    std::string m_value;
};

Token t1;              // 使用默认构造函数
Token t2("NUMBER", "42");  // 使用带参数的构造函数
```

---

### explicit 关键字

构造函数可以触发隐式类型转换，有时候这不是我们想要的：

```cpp
class Lexer {
public:
    Lexer(std::string source) { ... }
};

void run(Lexer lexer) { ... }

run("var x = 10;");  // 可以！"var x = 10;" 隐式转换成 Lexer
```

这里 `"var x = 10;"` 是一个 `const char*`，它先被转换成 `std::string`，再被用来构造 `Lexer`。这个隐式转换可能让代码难以理解。

用 `explicit` 关键字禁止隐式转换：

```cpp
class Lexer {
public:
    explicit Lexer(std::string source) { ... }
};

run("var x = 10;");     // 错误！不能隐式转换
run(Lexer("var x = 10;"));  // 正确：显式构造
```

对于只有一个参数的构造函数，`explicit` 几乎是标配——可以防止意外的隐式转换。

---

## 8.2 析构函数：对象如何消亡

析构函数（destructor）在对象被销毁时自动调用。

```cpp
class FileReader {
public:
    FileReader(const std::string& path) {
        m_file = std::fopen(path.c_str(), "r");
        if (!m_file) {
            throw std::runtime_error("Cannot open file: " + path);
        }
    }

    // 析构函数：~ 加类名，没有参数，没有返回类型
    ~FileReader() {
        if (m_file) {
            std::fclose(m_file);  // 自动关闭文件
            m_file = nullptr;
        }
    }

    // ... 其他成员函数

private:
    FILE* m_file = nullptr;
};
```

析构函数何时被调用？

```cpp
void process_file() {
    FileReader reader("script.taco");  // 构造函数调用，文件打开
    // ... 使用 reader
}   // 函数结束，reader 离开作用域，析构函数自动调用，文件关闭
```

**Python 没有析构函数的直接对应物**。Python 有 `__del__`，但它的调用时机是不确定的（取决于垃圾回收器），不能用于可靠的资源清理。C++ 的析构函数是确定性的——对象离开作用域，析构函数立刻调用。

---

### 什么时候需要析构函数

如果类持有需要手动释放的资源，就需要析构函数：

- 动态分配的内存（`new` 出来的）
- 文件句柄
- 网络连接
- 锁（mutex）
- 任何需要配对操作的资源（open/close、lock/unlock、acquire/release）

如果类只包含普通成员变量（`int`、`std::string`、`std::vector` 等），不需要定义析构函数——编译器自动生成的析构函数会正确销毁所有成员变量。

---

## 8.3 初始化列表

回到 Lexer 的构造函数。上一节用的是赋值方式初始化：

```cpp
Lexer::Lexer(std::string source) {
    m_source = std::move(source);  // 赋值
    m_pos    = 0;
    m_line   = 1;
    m_column = 1;
}
```

C++ 提供了另一种方式：**初始化列表**（member initializer list）：

```cpp
Lexer::Lexer(std::string source)
    : m_source(std::move(source))  // 直接初始化
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}
```

冒号 `:` 后面是初始化列表，每个成员变量用括号括起来要初始化的值。

**为什么要用初始化列表？**

有几个原因：

**原因一：效率**

用赋值方式，成员变量先被默认构造（对于 `std::string`，这会创建一个空字符串），然后再被赋值。用初始化列表，成员变量直接用给定的值构造，省去了默认构造这一步。

对于 `int` 这类简单类型，差别不大。但对于 `std::string`、`std::vector` 这类复杂类型，初始化列表更高效。

**原因二：必要性**

有些成员变量必须用初始化列表：

- `const` 成员变量：`const` 变量不能赋值，只能初始化
- 引用成员变量：引用必须在声明时绑定，不能用赋值
- 没有默认构造函数的成员变量

```cpp
class Config {
public:
    Config(int max_tokens, const std::string& filename)
        : m_max_tokens(max_tokens)   // const int 必须用初始化列表
        , m_filename(filename)       // const std::string& 引用
    {}

private:
    const int m_max_tokens;          // const，不能赋值
    const std::string& m_filename;   // 引用，不能重新绑定
};
```

**初始化顺序**

成员变量的初始化顺序取决于它们在类里的**声明顺序**，而不是初始化列表里的顺序。

```cpp
class Example {
    int m_a;
    int m_b;
public:
    Example(int a, int b)
        : m_b(b)   // 虽然 m_b 写在前面
        , m_a(a)   // 但 m_a 先初始化（因为声明在前）
    {}
};
```

为了避免混淆，最好让初始化列表的顺序和声明顺序一致。

---

## 8.4 RAII 初步：资源与对象生命周期绑定

RAII（Resource Acquisition Is Initialization）是 C++ 里最重要的设计模式。名字有点绕，但思想很简单：

> 用对象的生命周期来管理资源。对象构造时获取资源，对象析构时释放资源。

这样，只要对象的生命周期管理好了，资源就自动管理好了——不需要手动释放，也不会忘记释放。

### 没有 RAII 的世界

```cpp
void process_file(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return;

    // 处理文件...
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f)) {
        // ... 处理每一行
        if (some_error_condition) {
            std::fclose(f);  // 如果忘了这行，内存泄漏！
            return;
        }
    }

    std::fclose(f);  // 正常情况下关闭
}
```

每个 `return` 之前都需要手动关闭文件。如果有异常、有多个 `return`，很容易漏掉。这是 C 代码里常见的 bug 来源。

### 有 RAII 的世界

```cpp
class FileGuard {
public:
    explicit FileGuard(const std::string& path)
        : m_file(std::fopen(path.c_str(), "r"))
    {
        if (!m_file) {
            throw std::runtime_error("Cannot open: " + path);
        }
    }

    ~FileGuard() {
        if (m_file) std::fclose(m_file);
    }

    FILE* get() const { return m_file; }

    // 禁止拷贝（文件句柄不应该被复制）
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;

private:
    FILE* m_file;
};

void process_file(const std::string& path) {
    FileGuard guard(path);  // 打开文件

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), guard.get())) {
        if (some_error_condition) {
            return;  // guard 离开作用域，析构函数自动关闭文件
        }
    }
    // 函数结束，guard 析构，文件自动关闭
}
```

不管函数从哪里 `return`，甚至抛出异常，`FileGuard` 的析构函数都会被调用，文件一定被关闭。这就是 RAII 的魔力。

### RAII 无处不在

标准库里的容器都是 RAII 的：

```cpp
{
    std::vector<int> v = {1, 2, 3};  // 构造，分配内存
    v.push_back(4);
    // ... 使用 v
}  // v 析构，内存自动释放

{
    std::string s = "hello";  // 构造，分配内存
    s += " world";
}  // s 析构，内存自动释放
```

`std::mutex` 的 RAII 包装：

```cpp
std::mutex mu;

void safe_increment(int& count) {
    std::lock_guard<std::mutex> lock(mu);  // 加锁
    count++;
    // 函数结束，lock 析构，自动解锁
    // 不需要手动 mu.unlock()
}
```

第十六到十七章讲智能指针时，还会看到更多 RAII 的应用。

---

### 在 Taco 里的应用

Taco 的 `Lexer` 类虽然简单，但已经用到了 RAII 的思想：

```cpp
Lexer::Lexer(std::string source)
    : m_source(std::move(source))
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}
```

`m_source` 是 `std::string`，它自己管理内存。`Lexer` 不需要析构函数——当 `Lexer` 对象销毁时，`m_source` 的析构函数会自动被调用，内存自动释放。

这就是 RAII 的另一面：不只是自己写析构函数，而是尽量用已经实现好了 RAII 的类型，让它们帮你管理资源。

---

## *More About 构造函数*：委托构造与 = delete / = default

> 第一次读可以跳过。

### 委托构造

如果多个构造函数有重复的初始化逻辑，可以用委托构造（delegating constructor）：

```cpp
class Token {
public:
    // 完整构造函数
    Token(std::string type, std::string value, int line, int column)
        : m_type(std::move(type))
        , m_value(std::move(value))
        , m_line(line)
        , m_column(column)
    {}

    // 委托构造：调用完整构造函数，line 和 column 用默认值
    Token(std::string type, std::string value)
        : Token(std::move(type), std::move(value), 0, 0)
    {}

private:
    std::string m_type;
    std::string m_value;
    int m_line;
    int m_column;
};
```

委托构造避免了重复代码，初始化逻辑只写一次。

### = delete 和 = default

`= delete` 显式删除某个函数，让编译器报错而不是生成默认版本：

```cpp
class Lexer {
public:
    explicit Lexer(std::string source);

    // 禁止拷贝：Lexer 不应该被复制
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;

    // 允许移动（第十七章讲）
    Lexer(Lexer&&) = default;
    Lexer& operator=(Lexer&&) = default;
};

Lexer a("var x = 10;");
Lexer b = a;  // 错误！拷贝构造函数被删除
```

`= default` 显式要求编译器生成默认实现：

```cpp
class Token {
public:
    Token() = default;      // 生成默认构造函数
    ~Token() = default;     // 生成默认析构函数
    Token(const Token&) = default;  // 生成默认拷贝构造函数
};
```

这在需要明确表达意图时很有用：显式写 `= default` 比什么都不写更清晰——让读代码的人知道"这不是忘了写，而是故意让编译器生成"。

---

## 小结

这一章讲了对象的生命周期：

**构造函数**在对象创建时自动调用，负责初始化。`explicit` 防止意外的隐式转换，是单参数构造函数的标配。

**析构函数**在对象销毁时自动调用，负责清理资源。如果类持有需要手动释放的资源，就需要析构函数。

**初始化列表**是初始化成员变量的正确方式，比赋值更高效，而且对 `const` 成员和引用成员是唯一选择。

**RAII** 是 C++ 最重要的设计模式：用对象的生命周期管理资源。构造时获取，析构时释放。不需要手动管理，也不会忘记释放。标准库里的 `vector`、`string`、`mutex` 都是 RAII 的。

---

下一章讲拷贝与赋值——当一个对象被复制时，C++ 做了什么，以及什么时候这会变得危险。
# 第九章：拷贝与赋值

---

上一章讲了对象的诞生（构造）和消亡（析构）。这一章讲另一件事：对象的复制。

在 Python 里，赋值几乎总是创建一个引用，指向同一个对象：

```python
a = [1, 2, 3]
b = a        # b 和 a 指向同一个列表
b.append(4)
print(a)     # [1, 2, 3, 4]，a 也变了
```

C++ 里则不同。默认情况下，赋值会创建一个真正的副本。但当对象里有指针时，这个"默认"行为会带来大麻烦。

---

## 9.1 拷贝构造函数

当一个对象被用来初始化另一个同类型对象时，**拷贝构造函数**（copy constructor）会被调用：

```cpp
Token t1("NUMBER", "42");
Token t2 = t1;   // 拷贝构造：用 t1 初始化 t2
Token t3(t1);    // 同上，另一种写法

void print_token(Token t) { ... }  // 传值时也会触发拷贝构造
print_token(t1);
```

如果没有定义拷贝构造函数，编译器会自动生成一个——它会逐个拷贝所有成员变量。对于 `Token` 这样的简单类，这个默认行为完全够用：

```cpp
class Token {
public:
    std::string type;
    std::string value;
    int line;
    int column;
};

Token t1{"NUMBER", "42", 1, 5};
Token t2 = t1;
// t2.type == "NUMBER"，t2.value == "42"，t2.line == 1，t2.column == 5
// t2 是 t1 的完整副本，修改 t2 不影响 t1
```

---

## 9.2 拷贝赋值运算符

拷贝赋值运算符（copy assignment operator）在对象已经存在的情况下被赋值时调用：

```cpp
Token t1{"NUMBER", "42", 1, 5};
Token t2{"STRING", "hello", 2, 3};

t2 = t1;   // 拷贝赋值：t2 已存在，把 t1 的内容复制给 t2
```

编译器同样会自动生成一个拷贝赋值运算符，逐个拷贝成员变量。

拷贝构造函数和拷贝赋值运算符的区别：

```cpp
Token t2 = t1;  // 拷贝构造（t2 是新创建的）
t2 = t1;        // 拷贝赋值（t2 已存在）
```

---

## 9.3 Rule of Three

这里有一个重要的规则：**Rule of Three**（三法则）。

> 如果一个类需要自定义以下三个中的任何一个，那么它通常需要自定义全部三个：
> 1. 析构函数
> 2. 拷贝构造函数
> 3. 拷贝赋值运算符

为什么？因为需要自定义析构函数，通常意味着类里有需要手动管理的资源（比如裸指针）。如果有裸指针，默认的拷贝行为就会出问题——它只拷贝指针的值（地址），而不是指针指向的数据。

---

## 9.4 为什么拷贝有时候很危险

来看一个典型的例子，一个简单的字符串类（不用 `std::string`，手动管理内存）：

```cpp
class MyString {
public:
    MyString(const char* str) {
        m_length = std::strlen(str);
        m_data = new char[m_length + 1];  // 在堆上分配内存
        std::strcpy(m_data, str);
    }

    ~MyString() {
        delete[] m_data;  // 释放内存
    }

    void print() const {
        std::cout << m_data << "\n";
    }

private:
    char* m_data;
    int   m_length;
};
```

这个类有析构函数，但没有自定义拷贝构造函数和拷贝赋值运算符。会发生什么？

```cpp
MyString a("hello");
MyString b = a;   // 使用编译器生成的默认拷贝构造函数
```

默认拷贝构造函数做的事情：把 `a.m_data`（一个指针地址）复制给 `b.m_data`，把 `a.m_length` 复制给 `b.m_length`。

现在 `a.m_data` 和 `b.m_data` 指向**同一块内存**：

```
a.m_data ──┐
           ▼
         [h][e][l][l][o][\0]
           ▲
b.m_data ──┘
```

这就是**浅拷贝**（shallow copy）。问题来了：

```cpp
{
    MyString b = a;
}  // b 析构，delete[] b.m_data，释放了那块内存

a.print();  // 未定义行为！a.m_data 指向已经被释放的内存
```

`b` 析构时，`delete[] b.m_data` 释放了那块内存。但 `a.m_data` 还指向那里。之后访问 `a`，就是访问已释放的内存，程序可能崩溃，也可能输出乱码，这是一种严重的 bug。

更糟糕的是，当 `a` 自己析构时，`delete[] a.m_data` 会**再次释放同一块内存**（double free），这是未定义行为，通常会导致崩溃。

---

### 正确的做法：深拷贝

解决方法是实现**深拷贝**（deep copy）——不只复制指针，而是复制指针指向的数据：

```cpp
class MyString {
public:
    MyString(const char* str) {
        m_length = std::strlen(str);
        m_data = new char[m_length + 1];
        std::strcpy(m_data, str);
    }

    // 拷贝构造函数：深拷贝
    MyString(const MyString& other) {
        m_length = other.m_length;
        m_data = new char[m_length + 1];  // 分配新内存
        std::strcpy(m_data, other.m_data); // 复制数据
    }

    // 拷贝赋值运算符：深拷贝
    MyString& operator=(const MyString& other) {
        if (this == &other) return *this;  // 自我赋值检查

        delete[] m_data;  // 释放旧内存

        m_length = other.m_length;
        m_data = new char[m_length + 1];
        std::strcpy(m_data, other.m_data);

        return *this;
    }

    ~MyString() {
        delete[] m_data;
    }

private:
    char* m_data;
    int   m_length;
};
```

现在 `MyString b = a` 会分配新的内存，复制数据，两个对象互相独立：

```
a.m_data ──→ [h][e][l][l][o][\0]

b.m_data ──→ [h][e][l][l][o][\0]  （独立的副本）
```

注意拷贝赋值运算符里的**自我赋值检查**：`if (this == &other) return *this;`。如果写了 `a = a`，没有这个检查，`delete[] m_data` 会先释放自己的数据，然后再试图复制已经释放的数据——又是未定义行为。

---

### 现代 C++ 的建议

手动管理内存、手动实现深拷贝，这是 C++ 很容易出错的地方。现代 C++ 的建议是：**尽量不要手动管理内存**。

用 `std::string` 而不是 `char*`，用 `std::vector` 而不是裸数组，用 `std::unique_ptr` 而不是裸指针。这些标准库类都已经正确实现了深拷贝，用它们就不需要自己写拷贝构造函数和拷贝赋值运算符。

在 Taco 项目里，`Token` 的成员都是 `std::string` 和 `int`，`Lexer` 的成员是 `std::string` 和 `int`——都不涉及裸指针，所以不需要手动实现拷贝。

Rule of Three 在现代 C++ 里通常这样理解：**如果你觉得需要自定义析构函数，先想想能不能换成用标准库类来管理资源，从而完全避免手动实现拷贝**。

---

## *More About 拷贝*：深拷贝 vs 浅拷贝的底层图景

> 第一次读可以跳过。

### 浅拷贝的完整图景

浅拷贝只复制对象表面的内容：

```
对象 a：
  [m_data: 0x1234] [m_length: 5]
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存

浅拷贝后对象 b：
  [m_data: 0x1234] [m_length: 5]  ← m_data 和 a 相同
         │
         ▼（和 a 指向同一块内存）
       [h][e][l][l][o][\0]
```

浅拷贝之后：
- 修改 `b.m_data[0] = 'H'` 会同时影响 `a`
- `b` 析构后，`a.m_data` 变成悬空指针
- `a` 析构时，double free

### 深拷贝的完整图景

深拷贝复制对象的全部内容，包括指针指向的数据：

```
对象 a：
  [m_data: 0x1234] [m_length: 5]
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存 A

深拷贝后对象 b：
  [m_data: 0x5678] [m_length: 5]  ← m_data 指向不同位置
         │
         ▼
       [h][e][l][l][o][\0]  ← 堆上的内存 B（全新分配，内容相同）
```

深拷贝之后：
- 修改 `b` 不影响 `a`
- `b` 析构时，只释放内存 B
- `a` 析构时，只释放内存 A

### std::string 怎么做的

`std::string` 内部通常包含一个指向字符数据的指针（或者对短字符串用内联存储优化）。它实现了正确的深拷贝：

```cpp
std::string a = "hello";
std::string b = a;  // 深拷贝：b 有自己独立的字符数组

b[0] = 'H';
std::cout << a;  // "hello"，a 不受影响
std::cout << b;  // "Hello"
```

这就是为什么用 `std::string` 而不是 `char*` 可以避免很多问题——`std::string` 已经帮你处理好了所有的内存管理细节。

---

## 小结

这一章讲了 C++ 的拷贝机制：

**拷贝构造函数**在用一个对象初始化另一个对象时调用，**拷贝赋值运算符**在对已存在的对象赋值时调用。

**Rule of Three**：如果需要自定义析构函数，通常也需要自定义拷贝构造函数和拷贝赋值运算符。

**浅拷贝**只复制指针值，导致两个对象共享同一块内存，会产生 double free 和悬空指针等严重 bug。**深拷贝**分配新内存，复制数据，两个对象独立。

**现代 C++ 的建议**：用标准库类（`std::string`、`std::vector`、智能指针）代替裸指针，从根本上避免手动实现深拷贝。

---

下一章讲运算符重载——让自定义类型支持 `+`、`==`、`<<` 等运算符，让代码读起来更自然。
# 第十章：运算符重载

---

运算符重载（operator overloading）让自定义类型可以使用 `+`、`==`、`<<` 这样的运算符，让代码读起来像操作内置类型一样自然。

Python 里也有运算符重载，通过 `__add__`、`__eq__`、`__str__` 等魔法方法实现。C++ 的机制不同，但思路相同。

---

## 10.1 什么是运算符重载

当写 `a + b` 时，编译器实际上在调用一个函数。对于内置类型（`int`、`double`），这个函数是语言内置的。对于自定义类型，可以定义这个函数的行为——这就是运算符重载。

```cpp
// 这两行等价
Token result = t1 + t2;
Token result = operator+(t1, t2);  // 编译器看到的实际调用
```

运算符重载有两种写法：**成员函数**和**非成员函数**。

---

## 10.2 常用运算符重载

### == 和 != ：相等比较

在 Taco 里，比较两个 Token 是否相同很常见（比如检查当前 Token 是不是某个关键字）：

```cpp
// 成员函数写法
class Token {
public:
    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }

    bool operator!=(const Token& other) const {
        return !(*this == other);  // 复用 == 的逻辑
    }
};

Token t1{TokenType::Var, "var", 1, 1};
Token t2{TokenType::Var, "var", 1, 1};
Token t3{TokenType::Identifier, "x", 1, 5};

if (t1 == t2) { ... }  // true
if (t1 != t3) { ... }  // true
```

也可以只比较类型，不比较值：

```cpp
// 更常用的写法：直接和 TokenType 比较
bool operator==(TokenType t) const {
    return type == t;
}

// 使用
if (current_token == TokenType::Var) { ... }
```

---

### << ：输出运算符

重载 `<<` 让对象可以直接用 `std::cout` 输出：

```cpp
// 非成员函数写法（推荐）
// 因为左边是 std::ostream，不是 Token，不能写成 Token 的成员函数
std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "[" << token_type_to_string(token.type) << "] "
       << "\"" << token.value << "\""
       << " (line " << token.line << ")";
    return os;  // 返回 os 支持链式调用
}

// 使用
Token t{TokenType::Number, "42", 1, 5};
std::cout << t << "\n";  // [NUMBER] "42" (line 1)

// 链式调用
std::cout << "Token: " << t << ", next: " << t2 << "\n";
```

`<<` 运算符必须返回 `std::ostream&`，才能支持链式调用（`cout << a << b << c`）。

---

### + ：加法（字符串拼接的例子）

假设要实现一个简单的值类型，支持字符串拼接：

```cpp
class TacoString {
public:
    explicit TacoString(std::string s) : m_value(std::move(s)) {}

    // 成员函数写法
    TacoString operator+(const TacoString& other) const {
        return TacoString(m_value + other.m_value);
    }

    std::string value() const { return m_value; }

private:
    std::string m_value;
};

TacoString a("Hello, ");
TacoString b("World!");
TacoString c = a + b;
std::cout << c.value() << "\n";  // Hello, World!
```

---

### [] ：下标运算符

让对象支持 `obj[index]` 的访问方式：

```cpp
class TokenStream {
public:
    // 只读版本（const）
    const Token& operator[](int index) const {
        return m_tokens[index];
    }

    // 可写版本（非 const）
    Token& operator[](int index) {
        return m_tokens[index];
    }

    int size() const { return static_cast<int>(m_tokens.size()); }

private:
    std::vector<Token> m_tokens;
};

TokenStream stream;
// ...
Token first = stream[0];  // 使用 [] 运算符
```

---

### 布尔转换：operator bool

让对象可以在 `if` 条件里使用：

```cpp
class LexerResult {
public:
    explicit operator bool() const {
        return m_success;
    }

    bool m_success;
    std::vector<Token> m_tokens;
    std::string m_error;
};

LexerResult result = run_lexer("var x = 10;");
if (result) {  // 使用 operator bool
    // 成功
} else {
    std::cerr << result.m_error << "\n";
}
```

注意 `explicit` 关键字：它防止 `LexerResult` 在不期望的地方被隐式转换成 `bool`（比如用来做算术运算）。

---

## 10.3 什么时候该重载，什么时候不该

运算符重载可以让代码更简洁，但滥用会让代码更难懂。

**应该重载的情况**：

- 运算符的语义对于这个类型来说很自然（`+` 对字符串，`==` 对值类型）
- 代码读起来更清晰，而不是更神秘
- 标准库或常见约定期望这个类型支持某个运算符（比如放入 `std::map` 需要 `<`，`std::cout` 输出需要 `<<`）

**不应该重载的情况**：

- 运算符的含义不明显（`&` 重载成"查找"，`*` 重载成"执行"）
- 只是为了让代码看起来酷，但实际上增加了理解负担
- 有更清晰的命名函数可以代替

一个常见的反例是把 `+` 重载成完全不相关的操作：

```cpp
// 糟糕的设计
Lexer operator+(const Lexer& a, const std::string& extra_source) {
    // 把 extra_source 追加到 a 的源代码？
    // 这让人完全看不懂
}
```

**Python 对比**：

Python 的魔法方法和 C++ 的运算符重载思路相同：

```python
class Token:
    def __eq__(self, other):
        return self.type == other.type and self.value == other.value

    def __repr__(self):
        return f'[{self.type}] "{self.value}"'
```

```cpp
// C++ 对应
bool operator==(const Token& other) const { ... }
std::ostream& operator<<(std::ostream& os, const Token& t) { ... }
```

`__repr__` 在 C++ 里对应 `<<` 运算符（没有完全对应的，但 `<<` 是最接近的输出接口）。

---

### Taco 项目里的运算符重载

在 Taco 解释器里，目前最有用的是：

```cpp
// Token 的相等比较：检查 Token 类型
bool Token::operator==(TokenType t) const {
    return type == t;
}

// Token 的输出：调试时打印 Token 列表
std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "[" << token_type_to_string(token.type) << "] \""
       << token.value << "\"";
    return os;
}
```

用这两个，`main.cpp` 的调试输出可以改成：

```cpp
for (const auto& token : tokens) {
    std::cout << token << "\n";  // 直接用 << 输出
}

// 检查当前 token 是不是 var 关键字
if (tokens[0] == TokenType::Var) { ... }
```

---

## 小结

**运算符重载**让自定义类型可以使用 C++ 的运算符，让代码读起来更自然。

常用的重载：`==`/`!=` 用于相等比较，`<<` 用于输出，`[]` 用于下标访问，`bool` 转换用于条件判断。

**成员函数**还是**非成员函数**的选择：如果运算符的左操作数是这个类型，用成员函数；如果左操作数是其他类型（比如 `<<` 的左边是 `ostream`），用非成员函数。

**什么时候重载**：运算符语义自然、代码更清晰时才重载。不要为了"酷"而重载，不明显的运算符只会让人困惑。

---

下一章是第二部分的项目章节：用类来重构词法分析器，并实现语法分析器和 AST（v1）。
# 第十一章：项目 v1——语法分析器与 AST

---

第二部分学完了类、构造析构、拷贝、运算符重载。现在用这些知识做一件真正有意义的事：把 Token 列表变成一棵抽象语法树（AST），并对树上的节点求值。

v1 结束时，Taco 能运行这样的代码：

```taco
var x = 10 + 20;
var y = x * 2;
var name = "Miguel";
print(x);       // 30
print(y);       // 60
print(name);    // Miguel
```

---

## 11.1 什么是抽象语法树（AST）

词法分析器把源代码变成 Token 列表。Token 列表是线性的，没有层次结构——它不知道 `10 + 20` 是一个完整的表达式，也不知道 `var x = 10 + 20` 里 `x` 是变量名、`10 + 20` 是初始值。

语法分析器（Parser）读取 Token 列表，按照语法规则把它们组织成有层次的树形结构，就是 AST。

```
源代码：var x = 10 + 20;

Token 列表（线性）：
[VAR] [x] [=] [10] [+] [20] [;]

AST（树形）：
VarDecl
├── name: "x"
└── value: BinaryExpr
    ├── left:  NumberLiteral(10)
    ├── op:    "+"
    └── right: NumberLiteral(20)
```

AST 捕捉了代码的语义结构：变量声明包含变量名和初始值，初始值是一个二元表达式，二元表达式有左操作数、运算符、右操作数。

---

## 11.2 用类表示 AST 节点

AST 节点天然适合用继承来表示：所有节点共享一个基类 `Expr`，不同种类的节点是子类。

v1 只实现基础表达式：数字、字符串、布尔值、nil、变量引用、二元运算、变量声明、函数调用（只支持 `print`）。

### ast.h

```cpp
#pragma once
#include <string>
#include <vector>
#include <memory>

// 前向声明，避免循环包含
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// ────────────────────────────────
// 基类：所有 AST 节点的父类
// ────────────────────────────────

struct Expr {
    virtual ~Expr() = default;

    // 每个节点都能把自己转成字符串（用于调试）
    virtual std::string to_string() const = 0;
};

// ────────────────────────────────
// 字面量节点
// ────────────────────────────────

struct NumberLiteral : Expr {
    double value;

    explicit NumberLiteral(double v) : value(v) {}

    std::string to_string() const override {
        // 去掉多余的小数点（42.0 显示为 42）
        if (value == static_cast<int>(value)) {
            return std::to_string(static_cast<int>(value));
        }
        return std::to_string(value);
    }
};

struct StringLiteral : Expr {
    std::string value;

    explicit StringLiteral(std::string v) : value(std::move(v)) {}

    std::string to_string() const override {
        return "\"" + value + "\"";
    }
};

struct BoolLiteral : Expr {
    bool value;

    explicit BoolLiteral(bool v) : value(v) {}

    std::string to_string() const override {
        return value ? "true" : "false";
    }
};

struct NilLiteral : Expr {
    std::string to_string() const override { return "nil"; }
};

// ────────────────────────────────
// 变量引用节点
// ────────────────────────────────

struct Identifier : Expr {
    std::string name;

    explicit Identifier(std::string n) : name(std::move(n)) {}

    std::string to_string() const override { return name; }
};

// ────────────────────────────────
// 二元表达式节点
// ────────────────────────────────

struct BinaryExpr : Expr {
    ExprPtr     left;
    std::string op;
    ExprPtr     right;

    BinaryExpr(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l))
        , op(std::move(o))
        , right(std::move(r))
    {}

    std::string to_string() const override {
        return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
    }
};

// ────────────────────────────────
// 一元表达式节点（! 和 -）
// ────────────────────────────────

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr     operand;

    UnaryExpr(std::string o, ExprPtr expr)
        : op(std::move(o))
        , operand(std::move(expr))
    {}

    std::string to_string() const override {
        return "(" + op + operand->to_string() + ")";
    }
};

// ────────────────────────────────
// 语句节点（有副作用，不返回值）
// ────────────────────────────────

struct VarDecl : Expr {
    std::string name;
    ExprPtr     value;

    VarDecl(std::string n, ExprPtr v)
        : name(std::move(n))
        , value(std::move(v))
    {}

    std::string to_string() const override {
        return "var " + name + " = " + value->to_string();
    }
};

struct PrintStmt : Expr {
    ExprPtr argument;

    explicit PrintStmt(ExprPtr arg) : argument(std::move(arg)) {}

    std::string to_string() const override {
        return "print(" + argument->to_string() + ")";
    }
};

// 整个程序：一组语句
struct Program : Expr {
    std::vector<ExprPtr> statements;

    std::string to_string() const override {
        std::string result;
        for (const auto& stmt : statements) {
            result += stmt->to_string() + "\n";
        }
        return result;
    }
};
```

这里用了 `std::unique_ptr<Expr>`（别名 `ExprPtr`）来管理 AST 节点的所有权。第十六、十七章会详细讲智能指针，现在只需要知道：`unique_ptr` 会在对象销毁时自动释放内存，不需要手动 `delete`。

---

## 11.3 实现递归下降解析器

语法分析器用**递归下降**（recursive descent）方法：对每种语法结构写一个函数，函数之间互相调用，自然形成递归。

这是最直观的解析器实现方式，也是大多数手写解析器（包括 V8、Clang 等真实编译器）采用的方式。

### parser.h

```cpp
#pragma once
#include "token.h"
#include "ast.h"
#include <vector>
#include <stdexcept>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // 核心接口：解析整个程序
    std::unique_ptr<Program> parse();

private:
    std::vector<Token> m_tokens;
    int m_pos;  // 当前读取位置

    // ── 辅助函数 ──
    const Token& current() const;
    const Token& peek(int offset = 1) const;
    const Token& advance();
    bool is_at_end() const;

    // 如果当前 Token 类型匹配，消费并返回 true
    bool match(TokenType type);

    // 期望当前 Token 是某种类型，不是就报错
    const Token& expect(TokenType type, const std::string& message);

    // 报错
    [[noreturn]] void error(const std::string& message);

    // ── 解析函数（从上到下，优先级从低到高）──
    ExprPtr parse_statement();
    ExprPtr parse_var_decl();
    ExprPtr parse_print_stmt();
    ExprPtr parse_expression();
    ExprPtr parse_comparison();
    ExprPtr parse_addition();
    ExprPtr parse_multiplication();
    ExprPtr parse_unary();
    ExprPtr parse_primary();
};
```

### parser.cpp

```cpp
#include "parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))
    , m_pos(0)
{}

// ────────────────────────────────
// 辅助函数
// ────────────────────────────────

const Token& Parser::current() const {
    return m_tokens[m_pos];
}

const Token& Parser::peek(int offset) const {
    int idx = m_pos + offset;
    if (idx >= static_cast<int>(m_tokens.size())) {
        return m_tokens.back();  // 返回 EOF Token
    }
    return m_tokens[idx];
}

const Token& Parser::advance() {
    const Token& t = m_tokens[m_pos];
    if (!is_at_end()) m_pos++;
    return t;
}

bool Parser::is_at_end() const {
    return current().type == TokenType::EndOfFile;
}

bool Parser::match(TokenType type) {
    if (current().type == type) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& message) {
    if (current().type != type) {
        error(message);
    }
    return advance();
}

void Parser::error(const std::string& message) {
    throw std::runtime_error(
        "🌮 line " + std::to_string(current().line) + ": " + message
    );
}

// ────────────────────────────────
// 解析整个程序
// ────────────────────────────────

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();

    while (!is_at_end()) {
        program->statements.push_back(parse_statement());
    }

    return program;
}

// ────────────────────────────────
// 语句
// ────────────────────────────────

ExprPtr Parser::parse_statement() {
    if (current().type == TokenType::Var) {
        return parse_var_decl();
    }

    // print(...) 是内置函数，单独处理
    if (current().type == TokenType::Identifier &&
        current().value == "print") {
        return parse_print_stmt();
    }

    // 其他情况：把表达式当语句
    auto expr = parse_expression();
    expect(TokenType::Semicolon, "Expected ';' after expression.");
    return expr;
}

ExprPtr Parser::parse_var_decl() {
    expect(TokenType::Var, "Expected 'var'.");

    const Token& name_token = expect(
        TokenType::Identifier,
        "Expected variable name after 'var'."
    );
    std::string name = name_token.value;

    expect(TokenType::Assign, "Expected '=' after variable name.");

    ExprPtr value = parse_expression();

    expect(TokenType::Semicolon, "Expected ';' after variable declaration.");

    return std::make_unique<VarDecl>(name, std::move(value));
}

ExprPtr Parser::parse_print_stmt() {
    advance();  // 消费 "print"

    expect(TokenType::LeftParen, "Expected '(' after 'print'.");
    ExprPtr arg = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after print argument.");
    expect(TokenType::Semicolon, "Expected ';' after print statement.");

    return std::make_unique<PrintStmt>(std::move(arg));
}

// ────────────────────────────────
// 表达式（优先级从低到高）
// ────────────────────────────────

// expression → comparison
ExprPtr Parser::parse_expression() {
    return parse_comparison();
}

// comparison → addition (("==" | "!=" | "<" | ">" | "<=" | ">=") addition)*
ExprPtr Parser::parse_comparison() {
    ExprPtr left = parse_addition();

    while (current().type == TokenType::Equal      ||
           current().type == TokenType::NotEqual   ||
           current().type == TokenType::Less       ||
           current().type == TokenType::Greater    ||
           current().type == TokenType::LessEqual  ||
           current().type == TokenType::GreaterEqual) {
        std::string op = advance().value;
        ExprPtr right = parse_addition();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// addition → multiplication (("+" | "-") multiplication)*
ExprPtr Parser::parse_addition() {
    ExprPtr left = parse_multiplication();

    while (current().type == TokenType::Plus ||
           current().type == TokenType::Minus) {
        std::string op = advance().value;
        ExprPtr right = parse_multiplication();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// multiplication → unary (("*" | "/" | "%" | "^") unary)*
ExprPtr Parser::parse_multiplication() {
    ExprPtr left = parse_unary();

    while (current().type == TokenType::Star    ||
           current().type == TokenType::Slash   ||
           current().type == TokenType::Percent ||
           current().type == TokenType::Caret) {
        std::string op = advance().value;
        ExprPtr right = parse_unary();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// unary → ("!" | "-") unary | primary
ExprPtr Parser::parse_unary() {
    if (current().type == TokenType::Not ||
        current().type == TokenType::Minus) {
        std::string op = advance().value;
        ExprPtr operand = parse_unary();
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    }

    return parse_primary();
}

// primary → NUMBER | STRING | "true" | "false" | "nil" | IDENTIFIER | "(" expression ")"
ExprPtr Parser::parse_primary() {
    const Token& token = current();

    if (token.type == TokenType::Number) {
        advance();
        return std::make_unique<NumberLiteral>(std::stod(token.value));
    }

    if (token.type == TokenType::String) {
        advance();
        return std::make_unique<StringLiteral>(token.value);
    }

    if (token.type == TokenType::True) {
        advance();
        return std::make_unique<BoolLiteral>(true);
    }

    if (token.type == TokenType::False) {
        advance();
        return std::make_unique<BoolLiteral>(false);
    }

    if (token.type == TokenType::Nil) {
        advance();
        return std::make_unique<NilLiteral>();
    }

    if (token.type == TokenType::Identifier) {
        advance();
        return std::make_unique<Identifier>(token.value);
    }

    // 括号表达式
    if (token.type == TokenType::LeftParen) {
        advance();  // 消费 "("
        ExprPtr expr = parse_expression();
        expect(TokenType::RightParen, "Expected ')' after expression.");
        return expr;
    }

    error("Unexpected token: '" + token.value + "'.");
}
```

---

### 为什么这样分层？

`parse_addition` 调用 `parse_multiplication`，`parse_multiplication` 调用 `parse_unary`，`parse_unary` 调用 `parse_primary`。这个调用层级对应的是**运算符优先级**：

```
优先级（从低到高）：
  == != < > <= >=   （比较）
  + -               （加减）
  * / % ^           （乘除）
  ! -               （一元）
  字面量、括号       （基本）
```

优先级高的运算符在调用链的更深处处理，所以它们先被"绑定"。这就是为什么 `2 + 3 * 4` 会解析成 `2 + (3 * 4)` 而不是 `(2 + 3) * 4`。

---

## 11.4 实现求值器

有了 AST，现在实现求值器——遍历 AST，对每个节点求值。

v1 的求值器很简单：没有变量作用域，用一个全局 `map` 存所有变量。

### evaluator.h

```cpp
#pragma once
#include "ast.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <iostream>

// Taco 的值类型：数字、字符串、布尔、nil
using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;

// 把 TacoValue 转成字符串（用于 print）
std::string value_to_string(const TacoValue& val);

class Evaluator {
public:
    // 求值入口
    TacoValue evaluate(const Expr* expr);

private:
    // 变量环境：变量名 → 值
    std::unordered_map<std::string, TacoValue> m_env;

    // 各类节点的求值
    TacoValue eval_number(const NumberLiteral* node);
    TacoValue eval_string(const StringLiteral* node);
    TacoValue eval_bool(const BoolLiteral* node);
    TacoValue eval_nil(const NilLiteral* node);
    TacoValue eval_identifier(const Identifier* node);
    TacoValue eval_binary(const BinaryExpr* node);
    TacoValue eval_unary(const UnaryExpr* node);
    TacoValue eval_var_decl(const VarDecl* node);
    TacoValue eval_print(const PrintStmt* node);
    TacoValue eval_program(const Program* node);
};
```

### evaluator.cpp

```cpp
#include "evaluator.h"
#include <stdexcept>
#include <cmath>

// ────────────────────────────────
// TacoValue 工具函数
// ────────────────────────────────

std::string value_to_string(const TacoValue& val) {
    if (std::holds_alternative<double>(val)) {
        double d = std::get<double>(val);
        if (d == static_cast<int>(d)) {
            return std::to_string(static_cast<int>(d));
        }
        return std::to_string(d);
    }
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    }
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? "true" : "false";
    }
    return "nil";
}

static bool is_truthy(const TacoValue& val) {
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val);
    }
    if (std::holds_alternative<std::nullptr_t>(val)) {
        return false;  // nil 是 falsy
    }
    return true;  // 其他值都是 truthy
}

// ────────────────────────────────
// 求值入口：根据节点类型分发
// ────────────────────────────────

TacoValue Evaluator::evaluate(const Expr* expr) {
    // 用 dynamic_cast 判断节点类型（第十四章会详细讲）
    if (auto* node = dynamic_cast<const NumberLiteral*>(expr))
        return eval_number(node);
    if (auto* node = dynamic_cast<const StringLiteral*>(expr))
        return eval_string(node);
    if (auto* node = dynamic_cast<const BoolLiteral*>(expr))
        return eval_bool(node);
    if (auto* node = dynamic_cast<const NilLiteral*>(expr))
        return eval_nil(node);
    if (auto* node = dynamic_cast<const Identifier*>(expr))
        return eval_identifier(node);
    if (auto* node = dynamic_cast<const BinaryExpr*>(expr))
        return eval_binary(node);
    if (auto* node = dynamic_cast<const UnaryExpr*>(expr))
        return eval_unary(node);
    if (auto* node = dynamic_cast<const VarDecl*>(expr))
        return eval_var_decl(node);
    if (auto* node = dynamic_cast<const PrintStmt*>(expr))
        return eval_print(node);
    if (auto* node = dynamic_cast<const Program*>(expr))
        return eval_program(node);

    throw std::runtime_error("🌮 Unknown AST node type.");
}

// ────────────────────────────────
// 字面量求值（直接返回值）
// ────────────────────────────────

TacoValue Evaluator::eval_number(const NumberLiteral* node) {
    return node->value;
}

TacoValue Evaluator::eval_string(const StringLiteral* node) {
    return node->value;
}

TacoValue Evaluator::eval_bool(const BoolLiteral* node) {
    return node->value;
}

TacoValue Evaluator::eval_nil(const NilLiteral* node) {
    return nullptr;
}

// ────────────────────────────────
// 变量引用：从环境里查找
// ────────────────────────────────

TacoValue Evaluator::eval_identifier(const Identifier* node) {
    auto it = m_env.find(node->name);
    if (it == m_env.end()) {
        throw std::runtime_error(
            "🌮 '" + node->name + "' is not defined."
        );
    }
    return it->second;
}

// ────────────────────────────────
// 二元表达式
// ────────────────────────────────

TacoValue Evaluator::eval_binary(const BinaryExpr* node) {
    TacoValue left  = evaluate(node->left.get());
    TacoValue right = evaluate(node->right.get());
    const std::string& op = node->op;

    // 算术运算（要求两边都是数字）
    if (op == "+" || op == "-" || op == "*" ||
        op == "/" || op == "%" || op == "^") {

        // 字符串拼接：+ 两边都是字符串时
        if (op == "+" &&
            std::holds_alternative<std::string>(left) &&
            std::holds_alternative<std::string>(right)) {
            return std::get<std::string>(left) + std::get<std::string>(right);
        }

        if (!std::holds_alternative<double>(left) ||
            !std::holds_alternative<double>(right)) {
            throw std::runtime_error(
                "🌮 Operator '" + op + "' requires numbers."
            );
        }

        double l = std::get<double>(left);
        double r = std::get<double>(right);

        if (op == "+") return l + r;
        if (op == "-") return l - r;
        if (op == "*") return l * r;
        if (op == "/") {
            if (r == 0) throw std::runtime_error("🌮 Division by zero. Nobody can.");
            return l / r;
        }
        if (op == "%") return std::fmod(l, r);
        if (op == "^") return std::pow(l, r);
    }

    // 比较运算
    if (op == "==" || op == "!=") {
        bool equal = (left == right);
        return op == "==" ? equal : !equal;
    }

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!std::holds_alternative<double>(left) ||
            !std::holds_alternative<double>(right)) {
            throw std::runtime_error(
                "🌮 Operator '" + op + "' requires numbers."
            );
        }
        double l = std::get<double>(left);
        double r = std::get<double>(right);
        if (op == "<")  return l < r;
        if (op == ">")  return l > r;
        if (op == "<=") return l <= r;
        if (op == ">=") return l >= r;
    }

    // 逻辑运算
    if (op == "&&") return is_truthy(left) && is_truthy(right);
    if (op == "||") return is_truthy(left) || is_truthy(right);

    throw std::runtime_error("🌮 Unknown operator: '" + op + "'.");
}

// ────────────────────────────────
// 一元表达式
// ────────────────────────────────

TacoValue Evaluator::eval_unary(const UnaryExpr* node) {
    TacoValue val = evaluate(node->operand.get());

    if (node->op == "!") {
        return !is_truthy(val);
    }
    if (node->op == "-") {
        if (!std::holds_alternative<double>(val)) {
            throw std::runtime_error("🌮 Unary '-' requires a number.");
        }
        return -std::get<double>(val);
    }

    throw std::runtime_error("🌮 Unknown unary operator: '" + node->op + "'.");
}

// ────────────────────────────────
// 变量声明：求值后存入环境
// ────────────────────────────────

TacoValue Evaluator::eval_var_decl(const VarDecl* node) {
    TacoValue val = evaluate(node->value.get());
    m_env[node->name] = val;
    return val;
}

// ────────────────────────────────
// print 语句
// ────────────────────────────────

TacoValue Evaluator::eval_print(const PrintStmt* node) {
    TacoValue val = evaluate(node->argument.get());
    std::cout << value_to_string(val) << "\n";
    return nullptr;  // print 不返回值
}

// ────────────────────────────────
// 程序：依次求值所有语句
// ────────────────────────────────

TacoValue Evaluator::eval_program(const Program* node) {
    TacoValue last = nullptr;
    for (const auto& stmt : node->statements) {
        last = evaluate(stmt.get());
    }
    return last;
}
```

---

## 11.5 测试：把 `var x = 10 + 20;` 解析成 AST 并运行

### main.cpp

```cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"

// 读取文件内容
std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("🌮 Cannot open file: " + path);
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

void run(const std::string& source) {
    // 词法分析
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    // 语法分析
    Parser parser(std::move(tokens));
    auto program = parser.parse();

    // 求值
    Evaluator evaluator;
    evaluator.evaluate(program.get());
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // 没有参数：简单的 REPL（v6 会做成完整版）
        std::cout << "🌮 Taco 0.1.0\n";
        std::cout << "   It works on my machine.\n";

        std::string line;
        while (true) {
            std::cout << "> ";
            if (!std::getline(std::cin, line)) break;
            if (line == "exit") {
                std::cout << "🌮 Fine.\n";
                break;
            }
            try {
                run(line);
            } catch (const std::exception& e) {
                std::cerr << e.what() << "\n";
            }
        }
    } else {
        // 有参数：运行文件
        try {
            std::string source = read_file(argv[1]);
            run(source);
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
    }

    return 0;
}
```

### 更新 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_executable(taco
    src/main.cpp
    src/token.cpp
    src/lexer.cpp
    src/parser.cpp
    src/evaluator.cpp
)

target_include_directories(taco PRIVATE src)
```

### 测试文件 test.taco

```taco
var x = 10 + 20;
var y = x * 2;
var name = "Miguel";
var greeting = "Hola, " + name + "!";
var flag = true;

print(x);
print(y);
print(name);
print(greeting);
print(flag);
print(1 + 2 * 3);
print((1 + 2) * 3);
```

### 运行

```bash
cmake -B build
cmake --build build
./build/taco test.taco
```

### 输出

```
30
60
Miguel
Hola, Miguel!
true
7
9
```

也可以启动简单的 REPL：

```bash
./build/taco
🌮 Taco 0.1.0
   It works on my machine.
> var x = 10 + 20;
> print(x);
30
> print("Hello, " + "World!");
Hello, World!
> exit
🌮 Fine.
```

---

## 11.6 这个版本的局限性

v1 能运行基础的 Taco 程序，但有几个明显的不足：

**没有控制流**

```taco
if (x > 10) { print("big"); }  // 不支持
while (x > 0) { x = x - 1; }  // 不支持
```

控制流需要更多的 AST 节点类型，v2 会加。

**变量没有作用域**

所有变量都在同一个全局环境里。如果有函数，函数里的变量应该和外面的变量隔离，但现在做不到。

**`dynamic_cast` 的局限**

现在用 `dynamic_cast` 来判断节点类型，这是可以工作的，但不够优雅。第十三章学完多态之后，v2 会改用虚函数的方式——给每个节点加一个 `evaluate()` 虚函数，让每个节点知道如何求值自己。

**只有 print 内置函数**

其他内置函数（`input`、`type` 等）还没有实现，v4 会系统地加入标准库。

---

## 小结

v1 实现了解释器的第二层和第三层：语法分析器和求值器。

**AST** 用继承体系表示：`Expr` 是基类，各种节点是子类。`unique_ptr` 管理节点的生命周期，不需要手动释放内存。

**递归下降解析器**对每种语法结构写一个函数，函数层级对应运算符优先级。这是最直观、最容易理解的解析器实现方式。

**求值器**遍历 AST，对每个节点求值。`std::variant` 表示 Taco 的值类型，可以是数字、字符串、布尔或 nil。

**`std::variant`** 是 C++17 的特性，表示"可以是这几种类型之一"的值。第十九章会详细介绍，现在只需要知道用法。

---

第二部分到这里结束。第三部分进入继承和多态，学完之后 v2 会用虚函数重构求值器，并加入控制流。
# 第十二章：继承

---

第二部分学完了类的基础：成员变量、成员函数、构造析构、拷贝、运算符重载。这些已经足以写出相当复杂的程序。

第三部分引入继承和多态——这是面向对象编程的核心，也是 v2 重构求值器的关键。

---

## 12.1 继承的概念与语法

继承（inheritance）让一个类可以基于另一个类来定义，自动获得父类的成员，同时可以添加新成员或修改父类的行为。

Python 里已经熟悉这个概念：

```python
class Animal:
    def __init__(self, name):
        self.name = name

    def speak(self):
        print(f"{self.name} makes a sound")

class Dog(Animal):
    def speak(self):
        print(f"{self.name} barks")
```

C++ 的写法：

```cpp
class Animal {
public:
    Animal(std::string name) : m_name(std::move(name)) {}

    void speak() const {
        std::cout << m_name << " makes a sound\n";
    }

protected:
    std::string m_name;  // protected：子类可以访问
};

class Dog : public Animal {  // public 继承
public:
    Dog(std::string name) : Animal(std::move(name)) {}  // 调用父类构造函数

    void speak() const {
        std::cout << m_name << " barks\n";
    }
};

Dog d("Dante");
d.speak();  // Dante barks
```

语法上的区别：

- Python 用 `class Dog(Animal)`，C++ 用 `class Dog : public Animal`
- Python 用 `super().__init__(name)` 调用父类构造函数，C++ 用初始化列表 `: Animal(name)`
- C++ 需要指定继承方式（`public`、`protected`、`private`），几乎总是用 `public`

---

## 12.2 与 Python 继承的异同

**相同点**：

- 子类自动获得父类的成员（变量和函数）
- 子类可以覆盖（override）父类的函数
- 可以多层继承

**不同点**：

**C++ 需要显式调用父类构造函数**

Python 里如果不调用 `super().__init__()`，父类的 `__init__` 不会被执行（除非没有自定义 `__init__`）。C++ 里子类的构造函数必须在初始化列表里显式调用父类的构造函数：

```cpp
class Dog : public Animal {
public:
    // 必须显式调用 Animal 的构造函数
    Dog(std::string name) : Animal(std::move(name)) {}
};
```

如果不写，编译器会尝试调用父类的默认构造函数。如果父类没有默认构造函数，就会报错。

**C++ 的函数覆盖默认不是多态的**

这是 C++ 和 Python 最大的区别，下一章会详细讲。先看现象：

```cpp
Animal* a = new Dog("Dante");
a->speak();  // 输出什么？
```

在 Python 里，`a.speak()` 会调用 `Dog.speak()`（多态）。但在 C++ 里，如果 `speak()` 没有被声明为 `virtual`，`a->speak()` 会调用 `Animal::speak()`（静态绑定）。

这就是为什么 C++ 需要 `virtual` 关键字——没有 `virtual`，覆盖只是"隐藏"父类的函数，而不是多态。

---

## 12.3 构造函数的继承与调用顺序

构造函数和析构函数在继承时有严格的调用顺序。

**构造顺序**：从父到子。先调用父类构造函数，再调用子类构造函数。

```cpp
class Base {
public:
    Base() { std::cout << "Base constructed\n"; }
    ~Base() { std::cout << "Base destructed\n"; }
};

class Derived : public Base {
public:
    Derived() { std::cout << "Derived constructed\n"; }
    ~Derived() { std::cout << "Derived destructed\n"; }
};

{
    Derived d;
    // 输出：
    // Base constructed
    // Derived constructed
}
// 离开作用域，输出：
// Derived destructed
// Base destructed
```

**析构顺序**：从子到父。和构造顺序相反。

这个顺序是有道理的：子类的构造依赖父类已经初始化好，子类的析构也应该在父类析构之前完成清理。

---

### 多层继承的构造顺序

```cpp
class A {
public:
    A() { std::cout << "A\n"; }
};

class B : public A {
public:
    B() { std::cout << "B\n"; }
};

class C : public B {
public:
    C() { std::cout << "C\n"; }
};

C c;
// 输出：A B C（从最远的父类开始）
```

---

### 父类没有默认构造函数时

```cpp
class Animal {
public:
    Animal(std::string name) : m_name(std::move(name)) {}
    // 没有默认构造函数

protected:
    std::string m_name;
};

class Dog : public Animal {
public:
    // 必须在初始化列表里调用 Animal(name)
    Dog(std::string name, std::string breed)
        : Animal(std::move(name))   // 必须显式调用
        , m_breed(std::move(breed))
    {}

private:
    std::string m_breed;
};

// Dog d;  // 错误！Animal 没有默认构造函数
Dog d("Dante", "Labrador");  // 正确
```

---

## 12.4 protected 的真实含义

前面提到三种访问控制：`public`、`private`、`protected`。

`protected` 的含义：**类自己和它的子类可以访问，外部不能访问**。

```cpp
class Animal {
public:
    std::string get_name() const { return m_name; }  // 外部可以访问

protected:
    std::string m_name;  // 子类可以访问，外部不能直接访问

private:
    int m_age;  // 只有 Animal 自己可以访问，子类也不行
};

class Dog : public Animal {
public:
    void rename(std::string new_name) {
        m_name = new_name;  // 可以！m_name 是 protected
        // m_age = 5;       // 错误！m_age 是 private
    }
};

Dog d("Dante");
d.m_name = "Coco";      // 错误！外部不能访问 protected 成员
d.get_name();           // 可以，get_name 是 public
```

**什么时候用 protected？**

`protected` 是专门为继承设计的：它允许子类访问父类的内部实现，同时对外部保持封装。

但 `protected` 要谨慎使用。`protected` 成员是父类和子类之间的"内部接口"——如果修改了 `protected` 成员，所有子类都可能受影响。这让代码更难维护。

一个更稳健的做法是：父类的数据成员一律 `private`，通过 `protected` 的函数让子类访问：

```cpp
class Animal {
public:
    std::string get_name() const { return m_name; }

protected:
    // 给子类用的函数接口
    void set_name(std::string name) { m_name = std::move(name); }

private:
    std::string m_name;  // 数据成员完全私有
};
```

---

## *More About 继承*：多重继承与菱形问题

> 第一次读可以跳过。

### 多重继承

C++ 支持多重继承——一个类可以有多个父类：

```cpp
class Flyable {
public:
    void fly() { std::cout << "Flying\n"; }
};

class Swimmable {
public:
    void swim() { std::cout << "Swimming\n"; }
};

class Duck : public Flyable, public Swimmable {
    // Duck 既能飞又能游
};

Duck d;
d.fly();   // Flying
d.swim();  // Swimming
```

Python 也支持多重继承，语法是 `class Duck(Flyable, Swimmable)`。

### 菱形问题

多重继承会带来一个经典的问题——菱形继承（diamond inheritance）：

```cpp
class A {
public:
    int value = 0;
};

class B : public A {};
class C : public A {};

class D : public B, public C {
    // D 通过 B 和 C 都继承了 A
    // D 里有两份 A 的成员变量！
};

D d;
d.value = 10;  // 错误！歧义：是 B::A::value 还是 C::A::value？
d.B::value = 10;  // 需要显式指定
```

`D` 里有两份 `A` 的成员，访问时必须明确指定从哪条路径来的，非常麻烦。

### 虚继承

解决方案是**虚继承**（virtual inheritance）：

```cpp
class A {
public:
    int value = 0;
};

class B : public virtual A {};  // 虚继承
class C : public virtual A {};  // 虚继承

class D : public B, public C {
    // 现在 D 里只有一份 A 的成员
};

D d;
d.value = 10;  // 正确，没有歧义
```

虚继承让多个路径汇聚到同一份基类实例，解决了菱形问题。但虚继承本身有运行时开销，而且让对象的内存布局变得复杂。

**现代 C++ 的建议**：尽量避免多重继承，尤其是数据成员的多重继承。如果需要"多种能力的组合"，用接口（纯虚类，第十三章讲）来实现，而不是继承多个有数据的类。

---

## 小结

**继承**让子类自动获得父类的成员，同时可以添加新成员或覆盖父类的函数。

**构造顺序**从父到子，**析构顺序**从子到父。子类构造函数必须在初始化列表里显式调用父类构造函数。

**protected** 是专门为继承设计的访问级别：子类可以访问，外部不能访问。最好把数据成员设为 `private`，用 `protected` 函数提供给子类访问。

**多重继承**和**菱形问题**是 C++ 特有的复杂性，现代 C++ 建议用纯虚类（接口）代替。

---

下一章讲多态和虚函数——C++ 里"用父类指针调用子类函数"的机制，以及它的底层实现。
# 第十三章：多态与虚函数

---

上一章讲了继承的基础语法。这一章讲继承最重要的应用：**多态**（polymorphism）。

多态是面向对象编程的核心思想之一：用同一套接口，操作不同类型的对象，让对象自己决定怎么响应。

在 Taco 解释器里，多态的用途非常具体：AST 有很多种节点（`NumberLiteral`、`BinaryExpr`、`VarDecl`……），求值器需要对每种节点调用不同的求值逻辑。多态让求值器可以写成 `node->evaluate()`，不需要判断节点是哪种类型，让节点自己知道怎么求值。

---

## 13.1 为什么需要多态

先来看一个没有多态时的困境。

假设有一个动物园管理系统，需要让所有动物发声：

```cpp
class Animal { ... };
class Dog : public Animal { void speak() { cout << "Woof\n"; } };
class Cat : public Animal { void speak() { cout << "Meow\n"; } };
class Bird : public Animal { void speak() { cout << "Tweet\n"; } };
```

没有多态的做法：

```cpp
void make_all_speak(std::vector<Animal*> animals) {
    for (Animal* a : animals) {
        // 必须手动判断类型
        if (Dog* d = dynamic_cast<Dog*>(a)) {
            d->speak();
        } else if (Cat* c = dynamic_cast<Cat*>(a)) {
            c->speak();
        } else if (Bird* b = dynamic_cast<Bird*>(a)) {
            b->speak();
        }
        // 每加一种新动物，就要在这里加一个 else if
    }
}
```

这种写法有几个明显的问题：

- 每加一种新动物，就要修改 `make_all_speak`——违反了"开闭原则"（对扩展开放，对修改关闭）
- 如果忘了加 `else if`，新动物就会静默地什么都不做
- 如果有很多这样的函数，每个都要改

有了多态的做法：

```cpp
void make_all_speak(std::vector<Animal*> animals) {
    for (Animal* a : animals) {
        a->speak();  // 让动物自己决定怎么叫
    }
}
```

加新动物只需要写新的子类，不需要修改 `make_all_speak`。这才是面向对象的正确用法。

---

## 13.2 虚函数：运行时决策

要实现多态，需要 `virtual` 关键字。

```cpp
class Animal {
public:
    // virtual：这个函数可以被子类覆盖，调用时根据实际类型决定
    virtual void speak() const {
        std::cout << "Some animal sound\n";
    }

    virtual ~Animal() = default;  // 析构函数也应该是虚的（后面解释原因）
};

class Dog : public Animal {
public:
    void speak() const override {  // override：明确告诉编译器这是覆盖
        std::cout << "Woof!\n";
    }
};

class Cat : public Animal {
public:
    void speak() const override {
        std::cout << "Meow!\n";
    }
};
```

现在多态就生效了：

```cpp
Animal* a1 = new Dog();
Animal* a2 = new Cat();

a1->speak();  // Woof!（调用的是 Dog::speak，不是 Animal::speak）
a2->speak();  // Meow!（调用的是 Cat::speak）

delete a1;
delete a2;
```

关键在于：`a1` 的静态类型是 `Animal*`，但它实际指向一个 `Dog` 对象。有了 `virtual`，`a1->speak()` 会在运行时查看 `a1` 实际指向的对象类型，调用对应的函数。这就叫**运行时多态**（runtime polymorphism）。

---

### 没有 virtual 会怎样

如果去掉 `virtual`：

```cpp
class Animal {
public:
    void speak() const {  // 没有 virtual
        std::cout << "Some animal sound\n";
    }
};

class Dog : public Animal {
public:
    void speak() const {  // 这只是"隐藏"了父类的函数，不是多态
        std::cout << "Woof!\n";
    }
};

Animal* a = new Dog();
a->speak();  // 输出：Some animal sound（调用的是 Animal::speak！）
```

没有 `virtual`，编译器在编译时就决定了调用哪个函数——因为指针类型是 `Animal*`，所以调用 `Animal::speak`，不管实际指向的是什么对象。这叫**静态绑定**（static binding）。

有 `virtual`，调用哪个函数在运行时根据对象的实际类型决定。这叫**动态绑定**（dynamic binding）。

Python 的所有方法默认都是动态绑定的，相当于 C++ 里所有函数都是虚函数。C++ 的设计哲学是"你不用的东西不应该付出代价"——虚函数有运行时开销（查虚函数表），所以不默认开启。

---

### 为什么析构函数应该是虚的

如果通过父类指针删除子类对象，而析构函数不是虚的，会发生什么？

```cpp
class Base {
public:
    ~Base() { std::cout << "Base destroyed\n"; }
};

class Derived : public Base {
public:
    ~Derived() { std::cout << "Derived destroyed\n"; }
    int* data = new int[100];  // Derived 自己分配的内存
};

Base* p = new Derived();
delete p;  // 只调用 Base::~Base()，不调用 Derived::~Derived()！
           // data 指向的内存泄漏了！
```

输出只有 `Base destroyed`，`Derived::~Derived()` 没有被调用，`data` 内存泄漏。

解决方法：把父类的析构函数声明为 `virtual`：

```cpp
class Base {
public:
    virtual ~Base() { std::cout << "Base destroyed\n"; }
};

Base* p = new Derived();
delete p;
// 输出：
// Derived destroyed（先调用子类析构）
// Base destroyed（再调用父类析构）
```

**规则**：如果一个类有虚函数，那它的析构函数也应该是虚的。在 Taco 的 AST 里，`Expr` 基类有虚函数，所以它的析构函数声明为 `virtual ~Expr() = default`。

---

## 13.3 纯虚函数与抽象类

有时候父类的函数没有合理的默认实现——`Animal::speak()` 说"some animal sound"很奇怪，所有具体的动物都应该自己实现 `speak()`。

**纯虚函数**（pure virtual function）强制要求子类提供实现：

```cpp
class Animal {
public:
    // 纯虚函数：= 0 表示没有实现，子类必须提供
    virtual void speak() const = 0;

    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    void speak() const override {
        std::cout << "Woof!\n";
    }
};

// Animal a;  // 错误！Animal 是抽象类，不能实例化
Animal* a = new Dog();  // 正确：用父类指针指向子类对象
a->speak();  // Woof!
```

包含纯虚函数的类叫**抽象类**（abstract class）。抽象类不能直接实例化——它只是定义了一个接口，让子类去实现。

这个概念和 Python 的 `abc.ABC` / `@abstractmethod` 完全对应：

```python
from abc import ABC, abstractmethod

class Animal(ABC):
    @abstractmethod
    def speak(self):
        pass

class Dog(Animal):
    def speak(self):
        print("Woof!")
```

---

### 接口：只有纯虚函数的抽象类

一种常见的设计模式是用只有纯虚函数的抽象类来定义"接口"：

```cpp
// 接口：定义能力
class Printable {
public:
    virtual std::string to_string() const = 0;
    virtual ~Printable() = default;
};

class Evaluatable {
public:
    virtual TacoValue evaluate(Evaluator& eval) const = 0;
    virtual ~Evaluatable() = default;
};

// 具体类实现接口
class NumberLiteral : public Printable, public Evaluatable {
public:
    double value;

    std::string to_string() const override {
        return std::to_string(value);
    }

    TacoValue evaluate(Evaluator& eval) const override {
        return value;
    }
};
```

这种设计把"能做什么"和"怎么做"分离开来，非常灵活。

---

## 13.4 override 与 final

### override

`override` 关键字（C++11 引入）明确告诉编译器：这个函数是覆盖父类的虚函数。

```cpp
class Animal {
public:
    virtual void speak() const { ... }
    virtual void move() const { ... }
};

class Dog : public Animal {
public:
    void speak() const override { ... }  // 正确覆盖
    void moev() const override { ... }   // 错误！拼写错误，编译器报错
    // 如果没有 override，moev() 只是 Dog 的一个新函数，
    // 不会覆盖任何东西，也不会报错——这是难以发现的 bug
};
```

`override` 的价值在于：如果父类的函数签名改变了（比如加了 `const`），或者子类里有拼写错误，编译器会立刻报错，而不是悄悄地变成一个不相关的新函数。

**规则**：覆盖虚函数时，总是加 `override`。

### final

`final` 关键字（C++11 引入）禁止进一步覆盖或继承：

```cpp
// final 修饰虚函数：这个函数不能再被子类覆盖
class Dog : public Animal {
public:
    void speak() const override final {
        std::cout << "Woof!\n";
    }
};

class GoldenRetriever : public Dog {
    void speak() const override { ... }  // 错误！speak 是 final
};

// final 修饰类：这个类不能被继承
class Cat final : public Animal {
public:
    void speak() const override { ... }
};

class PersianCat : public Cat { ... };  // 错误！Cat 是 final
```

`final` 的使用场景不多，但在确定某个类或函数不应该被扩展时，`final` 是一种清晰的表达。它也让编译器可以做某些优化（知道这个函数不会被覆盖，可以直接调用而不用查虚函数表）。

---

## *More About 虚函数*：vtable 的底层实现

> 第一次读可以跳过。

虚函数的多态是怎么实现的？答案是**虚函数表**（vtable，virtual function table）。

### vtable 的结构

每个有虚函数的类，编译器都会为它生成一个 vtable——一个函数指针数组，每个元素对应一个虚函数：

```
Animal 的 vtable：
  [0] → Animal::speak
  [1] → Animal::~Animal

Dog 的 vtable：
  [0] → Dog::speak      （覆盖了 Animal::speak）
  [1] → Animal::~Animal （继承了，没有覆盖）
```

每个对象都有一个隐藏的指针（vptr，通常是对象的第一个成员），指向所属类的 vtable：

```
Dog 对象的内存布局：
  [vptr] → Dog 的 vtable
  [m_name] = "Dante"
```

### 虚函数调用的过程

```cpp
Animal* a = new Dog("Dante");
a->speak();
```

这行代码在底层实际上是：

```cpp
// 1. 通过 a 找到对象
// 2. 通过对象的 vptr 找到 vtable
// 3. 通过 vtable 找到 speak 函数的地址
// 4. 调用该地址对应的函数
(a->vptr)[0](a);  // 伪代码，实际更复杂
```

因为 `vptr` 指向 Dog 的 vtable，`vtable[0]` 是 `Dog::speak`，所以调用的是 `Dog::speak`。

### vtable 的开销

虚函数的开销来自两个地方：

**空间开销**：每个对象多一个 `vptr`（通常是 8 字节）。对象很小的时候，这个开销比较显著。

**时间开销**：虚函数调用需要间接寻址（vptr → vtable → 函数地址），比直接函数调用多一次内存访问。现代 CPU 有分支预测和缓存，实际开销通常可以忽略不计，但在极度性能敏感的场景（比如每帧调用几百万次）可能有影响。

这就是为什么 C++ 不把所有函数都设为虚函数——它遵循"零开销抽象"的原则，让程序员选择是否承担虚函数的开销。

### 没有 vtable 时的调用

非虚函数的调用是直接的：编译器在编译时就知道要调用哪个函数，直接生成调用该函数的机器码。这叫**编译期绑定**（compile-time binding）或**静态绑定**。

```cpp
Animal a("Generic");
a.speak();  // 如果 speak 不是虚函数，直接调用 Animal::speak，没有间接寻址
```

---

## 小结

**多态**让父类指针可以操作不同类型的子类对象，每个对象根据自身类型响应调用。这是面向对象编程"开闭原则"的核心实现机制。

**虚函数**（`virtual`）开启动态绑定——调用哪个函数在运行时决定，而不是在编译时决定。没有 `virtual` 的函数是静态绑定，通过父类指针调用时只会调用父类的版本。

**纯虚函数**（`= 0`）让类变成抽象类，强制子类提供实现。只有纯虚函数的抽象类相当于"接口"。

**`override`** 明确标记覆盖父类的虚函数，让编译器帮你检查签名是否匹配。总是加 `override`。

**虚析构函数**：如果类有虚函数，析构函数也必须是虚的，否则通过父类指针删除子类对象时会内存泄漏。

**底层**：虚函数通过 vtable（虚函数表）实现，每个对象有一个隐藏的 vptr 指向所属类的 vtable，调用虚函数时通过 vptr 间接找到函数地址。

---

下一章讲类型转换——`dynamic_cast` 如何在运行时安全地把父类指针转成子类指针，以及 C++ 的四种类型转换各自的用途。
# 第十四章：类型转换

---

C++ 有四种命名的类型转换运算符：`static_cast`、`dynamic_cast`、`reinterpret_cast`、`const_cast`。每种有特定的用途和安全性保证。在 C 里，所有转换都用 `(type)value` 这种 C 风格转换，C++ 把它拆成四种，目的是让意图更明确，错误更容易发现。

---

## 14.1 隐式转换的风险

在讲显式转换之前，先理解为什么要避免 C 风格的隐式转换。

### 隐式数值转换

C++ 和 C 一样，会在必要时自动做数值类型的隐式转换：

```cpp
int i = 42;
double d = i;   // 隐式转换：int → double，安全，没有精度损失

double pi = 3.14159;
int truncated = pi;  // 隐式转换：double → int，截断小数部分，有信息损失
                     // 编译器通常会发出警告
```

这类转换在 C 里无处不在，通常没有问题，但有时候会造成难以发现的 bug：

```cpp
int total = 100;
int count = 3;
double average = total / count;  // 整数除法！结果是 33，不是 33.333...
// 应该写成：
double average = static_cast<double>(total) / count;
```

### C 风格转换的问题

C 风格转换 `(type)value` 太过"万能"——它会尝试各种转换，其中有些非常危险：

```cpp
const int x = 42;
int* p = (int*)&x;  // C 风格强制转换掉 const！合法但危险
*p = 100;           // 未定义行为

// 更糟糕：
int i = 42;
double* dp = (double*)&i;  // 把 int 的地址当成 double 的地址！
*dp = 3.14;  // 严重的内存损坏
```

C 风格转换会悄悄地做任何事情，不会在代码里留下明显的痕迹。C++ 的四种命名转换让不同程度的"危险操作"在代码里一目了然：看到 `reinterpret_cast` 就知道这里有潜在风险，需要仔细审查。

---

## 14.2 四种类型转换

### static_cast：编译时已知的转换

`static_cast` 用于在编译时就能确定是否安全的类型转换：

```cpp
// 数值类型之间的转换
double d = 3.14;
int i = static_cast<int>(d);   // 3，截断小数
float f = static_cast<float>(d);  // 3.14f

// 向上转型（子类指针 → 父类指针）：总是安全的
Dog* dog = new Dog("Dante");
Animal* animal = static_cast<Animal*>(dog);  // 安全
// 更常见的写法：直接赋值，隐式转换
Animal* animal2 = dog;

// 向下转型（父类指针 → 子类指针）：不安全，没有运行时检查
Animal* a = new Dog("Dante");
Dog* d2 = static_cast<Dog*>(a);  // 可以通过，但如果 a 实际不是 Dog，行为未定义
```

`static_cast` 的特点是：
- **编译时决定**：转换是否合理由编译器在编译时判断
- **没有运行时开销**：不做任何运行时检查
- **向下转型不安全**：如果父类指针实际指向的不是目标子类，行为未定义

规则：当确定转换是安全的，或者想要明确表达"我知道这个转换可能损失信息"时，用 `static_cast`。

---

### dynamic_cast：运行时安全的向下转型

`dynamic_cast` 在运行时检查转换是否合法，只用于多态类型（有虚函数的类）之间的转换：

```cpp
Animal* a = new Dog("Dante");

// 向下转型：dynamic_cast 在运行时检查
Dog* d = dynamic_cast<Dog*>(a);
if (d != nullptr) {
    // 转换成功，a 确实指向 Dog 对象
    d->fetch();
} else {
    // 转换失败，a 不是 Dog
}

// 如果用引用版本，转换失败会抛出 std::bad_cast 异常
try {
    Dog& d_ref = dynamic_cast<Dog&>(*a);
    d_ref.fetch();
} catch (const std::bad_cast& e) {
    std::cerr << "Not a Dog!\n";
}
```

`dynamic_cast` 的特点是：
- **运行时检查**：会查看对象的实际类型
- **安全**：转换失败返回 `nullptr`（指针版本）或抛出异常（引用版本）
- **有开销**：需要运行时类型信息（RTTI），比 `static_cast` 慢
- **只适用于多态类型**：类必须有至少一个虚函数

---

### 什么时候用 dynamic_cast，什么时候用虚函数

在 v1 的求值器里，用了 `dynamic_cast` 来判断节点类型：

```cpp
TacoValue Evaluator::evaluate(const Expr* expr) {
    if (auto* node = dynamic_cast<const NumberLiteral*>(expr))
        return eval_number(node);
    if (auto* node = dynamic_cast<const BinaryExpr*>(expr))
        return eval_binary(node);
    // ...
}
```

这能工作，但有一个问题：每次调用 `evaluate`，都要做多次 `dynamic_cast`，直到找到匹配的类型。如果有 20 种节点类型，最坏情况要做 20 次 `dynamic_cast`，每次都有运行时开销。

更好的做法是用虚函数——让每个节点自己知道怎么求值：

```cpp
struct Expr {
    virtual TacoValue evaluate(Evaluator& eval) const = 0;
    virtual ~Expr() = default;
};

struct NumberLiteral : Expr {
    double value;
    TacoValue evaluate(Evaluator& eval) const override {
        return value;
    }
};
```

这样 `eval.evaluate(node)` 变成 `node->evaluate(eval)`，只需要一次虚函数调用，没有多次 `dynamic_cast`。这就是 v2 要做的重构，下一章会看到。

**原则**：如果要根据对象类型做不同的事，优先用虚函数，而不是 `dynamic_cast`。`dynamic_cast` 适合在真正需要向下转型时使用（比如获取子类特有的功能）。

---

### reinterpret_cast：危险的位重解释

`reinterpret_cast` 把一块内存强制解释成另一种类型，不做任何转换，只是改变编译器看待这块内存的方式：

```cpp
int i = 42;
// 把 int* 解释成 char*，查看 int 的字节内容
char* bytes = reinterpret_cast<char*>(&i);
for (int j = 0; j < sizeof(int); j++) {
    printf("%02x ", (unsigned char)bytes[j]);
}
// 在小端序机器上输出：2a 00 00 00

// 把函数指针转换成 void*（用于某些底层 API）
void* p = reinterpret_cast<void*>(&some_function);
```

`reinterpret_cast` 几乎不做任何安全检查，结果高度依赖平台（字节序、对齐方式等）。在大多数 C++ 代码里，很少需要用到它。看到 `reinterpret_cast` 就应该提高警惕——这里可能有平台相关的代码或底层黑科技。

在 Taco 解释器里，几乎不会用到 `reinterpret_cast`。

---

### const_cast：去掉或添加 const

`const_cast` 用于在 `const` 和非 `const` 之间转换：

```cpp
void legacy_print(char* str) {  // 老旧 API，不接受 const char*
    printf("%s\n", str);
}

const char* message = "Hello";
legacy_print(const_cast<char*>(message));  // 去掉 const，调用老旧 API
// 注意：不能通过这个指针修改内容，只是骗过了编译器的类型检查
```

`const_cast` 的用途很窄：主要用于和不接受 `const` 参数的老旧 C API 交互。

**绝对不能做的事**：用 `const_cast` 去掉 `const`，然后通过得到的指针修改原本 `const` 的数据。这是未定义行为：

```cpp
const int x = 42;
int* p = const_cast<int*>(&x);
*p = 100;  // 未定义行为！x 是真正的常量，不能修改
```

在 Taco 项目里，也很少会用到 `const_cast`。

---

## 14.3 dynamic_cast 与运行时类型识别（RTTI）

`dynamic_cast` 依赖于**运行时类型信息**（RTTI，Runtime Type Information）。RTTI 是编译器在编译时嵌入对象里的类型信息，让程序在运行时可以查询对象的实际类型。

### typeid

`typeid` 运算符返回一个 `std::type_info` 对象，可以查询类型名称或比较类型：

```cpp
#include <typeinfo>

Animal* a = new Dog("Dante");

std::cout << typeid(*a).name() << "\n";  // 输出类型名（格式依赖编译器）
// GCC 可能输出：3Dog，Clang 可能输出：Dog

// 比较类型
if (typeid(*a) == typeid(Dog)) {
    std::cout << "a is a Dog\n";
}
```

`typeid` 在调试时有用，但在正式代码里，应该优先用虚函数而不是 `typeid`——如果需要根据类型做不同的事，这通常意味着应该用多态。

### RTTI 的开销

启用 RTTI 会增加可执行文件的大小（每个多态类型需要存储类型信息），并且让 `dynamic_cast` 有运行时开销。在某些嵌入式或高性能场景，RTTI 会被关闭（用编译器选项 `-fno-rtti`），这时 `dynamic_cast` 和 `typeid` 无法使用。

在 Taco 解释器里，RTTI 是默认开启的，不需要担心这个问题。

---

### 四种转换的对比总结

| 转换 | 用途 | 编译时检查 | 运行时检查 | 安全性 |
|------|------|-----------|-----------|--------|
| `static_cast` | 数值转换、已知安全的类型转换 | 有 | 无 | 中等 |
| `dynamic_cast` | 多态类的向下转型 | 有 | 有 | 高 |
| `reinterpret_cast` | 位重解释，底层操作 | 几乎无 | 无 | 低 |
| `const_cast` | 添加或去掉 const | 有 | 无 | 视情况而定 |

**使用频率**：`static_cast` > `dynamic_cast` >> `const_cast` > `reinterpret_cast`。

---

## 小结

**C 风格转换** `(type)value` 太强大也太危险，C++ 提供了四种命名转换来替代它，让意图更清晰。

**`static_cast`** 用于编译时已知安全的转换，最常用，无运行时开销。向上转型安全，向下转型不检查。

**`dynamic_cast`** 用于多态类的安全向下转型，运行时检查，失败返回 `nullptr`（指针版本）或抛异常（引用版本）。有运行时开销，但安全。

**`reinterpret_cast`** 是危险的位重解释，几乎不做检查，高度平台相关，尽量避免。

**`const_cast`** 用于去掉或添加 `const`，主要用于和老旧 C API 交互。不能用它去掉真正常量的 `const`。

**RTTI**（运行时类型信息）支撑了 `dynamic_cast` 和 `typeid`，默认开启，有少量开销。

---

下一章是第三部分的项目章节：用虚函数重构求值器，并加入控制流（if/while/for/switch），完成 v2。
# 第十五章：项目 v2——求值器与控制流

---

第三部分学完了继承、多态、虚函数、类型转换。现在用这些知识做两件事：

1. **重构求值器**：把 v1 里用 `dynamic_cast` 判断节点类型的方式，换成用虚函数让节点自己求值
2. **加入控制流**：`if/elseif/else`、`while`、C 风格 `for`、`range` 风格 `for`、`switch`

v2 结束时，Taco 能运行这样的代码：

```taco
var score = 85;

if (score >= 90) {
    print("A");
} elseif (score >= 80) {
    print("B");
} else {
    print("C");
}

var sum = 0;
for (var i = 1; i <= 10; i++) {
    sum = sum + i;
}
print(sum);

for i in range(0, 5) {
    print(i);
}

var x = 10;
while (x > 0) {
    x = x - 1;
}
print(x);

switch (score) {
    case 100 { print("perfect"); }
    default  { print("not perfect"); }
}
```

---

## 15.1 用继承重新设计 AST 节点体系

v1 的 AST 节点是简单的数据结构，求值逻辑完全在 `Evaluator` 里。v2 改成让每个节点自己知道怎么求值——通过虚函数 `evaluate()`。

这个设计叫**访问者模式**（Visitor Pattern）的简化版。完整的访问者模式会在第二十九章讲到，现在先用更直接的方式。

### 为什么这样更好

v1 的求值器：

```cpp
// 每次调用都要做多次 dynamic_cast，直到找到匹配的类型
TacoValue Evaluator::evaluate(const Expr* expr) {
    if (auto* node = dynamic_cast<const NumberLiteral*>(expr))
        return eval_number(node);
    if (auto* node = dynamic_cast<const BinaryExpr*>(expr))
        return eval_binary(node);
    // ... 每增加一种节点，就要在这里加一行
}
```

v2 的求值器：

```cpp
// 直接调用虚函数，让节点自己处理
TacoValue Evaluator::evaluate(const Expr* expr) {
    return expr->evaluate(*this);
}
```

v2 的方式：
- **性能更好**：一次虚函数调用，不需要多次 `dynamic_cast`
- **扩展更方便**：加新节点类型只需要在新类里实现 `evaluate()`，不需要修改求值器
- **结构更清晰**：每种节点的求值逻辑和节点定义在一起

---

### 新的 AST 设计

前向声明 `Evaluator`，因为 `Expr::evaluate()` 需要接收它：

```cpp
// ast.h
#pragma once
#include <string>
#include <vector>
#include <memory>

// 前向声明
class Evaluator;

// TacoValue 的前向声明（实际定义在 value.h）
#include "value.h"

using ExprPtr = std::unique_ptr<struct Expr>;

// ────────────────────────────────
// 基类：纯虚，不能直接实例化
// ────────────────────────────────

struct Expr {
    virtual ~Expr() = default;

    // 核心：每个节点自己实现求值
    virtual TacoValue evaluate(Evaluator& eval) const = 0;

    // 调试用
    virtual std::string to_string() const = 0;
};
```

把 Taco 的值类型单独放到一个文件里：

```cpp
// value.h
#pragma once
#include <string>
#include <variant>

// Taco 的值：数字、字符串、布尔、nil
using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;

std::string value_to_string(const TacoValue& val);
bool is_truthy(const TacoValue& val);
bool values_equal(const TacoValue& a, const TacoValue& b);
```

```cpp
// value.cpp
#include "value.h"
#include <stdexcept>

std::string value_to_string(const TacoValue& val) {
    if (std::holds_alternative<double>(val)) {
        double d = std::get<double>(val);
        if (d == static_cast<long long>(d)) {
            return std::to_string(static_cast<long long>(d));
        }
        return std::to_string(d);
    }
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    }
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? "true" : "false";
    }
    return "nil";
}

bool is_truthy(const TacoValue& val) {
    if (std::holds_alternative<bool>(val))
        return std::get<bool>(val);
    if (std::holds_alternative<std::nullptr_t>(val))
        return false;
    return true;
}

bool values_equal(const TacoValue& a, const TacoValue& b) {
    return a == b;
}
```

---

## 15.2 完整的 AST 节点定义

现在把所有节点的 `evaluate()` 都加上。`Evaluator` 类会在 evaluator.h 里定义，这里只需要前向声明。

```cpp
// ast.h（完整版）
#pragma once
#include "value.h"
#include <string>
#include <vector>
#include <memory>

class Evaluator;
using ExprPtr = std::unique_ptr<struct Expr>;

// ────────────────────────────────
// 基类
// ────────────────────────────────

struct Expr {
    virtual ~Expr() = default;
    virtual TacoValue evaluate(Evaluator& eval) const = 0;
    virtual std::string to_string() const = 0;
};

// ────────────────────────────────
// 字面量
// ────────────────────────────────

struct NumberLiteral : Expr {
    double value;
    explicit NumberLiteral(double v) : value(v) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override;
};

struct StringLiteral : Expr {
    std::string value;
    explicit StringLiteral(std::string v) : value(std::move(v)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "\"" + value + "\""; }
};

struct BoolLiteral : Expr {
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return value ? "true" : "false"; }
};

struct NilLiteral : Expr {
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "nil"; }
};

// ────────────────────────────────
// 变量引用
// ────────────────────────────────

struct Identifier : Expr {
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return name; }
};

// ────────────────────────────────
// 赋值
// ────────────────────────────────

struct AssignExpr : Expr {
    std::string name;
    ExprPtr     value;
    AssignExpr(std::string n, ExprPtr v)
        : name(std::move(n)), value(std::move(v)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return name + " = " + value->to_string();
    }
};

// ────────────────────────────────
// 运算
// ────────────────────────────────

struct BinaryExpr : Expr {
    ExprPtr     left;
    std::string op;
    ExprPtr     right;
    BinaryExpr(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
    }
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr     operand;
    UnaryExpr(std::string o, ExprPtr e)
        : op(std::move(o)), operand(std::move(e)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "(" + op + operand->to_string() + ")";
    }
};

// ────────────────────────────────
// 变量声明
// ────────────────────────────────

struct VarDecl : Expr {
    std::string name;
    ExprPtr     value;
    VarDecl(std::string n, ExprPtr v)
        : name(std::move(n)), value(std::move(v)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "var " + name + " = " + value->to_string();
    }
};

// ────────────────────────────────
// 控制流
// ────────────────────────────────

// if/elseif/else
struct IfStmt : Expr {
    // 每个分支是一个 (条件, 语句块) 对
    // branches[0] = if 分支
    // branches[1..n-1] = elseif 分支
    struct Branch {
        ExprPtr condition;
        std::vector<ExprPtr> body;
    };
    std::vector<Branch>      branches;
    std::vector<ExprPtr>     else_body;  // else 分支（可能为空）

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "if(...)"; }
};

// while 循环
struct WhileStmt : Expr {
    ExprPtr              condition;
    std::vector<ExprPtr> body;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "while(...)"; }
};

// C 风格 for 循环：for (init; condition; update) { body }
struct ForStmt : Expr {
    ExprPtr              init;       // var i = 0
    ExprPtr              condition;  // i < 10
    ExprPtr              update;     // i++（实现为 i = i + 1）
    std::vector<ExprPtr> body;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "for(...)"; }
};

// range 风格 for 循环：for i in range(0, 10)
struct ForRangeStmt : Expr {
    std::string          var_name;
    ExprPtr              start;
    ExprPtr              end;
    std::vector<ExprPtr> body;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "for i in range(...)"; }
};

// switch 语句
struct SwitchStmt : Expr {
    struct Case {
        ExprPtr              value;  // nullptr 表示 default
        std::vector<ExprPtr> body;
    };
    ExprPtr       subject;
    std::vector<Case> cases;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "switch(...)"; }
};

// ────────────────────────────────
// 内置函数调用（暂时只有 print）
// ────────────────────────────────

struct CallExpr : Expr {
    std::string              callee;
    std::vector<ExprPtr>     arguments;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return callee + "(...)"; }
};

// ────────────────────────────────
// return 语句（用异常实现控制流）
// ────────────────────────────────

struct ReturnStmt : Expr {
    ExprPtr value;  // 可以是 nullptr（return;）
    explicit ReturnStmt(ExprPtr v) : value(std::move(v)) {}
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "return ..."; }
};

// ────────────────────────────────
// 程序：所有顶层语句
// ────────────────────────────────

struct Program : Expr {
    std::vector<ExprPtr> statements;
    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        std::string r;
        for (const auto& s : statements) r += s->to_string() + "\n";
        return r;
    }
};
```

---

## 15.3 实现求值器

求值器现在简洁很多——它只需要管理环境（变量），具体的求值逻辑在每个节点里。

```cpp
// evaluator.h
#pragma once
#include "value.h"
#include "ast.h"
#include <unordered_map>
#include <string>
#include <stdexcept>

// 用异常实现 return 语句的控制流跳转
struct ReturnException {
    TacoValue value;
};

class Evaluator {
public:
    Evaluator();

    // 主入口
    TacoValue evaluate(const Expr* expr);

    // 变量环境操作（供 AST 节点调用）
    TacoValue get_var(const std::string& name) const;
    void      set_var(const std::string& name, TacoValue val);
    void      define_var(const std::string& name, TacoValue val);

    // 作用域管理
    void push_scope();
    void pop_scope();

private:
    // 作用域链：栈顶是当前作用域
    std::vector<std::unordered_map<std::string, TacoValue>> m_scopes;
};
```

```cpp
// evaluator.cpp
#include "evaluator.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

Evaluator::Evaluator() {
    // 推入全局作用域
    m_scopes.push_back({});
}

TacoValue Evaluator::evaluate(const Expr* expr) {
    return expr->evaluate(*this);
}

TacoValue Evaluator::get_var(const std::string& name) const {
    // 从内向外查找变量
    for (int i = static_cast<int>(m_scopes.size()) - 1; i >= 0; i--) {
        auto it = m_scopes[i].find(name);
        if (it != m_scopes[i].end()) {
            return it->second;
        }
    }
    throw std::runtime_error("🌮 '" + name + "' is not defined.");
}

void Evaluator::set_var(const std::string& name, TacoValue val) {
    // 从内向外找到变量，修改它
    for (int i = static_cast<int>(m_scopes.size()) - 1; i >= 0; i--) {
        auto it = m_scopes[i].find(name);
        if (it != m_scopes[i].end()) {
            it->second = std::move(val);
            return;
        }
    }
    throw std::runtime_error("🌮 '" + name + "' is not defined.");
}

void Evaluator::define_var(const std::string& name, TacoValue val) {
    // 在当前作用域定义新变量
    m_scopes.back()[name] = std::move(val);
}

void Evaluator::push_scope() {
    m_scopes.push_back({});
}

void Evaluator::pop_scope() {
    if (m_scopes.size() > 1) {
        m_scopes.pop_back();
    }
}
```

---

## 15.4 各节点的 evaluate() 实现

把各个节点的求值实现单独放在 `ast.cpp` 里：

```cpp
// ast.cpp
#include "ast.h"
#include "evaluator.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

// ────────────────────────────────
// 字面量
// ────────────────────────────────

TacoValue NumberLiteral::evaluate(Evaluator& eval) const {
    return value;
}

std::string NumberLiteral::to_string() const {
    if (value == static_cast<long long>(value))
        return std::to_string(static_cast<long long>(value));
    return std::to_string(value);
}

TacoValue StringLiteral::evaluate(Evaluator& eval) const {
    return value;
}

TacoValue BoolLiteral::evaluate(Evaluator& eval) const {
    return value;
}

TacoValue NilLiteral::evaluate(Evaluator& eval) const {
    return nullptr;
}

// ────────────────────────────────
// 变量引用和赋值
// ────────────────────────────────

TacoValue Identifier::evaluate(Evaluator& eval) const {
    return eval.get_var(name);
}

TacoValue AssignExpr::evaluate(Evaluator& eval) const {
    TacoValue val = eval.evaluate(value.get());
    eval.set_var(name, val);
    return val;
}

// ────────────────────────────────
// 运算
// ────────────────────────────────

TacoValue BinaryExpr::evaluate(Evaluator& eval) const {
    // 短路求值：&& 和 || 先求左边
    if (op == "&&") {
        TacoValue l = eval.evaluate(left.get());
        if (!is_truthy(l)) return false;
        return is_truthy(eval.evaluate(right.get()));
    }
    if (op == "||") {
        TacoValue l = eval.evaluate(left.get());
        if (is_truthy(l)) return true;
        return is_truthy(eval.evaluate(right.get()));
    }

    TacoValue l = eval.evaluate(left.get());
    TacoValue r = eval.evaluate(right.get());

    // 字符串拼接
    if (op == "+" &&
        std::holds_alternative<std::string>(l) &&
        std::holds_alternative<std::string>(r)) {
        return std::get<std::string>(l) + std::get<std::string>(r);
    }

    // 算术运算
    if (op == "+" || op == "-" || op == "*" ||
        op == "/" || op == "%" || op == "^") {
        if (!std::holds_alternative<double>(l) ||
            !std::holds_alternative<double>(r)) {
            throw std::runtime_error(
                "🌮 Operator '" + op + "' requires numbers."
            );
        }
        double lv = std::get<double>(l);
        double rv = std::get<double>(r);

        if (op == "+") return lv + rv;
        if (op == "-") return lv - rv;
        if (op == "*") return lv * rv;
        if (op == "/") {
            if (rv == 0)
                throw std::runtime_error("🌮 Division by zero. Nobody can.");
            return lv / rv;
        }
        if (op == "%") return std::fmod(lv, rv);
        if (op == "^") return std::pow(lv, rv);
    }

    // 比较运算
    if (op == "==") return values_equal(l, r);
    if (op == "!=") return !values_equal(l, r);

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!std::holds_alternative<double>(l) ||
            !std::holds_alternative<double>(r)) {
            throw std::runtime_error(
                "🌮 Operator '" + op + "' requires numbers."
            );
        }
        double lv = std::get<double>(l);
        double rv = std::get<double>(r);
        if (op == "<")  return lv < rv;
        if (op == ">")  return lv > rv;
        if (op == "<=") return lv <= rv;
        if (op == ">=") return lv >= rv;
    }

    throw std::runtime_error("🌮 Unknown operator: '" + op + "'.");
}

TacoValue UnaryExpr::evaluate(Evaluator& eval) const {
    TacoValue val = eval.evaluate(operand.get());
    if (op == "!") return !is_truthy(val);
    if (op == "-") {
        if (!std::holds_alternative<double>(val))
            throw std::runtime_error("🌮 Unary '-' requires a number.");
        return -std::get<double>(val);
    }
    throw std::runtime_error("🌮 Unknown unary operator: '" + op + "'.");
}

// ────────────────────────────────
// 变量声明
// ────────────────────────────────

TacoValue VarDecl::evaluate(Evaluator& eval) const {
    TacoValue val = eval.evaluate(value.get());
    eval.define_var(name, val);
    return val;
}

// ────────────────────────────────
// 控制流
// ────────────────────────────────

TacoValue IfStmt::evaluate(Evaluator& eval) const {
    for (const auto& branch : branches) {
        TacoValue cond = eval.evaluate(branch.condition.get());
        if (is_truthy(cond)) {
            eval.push_scope();
            TacoValue result = nullptr;
            for (const auto& stmt : branch.body) {
                result = eval.evaluate(stmt.get());
            }
            eval.pop_scope();
            return result;
        }
    }
    // else 分支
    if (!else_body.empty()) {
        eval.push_scope();
        TacoValue result = nullptr;
        for (const auto& stmt : else_body) {
            result = eval.evaluate(stmt.get());
        }
        eval.pop_scope();
        return result;
    }
    return nullptr;
}

TacoValue WhileStmt::evaluate(Evaluator& eval) const {
    TacoValue result = nullptr;
    while (is_truthy(eval.evaluate(condition.get()))) {
        eval.push_scope();
        for (const auto& stmt : body) {
            result = eval.evaluate(stmt.get());
        }
        eval.pop_scope();
    }
    return result;
}

TacoValue ForStmt::evaluate(Evaluator& eval) const {
    eval.push_scope();  // for 循环的作用域

    // 初始化
    if (init) eval.evaluate(init.get());

    TacoValue result = nullptr;
    while (true) {
        // 检查条件
        if (condition) {
            TacoValue cond = eval.evaluate(condition.get());
            if (!is_truthy(cond)) break;
        }

        // 执行循环体
        eval.push_scope();
        for (const auto& stmt : body) {
            result = eval.evaluate(stmt.get());
        }
        eval.pop_scope();

        // 更新
        if (update) eval.evaluate(update.get());
    }

    eval.pop_scope();
    return result;
}

TacoValue ForRangeStmt::evaluate(Evaluator& eval) const {
    TacoValue start_val = eval.evaluate(start.get());
    TacoValue end_val   = eval.evaluate(end.get());

    if (!std::holds_alternative<double>(start_val) ||
        !std::holds_alternative<double>(end_val)) {
        throw std::runtime_error("🌮 range() requires numbers.");
    }

    double from = std::get<double>(start_val);
    double to   = std::get<double>(end_val);

    TacoValue result = nullptr;
    for (double i = from; i < to; i++) {
        eval.push_scope();
        eval.define_var(var_name, i);
        for (const auto& stmt : body) {
            result = eval.evaluate(stmt.get());
        }
        eval.pop_scope();
    }
    return result;
}

TacoValue SwitchStmt::evaluate(Evaluator& eval) const {
    TacoValue subject_val = eval.evaluate(subject.get());

    for (const auto& c : cases) {
        bool matches = false;

        if (c.value == nullptr) {
            // default 分支：总是匹配（如果没有其他分支匹配）
            matches = true;
        } else {
            TacoValue case_val = eval.evaluate(c.value.get());
            matches = values_equal(subject_val, case_val);
        }

        if (matches) {
            eval.push_scope();
            TacoValue result = nullptr;
            for (const auto& stmt : c.body) {
                result = eval.evaluate(stmt.get());
            }
            eval.pop_scope();
            return result;  // 每个 case 默认不穿透
        }
    }

    return nullptr;
}

// ────────────────────────────────
// 函数调用（暂时只有内置函数）
// ────────────────────────────────

TacoValue CallExpr::evaluate(Evaluator& eval) const {
    // 目前只支持内置函数，v3 会加用户定义函数
    if (callee == "print") {
        if (arguments.size() != 1) {
            throw std::runtime_error("🌮 print() takes exactly 1 argument.");
        }
        TacoValue val = eval.evaluate(arguments[0].get());
        std::cout << value_to_string(val) << "\n";
        return nullptr;
    }

    if (callee == "range") {
        // range 在 ForRangeStmt 里直接处理，这里不应该被调用
        throw std::runtime_error("🌮 range() can only be used in for loops.");
    }

    throw std::runtime_error("🌮 '" + callee + "' is not defined.");
}

// ────────────────────────────────
// return 语句
// ────────────────────────────────

TacoValue ReturnStmt::evaluate(Evaluator& eval) const {
    TacoValue val = value ? eval.evaluate(value.get()) : TacoValue{nullptr};
    throw ReturnException{val};  // 用异常实现 return 的控制流跳转
}

// ────────────────────────────────
// 程序
// ────────────────────────────────

TacoValue Program::evaluate(Evaluator& eval) const {
    TacoValue last = nullptr;
    for (const auto& stmt : statements) {
        last = eval.evaluate(stmt.get());
    }
    return last;
}
```

---

## 15.5 更新语法分析器

语法分析器需要支持新的语法结构：if、while、for、switch。

完整的 parser.cpp 太长，这里展示新增的控制流解析部分：

```cpp
// parser.cpp（新增部分）

ExprPtr Parser::parse_statement() {
    switch (current().type) {
        case TokenType::Var:    return parse_var_decl();
        case TokenType::If:     return parse_if_stmt();
        case TokenType::While:  return parse_while_stmt();
        case TokenType::For:    return parse_for_stmt();
        case TokenType::Switch: return parse_switch_stmt();
        case TokenType::Return: return parse_return_stmt();
        default: break;
    }

    // 表达式语句
    auto expr = parse_expression();
    expect(TokenType::Semicolon, "Expected ';' after expression.");
    return expr;
}

// if/elseif/else
ExprPtr Parser::parse_if_stmt() {
    auto node = std::make_unique<IfStmt>();

    // 解析 if 分支
    expect(TokenType::If, "Expected 'if'.");
    expect(TokenType::LeftParen, "Expected '(' after 'if'.");

    IfStmt::Branch if_branch;
    if_branch.condition = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after condition.");
    expect(TokenType::LeftBrace, "Expected '{' after condition.");
    if_branch.body = parse_block();
    node->branches.push_back(std::move(if_branch));

    // 解析 elseif 分支
    while (current().type == TokenType::Elseif) {
        advance();
        expect(TokenType::LeftParen, "Expected '(' after 'elseif'.");

        IfStmt::Branch elseif_branch;
        elseif_branch.condition = parse_expression();
        expect(TokenType::RightParen, "Expected ')' after condition.");
        expect(TokenType::LeftBrace, "Expected '{'.");
        elseif_branch.body = parse_block();
        node->branches.push_back(std::move(elseif_branch));
    }

    // 解析 else 分支
    if (current().type == TokenType::Else) {
        advance();
        expect(TokenType::LeftBrace, "Expected '{' after 'else'.");
        node->else_body = parse_block();
    }

    return node;
}

// while 循环
ExprPtr Parser::parse_while_stmt() {
    expect(TokenType::While, "Expected 'while'.");
    expect(TokenType::LeftParen, "Expected '(' after 'while'.");

    auto node = std::make_unique<WhileStmt>();
    node->condition = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after condition.");
    expect(TokenType::LeftBrace, "Expected '{'.");
    node->body = parse_block();

    return node;
}

// for 循环（C 风格或 range 风格）
ExprPtr Parser::parse_for_stmt() {
    expect(TokenType::For, "Expected 'for'.");

    // range 风格：for i in range(0, 10)
    if (current().type == TokenType::Identifier &&
        peek().type == TokenType::In) {
        return parse_for_range_stmt();
    }

    // C 风格：for (var i = 0; i < 10; i++)
    expect(TokenType::LeftParen, "Expected '(' after 'for'.");

    auto node = std::make_unique<ForStmt>();

    // 初始化
    if (current().type != TokenType::Semicolon) {
        node->init = parse_statement_no_brace();
    } else {
        advance();  // 跳过空的初始化部分的分号
    }

    // 条件
    if (current().type != TokenType::Semicolon) {
        node->condition = parse_expression();
    }
    expect(TokenType::Semicolon, "Expected ';' after for condition.");

    // 更新
    if (current().type != TokenType::RightParen) {
        node->update = parse_expression();
    }
    expect(TokenType::RightParen, "Expected ')' after for clauses.");
    expect(TokenType::LeftBrace, "Expected '{'.");
    node->body = parse_block();

    return node;
}

ExprPtr Parser::parse_for_range_stmt() {
    auto node = std::make_unique<ForRangeStmt>();

    // 变量名
    node->var_name = expect(TokenType::Identifier,
                            "Expected variable name.").value;
    expect(TokenType::In, "Expected 'in'.");

    // range(start, end)
    expect(TokenType::Identifier, "Expected 'range'.");  // 消费 "range"
    expect(TokenType::LeftParen, "Expected '(' after 'range'.");
    node->start = parse_expression();
    expect(TokenType::Comma, "Expected ',' in range.");
    node->end = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after range.");
    expect(TokenType::LeftBrace, "Expected '{'.");
    node->body = parse_block();

    return node;
}

// switch 语句
ExprPtr Parser::parse_switch_stmt() {
    expect(TokenType::Switch, "Expected 'switch'.");
    expect(TokenType::LeftParen, "Expected '(' after 'switch'.");

    auto node = std::make_unique<SwitchStmt>();
    node->subject = parse_expression();
    expect(TokenType::RightParen, "Expected ')' after switch expression.");
    expect(TokenType::LeftBrace, "Expected '{'.");

    while (current().type != TokenType::RightBrace && !is_at_end()) {
        SwitchStmt::Case c;

        if (current().type == TokenType::Case) {
            advance();
            c.value = parse_expression();
        } else if (current().type == TokenType::Default) {
            advance();
            c.value = nullptr;  // nullptr 表示 default
        } else {
            error("Expected 'case' or 'default'.");
        }

        expect(TokenType::LeftBrace, "Expected '{' after case.");
        c.body = parse_block();
        node->cases.push_back(std::move(c));
    }

    expect(TokenType::RightBrace, "Expected '}' after switch.");
    return node;
}

// return 语句
ExprPtr Parser::parse_return_stmt() {
    expect(TokenType::Return, "Expected 'return'.");

    ExprPtr value = nullptr;
    if (current().type != TokenType::Semicolon) {
        value = parse_expression();
    }
    expect(TokenType::Semicolon, "Expected ';' after return.");

    return std::make_unique<ReturnStmt>(std::move(value));
}

// 解析 { ... } 块里的语句列表
std::vector<ExprPtr> Parser::parse_block() {
    std::vector<ExprPtr> stmts;
    while (current().type != TokenType::RightBrace && !is_at_end()) {
        stmts.push_back(parse_statement());
    }
    expect(TokenType::RightBrace, "Expected '}' after block.");
    return stmts;
}
```

---

## 15.6 字符串插值的实现

Taco 支持字符串插值：`"Hola, {name}!"`。在词法分析阶段，字符串被整体读取，插值表达式没有被识别。

处理插值需要在字符串求值时，扫描 `{...}` 并对其中的表达式求值：

```cpp
// 在 StringLiteral::evaluate 里处理插值
TacoValue StringLiteral::evaluate(Evaluator& eval) const {
    std::string result;
    int i = 0;

    while (i < static_cast<int>(value.size())) {
        if (value[i] == '{') {
            // 找到插值表达式的结尾
            int j = i + 1;
            while (j < static_cast<int>(value.size()) && value[j] != '}') {
                j++;
            }
            if (j >= static_cast<int>(value.size())) {
                result += '{';  // 没有配对的 }，当普通字符处理
                i++;
                continue;
            }

            // 提取 {} 里的表达式
            std::string expr_src = value.substr(i + 1, j - i - 1);

            // 对这个表达式单独做词法分析和语法分析
            try {
                Lexer lexer(expr_src);
                auto tokens = lexer.tokenize();
                Parser parser(std::move(tokens));
                // 解析单个表达式
                auto expr = parser.parse_expression_only();
                TacoValue val = eval.evaluate(expr.get());
                result += value_to_string(val);
            } catch (...) {
                // 解析失败，当普通文本处理
                result += value.substr(i, j - i + 1);
            }

            i = j + 1;
        } else {
            result += value[i++];
        }
    }

    return result;
}
```

---

## 15.7 测试：运行第一个真实的 Taco 程序

创建 `test_v2.taco`：

```taco
// 基础计算
var x = 100;
var y = x / 4 + 5;
print(y);  // 30

// 字符串插值
var name = "Miguel";
print("Hola, {name}!");  // Hola, Miguel!

// if/elseif/else
var score = 85;
if (score >= 90) {
    print("A");
} elseif (score >= 80) {
    print("B");
} else {
    print("C");
}

// while 循环
var n = 5;
var factorial = 1;
while (n > 0) {
    factorial = factorial * n;
    n = n - 1;
}
print(factorial);  // 120

// C 风格 for 循环（求和）
var sum = 0;
for (var i = 1; i <= 10; i = i + 1) {
    sum = sum + i;
}
print(sum);  // 55

// range 风格 for 循环
for i in range(0, 5) {
    print(i);
}

// switch 语句
var day = 3;
switch (day) {
    case 1 { print("Monday"); }
    case 2 { print("Tuesday"); }
    case 3 { print("Wednesday"); }
    default { print("Other"); }
}
```

### 运行结果

```
30
Hola, Miguel!
B
120
55
0
1
2
3
4
Wednesday
```

---

## 15.8 这个版本的局限性

**没有用户定义函数**

`func greet(name) { ... }` 还不支持。函数需要闭包和作用域的完整实现，v3 会加。

**`i++` 还不支持**

for 循环的更新部分现在要写成 `i = i + 1`，因为后缀自增运算符还没实现。这是一个小的语法糖，可以在解析器里加一个特殊处理，这里先留着。

**没有 break 和 continue**

循环里没有 `break` 和 `continue`。实现方式和 `return` 类似——用特殊的异常来中断控制流。

**字符串插值是"嵌套解析"**

目前的字符串插值实现方式是：对 `{}` 里的内容再做一次完整的词法分析和语法分析。这在工程上不是最干净的做法，完整的实现应该在词法分析阶段就把插值字符串拆开。这留给以后的版本。

---

## 小结

v2 完成了两件大事：

**重构求值器**：从 `dynamic_cast` 链改成虚函数。每个 AST 节点自己实现 `evaluate()`，求值器只需要调用 `expr->evaluate(*this)`。代码更简洁，性能更好，扩展更容易。

**加入控制流**：`if/elseif/else`、`while`、C 风格 `for`、range 风格 `for`、`switch`。每种控制结构都有对应的 AST 节点，求值逻辑在节点的 `evaluate()` 里。

**作用域**用作用域链实现：一个 `vector` 存储多层 `map`，查找变量从内向外遍历。进入新的代码块时 `push_scope()`，退出时 `pop_scope()`。

**`return` 语句**用异常实现控制流跳转——这是一种常见的解释器实现技巧，让 `return` 可以从任意深度的嵌套中跳出。

---

第三部分到这里结束。第四部分进入现代 C++ 核心：所有权、智能指针、移动语义。学完之后 v3 会用智能指针管理 AST 节点，并加入用户定义函数和闭包。
# 第十六章：所有权与生命周期

---

第四部分进入现代 C++ 最核心的领域：内存管理。这是 C++ 和几乎所有其他语言差距最大的地方，也是 C++ 程序员必须真正理解的东西。

Python 用垃圾回收，内存管理对程序员透明。C 把内存管理完全交给程序员，`malloc`/`free` 要配对。C++ 走了第三条路：**用语言机制（RAII + 智能指针）让内存管理既安全又高效，既不需要垃圾回收，也不需要手动 `free`**。

这一章先把基础讲清楚：栈和堆的区别，裸指针的问题，所有权的概念。

---

## 16.1 C++ 的内存模型：栈与堆

程序运行时使用的内存分为两个主要区域：**栈**（stack）和**堆**（heap）。理解这两者的区别，是理解 C++ 内存管理的基础。

### 栈

栈内存由编译器自动管理。函数调用时，局部变量被压入栈；函数返回时，这些变量被自动弹出。

```cpp
void foo() {
    int x = 10;      // x 在栈上
    double y = 3.14; // y 在栈上
    // ...
}  // foo 返回，x 和 y 自动销毁
```

栈的特点：
- **自动管理**：变量离开作用域自动销毁，不需要手动释放
- **速度极快**：分配只是移动栈指针，几乎没有开销
- **大小有限**：通常是几 MB，存太多数据会栈溢出（stack overflow）
- **生命周期固定**：绑定到函数调用，不能超出函数范围

### 堆

堆内存需要手动管理（或通过智能指针间接管理）。用 `new` 在堆上分配，用 `delete` 释放：

```cpp
int* p = new int(42);  // 在堆上分配一个 int，值为 42
// p 本身在栈上，指向堆上的内存

*p = 100;   // 通过指针修改堆上的值
delete p;   // 手动释放堆上的内存
p = nullptr; // 好习惯：释放后置空
```

堆的特点：
- **手动管理**（或通过智能指针）：需要显式释放
- **大小灵活**：可以用几个 GB
- **生命周期灵活**：可以跨函数存在
- **速度相对慢**：分配需要向操作系统申请内存，有一定开销

### 一个直观的对比

```cpp
void stack_example() {
    int arr[1000];  // 在栈上，4000 字节，自动管理
}  // 函数返回，arr 消失

void heap_example() {
    int* arr = new int[1000];  // 在堆上，4000 字节
    // 可以把 arr 传出去，继续使用
    delete[] arr;  // 必须手动释放
}
```

在 Taco 解释器里，AST 节点需要跨越多个函数调用存在（从解析一直到求值），所以必须放在堆上。`std::unique_ptr` 管理它们的生命周期，不需要手动 `delete`。

---

### 对象的存储位置

C++ 里对象的存储位置取决于怎么创建它：

```cpp
class Token { ... };

// 在栈上创建
Token t1("NUMBER", "42");  // t1 在栈上，函数返回自动销毁

// 在堆上创建（裸指针，不推荐）
Token* t2 = new Token("STRING", "hello");  // 在堆上，需要手动 delete
delete t2;

// 在堆上创建（unique_ptr，推荐）
auto t3 = std::make_unique<Token>("IDENTIFIER", "x");  // 在堆上，自动管理

// 在容器里（容器管理内存）
std::vector<Token> tokens;
tokens.push_back(Token("PLUS", "+"));  // Token 在 vector 内部的堆内存里
```

---

## 16.2 裸指针的问题

裸指针（raw pointer）就是 C 里的普通指针：`int*`、`Token*`、`Expr*`。裸指针本身没有问题，问题在于**手动管理生命周期非常容易出错**。

### 内存泄漏

```cpp
void process(const std::string& path) {
    Token* t = new Token("STRING", path);

    if (path.empty()) {
        return;  // 忘了 delete t！内存泄漏
    }

    // 使用 t...
    delete t;
}
```

函数提前返回时，`delete t` 被跳过，分配的内存永远不会被释放。程序运行时间越长，泄漏越多，最终可能导致内存耗尽崩溃。

### 悬空指针

```cpp
Token* t = new Token("NUMBER", "42");
delete t;    // 释放内存
t->value;    // 未定义行为！t 是悬空指针
             // 可能崩溃，可能输出乱码，可能"正常运行"（最危险的情况）
```

### Double Free

```cpp
Token* t = new Token("NUMBER", "42");
delete t;
delete t;  // 未定义行为！同一块内存释放两次
           // 通常导致程序崩溃
```

### 所有权不明确

```cpp
Token* create_token() {
    return new Token("VAR", "var");
}

void use_token() {
    Token* t = create_token();
    // 问题来了：谁负责 delete t？
    // 调用者？create_token 函数内部？
    // 文档里没写，代码里也看不出来
}
```

裸指针传来传去，所有权不清晰，很难判断什么时候、谁来释放内存。

---

## 16.3 所有权是什么

**所有权**（ownership）是 C++ 的核心概念：**某个对象"拥有"某块内存，负责在适当的时候释放它**。

所有权有几个关键性质：

**唯一性**：一块内存最好只有一个所有者，否则很容易 double free。

**可转移**：所有权可以从一个对象转移到另一个对象（移动语义，第十八章讲）。

**可借用**：可以在不转移所有权的情况下，临时使用某块内存（引用/指针）。

所有权模型用一句话总结：**谁创建，谁负责销毁；或者把所有权交给另一个对象，让它负责。**

---

### Rust 的所有权 vs C++ 的所有权

Rust 把所有权做成了语言的核心特性，用编译器强制检查所有权规则，保证内存安全。这是 Rust 最大的创新。

C++ 没有语言层面的所有权检查，但通过以下工具在实践中实现所有权管理：

- `std::unique_ptr`：独占所有权
- `std::shared_ptr`：共享所有权
- RAII：把所有权绑定到对象生命周期
- 移动语义：高效转移所有权

这些工具不强制使用（裸指针也可以），所以 C++ 需要程序员有更强的纪律性——但换来的是更大的灵活性和更低的运行时开销（`unique_ptr` 没有引用计数，`shared_ptr` 才有）。

---

## 16.4 RAII 深讲

RAII（Resource Acquisition Is Initialization）在第八章提过，这里深入讲一遍。

RAII 的精髓是：**把资源的获取和释放，绑定到对象的构造和析构**。

```
对象构造 → 资源获取（打开文件、分配内存、加锁）
对象析构 → 资源释放（关闭文件、释放内存、解锁）
```

这样，只要对象的生命周期是确定的（栈上对象离开作用域自动析构），资源的生命周期也是确定的。

### RAII 的经典例子

**文件**：

```cpp
class File {
public:
    explicit File(const std::string& path, const std::string& mode = "r")
        : m_file(std::fopen(path.c_str(), mode.c_str()))
    {
        if (!m_file) {
            throw std::runtime_error("Cannot open: " + path);
        }
    }

    ~File() {
        if (m_file) std::fclose(m_file);
    }

    // 禁止拷贝（文件句柄不能复制）
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    // 允许移动
    File(File&& other) noexcept : m_file(other.m_file) {
        other.m_file = nullptr;
    }

    FILE* get() const { return m_file; }

private:
    FILE* m_file;
};

void read_taco_file(const std::string& path) {
    File f(path);          // 打开文件
    // 使用 f.get()...
}  // f 析构，文件自动关闭——不管函数从哪里返回
```

**互斥锁**：

```cpp
std::mutex m;

void thread_safe_increment(int& count) {
    std::lock_guard<std::mutex> lock(m);  // 构造时加锁
    count++;
    // lock 析构时自动解锁，不管函数怎么返回
}
```

**内存**（这就是 `unique_ptr` 做的事）：

```cpp
// RAII 包装裸指针的概念演示（实际用 unique_ptr）
template<typename T>
class UniqueOwner {
public:
    explicit UniqueOwner(T* ptr) : m_ptr(ptr) {}

    ~UniqueOwner() {
        delete m_ptr;  // 析构时释放内存
    }

    T* get() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    T* operator->() const { return m_ptr; }

    // 禁止拷贝（所有权是唯一的）
    UniqueOwner(const UniqueOwner&) = delete;
    UniqueOwner& operator=(const UniqueOwner&) = delete;

private:
    T* m_ptr;
};

void example() {
    UniqueOwner<Token> t(new Token("VAR", "var"));
    // 使用 t->value，*t 等
}  // t 析构，Token 自动释放
```

这个 `UniqueOwner` 就是 `std::unique_ptr` 的简化版。下一章讲 `unique_ptr` 的完整用法。

---

### RAII 的层次

RAII 可以层层嵌套——RAII 对象管理 RAII 对象，最终形成一个资源管理的层次结构：

```cpp
class Evaluator {
    // m_scopes 是 vector of unordered_map
    // vector 管理 unordered_map 的内存
    // unordered_map 管理 string 和 TacoValue 的内存
    // TacoValue（variant）管理内部值的内存
    // 所有这些都是 RAII，Evaluator 析构时全部自动释放
    std::vector<std::unordered_map<std::string, TacoValue>> m_scopes;
};
```

```cpp
class Parser {
    // tokens 是 vector<Token>
    // 每个 Token 里的 string 是 RAII
    std::vector<Token> m_tokens;

    // AST 节点用 unique_ptr 管理
    // parse() 返回的 unique_ptr<Program> 拥有整棵树
    // 树里每个节点用 unique_ptr 管理子节点
};
```

在 Taco 解释器里，几乎所有内存都由标准库容器或 `unique_ptr` 管理，不需要手动写任何析构函数。

---

### 异常安全

RAII 的另一个重要好处是**异常安全**（exception safety）。

没有 RAII 时，异常可能导致资源泄漏：

```cpp
void unsafe_function() {
    FILE* f = std::fopen("data.taco", "r");
    Token* t = new Token("VAR", "var");

    if (some_condition()) {
        throw std::runtime_error("Error!");  // 异常！
        // f 和 t 都泄漏了，下面的 delete 和 fclose 不会执行
    }

    delete t;
    std::fclose(f);
}
```

用 RAII 时，异常触发析构函数，资源自动释放：

```cpp
void safe_function() {
    File f("data.taco");                    // RAII
    auto t = std::make_unique<Token>("VAR", "var");  // RAII

    if (some_condition()) {
        throw std::runtime_error("Error!");
        // 异常！但 f 和 t 的析构函数仍然会被调用
        // 文件自动关闭，Token 自动释放
    }
}
```

C++ 保证：当异常被抛出时，已经构造完成的局部对象的析构函数会被调用。这叫**栈展开**（stack unwinding）。RAII 利用这个保证来实现异常安全。

---

## 小结

**栈**自动管理，速度快，大小有限，生命周期绑定函数调用。**堆**需要手动管理（或通过智能指针），大小灵活，生命周期自由。

**裸指针**的三大问题：内存泄漏、悬空指针、double free——都源于手动管理生命周期的不可靠性。

**所有权**是解决这些问题的核心概念：明确谁拥有某块内存，谁负责释放。C++ 用 `unique_ptr`（唯一所有权）和 `shared_ptr`（共享所有权）来表达和管理所有权。

**RAII** 把资源管理绑定到对象生命周期：构造时获取，析构时释放。RAII 让异常安全变得自然，因为析构函数在任何情况下（包括异常）都会被调用。

---

下一章讲 `unique_ptr` 和 `shared_ptr`——这是现代 C++ 内存管理的核心工具，也是 v3 重构 AST 所有权的基础。
# 第十七章：智能指针

---

上一章讲了裸指针的问题和 RAII 的思想。这一章讲 C++ 标准库提供的三种智能指针：`unique_ptr`、`shared_ptr`、`weak_ptr`。

智能指针是 RAII 思想的直接应用——它们是包装了裸指针的类，在析构时自动释放内存。使用智能指针，几乎可以完全避免手动 `new`/`delete`。

---

## 17.1 unique_ptr：独占所有权

`std::unique_ptr<T>` 表示对某块内存的**唯一所有权**：同一时刻只有一个 `unique_ptr` 拥有某块内存，它销毁时内存自动释放。

### 基本用法

```cpp
#include <memory>

// 创建 unique_ptr（推荐用 make_unique）
auto p = std::make_unique<Token>("NUMBER", "42");
// 等价于：std::unique_ptr<Token> p(new Token("NUMBER", "42"));
// 但 make_unique 更安全（异常情况下不会泄漏）

// 使用：和裸指针一样
std::cout << p->value << "\n";   // 用 -> 访问成员
std::cout << (*p).value << "\n"; // 用 * 解引用

// 获取裸指针（不转移所有权）
Token* raw = p.get();

// p 离开作用域时，Token 自动释放
```

### 所有权是唯一的：不能拷贝

`unique_ptr` 的"唯一"体现在：它不能被拷贝，只能被移动：

```cpp
auto p1 = std::make_unique<Token>("VAR", "var");

// 拷贝：被禁止
auto p2 = p1;  // 错误！unique_ptr 不能拷贝

// 移动：转移所有权
auto p2 = std::move(p1);
// 现在 p2 拥有 Token，p1 变成 nullptr
// 访问 p1 是未定义行为！

if (p1 == nullptr) {
    std::cout << "p1 is now empty\n";
}
std::cout << p2->value << "\n";  // var
```

为什么不能拷贝？因为如果允许拷贝，两个 `unique_ptr` 会指向同一块内存，都认为自己是所有者，析构时会 double free。

### 从函数返回 unique_ptr

这是最常见的用法之一——函数创建对象，通过 `unique_ptr` 返回，调用者获得所有权：

```cpp
std::unique_ptr<Expr> make_number(double value) {
    return std::make_unique<NumberLiteral>(value);
}

// 调用：
auto expr = make_number(42.0);  // expr 拥有 NumberLiteral
// expr 离开作用域时，NumberLiteral 自动释放
```

返回局部 `unique_ptr` 时，编译器会自动应用移动语义，不需要显式写 `std::move`。

### 把 unique_ptr 作为函数参数

有三种方式，语义不同：

```cpp
// 1. 传值：转移所有权给函数，调用后调用者不再拥有对象
void take_ownership(std::unique_ptr<Token> t) {
    // 函数拥有 t，函数返回时 t 自动释放
}

auto t = std::make_unique<Token>("VAR", "var");
take_ownership(std::move(t));  // 必须显式 move
// 之后 t 是 nullptr，不能再使用

// 2. 传引用：借用，不转移所有权
void borrow(std::unique_ptr<Token>& t) {
    t->value = "modified";  // 修改对象
    // 函数结束，t 的所有权不变
}

// 3. 传裸指针/引用：只是访问，不涉及所有权
void just_read(const Token* t) {
    std::cout << t->value << "\n";
}

auto t2 = std::make_unique<Token>("STRING", "hello");
just_read(t2.get());  // get() 返回裸指针，不转移所有权
```

**规则**：函数需要使用对象但不拥有它，传 `const T*` 或 `const T&`；函数需要修改对象但不拥有它，传 `T*` 或 `T&`；函数需要拥有对象，传 `unique_ptr<T>`（值传递）。

### unique_ptr 在 Taco AST 里的应用

v1/v2 里已经用了 `unique_ptr` 管理 AST 节点，这就是正确的做法：

```cpp
// ExprPtr 是 unique_ptr<Expr> 的别名
using ExprPtr = std::unique_ptr<Expr>;

struct BinaryExpr : Expr {
    ExprPtr left;   // BinaryExpr 拥有左子节点
    ExprPtr right;  // BinaryExpr 拥有右子节点
    std::string op;

    BinaryExpr(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l))  // 移动，转移所有权
        , op(std::move(o))
        , right(std::move(r))
    {}

    ~BinaryExpr() = default;
    // 不需要手动 delete left 和 right
    // BinaryExpr 析构时，left 和 right 的 unique_ptr 析构函数自动调用
    // 递归释放整棵子树
};
```

整棵 AST 树的所有权是这样的：

```
unique_ptr<Program>
└── vector<ExprPtr>
    ├── unique_ptr<VarDecl>
    │   └── unique_ptr<BinaryExpr>
    │       ├── unique_ptr<NumberLiteral>
    │       └── unique_ptr<NumberLiteral>
    └── unique_ptr<PrintStmt>
        └── unique_ptr<Identifier>
```

`Program` 被一个 `unique_ptr` 拥有。`Program` 里的每个语句被 `vector` 里的 `unique_ptr` 拥有。每个节点拥有它的子节点。当顶层的 `unique_ptr<Program>` 析构时，整棵树递归析构，所有节点自动释放。

---

## 17.2 shared_ptr：共享所有权

`std::shared_ptr<T>` 表示对某块内存的**共享所有权**：可以有多个 `shared_ptr` 指向同一块内存，内部用引用计数追踪有多少个 `shared_ptr` 拥有它，当最后一个 `shared_ptr` 析构时，内存才被释放。

### 基本用法

```cpp
// 创建 shared_ptr（推荐用 make_shared）
auto p1 = std::make_shared<Token>("NUMBER", "42");

// 拷贝：增加引用计数
auto p2 = p1;  // p1 和 p2 共同拥有 Token
               // 引用计数变为 2

{
    auto p3 = p1;  // 引用计数变为 3
}  // p3 析构，引用计数变为 2，Token 不释放

// 查看引用计数（调试用）
std::cout << p1.use_count() << "\n";  // 2

p1.reset();  // p1 放弃所有权，引用计数变为 1
// p2 析构，引用计数变为 0，Token 释放
```

### shared_ptr 的开销

`shared_ptr` 比 `unique_ptr` 有更多开销：

- **空间开销**：引用计数通常存在堆上（控制块），每个 `shared_ptr` 持有两个指针（指向对象的指针和指向控制块的指针）
- **时间开销**：每次拷贝或析构需要原子操作修改引用计数（线程安全的引用计数修改比普通加减慢）

`make_shared` 可以优化空间：它把对象和控制块分配在同一块内存里，只需要一次内存分配：

```cpp
// 两次内存分配（先 new Token，再分配控制块）
std::shared_ptr<Token> p(new Token("VAR", "var"));

// 一次内存分配（对象和控制块在一起）——推荐
auto p = std::make_shared<Token>("VAR", "var");
```

### 什么时候用 shared_ptr

当一块内存真的需要被多个对象共享，且无法确定谁最后一个用完时，用 `shared_ptr`。

在 Taco 解释器里，典型的场景是**环境（Environment）**——闭包需要捕获创建它时的外层作用域。外层函数可能已经返回，但闭包还在使用那个作用域。这时，外层函数和闭包共同拥有那个作用域，用 `shared_ptr` 管理：

```cpp
// 环境：变量名 → 值的映射
class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;

    Environment(Ptr parent = nullptr)
        : m_parent(std::move(parent))
    {}

    // ...变量的查找和定义

private:
    Ptr m_parent;  // 外层作用域
    std::unordered_map<std::string, TacoValue> m_vars;
};

// 创建函数时，捕获当前环境
class Function {
public:
    Environment::Ptr closure_env;  // shared_ptr，和创建时的作用域共享
    // ...
};
```

这样，即使创建函数的作用域已经退出，闭包仍然持有对那个环境的引用，环境不会被释放。v3 实现函数时会看到这个模式。

---

## 17.3 weak_ptr：打破循环引用

`std::weak_ptr<T>` 是 `shared_ptr` 的"弱引用"——它可以观察 `shared_ptr` 指向的对象，但不增加引用计数，也不拥有对象。

### 循环引用问题

`shared_ptr` 有一个著名的问题：循环引用导致内存泄漏。

```cpp
struct Node {
    std::shared_ptr<Node> next;
    int value;
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();

a->next = b;  // a 引用 b，b 的引用计数变为 2
b->next = a;  // b 引用 a，a 的引用计数变为 2

// 函数结束，a 和 b 局部变量析构
// a 的引用计数：2 → 1（b->next 还引用着 a）
// b 的引用计数：2 → 1（a->next 还引用着 b）
// 两者都不会到 0，内存泄漏！
```

两个节点互相引用，形成一个环，引用计数永远不会降到 0。

### weak_ptr 解决循环引用

```cpp
struct Node {
    std::weak_ptr<Node> next;  // 改用 weak_ptr，不增加引用计数
    int value;
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();

a->next = b;  // 不增加 b 的引用计数
b->next = a;  // 不增加 a 的引用计数

// 函数结束，a 和 b 析构
// a 的引用计数：1 → 0，a 释放
// b 的引用计数：1 → 0，b 释放
```

### weak_ptr 的使用

`weak_ptr` 不能直接访问对象，必须先转成 `shared_ptr`（`lock()` 方法），如果对象已经释放，`lock()` 返回空 `shared_ptr`：

```cpp
std::weak_ptr<Token> weak;

{
    auto strong = std::make_shared<Token>("VAR", "var");
    weak = strong;  // weak_ptr 指向 Token，但不增加引用计数

    // 通过 weak_ptr 访问对象
    if (auto p = weak.lock()) {  // lock() 返回 shared_ptr
        std::cout << p->value << "\n";  // var
    }
}  // strong 析构，Token 释放

// 现在 Token 已经释放
if (auto p = weak.lock()) {
    // 不会进入这里
} else {
    std::cout << "Token already released\n";
}
```

---

## 17.4 什么时候用哪个

一个简单的决策流程：

**首先考虑是否需要堆上分配**：很多时候，栈上的对象或标准库容器里的对象就够了，不需要任何智能指针。

**如果需要堆上分配，所有权是唯一的**：用 `unique_ptr`。这是最常见的情况。Taco AST 节点就是这样的——每个节点只有一个父节点，父节点拥有子节点。

**如果所有权真的需要共享**：用 `shared_ptr`。Taco 的闭包环境是这样的——函数和它捕获的外层作用域共同拥有那个环境。

**如果需要引用 `shared_ptr` 管理的对象，但不想拥有它**：用 `weak_ptr`。主要用于打破 `shared_ptr` 的循环引用。

**如果只是暂时访问某个对象，不涉及所有权**：用裸指针或引用。`unique_ptr::get()` 可以获取裸指针，但不要存储它——它可能随时失效。

```cpp
// Taco 里典型的几种情况：

// 1. AST 节点：unique_ptr，所有权清晰
using ExprPtr = std::unique_ptr<Expr>;
ExprPtr node = std::make_unique<NumberLiteral>(42.0);

// 2. 求值时访问节点：裸指针（不拥有）
TacoValue evaluate(const Expr* expr) {  // 不转移所有权
    return expr->evaluate(*this);
}

// 3. 环境（作用域）：shared_ptr，闭包和调用者共享
using EnvPtr = std::shared_ptr<Environment>;
EnvPtr env = std::make_shared<Environment>(parent_env);

// 4. 可能失效的引用：weak_ptr
std::weak_ptr<Environment> weak_env = env;
```

---

## *More About 智能指针*：引用计数的底层实现

> 第一次读可以跳过。

`shared_ptr` 的引用计数是怎么实现的？

### 控制块

`shared_ptr` 内部有一个**控制块**（control block），存放在堆上，包含：

- 引用计数（strong count）：有多少个 `shared_ptr` 指向这个对象
- 弱引用计数（weak count）：有多少个 `weak_ptr` 指向这个对象
- 指向对象的指针（或者对象本身，如果用了 `make_shared`）

```
shared_ptr<Token>:
  [ptr to Token] [ptr to control block]
                        │
                        ▼
                 [strong count: 2]
                 [weak count: 1]
                 [ptr to Token (or Token itself)]
```

每个 `shared_ptr` 持有两个指针：一个指向被管理的对象，一个指向控制块。

### 引用计数的原子操作

引用计数的修改必须是**原子操作**（atomic operation），保证多线程安全——多个线程可以同时持有指向同一对象的 `shared_ptr`，拷贝或析构时修改引用计数必须不冲突。

```cpp
// 伪代码：shared_ptr 拷贝构造
shared_ptr(const shared_ptr& other)
    : m_ptr(other.m_ptr)
    , m_control(other.m_control)
{
    if (m_control) {
        m_control->strong_count.fetch_add(1, std::memory_order_relaxed);
        // fetch_add 是原子操作，线程安全
    }
}

// 伪代码：shared_ptr 析构
~shared_ptr() {
    if (m_control) {
        if (m_control->strong_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // 引用计数降到 0，释放对象
            delete m_ptr;

            if (m_control->weak_count.load() == 0) {
                // 弱引用也没了，释放控制块
                delete m_control;
            }
        }
    }
}
```

原子操作比普通整数操作慢，这是 `shared_ptr` 比 `unique_ptr` 慢的根本原因。在性能敏感的场景，应该优先考虑 `unique_ptr`。

### make_shared 的优化

```cpp
// 两次内存分配
std::shared_ptr<Token> p(new Token("VAR", "var"));
// 1. new Token：分配 Token 的内存
// 2. shared_ptr 构造函数：分配控制块的内存

// 一次内存分配
auto p = std::make_shared<Token>("VAR", "var");
// make_shared 把 Token 和控制块分配在同一块连续内存里
```

`make_shared` 的一次分配不只是少了一次 `malloc` 调用，还有缓存友好性的好处——Token 和控制块在内存里相邻，访问控制块时 Token 很可能也在 CPU 缓存里。

缺点：如果有 `weak_ptr` 指向这个对象，即使 `shared_ptr` 引用计数降到 0（对象应该释放），也要等 `weak_ptr` 引用计数也降到 0，整块内存才能释放（因为 Token 和控制块在一起）。普通 `shared_ptr`（不用 `make_shared`）在强引用计数为 0 时就能释放 Token 的内存，控制块等弱引用也消失后再释放。

---

## 小结

**`unique_ptr`** 表示唯一所有权：不能拷贝，只能移动。销毁时自动释放内存。这是最常用的智能指针，几乎没有额外开销。Taco AST 节点用 `unique_ptr` 管理。

**`shared_ptr`** 表示共享所有权：引用计数管理生命周期，最后一个 `shared_ptr` 析构时释放内存。有引用计数的时间和空间开销。Taco 的闭包环境用 `shared_ptr` 管理。

**`weak_ptr`** 是观察者，不增加引用计数，必须通过 `lock()` 转成 `shared_ptr` 才能访问对象。主要用途是打破 `shared_ptr` 的循环引用。

**选择原则**：优先 `unique_ptr`，需要共享时用 `shared_ptr`，需要打破循环时用 `weak_ptr`。

---

下一章讲移动语义——`std::move` 是什么，右值引用是什么，以及为什么移动比拷贝更高效。
# 第十八章：移动语义

---

移动语义（move semantics）是 C++11 最重要的特性之一。它解决了一个长期存在的问题：当对象被"转移"时（从函数返回、放入容器、传给另一个对象），C++ 之前总是做拷贝，即使那个对象马上就要被销毁。移动语义让"转移"变得高效——不是拷贝数据，而是直接把资源的所有权转过去。

---

## 18.1 左值与右值

理解移动语义，首先要理解左值（lvalue）和右值（rvalue）。

**左值**（lvalue）：有名字、有持久地址的值，可以出现在赋值号左边：

```cpp
int x = 10;   // x 是左值
x = 20;       // 可以赋值给 x

std::string name = "Miguel";  // name 是左值
name = "Dante";               // 可以赋值
```

**右值**（rvalue）：临时的、没有名字的值，不能出现在赋值号左边：

```cpp
10 = x;       // 错误！10 是右值，不能赋值给它
"Miguel" = name;  // 错误！字符串字面量是右值

int x = 10 + 20;  // 10 + 20 是右值（计算结果是临时的）
std::string s = std::string("hello") + " world";
// std::string("hello") 是右值（临时对象）
// std::string("hello") + " world" 也是右值
```

一个简单的判断方法：能不能取地址——能取地址的是左值（`&x` 合法），不能的是右值（`&10` 非法）。

---

### 右值引用

C++11 引入了**右值引用**（rvalue reference），用 `&&` 表示：

```cpp
int x = 10;
int& lref = x;   // 左值引用：只能绑定到左值
int&& rref = 10; // 右值引用：只能绑定到右值
int&& rref2 = x + 5;  // x + 5 是右值，可以绑定

// int&& rref3 = x;  // 错误！x 是左值，不能绑定到右值引用
```

右值引用的存在，让编译器能区分"临时对象"和"有名字的对象"，从而在处理临时对象时选择"移动"而不是"拷贝"。

---

## 18.2 移动构造函数与移动赋值运算符

来看一个具体的例子：一个管理动态数组的类（简化版的 `vector`）：

```cpp
class DynArray {
public:
    DynArray(int size)
        : m_data(new int[size])
        , m_size(size)
    {}

    ~DynArray() {
        delete[] m_data;
    }

    // 拷贝构造：深拷贝，分配新内存
    DynArray(const DynArray& other)
        : m_data(new int[other.m_size])
        , m_size(other.m_size)
    {
        std::copy(other.m_data, other.m_data + m_size, m_data);
        std::cout << "Copy constructed (" << m_size << " ints)\n";
    }

    // 移动构造：偷走资源，不分配新内存
    DynArray(DynArray&& other) noexcept
        : m_data(other.m_data)  // 直接拿走指针
        , m_size(other.m_size)
    {
        other.m_data = nullptr;  // 让 other 放弃对内存的所有权
        other.m_size = 0;
        std::cout << "Move constructed (" << m_size << " ints)\n";
    }

    // 移动赋值运算符
    DynArray& operator=(DynArray&& other) noexcept {
        if (this != &other) {
            delete[] m_data;        // 释放自己的旧内存
            m_data = other.m_data;  // 拿走 other 的内存
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

private:
    int* m_data;
    int  m_size;
};
```

移动构造函数做的事情：
1. 把 `other` 的指针直接"偷"过来，不分配新内存
2. 把 `other` 的指针置为 `nullptr`，防止 `other` 析构时 double free

拷贝构造：分配新内存，复制所有数据，O(n) 时间。
移动构造：只复制指针，O(1) 时间。对于大数组，差距可以非常显著。

---

### noexcept 关键字

移动构造函数通常标记为 `noexcept`——声明这个函数不会抛出异常：

```cpp
DynArray(DynArray&& other) noexcept { ... }
```

这不只是一个文档说明，`noexcept` 让标准库容器（如 `std::vector`）在扩容时能安全地使用移动语义。如果移动构造函数没有 `noexcept`，`vector` 扩容时会用拷贝而不是移动，以保证异常安全。

移动操作通常不应该失败（只是在内存里移动指针），所以标记 `noexcept` 是正确的。

---

## 18.3 std::move 的本质

`std::move` 并不真的"移动"任何东西——它只是把一个左值强制转换成右值引用，告诉编译器"我允许对这个对象使用移动语义"：

```cpp
std::string a = "hello";
std::string b = std::move(a);  // 把 a 转成右值引用，触发移动构造
// 现在 b 是 "hello"，a 是空字符串（或某种有效的空状态）
```

`std::move` 的实现非常简单：

```cpp
// std::move 的简化版实现
template<typename T>
typename std::remove_reference<T>::type&& move(T&& t) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(t);
}
```

本质就是一个 `static_cast` 到右值引用类型。

**重要**：`std::move` 之后，原来的对象处于"有效但未指定的状态"——可以安全析构，但不能假设它有任何特定的值。

```cpp
std::string a = "hello";
std::string b = std::move(a);

// a 可以安全析构（不会 double free）
// 但不要再使用 a 的值！
std::cout << a;  // 可能输出空字符串，也可能是其他值
a = "world";     // 重新赋值后可以正常使用
```

---

### 什么时候用 std::move

**在函数参数传递时**：

```cpp
// 如果确定不再需要 source，用 move 避免拷贝
void process(std::string source) { ... }

std::string s = "hello";
process(std::move(s));  // 移动，不拷贝
// s 现在是空的，不要再使用
```

**在构造函数的初始化列表里**：

```cpp
class Lexer {
public:
    Lexer(std::string source)
        : m_source(std::move(source))  // 移动，不拷贝
    {}
private:
    std::string m_source;
};
```

**从函数返回时（通常不需要）**：

```cpp
std::string make_greeting(const std::string& name) {
    std::string result = "Hello, " + name + "!";
    return result;  // 不需要 std::move，NRVO 会自动优化
}
```

函数返回局部变量时，编译器会自动应用**命名返回值优化**（NRVO，Named Return Value Optimization），直接在调用者的内存里构造返回值，不需要拷贝或移动。显式写 `return std::move(result)` 反而可能阻止 NRVO。

---

## 18.4 Rule of Five

第九章讲了 Rule of Three：如果需要自定义析构函数、拷贝构造函数、拷贝赋值运算符中的任何一个，通常需要自定义全部三个。

C++11 引入移动语义后，这个规则扩展成了 **Rule of Five**：

> 如果需要自定义以下五个中的任何一个，通常需要自定义全部五个：
> 1. 析构函数
> 2. 拷贝构造函数
> 3. 拷贝赋值运算符
> 4. 移动构造函数
> 5. 移动赋值运算符

```cpp
class Resource {
public:
    Resource(int size) : m_data(new int[size]), m_size(size) {}

    // 1. 析构函数
    ~Resource() { delete[] m_data; }

    // 2. 拷贝构造函数
    Resource(const Resource& other)
        : m_data(new int[other.m_size])
        , m_size(other.m_size)
    {
        std::copy(other.m_data, other.m_data + m_size, m_data);
    }

    // 3. 拷贝赋值运算符
    Resource& operator=(const Resource& other) {
        if (this != &other) {
            delete[] m_data;
            m_size = other.m_size;
            m_data = new int[m_size];
            std::copy(other.m_data, other.m_data + m_size, m_data);
        }
        return *this;
    }

    // 4. 移动构造函数
    Resource(Resource&& other) noexcept
        : m_data(other.m_data)
        , m_size(other.m_size)
    {
        other.m_data = nullptr;
        other.m_size = 0;
    }

    // 5. 移动赋值运算符
    Resource& operator=(Resource&& other) noexcept {
        if (this != &other) {
            delete[] m_data;
            m_data = other.m_data;
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

private:
    int* m_data;
    int  m_size;
};
```

**现代 C++ 的建议**：如果能用标准库类型（`std::vector`、`std::string`、`std::unique_ptr`）管理资源，就不需要手动实现这五个函数。标准库类型已经正确实现了所有五个。

在 Taco 项目里，`Lexer`、`Parser`、`Evaluator` 的成员都是标准库类型或智能指针，不需要手动实现任何一个。

---

### 移动语义在 Taco 里的应用

移动语义在 Taco 里随处可见，但大多数时候是隐式发生的：

```cpp
// Parser 构造：移动 tokens 进去，避免拷贝整个 vector
Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))  // 移动
    , m_pos(0)
{}

// parse() 返回时：移动 unique_ptr，不拷贝
std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    // ...
    return program;  // NRVO 或隐式移动
}

// 调用者：
auto program = parser.parse();  // program 拥有 Program，没有拷贝

// Evaluator 里移动 TacoValue
TacoValue Evaluator::evaluate(const Expr* expr) {
    return expr->evaluate(*this);  // 返回时移动，不拷贝
}
```

`std::string` 的移动操作只是交换内部指针，O(1)。`std::vector` 的移动操作只是交换内部指针和大小，O(1)。`std::unique_ptr` 的移动操作只是转移指针，O(1)。

这就是现代 C++ 的性能优势：大量的"拷贝"在底层都变成了移动，代码简洁的同时性能不受影响。

---

## *More About 移动语义*：完美转发与 std::forward

> 第一次读可以跳过。

### 转发问题

考虑这样一个包装函数：

```cpp
template<typename T>
void wrapper(T arg) {
    // 想把 arg 原封不动地传给 process
    process(arg);
}
```

问题来了：如果调用者传的是右值：

```cpp
wrapper(std::string("hello"));
```

`wrapper` 的参数 `arg` 是一个有名字的局部变量（左值），把它传给 `process` 会触发拷贝，而不是移动——即使原来的调用者传的是右值。

### 完美转发

**完美转发**（perfect forwarding）解决这个问题：保持参数的值类别（左值/右值）原封不动地传递。

```cpp
template<typename T>
void wrapper(T&& arg) {  // 这里的 && 是"转发引用"，不是右值引用
    process(std::forward<T>(arg));  // 完美转发
}
```

`std::forward<T>(arg)` 的作用：
- 如果 `T` 是左值引用类型（调用者传了左值），返回左值引用
- 如果 `T` 是普通类型（调用者传了右值），返回右值引用

这样 `process` 收到的参数和调用者传给 `wrapper` 的参数有相同的值类别。

### 在 Taco 里的应用

完美转发主要用于模板代码和工厂函数。`std::make_unique` 内部就用了完美转发：

```cpp
// make_unique 的简化实现
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 调用时，参数原封不动地传给 T 的构造函数
auto p = std::make_unique<NumberLiteral>(42.0);
// 42.0 是右值，通过 forward 传给 NumberLiteral 的构造函数时仍然是右值
```

完美转发和可变参数模板（`Args&&...`）是模板编程里的标准组合，第二十八章讲模板时会再深入。

---

## 小结

**左值**是有名字、有地址的值；**右值**是临时的、没有名字的值。

**右值引用**（`T&&`）让函数可以区分临时对象和普通对象，从而对临时对象使用移动而不是拷贝。

**移动构造函数和移动赋值运算符**：从 `other` "偷"资源（交换指针），而不是深拷贝。移动是 O(1) 的，拷贝是 O(n) 的。移动后 `other` 处于有效但未指定的状态。

**`std::move`**：把左值强制转成右值引用，触发移动语义。`std::move` 本身不移动任何东西，只是一个 `static_cast`。

**Rule of Five**：如果需要自定义析构函数，通常也需要自定义拷贝构造、拷贝赋值、移动构造、移动赋值。用标准库类型管理资源可以避免手动实现这五个。

---

下一章讲常用标准库补充：`<chrono>`、`<optional>`、`<variant>`、`<random>`，这些都是 v3 会用到的工具。
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
# 第二十章：项目 v3——函数、闭包与环境

---

第四部分学完了所有权、智能指针、移动语义、常用标准库工具。现在用这些知识做 v3 最核心的事：**实现用户定义函数和闭包**。

v3 结束时，Taco 能运行这样的代码：

```taco
// 基础函数
func greet(name) {
    print("Hola, " + name + "!");
}
greet("Miguel");

// 递归
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
print(fib(10));  // 55

// 闭包：函数捕获外部变量
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}

var counter = makeCounter();
print(counter());  // 1
print(counter());  // 2
print(counter());  // 3

// 命名参数
func greet2(name, greeting: "Hola") {
    print(greeting + ", " + name + "!");
}
greet2("Dante");              // Hola, Dante!
greet2("Miguel", greeting: "Hey");  // Hey, Miguel!

// 多返回值
func minmax(arr) {
    return arr[0], arr[arr.len() - 1];
}
var lo, hi = minmax([3, 1, 4, 1, 5, 9]);
print(lo);  // 3
print(hi);  // 9
```

---

## 20.1 函数是什么：从解释器的角度

在实现函数之前，先想清楚函数在解释器里是什么东西。

函数是一个**值**——就像数字、字符串一样，函数可以被赋给变量、作为参数传递、从函数里返回。这就是"函数是一等公民"（first-class function）的含义。

在 Taco 里，函数值包含：
- **参数列表**：函数接受哪些参数，各自的默认值是什么
- **函数体**：一组语句，也就是 AST 节点列表
- **闭包环境**：函数定义时所在的作用域（用于捕获外部变量）

当调用函数时，解释器：
1. 创建一个新的作用域，以闭包环境为父作用域
2. 把实参绑定到形参名上，存入新作用域
3. 在这个新作用域里依次执行函数体的每条语句
4. 如果遇到 `return`，捕获返回值，退出

---

## 20.2 环境（Environment）：作用域的实现

作用域是函数实现的核心。v2 用一个简单的 `vector<unordered_map>` 实现作用域链，但这个设计有一个致命问题：**不支持闭包**。

来看一个例子：

```taco
func makeCounter() {
    var count = 0;     // count 在 makeCounter 的作用域里

    return func() {
        count = count + 1;  // 内层函数访问外层的 count
        return count;
    };
}

var counter = makeCounter();
// makeCounter 已经返回，它的局部作用域应该销毁了
// 但 counter 还要访问 count！
counter();  // 还能访问到 count，因为闭包捕获了外层作用域
```

v2 的 `vector<map>` 实现里，当 `makeCounter` 返回时，它的作用域从 `vector` 里弹出，`count` 就消失了。之后调用 `counter()` 就找不到 `count`。

解决方案：**把每个作用域做成堆上的对象，用 `shared_ptr` 管理，闭包持有对外层作用域的共享引用**。这样即使外层函数返回，只要还有闭包引用那个作用域，它就不会被销毁。

### 新的 Environment 设计

```cpp
// environment.h
#pragma once
#include "value.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>

class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;

    // 创建全局环境（没有父环境）
    Environment() : m_parent(nullptr) {}

    // 创建子环境（有父环境）
    explicit Environment(Ptr parent)
        : m_parent(std::move(parent))
    {}

    // 在当前作用域定义新变量
    void define(const std::string& name, TacoValue value) {
        m_vars[name] = std::move(value);
    }

    // 查找变量：先在当前作用域找，找不到就往上找
    TacoValue get(const std::string& name) const {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) {
            return it->second;
        }
        if (m_parent) {
            return m_parent->get(name);  // 递归向上查找
        }
        throw std::runtime_error("🌮 '" + name + "' is not defined.");
    }

    // 修改变量：在定义它的那个作用域里修改
    void set(const std::string& name, TacoValue value) {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) {
            it->second = std::move(value);
            return;
        }
        if (m_parent) {
            m_parent->set(name, std::move(value));  // 递归向上修改
            return;
        }
        throw std::runtime_error("🌮 '" + name + "' is not defined.");
    }

    // 获取父环境
    Ptr parent() const { return m_parent; }

private:
    Ptr m_parent;  // shared_ptr：可以和闭包共享
    std::unordered_map<std::string, TacoValue> m_vars;
};
```

**为什么 `m_parent` 用 `shared_ptr`？**

当外层函数返回时，它的 `Environment` 对象本来应该销毁。但如果内层函数（闭包）还持有一个指向外层 `Environment` 的 `shared_ptr`，那个 `Environment` 就不会被销毁——它的引用计数还不是 0。

这正是我们想要的：只要闭包还存在，它捕获的外层作用域就存在。当闭包也被销毁时，引用计数降到 0，外层作用域才被释放。

---

## 20.3 函数值的表示

函数是一个值，所以它要能存在 `TacoValue` 里。首先定义函数的数据结构：

```cpp
// function.h
#pragma once
#include "value.h"
#include "ast.h"
#include "environment.h"
#include <string>
#include <vector>
#include <functional>

// 参数：名字 + 可选的默认值
struct Param {
    std::string              name;
    std::unique_ptr<Expr>    default_value;  // nullptr 表示没有默认值
    bool                     is_named;       // 是否是命名参数
};

// 用户定义的函数
struct TacoFunction {
    std::string              name;        // 函数名（匿名函数为空）
    std::vector<Param>       params;      // 参数列表
    std::vector<ExprPtr>     body;        // 函数体（语句列表）
    Environment::Ptr         closure;     // 闭包环境（定义时的作用域）

    // 使用 shared_ptr 包装，让 TacoValue 可以存放函数
    using Ptr = std::shared_ptr<TacoFunction>;
};

// 内置函数：用 C++ lambda 实现
using NativeFunction = std::function<TacoValue(std::vector<TacoValue>)>;

struct TacoNative {
    std::string    name;
    NativeFunction fn;
    using Ptr = std::shared_ptr<TacoNative>;
};
```

现在更新 `TacoValue`，加入函数类型：

```cpp
// value.h（更新版）
#pragma once
#include <variant>
#include <string>
#include <memory>

// 前向声明
struct TacoFunction;
struct TacoNative;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,  // 用户定义函数
    std::shared_ptr<TacoNative>     // 内置函数
>;

std::string value_to_string(const TacoValue& val);
bool        is_truthy(const TacoValue& val);
bool        values_equal(const TacoValue& a, const TacoValue& b);
```

用 `shared_ptr<TacoFunction>` 而不是直接存 `TacoFunction`，是因为：
1. `TacoValue` 是 `variant`，如果直接存 `TacoFunction`，`TacoFunction` 里有 `vector<ExprPtr>`（独占指针的 vector），`variant` 的拷贝就会失败
2. `shared_ptr` 让函数对象可以被多个变量引用（把函数赋给多个变量），不需要深拷贝

---

## 20.4 更新 AST：函数定义和调用

新增两种 AST 节点：函数定义（`FuncDecl`）和函数调用（完整版的 `CallExpr`）。

```cpp
// ast.h 新增部分

// 函数声明：func name(params) { body }
struct FuncDecl : Expr {
    std::string          name;
    std::vector<Param>   params;
    std::vector<ExprPtr> body;
    bool                 is_anonymous;  // 匿名函数

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "func " + name + "(...)";
    }
};

// 函数调用：callee(args) 或 callee(named: value)
struct CallExpr : Expr {
    ExprPtr              callee;       // 被调用的表达式（可以是变量、方法等）
    std::vector<ExprPtr> args;         // 位置参数
    std::vector<std::pair<std::string, ExprPtr>> named_args;  // 命名参数

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "(call)"; }
};

// 多返回值赋值：var a, b = func()
struct MultiAssign : Expr {
    std::vector<std::string> names;
    ExprPtr                  value;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "(multi-assign)"; }
};
```

---

## 20.5 实现 RAII 管理函数调用栈

函数调用时需要创建新作用域，函数返回时销毁这个作用域。这正是 RAII 的使用场景：

```cpp
// evaluator.h（新增）

class Evaluator {
public:
    // ...

    // 当前环境
    Environment::Ptr current_env() const { return m_env; }

    // 进入新作用域
    void push_env(Environment::Ptr env) {
        m_env = std::move(env);
    }

    // 退出当前作用域，恢复父作用域
    void pop_env() {
        if (m_env->parent()) {
            m_env = m_env->parent();
        }
    }

private:
    Environment::Ptr m_env;  // 当前环境
};

// RAII 作用域守卫：构造时进入新作用域，析构时退出
class ScopeGuard {
public:
    ScopeGuard(Evaluator& eval, Environment::Ptr new_env)
        : m_eval(eval)
        , m_old_env(eval.current_env())
    {
        eval.push_env(std::move(new_env));
    }

    ~ScopeGuard() {
        m_eval.push_env(m_old_env);  // 恢复旧环境
    }

    // 禁止拷贝和移动（守卫不应该被复制）
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

private:
    Evaluator&       m_eval;
    Environment::Ptr m_old_env;
};
```

`ScopeGuard` 是一个经典的 RAII 守卫：

```cpp
// 使用示例
TacoValue call_function(Evaluator& eval, TacoFunction::Ptr fn,
                         std::vector<TacoValue> args) {
    // 创建函数的执行环境，父环境是闭包环境
    auto fn_env = std::make_shared<Environment>(fn->closure);

    // 绑定参数
    for (int i = 0; i < static_cast<int>(fn->params.size()); i++) {
        fn_env->define(fn->params[i].name, args[i]);
    }

    // RAII 守卫：进入函数作用域
    ScopeGuard guard(eval, fn_env);
    // 现在 eval.current_env() 是 fn_env

    TacoValue result = nullptr;
    try {
        for (const auto& stmt : fn->body) {
            result = eval.evaluate(stmt.get());
        }
    } catch (ReturnException& ret) {
        result = ret.value;  // 捕获 return 的值
    }

    // guard 析构，自动恢复之前的环境
    return result;
}
```

不管函数是正常返回还是通过 `return` 跳出，`ScopeGuard` 的析构函数都会恢复之前的环境。这就是 RAII 处理异常的价值。

---

## 20.6 实现闭包

闭包的关键：**函数在定义时捕获当前环境，存在函数对象里**。

```cpp
// ast.cpp

TacoValue FuncDecl::evaluate(Evaluator& eval) const {
    // 创建函数对象，捕获当前环境
    auto fn = std::make_shared<TacoFunction>();
    fn->name      = name;
    fn->closure   = eval.current_env();  // 捕获当前环境！
    fn->is_anonymous = is_anonymous;

    // 拷贝参数列表（params 里有 default_value，需要深拷贝 AST 节点）
    // 为简单起见，这里存 shared_ptr 而不是 unique_ptr
    for (const auto& p : params) {
        Param copy;
        copy.name     = p.name;
        copy.is_named = p.is_named;
        // default_value 的拷贝需要 clone 机制，暂时简化处理
        fn->params.push_back(std::move(copy));
    }

    // 函数体：存对语句的引用（不拷贝，因为 FuncDecl 的生命周期足够长）
    // 更严格的实现应该把 body 拷贝进去，这里简化
    for (const auto& stmt : body) {
        fn->body.push_back(clone_expr(stmt.get()));  // 需要 clone 函数
    }

    if (!name.empty()) {
        // 具名函数：把自己存到当前作用域里
        eval.current_env()->define(name, fn);
    }

    return fn;  // 返回函数值
}
```

闭包的神奇之处：`fn->closure = eval.current_env()` 这一行。函数对象持有对当前环境的 `shared_ptr`，当函数被存到变量里并传出去时，那个环境因为有引用而不会被销毁。

**来看闭包的具体执行过程**：

```taco
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}
var counter = makeCounter();
```

1. 调用 `makeCounter()`：
   - 创建 `makeCounter` 的执行环境 `env_mk`，父环境是全局环境
   - 在 `env_mk` 里定义 `count = 0`

2. 执行 `return func() { ... }`：
   - 创建匿名函数对象 `fn_inner`
   - `fn_inner.closure = env_mk`（捕获 `makeCounter` 的环境）
   - 返回 `fn_inner`

3. `makeCounter()` 返回：
   - `makeCounter` 的调用栈帧弹出
   - 但 `env_mk` 没有销毁！因为 `fn_inner.closure` 持有它的 `shared_ptr`

4. `var counter = makeCounter()`：
   - `counter` 现在持有 `fn_inner`（通过 `shared_ptr`）
   - `fn_inner` 持有 `env_mk`（通过 `shared_ptr`）

5. 调用 `counter()`：
   - 创建新环境 `env_call`，父环境是 `fn_inner.closure`（即 `env_mk`）
   - 执行 `count = count + 1`：在 `env_call` 里找 `count`，找不到，向上找，在 `env_mk` 里找到！
   - 修改 `env_mk` 里的 `count`，从 0 变成 1
   - 返回 1

6. 再次调用 `counter()`：
   - `env_mk` 里的 `count` 已经是 1
   - 执行后变成 2，返回 2

这就是闭包的工作原理：内层函数通过 `shared_ptr` 持有外层的环境，每次调用都能访问并修改同一份 `count`。

---

## 20.7 实现函数调用

```cpp
// ast.cpp

TacoValue CallExpr::evaluate(Evaluator& eval) const {
    // 先求值被调用的表达式
    TacoValue callee_val = eval.evaluate(callee.get());

    // 求值所有位置参数
    std::vector<TacoValue> arg_vals;
    for (const auto& arg : args) {
        arg_vals.push_back(eval.evaluate(arg.get()));
    }

    // 求值命名参数
    std::vector<std::pair<std::string, TacoValue>> named_vals;
    for (const auto& [name, expr] : named_args) {
        named_vals.push_back({name, eval.evaluate(expr.get())});
    }

    // 分派：内置函数还是用户定义函数
    if (std::holds_alternative<std::shared_ptr<TacoNative>>(callee_val)) {
        auto& native = std::get<std::shared_ptr<TacoNative>>(callee_val);
        return native->fn(arg_vals);
    }

    if (std::holds_alternative<std::shared_ptr<TacoFunction>>(callee_val)) {
        auto& fn = std::get<std::shared_ptr<TacoFunction>>(callee_val);
        return call_user_function(eval, fn, arg_vals, named_vals);
    }

    throw std::runtime_error("🌮 '" + value_to_string(callee_val) +
                             "' is not a function.");
}

static TacoValue call_user_function(
    Evaluator& eval,
    const std::shared_ptr<TacoFunction>& fn,
    const std::vector<TacoValue>& args,
    const std::vector<std::pair<std::string, TacoValue>>& named_args)
{
    // 创建函数的执行环境，父环境是闭包环境
    auto fn_env = std::make_shared<Environment>(fn->closure);

    // 处理参数绑定
    int pos_idx = 0;  // 当前处理到第几个位置参数

    for (const auto& param : fn->params) {
        TacoValue val = nullptr;

        if (param.is_named) {
            // 命名参数：先从 named_args 里找
            bool found = false;
            for (const auto& [name, v] : named_args) {
                if (name == param.name) {
                    val = v;
                    found = true;
                    break;
                }
            }
            // 没找到就用默认值
            if (!found) {
                if (param.default_value) {
                    val = eval.evaluate(param.default_value.get());
                } else {
                    throw std::runtime_error(
                        "🌮 Missing argument for parameter '" + param.name + "'."
                    );
                }
            }
        } else {
            // 位置参数
            if (pos_idx < static_cast<int>(args.size())) {
                val = args[pos_idx++];
            } else if (param.default_value) {
                val = eval.evaluate(param.default_value.get());
            } else {
                throw std::runtime_error(
                    "🌮 Missing argument for parameter '" + param.name + "'."
                );
            }
        }

        fn_env->define(param.name, std::move(val));
    }

    // RAII 守卫：进入函数作用域
    ScopeGuard guard(eval, fn_env);

    // 执行函数体
    TacoValue result = nullptr;
    try {
        for (const auto& stmt : fn->body) {
            result = eval.evaluate(stmt.get());
        }
    } catch (ReturnException& ret) {
        return ret.value;
    }

    return result;
}
```

---

## 20.8 多返回值的实现

Taco 支持多返回值：

```taco
func minmax(arr) {
    return arr[0], arr[arr.len() - 1];
}
var lo, hi = minmax([3, 1, 4, 1, 5]);
```

实现思路：多返回值在内部用一个特殊的"元组"值表示（用 `vector<TacoValue>` 包装），解构赋值时拆开：

更新 `TacoValue`，加入数组类型（同时也支持 Taco 的 array 类型）：

```cpp
using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>,
    std::shared_ptr<std::vector<TacoValue>>  // array 和多返回值都用这个
>;
```

`ReturnStmt` 的求值：

```cpp
TacoValue ReturnStmt::evaluate(Evaluator& eval) const {
    if (!value) {
        throw ReturnException{nullptr};
    }

    // 检查是否是多返回值（逗号分隔的表达式列表）
    if (auto* multi = dynamic_cast<const MultiReturn*>(value.get())) {
        auto arr = std::make_shared<std::vector<TacoValue>>();
        for (const auto& expr : multi->values) {
            arr->push_back(eval.evaluate(expr.get()));
        }
        throw ReturnException{arr};
    }

    throw ReturnException{eval.evaluate(value.get())};
}
```

`MultiAssign` 的求值：

```cpp
TacoValue MultiAssign::evaluate(Evaluator& eval) const {
    TacoValue result = eval.evaluate(value.get());

    // 期望结果是一个 array（多返回值）
    if (!std::holds_alternative<std::shared_ptr<std::vector<TacoValue>>>(result)) {
        throw std::runtime_error(
            "🌮 Expected multiple return values, got a single value."
        );
    }

    auto& arr = *std::get<std::shared_ptr<std::vector<TacoValue>>>(result);

    if (arr.size() != names.size()) {
        throw std::runtime_error(
            "🌮 Expected " + std::to_string(names.size()) +
            " values, got " + std::to_string(arr.size()) + "."
        );
    }

    for (int i = 0; i < static_cast<int>(names.size()); i++) {
        eval.current_env()->define(names[i], arr[i]);
    }

    return result;
}
```

---

## 20.9 内置函数的实现

内置函数用 C++ lambda 实现，存为 `TacoNative`：

```cpp
// builtins.cpp
#include "builtins.h"
#include "value.h"
#include <iostream>
#include <stdexcept>

void register_builtins(Environment::Ptr env) {
    // print
    env->define("print", std::make_shared<TacoNative>(TacoNative{
        "print",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 print() takes 1 argument.");
            }
            std::cout << value_to_string(args[0]) << "\n";
            return nullptr;
        }
    }));

    // type：返回值的类型名
    env->define("type", std::make_shared<TacoNative>(TacoNative{
        "type",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 type() takes 1 argument.");
            }
            return std::visit([](auto&& v) -> std::string {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>)       return "number";
                if constexpr (std::is_same_v<T, std::string>)  return "string";
                if constexpr (std::is_same_v<T, bool>)         return "bool";
                if constexpr (std::is_same_v<T, std::nullptr_t>) return "nil";
                return "function";
            }, args[0]);
        }
    }));

    // input：读取用户输入
    env->define("input", std::make_shared<TacoNative>(TacoNative{
        "input",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (!args.empty()) {
                std::cout << value_to_string(args[0]);
            }
            std::string line;
            std::getline(std::cin, line);
            return line;
        }
    }));

    // number：把值转成数字
    env->define("number", std::make_shared<TacoNative>(TacoNative{
        "number",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 number() takes 1 argument.");
            }
            return std::visit([](auto&& v) -> TacoValue {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>)      return v;
                if constexpr (std::is_same_v<T, std::string>) {
                    try { return std::stod(v); }
                    catch (...) {
                        throw std::runtime_error(
                            "🌮 Cannot convert '" + v + "' to number."
                        );
                    }
                }
                if constexpr (std::is_same_v<T, bool>) return v ? 1.0 : 0.0;
                throw std::runtime_error("🌮 Cannot convert nil to number.");
                return nullptr;
            }, args[0]);
        }
    }));

    // string：把值转成字符串
    env->define("string", std::make_shared<TacoNative>(TacoNative{
        "string",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 string() takes 1 argument.");
            }
            return value_to_string(args[0]);
        }
    }));
}
```

---

## 20.10 测试：斐波那契数列与闭包

创建 `test_v3.taco`：

```taco
// 斐波那契（递归）
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}

for i in range(0, 11) {
    print(fib(i));
}
// 0 1 1 2 3 5 8 13 21 34 55

// 闭包：计数器
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}

var c1 = makeCounter();
var c2 = makeCounter();

print(c1());  // 1
print(c1());  // 2
print(c2());  // 1（c2 有自己的 count，和 c1 独立）
print(c1());  // 3

// 闭包：累加器
func makeAdder(n) {
    return func(x) {
        return x + n;
    };
}

var add5 = makeAdder(5);
var add10 = makeAdder(10);

print(add5(3));   // 8
print(add10(3));  // 13

// 命名参数
func greet(name, greeting: "Hola") {
    print(greeting + ", " + name + "!");
}

greet("Miguel");
greet("Dante", greeting: "Hey");

// 高阶函数：把函数作为参数传递
func apply(f, x) {
    return f(x);
}

func double(x) { return x * 2; }
func square(x) { return x * x; }

print(apply(double, 5));  // 10
print(apply(square, 5));  // 25
```

### 运行结果

```
0
1
1
2
3
5
8
13
21
34
55
1
2
1
3
8
13
Hola, Miguel!
Hey, Dante!
10
25
```

---

## 20.11 这个版本的局限性

**还没有 array 和 map 的内置方法**

`arr.filter`、`arr.map`、`str.getLines()` 这些还不支持，v4 会系统地实现标准库。

**参数列表的 AST 节点没有完全实现**

`Param` 里的 `default_value` 用了 `unique_ptr<Expr>`，但在 `FuncDecl::evaluate` 里克隆 AST 节点需要实现一个 `clone` 函数，这里做了简化处理。完整实现会在 v4 里补全。

**没有尾调用优化**

Taco 里的递归函数，每次调用都会在 C++ 的调用栈上创建一个新帧。调用层次太深（比如 `fib(100)`）会导致 C++ 的栈溢出。真实的解释器通常会实现尾调用优化（TCO）或者用显式栈来避免这个问题。

**没有 OOP**

`class`、`struct`、`enum` 还没实现，这些会在综合收尾章节里加入。

---

## 小结

v3 是 Taco 解释器最核心的一步进化。

**Environment（作用域链）** 用 `shared_ptr` 管理，支持闭包捕获外层变量。每个作用域是堆上的一个对象，内层函数持有对外层作用域的共享引用，外层函数返回后作用域仍然存活。

**函数是值**：函数被表示为 `TacoFunction`（用 `shared_ptr` 存在 `TacoValue` 里），包含参数列表、函数体 AST、闭包环境三个部分。

**RAII 管理函数调用栈**：`ScopeGuard` 在构造时进入新作用域，在析构时（不管正常返回还是异常）恢复旧作用域。这让函数调用的资源管理完全自动化。

**内置函数** 用 `std::function` 包装 C++ lambda，统一注册到全局环境，和用户定义函数的调用方式一致。

**闭包** 在函数定义时捕获当前环境（`fn->closure = eval.current_env()`），调用时以闭包环境为父环境创建新作用域，实现对外层变量的访问和修改。

---

第四部分到这里结束。第五部分进入 STL，学完之后 v4 会用 STL 容器和算法实现 Taco 的 array、map，以及完整的内置标准库和 pipeline 风格。
# 第二十章：项目 v3——函数、闭包与环境

---

第四部分学完了所有权、智能指针、移动语义、常用标准库工具。现在用这些知识做 v3 最核心的事：**实现用户定义函数和闭包**。

v3 结束时，Taco 能运行这样的代码：

```taco
// 基础函数
func greet(name) {
    print("Hola, " + name + "!");
}
greet("Miguel");

// 递归
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
print(fib(10));  // 55

// 闭包：函数捕获外部变量
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}

var counter = makeCounter();
print(counter());  // 1
print(counter());  // 2
print(counter());  // 3

// 命名参数
func greet2(name, greeting: "Hola") {
    print(greeting + ", " + name + "!");
}
greet2("Dante");              // Hola, Dante!
greet2("Miguel", greeting: "Hey");  // Hey, Miguel!

// 多返回值
func minmax(arr) {
    return arr[0], arr[arr.len() - 1];
}
var lo, hi = minmax([3, 1, 4, 1, 5, 9]);
print(lo);  // 3
print(hi);  // 9
```

---

## 20.1 函数是什么：从解释器的角度

在实现函数之前，先想清楚函数在解释器里是什么东西。

函数是一个**值**——就像数字、字符串一样，函数可以被赋给变量、作为参数传递、从函数里返回。这就是"函数是一等公民"（first-class function）的含义。

在 Taco 里，函数值包含：
- **参数列表**：函数接受哪些参数，各自的默认值是什么
- **函数体**：一组语句，也就是 AST 节点列表
- **闭包环境**：函数定义时所在的作用域（用于捕获外部变量）

当调用函数时，解释器：
1. 创建一个新的作用域，以闭包环境为父作用域
2. 把实参绑定到形参名上，存入新作用域
3. 在这个新作用域里依次执行函数体的每条语句
4. 如果遇到 `return`，捕获返回值，退出

---

## 20.2 环境（Environment）：作用域的实现

作用域是函数实现的核心。v2 用一个简单的 `vector<unordered_map>` 实现作用域链，但这个设计有一个致命问题：**不支持闭包**。

来看一个例子：

```taco
func makeCounter() {
    var count = 0;     // count 在 makeCounter 的作用域里

    return func() {
        count = count + 1;  // 内层函数访问外层的 count
        return count;
    };
}

var counter = makeCounter();
// makeCounter 已经返回，它的局部作用域应该销毁了
// 但 counter 还要访问 count！
counter();  // 还能访问到 count，因为闭包捕获了外层作用域
```

v2 的 `vector<map>` 实现里，当 `makeCounter` 返回时，它的作用域从 `vector` 里弹出，`count` 就消失了。之后调用 `counter()` 就找不到 `count`。

解决方案：**把每个作用域做成堆上的对象，用 `shared_ptr` 管理，闭包持有对外层作用域的共享引用**。这样即使外层函数返回，只要还有闭包引用那个作用域，它就不会被销毁。

### 新的 Environment 设计

```cpp
// environment.h
#pragma once
#include "value.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>

class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;

    // 创建全局环境（没有父环境）
    Environment() : m_parent(nullptr) {}

    // 创建子环境（有父环境）
    explicit Environment(Ptr parent)
        : m_parent(std::move(parent))
    {}

    // 在当前作用域定义新变量
    void define(const std::string& name, TacoValue value) {
        m_vars[name] = std::move(value);
    }

    // 查找变量：先在当前作用域找，找不到就往上找
    TacoValue get(const std::string& name) const {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) {
            return it->second;
        }
        if (m_parent) {
            return m_parent->get(name);  // 递归向上查找
        }
        throw std::runtime_error("🌮 '" + name + "' is not defined.");
    }

    // 修改变量：在定义它的那个作用域里修改
    void set(const std::string& name, TacoValue value) {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) {
            it->second = std::move(value);
            return;
        }
        if (m_parent) {
            m_parent->set(name, std::move(value));  // 递归向上修改
            return;
        }
        throw std::runtime_error("🌮 '" + name + "' is not defined.");
    }

    // 获取父环境
    Ptr parent() const { return m_parent; }

private:
    Ptr m_parent;  // shared_ptr：可以和闭包共享
    std::unordered_map<std::string, TacoValue> m_vars;
};
```

**为什么 `m_parent` 用 `shared_ptr`？**

当外层函数返回时，它的 `Environment` 对象本来应该销毁。但如果内层函数（闭包）还持有一个指向外层 `Environment` 的 `shared_ptr`，那个 `Environment` 就不会被销毁——它的引用计数还不是 0。

这正是我们想要的：只要闭包还存在，它捕获的外层作用域就存在。当闭包也被销毁时，引用计数降到 0，外层作用域才被释放。

---

## 20.3 函数值的表示

函数是一个值，所以它要能存在 `TacoValue` 里。首先定义函数的数据结构：

```cpp
// function.h
#pragma once
#include "value.h"
#include "ast.h"
#include "environment.h"
#include <string>
#include <vector>
#include <functional>

// 参数：名字 + 可选的默认值
struct Param {
    std::string              name;
    std::unique_ptr<Expr>    default_value;  // nullptr 表示没有默认值
    bool                     is_named;       // 是否是命名参数
};

// 用户定义的函数
struct TacoFunction {
    std::string              name;        // 函数名（匿名函数为空）
    std::vector<Param>       params;      // 参数列表
    std::vector<ExprPtr>     body;        // 函数体（语句列表）
    Environment::Ptr         closure;     // 闭包环境（定义时的作用域）

    // 使用 shared_ptr 包装，让 TacoValue 可以存放函数
    using Ptr = std::shared_ptr<TacoFunction>;
};

// 内置函数：用 C++ lambda 实现
using NativeFunction = std::function<TacoValue(std::vector<TacoValue>)>;

struct TacoNative {
    std::string    name;
    NativeFunction fn;
    using Ptr = std::shared_ptr<TacoNative>;
};
```

现在更新 `TacoValue`，加入函数类型：

```cpp
// value.h（更新版）
#pragma once
#include <variant>
#include <string>
#include <memory>

// 前向声明
struct TacoFunction;
struct TacoNative;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,  // 用户定义函数
    std::shared_ptr<TacoNative>     // 内置函数
>;

std::string value_to_string(const TacoValue& val);
bool        is_truthy(const TacoValue& val);
bool        values_equal(const TacoValue& a, const TacoValue& b);
```

用 `shared_ptr<TacoFunction>` 而不是直接存 `TacoFunction`，是因为：
1. `TacoValue` 是 `variant`，如果直接存 `TacoFunction`，`TacoFunction` 里有 `vector<ExprPtr>`（独占指针的 vector），`variant` 的拷贝就会失败
2. `shared_ptr` 让函数对象可以被多个变量引用（把函数赋给多个变量），不需要深拷贝

---

## 20.4 更新 AST：函数定义和调用

新增两种 AST 节点：函数定义（`FuncDecl`）和函数调用（完整版的 `CallExpr`）。

```cpp
// ast.h 新增部分

// 函数声明：func name(params) { body }
struct FuncDecl : Expr {
    std::string          name;
    std::vector<Param>   params;
    std::vector<ExprPtr> body;
    bool                 is_anonymous;  // 匿名函数

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return "func " + name + "(...)";
    }
};

// 函数调用：callee(args) 或 callee(named: value)
struct CallExpr : Expr {
    ExprPtr              callee;       // 被调用的表达式（可以是变量、方法等）
    std::vector<ExprPtr> args;         // 位置参数
    std::vector<std::pair<std::string, ExprPtr>> named_args;  // 命名参数

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "(call)"; }
};

// 多返回值赋值：var a, b = func()
struct MultiAssign : Expr {
    std::vector<std::string> names;
    ExprPtr                  value;

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "(multi-assign)"; }
};
```

---

## 20.5 实现 RAII 管理函数调用栈

函数调用时需要创建新作用域，函数返回时销毁这个作用域。这正是 RAII 的使用场景：

```cpp
// evaluator.h（新增）

class Evaluator {
public:
    // ...

    // 当前环境
    Environment::Ptr current_env() const { return m_env; }

    // 进入新作用域
    void push_env(Environment::Ptr env) {
        m_env = std::move(env);
    }

    // 退出当前作用域，恢复父作用域
    void pop_env() {
        if (m_env->parent()) {
            m_env = m_env->parent();
        }
    }

private:
    Environment::Ptr m_env;  // 当前环境
};

// RAII 作用域守卫：构造时进入新作用域，析构时退出
class ScopeGuard {
public:
    ScopeGuard(Evaluator& eval, Environment::Ptr new_env)
        : m_eval(eval)
        , m_old_env(eval.current_env())
    {
        eval.push_env(std::move(new_env));
    }

    ~ScopeGuard() {
        m_eval.push_env(m_old_env);  // 恢复旧环境
    }

    // 禁止拷贝和移动（守卫不应该被复制）
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

private:
    Evaluator&       m_eval;
    Environment::Ptr m_old_env;
};
```

`ScopeGuard` 是一个经典的 RAII 守卫：

```cpp
// 使用示例
TacoValue call_function(Evaluator& eval, TacoFunction::Ptr fn,
                         std::vector<TacoValue> args) {
    // 创建函数的执行环境，父环境是闭包环境
    auto fn_env = std::make_shared<Environment>(fn->closure);

    // 绑定参数
    for (int i = 0; i < static_cast<int>(fn->params.size()); i++) {
        fn_env->define(fn->params[i].name, args[i]);
    }

    // RAII 守卫：进入函数作用域
    ScopeGuard guard(eval, fn_env);
    // 现在 eval.current_env() 是 fn_env

    TacoValue result = nullptr;
    try {
        for (const auto& stmt : fn->body) {
            result = eval.evaluate(stmt.get());
        }
    } catch (ReturnException& ret) {
        result = ret.value;  // 捕获 return 的值
    }

    // guard 析构，自动恢复之前的环境
    return result;
}
```

不管函数是正常返回还是通过 `return` 跳出，`ScopeGuard` 的析构函数都会恢复之前的环境。这就是 RAII 处理异常的价值。

---

## 20.6 实现闭包

闭包的关键：**函数在定义时捕获当前环境，存在函数对象里**。

```cpp
// ast.cpp

TacoValue FuncDecl::evaluate(Evaluator& eval) const {
    // 创建函数对象，捕获当前环境
    auto fn = std::make_shared<TacoFunction>();
    fn->name      = name;
    fn->closure   = eval.current_env();  // 捕获当前环境！
    fn->is_anonymous = is_anonymous;

    // 拷贝参数列表（params 里有 default_value，需要深拷贝 AST 节点）
    // 为简单起见，这里存 shared_ptr 而不是 unique_ptr
    for (const auto& p : params) {
        Param copy;
        copy.name     = p.name;
        copy.is_named = p.is_named;
        // default_value 的拷贝需要 clone 机制，暂时简化处理
        fn->params.push_back(std::move(copy));
    }

    // 函数体：存对语句的引用（不拷贝，因为 FuncDecl 的生命周期足够长）
    // 更严格的实现应该把 body 拷贝进去，这里简化
    for (const auto& stmt : body) {
        fn->body.push_back(clone_expr(stmt.get()));  // 需要 clone 函数
    }

    if (!name.empty()) {
        // 具名函数：把自己存到当前作用域里
        eval.current_env()->define(name, fn);
    }

    return fn;  // 返回函数值
}
```

闭包的神奇之处：`fn->closure = eval.current_env()` 这一行。函数对象持有对当前环境的 `shared_ptr`，当函数被存到变量里并传出去时，那个环境因为有引用而不会被销毁。

**来看闭包的具体执行过程**：

```taco
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}
var counter = makeCounter();
```

1. 调用 `makeCounter()`：
   - 创建 `makeCounter` 的执行环境 `env_mk`，父环境是全局环境
   - 在 `env_mk` 里定义 `count = 0`

2. 执行 `return func() { ... }`：
   - 创建匿名函数对象 `fn_inner`
   - `fn_inner.closure = env_mk`（捕获 `makeCounter` 的环境）
   - 返回 `fn_inner`

3. `makeCounter()` 返回：
   - `makeCounter` 的调用栈帧弹出
   - 但 `env_mk` 没有销毁！因为 `fn_inner.closure` 持有它的 `shared_ptr`

4. `var counter = makeCounter()`：
   - `counter` 现在持有 `fn_inner`（通过 `shared_ptr`）
   - `fn_inner` 持有 `env_mk`（通过 `shared_ptr`）

5. 调用 `counter()`：
   - 创建新环境 `env_call`，父环境是 `fn_inner.closure`（即 `env_mk`）
   - 执行 `count = count + 1`：在 `env_call` 里找 `count`，找不到，向上找，在 `env_mk` 里找到！
   - 修改 `env_mk` 里的 `count`，从 0 变成 1
   - 返回 1

6. 再次调用 `counter()`：
   - `env_mk` 里的 `count` 已经是 1
   - 执行后变成 2，返回 2

这就是闭包的工作原理：内层函数通过 `shared_ptr` 持有外层的环境，每次调用都能访问并修改同一份 `count`。

---

## 20.7 实现函数调用

```cpp
// ast.cpp

TacoValue CallExpr::evaluate(Evaluator& eval) const {
    // 先求值被调用的表达式
    TacoValue callee_val = eval.evaluate(callee.get());

    // 求值所有位置参数
    std::vector<TacoValue> arg_vals;
    for (const auto& arg : args) {
        arg_vals.push_back(eval.evaluate(arg.get()));
    }

    // 求值命名参数
    std::vector<std::pair<std::string, TacoValue>> named_vals;
    for (const auto& [name, expr] : named_args) {
        named_vals.push_back({name, eval.evaluate(expr.get())});
    }

    // 分派：内置函数还是用户定义函数
    if (std::holds_alternative<std::shared_ptr<TacoNative>>(callee_val)) {
        auto& native = std::get<std::shared_ptr<TacoNative>>(callee_val);
        return native->fn(arg_vals);
    }

    if (std::holds_alternative<std::shared_ptr<TacoFunction>>(callee_val)) {
        auto& fn = std::get<std::shared_ptr<TacoFunction>>(callee_val);
        return call_user_function(eval, fn, arg_vals, named_vals);
    }

    throw std::runtime_error("🌮 '" + value_to_string(callee_val) +
                             "' is not a function.");
}

static TacoValue call_user_function(
    Evaluator& eval,
    const std::shared_ptr<TacoFunction>& fn,
    const std::vector<TacoValue>& args,
    const std::vector<std::pair<std::string, TacoValue>>& named_args)
{
    // 创建函数的执行环境，父环境是闭包环境
    auto fn_env = std::make_shared<Environment>(fn->closure);

    // 处理参数绑定
    int pos_idx = 0;  // 当前处理到第几个位置参数

    for (const auto& param : fn->params) {
        TacoValue val = nullptr;

        if (param.is_named) {
            // 命名参数：先从 named_args 里找
            bool found = false;
            for (const auto& [name, v] : named_args) {
                if (name == param.name) {
                    val = v;
                    found = true;
                    break;
                }
            }
            // 没找到就用默认值
            if (!found) {
                if (param.default_value) {
                    val = eval.evaluate(param.default_value.get());
                } else {
                    throw std::runtime_error(
                        "🌮 Missing argument for parameter '" + param.name + "'."
                    );
                }
            }
        } else {
            // 位置参数
            if (pos_idx < static_cast<int>(args.size())) {
                val = args[pos_idx++];
            } else if (param.default_value) {
                val = eval.evaluate(param.default_value.get());
            } else {
                throw std::runtime_error(
                    "🌮 Missing argument for parameter '" + param.name + "'."
                );
            }
        }

        fn_env->define(param.name, std::move(val));
    }

    // RAII 守卫：进入函数作用域
    ScopeGuard guard(eval, fn_env);

    // 执行函数体
    TacoValue result = nullptr;
    try {
        for (const auto& stmt : fn->body) {
            result = eval.evaluate(stmt.get());
        }
    } catch (ReturnException& ret) {
        return ret.value;
    }

    return result;
}
```

---

## 20.8 多返回值的实现

Taco 支持多返回值：

```taco
func minmax(arr) {
    return arr[0], arr[arr.len() - 1];
}
var lo, hi = minmax([3, 1, 4, 1, 5]);
```

实现思路：多返回值在内部用一个特殊的"元组"值表示（用 `vector<TacoValue>` 包装），解构赋值时拆开：

更新 `TacoValue`，加入数组类型（同时也支持 Taco 的 array 类型）：

```cpp
using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>,
    std::shared_ptr<std::vector<TacoValue>>  // array 和多返回值都用这个
>;
```

`ReturnStmt` 的求值：

```cpp
TacoValue ReturnStmt::evaluate(Evaluator& eval) const {
    if (!value) {
        throw ReturnException{nullptr};
    }

    // 检查是否是多返回值（逗号分隔的表达式列表）
    if (auto* multi = dynamic_cast<const MultiReturn*>(value.get())) {
        auto arr = std::make_shared<std::vector<TacoValue>>();
        for (const auto& expr : multi->values) {
            arr->push_back(eval.evaluate(expr.get()));
        }
        throw ReturnException{arr};
    }

    throw ReturnException{eval.evaluate(value.get())};
}
```

`MultiAssign` 的求值：

```cpp
TacoValue MultiAssign::evaluate(Evaluator& eval) const {
    TacoValue result = eval.evaluate(value.get());

    // 期望结果是一个 array（多返回值）
    if (!std::holds_alternative<std::shared_ptr<std::vector<TacoValue>>>(result)) {
        throw std::runtime_error(
            "🌮 Expected multiple return values, got a single value."
        );
    }

    auto& arr = *std::get<std::shared_ptr<std::vector<TacoValue>>>(result);

    if (arr.size() != names.size()) {
        throw std::runtime_error(
            "🌮 Expected " + std::to_string(names.size()) +
            " values, got " + std::to_string(arr.size()) + "."
        );
    }

    for (int i = 0; i < static_cast<int>(names.size()); i++) {
        eval.current_env()->define(names[i], arr[i]);
    }

    return result;
}
```

---

## 20.9 内置函数的实现

内置函数用 C++ lambda 实现，存为 `TacoNative`：

```cpp
// builtins.cpp
#include "builtins.h"
#include "value.h"
#include <iostream>
#include <stdexcept>

void register_builtins(Environment::Ptr env) {
    // print
    env->define("print", std::make_shared<TacoNative>(TacoNative{
        "print",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 print() takes 1 argument.");
            }
            std::cout << value_to_string(args[0]) << "\n";
            return nullptr;
        }
    }));

    // type：返回值的类型名
    env->define("type", std::make_shared<TacoNative>(TacoNative{
        "type",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 type() takes 1 argument.");
            }
            return std::visit([](auto&& v) -> std::string {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>)       return "number";
                if constexpr (std::is_same_v<T, std::string>)  return "string";
                if constexpr (std::is_same_v<T, bool>)         return "bool";
                if constexpr (std::is_same_v<T, std::nullptr_t>) return "nil";
                return "function";
            }, args[0]);
        }
    }));

    // input：读取用户输入
    env->define("input", std::make_shared<TacoNative>(TacoNative{
        "input",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (!args.empty()) {
                std::cout << value_to_string(args[0]);
            }
            std::string line;
            std::getline(std::cin, line);
            return line;
        }
    }));

    // number：把值转成数字
    env->define("number", std::make_shared<TacoNative>(TacoNative{
        "number",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 number() takes 1 argument.");
            }
            return std::visit([](auto&& v) -> TacoValue {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>)      return v;
                if constexpr (std::is_same_v<T, std::string>) {
                    try { return std::stod(v); }
                    catch (...) {
                        throw std::runtime_error(
                            "🌮 Cannot convert '" + v + "' to number."
                        );
                    }
                }
                if constexpr (std::is_same_v<T, bool>) return v ? 1.0 : 0.0;
                throw std::runtime_error("🌮 Cannot convert nil to number.");
                return nullptr;
            }, args[0]);
        }
    }));

    // string：把值转成字符串
    env->define("string", std::make_shared<TacoNative>(TacoNative{
        "string",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() != 1) {
                throw std::runtime_error("🌮 string() takes 1 argument.");
            }
            return value_to_string(args[0]);
        }
    }));
}
```

---

## 20.10 测试：斐波那契数列与闭包

创建 `test_v3.taco`：

```taco
// 斐波那契（递归）
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}

for i in range(0, 11) {
    print(fib(i));
}
// 0 1 1 2 3 5 8 13 21 34 55

// 闭包：计数器
func makeCounter() {
    var count = 0;
    return func() {
        count = count + 1;
        return count;
    };
}

var c1 = makeCounter();
var c2 = makeCounter();

print(c1());  // 1
print(c1());  // 2
print(c2());  // 1（c2 有自己的 count，和 c1 独立）
print(c1());  // 3

// 闭包：累加器
func makeAdder(n) {
    return func(x) {
        return x + n;
    };
}

var add5 = makeAdder(5);
var add10 = makeAdder(10);

print(add5(3));   // 8
print(add10(3));  // 13

// 命名参数
func greet(name, greeting: "Hola") {
    print(greeting + ", " + name + "!");
}

greet("Miguel");
greet("Dante", greeting: "Hey");

// 高阶函数：把函数作为参数传递
func apply(f, x) {
    return f(x);
}

func double(x) { return x * 2; }
func square(x) { return x * x; }

print(apply(double, 5));  // 10
print(apply(square, 5));  // 25
```

### 运行结果

```
0
1
1
2
3
5
8
13
21
34
55
1
2
1
3
8
13
Hola, Miguel!
Hey, Dante!
10
25
```

---

## 20.11 这个版本的局限性

**还没有 array 和 map 的内置方法**

`arr.filter`、`arr.map`、`str.getLines()` 这些还不支持，v4 会系统地实现标准库。

**参数列表的 AST 节点没有完全实现**

`Param` 里的 `default_value` 用了 `unique_ptr<Expr>`，但在 `FuncDecl::evaluate` 里克隆 AST 节点需要实现一个 `clone` 函数，这里做了简化处理。完整实现会在 v4 里补全。

**没有尾调用优化**

Taco 里的递归函数，每次调用都会在 C++ 的调用栈上创建一个新帧。调用层次太深（比如 `fib(100)`）会导致 C++ 的栈溢出。真实的解释器通常会实现尾调用优化（TCO）或者用显式栈来避免这个问题。

**没有 OOP**

`class`、`struct`、`enum` 还没实现，这些会在综合收尾章节里加入。

---

## 小结

v3 是 Taco 解释器最核心的一步进化。

**Environment（作用域链）** 用 `shared_ptr` 管理，支持闭包捕获外层变量。每个作用域是堆上的一个对象，内层函数持有对外层作用域的共享引用，外层函数返回后作用域仍然存活。

**函数是值**：函数被表示为 `TacoFunction`（用 `shared_ptr` 存在 `TacoValue` 里），包含参数列表、函数体 AST、闭包环境三个部分。

**RAII 管理函数调用栈**：`ScopeGuard` 在构造时进入新作用域，在析构时（不管正常返回还是异常）恢复旧作用域。这让函数调用的资源管理完全自动化。

**内置函数** 用 `std::function` 包装 C++ lambda，统一注册到全局环境，和用户定义函数的调用方式一致。

**闭包** 在函数定义时捕获当前环境（`fn->closure = eval.current_env()`），调用时以闭包环境为父环境创建新作用域，实现对外层变量的访问和修改。

---

第四部分到这里结束。第五部分进入 STL，学完之后 v4 会用 STL 容器和算法实现 Taco 的 array、map，以及完整的内置标准库和 pipeline 风格。
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
# 第二十五章：项目 v4——array、map、enum 与标准库

---

第五部分学完了 STL 容器、迭代器、算法、Lambda。现在用这些知识做 v4 最重要的两件事：

1. **实现 Taco 的 array 和 map 类型**，以及对应的 pipeline 方法（`filter`、`map`、`each`、`reduce`）
2. **实现 Taco 的内置标准库**，包括字符串、文件、系统操作等

v4 结束时，Taco 能运行这样的代码：

```taco
// array 和 pipeline
var nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

nums
    .filter { n in n % 2 == 0 }
    .map { n in n * n }
    .each { n in print(n) };
// 4 16 36 64 100

// array 方法
var arr = [3, 1, 4, 1, 5, 9, 2, 6];
print(arr.sum());         // 31
print(arr.min());         // 1
print(arr.max());         // 9
print(arr.len());         // 8
print(arr.contains(5));   // true
print(arr.getFirst(3));   // [3, 1, 4]
print(arr.getLast(3));    // [2, 6]... 不对，是最后3个 [9, 2, 6]

// map（字典）
var person = {"name": "Miguel", "age": 12};
print(person["name"]);    // Miguel
print(person.name);       // Miguel（点语法）
person["city"] = "Oaxaca";
print(person.getKeys());  // ["name", "age", "city"]

// 字符串方法
var s = "Hello, World!";
print(s.len());           // 13
print(s.upper());         // HELLO, WORLD!
print(s.contains("World")); // true
print(s.split(", "));     // ["Hello", "World!"]
print(s.getChars());      // ["H", "e", "l", ...]

// 文件操作
var content = cat("test.taco");
print(content.getLines().len());

// 系统操作
var files = ls(".");
files.filter { f in f.endsWith(".taco") }.each { f in print(f) };
```

---

## 25.1 用 STL 容器实现 Taco 的 array 和 map

### 更新值类型

```cpp
// value.h（v4 版本）
#pragma once
#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

struct TacoFunction;
struct TacoNative;

// Taco 的 array：vector<TacoValue> 的 shared_ptr 包装
// 用 shared_ptr 是因为 array 是引用类型（赋值是共享，不是拷贝）
struct TacoArray;
struct TacoMap;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>
>;

// 定义在 value.h 里（或 array.h/map.h 里）
struct TacoArray {
    std::vector<TacoValue> elements;

    TacoArray() = default;
    explicit TacoArray(std::vector<TacoValue> elems)
        : elements(std::move(elems)) {}
};

struct TacoMap {
    // 用 vector<pair> 保持插入顺序（unordered_map 不保证顺序）
    std::vector<std::pair<std::string, TacoValue>> entries;

    TacoMap() = default;

    TacoValue get(const std::string& key) const;
    void set(const std::string& key, TacoValue val);
    bool has(const std::string& key) const;
    void remove(const std::string& key);
};
```

为什么 `TacoArray` 和 `TacoMap` 用 `shared_ptr` 包装？

在 Taco 里，array 和 map 是**引用类型**（和 class 一样）：

```taco
var a = [1, 2, 3];
var b = a;        // b 和 a 指向同一个 array
b.push(4);
print(a.len());   // 4，a 也变了
```

用 `shared_ptr` 可以让多个变量指向同一个底层数组，赋值时只拷贝指针，不拷贝数组内容。

---

### TacoMap 的实现

```cpp
// value.cpp（部分）

TacoValue TacoMap::get(const std::string& key) const {
    for (const auto& [k, v] : entries) {
        if (k == key) return v;
    }
    throw std::runtime_error("🌮 Key '" + key + "' not found.");
}

void TacoMap::set(const std::string& key, TacoValue val) {
    for (auto& [k, v] : entries) {
        if (k == key) {
            v = std::move(val);
            return;
        }
    }
    entries.push_back({key, std::move(val)});
}

bool TacoMap::has(const std::string& key) const {
    for (const auto& [k, v] : entries) {
        if (k == key) return true;
    }
    return false;
}

void TacoMap::remove(const std::string& key) {
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
                       [&key](const auto& pair) { return pair.first == key; }),
        entries.end()
    );
}
```

这里用 `vector<pair>` 而不是 `unordered_map`，是因为 Taco 的 map 需要保持插入顺序（和 Python 3.7+ 的 `dict` 类似）。查找是 O(n)，对于小型字典完全够用。如果需要高性能，可以改成 `unordered_map`。

---

## 25.2 实现 enum

Taco 的 enum 是一组命名常量：

```taco
enum Direction {
    North,
    South,
    East,
    West
}

var d = Direction.North;
switch (d) {
    case Direction.North { print("going north"); }
    default { print("other"); }
}
```

在 C++ 里，Taco 的 enum 值用字符串表示：`Direction.North` 就是字符串 `"Direction.North"`。

```cpp
// ast.h 新增
struct EnumDecl : Expr {
    std::string              name;
    std::vector<std::string> variants;  // 枚举值的名字列表

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "enum " + name; }
};
```

```cpp
// ast.cpp
TacoValue EnumDecl::evaluate(Evaluator& eval) const {
    // 把每个变体注册为字符串常量
    // Direction.North = "Direction.North"
    for (const auto& variant : variants) {
        std::string full_name = name + "." + variant;
        eval.current_env()->define(name + "." + variant,
                                   std::string(full_name));
    }
    return nullptr;
}
```

带关联值的 enum：

```taco
enum Result {
    Ok(value),
    Err(message)
}

var r = Result.Ok(42);
```

带关联值的枚举变体是一个函数，调用它返回一个包含类型标签和值的 map：

```cpp
// Result.Ok 是一个函数，调用后返回 {"__type": "Result.Ok", "value": 42}
TacoValue make_enum_variant(const std::string& tag, std::vector<TacoValue> args) {
    auto m = std::make_shared<TacoMap>();
    m->set("__type", std::string(tag));

    // 根据关联值数量绑定到对应的字段名
    // 简化实现：只有一个关联值时用 "value"
    if (!args.empty()) {
        m->set("value", args[0]);
    }
    return m;
}
```

---

## 25.3 实现 pipeline：filter、map、each、reduce

Pipeline 是 Taco 最有特色的功能。`arr.filter { x in x > 3 }` 这样的方法链，每个方法接受一个闭包，返回新的 array。

在 C++ 里，这些方法挂在 `TacoArray` 上（通过方法分派），或者在求值器里作为内置方法处理。

### 方法调用的 AST 节点

```cpp
// ast.h 新增
struct MethodCallExpr : Expr {
    ExprPtr              object;     // 调用方法的对象
    std::string          method;     // 方法名
    std::vector<ExprPtr> args;       // 普通参数
    ExprPtr              closure_arg; // 闭包参数（{ x in ... } 风格）

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return object->to_string() + "." + method + "(...)";
    }
};
```

### 方法调用的求值

```cpp
// ast.cpp
TacoValue MethodCallExpr::evaluate(Evaluator& eval) const {
    TacoValue obj = eval.evaluate(object.get());

    // 求值普通参数
    std::vector<TacoValue> arg_vals;
    for (const auto& arg : args) {
        arg_vals.push_back(eval.evaluate(arg.get()));
    }

    // 求值闭包参数（如果有）
    TacoValue closure_val = nullptr;
    if (closure_arg) {
        closure_val = eval.evaluate(closure_arg.get());
    }

    // 分派到对应类型的方法
    if (std::holds_alternative<std::string>(obj)) {
        return dispatch_string_method(
            std::get<std::string>(obj), method, arg_vals, closure_val, eval
        );
    }

    if (std::holds_alternative<std::shared_ptr<TacoArray>>(obj)) {
        return dispatch_array_method(
            std::get<std::shared_ptr<TacoArray>>(obj), method, arg_vals, closure_val, eval
        );
    }

    if (std::holds_alternative<std::shared_ptr<TacoMap>>(obj)) {
        return dispatch_map_method(
            std::get<std::shared_ptr<TacoMap>>(obj), method, arg_vals, closure_val, eval
        );
    }

    throw std::runtime_error(
        "🌮 '" + value_to_string(obj) + "' has no method '" + method + "'."
    );
}
```

### array 的方法实现

```cpp
// array_methods.cpp

// 调用一个 Taco 函数（闭包）
static TacoValue call_closure(Evaluator& eval, const TacoValue& fn,
                               std::vector<TacoValue> args) {
    if (std::holds_alternative<std::shared_ptr<TacoFunction>>(fn)) {
        auto& f = std::get<std::shared_ptr<TacoFunction>>(fn);
        return call_user_function(eval, f, args, {});
    }
    throw std::runtime_error("🌮 Expected a function.");
}

TacoValue dispatch_array_method(
    std::shared_ptr<TacoArray> arr,
    const std::string& method,
    const std::vector<TacoValue>& args,
    const TacoValue& closure,
    Evaluator& eval)
{
    // ── filter ──────────────────────────────────────────
    if (method == "filter") {
        auto result = std::make_shared<TacoArray>();
        for (const auto& elem : arr->elements) {
            TacoValue keep = call_closure(eval, closure, {elem});
            if (is_truthy(keep)) {
                result->elements.push_back(elem);
            }
        }
        return result;
    }

    // ── map ─────────────────────────────────────────────
    if (method == "map") {
        auto result = std::make_shared<TacoArray>();
        result->elements.reserve(arr->elements.size());
        std::transform(
            arr->elements.begin(), arr->elements.end(),
            std::back_inserter(result->elements),
            [&](const TacoValue& elem) {
                return call_closure(eval, closure, {elem});
            }
        );
        return result;
    }

    // ── each ────────────────────────────────────────────
    if (method == "each") {
        std::for_each(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                call_closure(eval, closure, {elem});
            }
        );
        return nullptr;
    }

    // ── reduce ──────────────────────────────────────────
    if (method == "reduce") {
        if (arr->elements.empty()) {
            return args.empty() ? TacoValue{nullptr} : args[0];
        }

        TacoValue acc = args.empty() ? arr->elements[0] : args[0];
        int start = args.empty() ? 1 : 0;

        for (int i = start; i < static_cast<int>(arr->elements.size()); i++) {
            acc = call_closure(eval, closure, {acc, arr->elements[i]});
        }
        return acc;
    }

    // ── find ────────────────────────────────────────────
    if (method == "find") {
        auto it = std::find_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                return is_truthy(call_closure(eval, closure, {elem}));
            }
        );
        return it != arr->elements.end() ? *it : TacoValue{nullptr};
    }

    // ── sortBy ──────────────────────────────────────────
    if (method == "sortBy") {
        auto result = std::make_shared<TacoArray>(*arr);  // 拷贝
        std::sort(
            result->elements.begin(), result->elements.end(),
            [&](const TacoValue& a, const TacoValue& b) {
                TacoValue ka = call_closure(eval, closure, {a});
                TacoValue kb = call_closure(eval, closure, {b});
                if (std::holds_alternative<double>(ka) &&
                    std::holds_alternative<double>(kb)) {
                    return std::get<double>(ka) < std::get<double>(kb);
                }
                if (std::holds_alternative<std::string>(ka) &&
                    std::holds_alternative<std::string>(kb)) {
                    return std::get<std::string>(ka) < std::get<std::string>(kb);
                }
                return false;
            }
        );
        return result;
    }

    // ── push ────────────────────────────────────────────
    if (method == "push") {
        if (args.empty()) throw std::runtime_error("🌮 push() needs an argument.");
        arr->elements.push_back(args[0]);
        return nullptr;
    }

    // ── pop ─────────────────────────────────────────────
    if (method == "pop") {
        if (arr->elements.empty()) throw std::runtime_error("🌮 pop() on empty array.");
        TacoValue last = arr->elements.back();
        arr->elements.pop_back();
        return last;
    }

    // ── len ─────────────────────────────────────────────
    if (method == "len") {
        return static_cast<double>(arr->elements.size());
    }

    // ── contains ────────────────────────────────────────
    if (method == "contains") {
        if (args.empty()) throw std::runtime_error("🌮 contains() needs an argument.");
        auto it = std::find_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) { return values_equal(elem, args[0]); }
        );
        return it != arr->elements.end();
    }

    // ── sum ─────────────────────────────────────────────
    if (method == "sum") {
        double total = std::accumulate(
            arr->elements.begin(), arr->elements.end(), 0.0,
            [](double acc, const TacoValue& v) {
                if (std::holds_alternative<double>(v))
                    return acc + std::get<double>(v);
                return acc;
            }
        );
        return total;
    }

    // ── avg ─────────────────────────────────────────────
    if (method == "avg") {
        if (arr->elements.empty()) return 0.0;
        TacoValue sum_val = dispatch_array_method(arr, "sum", {}, nullptr, eval);
        return std::get<double>(sum_val) / arr->elements.size();
    }

    // ── min / max ────────────────────────────────────────
    if (method == "min" || method == "max") {
        if (arr->elements.empty())
            throw std::runtime_error("🌮 min()/max() on empty array.");

        auto cmp = [](const TacoValue& a, const TacoValue& b) {
            if (std::holds_alternative<double>(a) &&
                std::holds_alternative<double>(b)) {
                return std::get<double>(a) < std::get<double>(b);
            }
            return false;
        };

        auto it = (method == "min")
            ? std::min_element(arr->elements.begin(), arr->elements.end(), cmp)
            : std::max_element(arr->elements.begin(), arr->elements.end(), cmp);

        return *it;
    }

    // ── getFirst / getLast ───────────────────────────────
    if (method == "getFirst") {
        if (arr->elements.empty()) return nullptr;
        if (args.empty()) return arr->elements.front();

        int n = static_cast<int>(std::get<double>(args[0]));
        int count = std::min(n, static_cast<int>(arr->elements.size()));
        auto result = std::make_shared<TacoArray>();
        result->elements.assign(arr->elements.begin(),
                                arr->elements.begin() + count);
        return result;
    }

    if (method == "getLast") {
        if (arr->elements.empty()) return nullptr;
        if (args.empty()) return arr->elements.back();

        int n = static_cast<int>(std::get<double>(args[0]));
        int count = std::min(n, static_cast<int>(arr->elements.size()));
        auto result = std::make_shared<TacoArray>();
        result->elements.assign(arr->elements.end() - count,
                                arr->elements.end());
        return result;
    }

    // ── findFirst / findLast ─────────────────────────────
    if (method == "findFirst") {
        if (arr->elements.empty()) return nullptr;
        return arr->elements.front();
    }

    if (method == "findLast") {
        if (arr->elements.empty()) return nullptr;
        return arr->elements.back();
    }

    // ── groupBy ──────────────────────────────────────────
    if (method == "groupBy") {
        auto result = std::make_shared<TacoMap>();
        for (const auto& elem : arr->elements) {
            TacoValue key_val = call_closure(eval, closure, {elem});
            std::string key = value_to_string(key_val);

            if (!result->has(key)) {
                result->set(key, std::make_shared<TacoArray>());
            }
            auto& group = std::get<std::shared_ptr<TacoArray>>(result->get(key));
            group->elements.push_back(elem);
        }
        return result;
    }

    // ── countBy ──────────────────────────────────────────
    if (method == "countBy") {
        int count = static_cast<int>(std::count_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                return is_truthy(call_closure(eval, closure, {elem}));
            }
        ));
        return static_cast<double>(count);
    }

    throw std::runtime_error(
        "🌮 Array has no method '" + method + "'."
    );
}
```

---

## 25.4 字符串方法的实现

```cpp
// string_methods.cpp

TacoValue dispatch_string_method(
    const std::string& str,
    const std::string& method,
    const std::vector<TacoValue>& args,
    const TacoValue& closure,
    Evaluator& eval)
{
    if (method == "len") {
        return static_cast<double>(str.size());
    }

    if (method == "upper") {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    if (method == "lower") {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    if (method == "contains") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 contains() needs a string argument.");
        return str.find(std::get<std::string>(args[0])) != std::string::npos;
    }

    if (method == "startsWith") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 startsWith() needs a string argument.");
        const auto& prefix = std::get<std::string>(args[0]);
        return str.size() >= prefix.size() &&
               str.substr(0, prefix.size()) == prefix;
    }

    if (method == "endsWith") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 endsWith() needs a string argument.");
        const auto& suffix = std::get<std::string>(args[0]);
        return str.size() >= suffix.size() &&
               str.substr(str.size() - suffix.size()) == suffix;
    }

    if (method == "trimSpace") {
        std::string result = str;
        // 去掉头部空白
        result.erase(result.begin(),
                     std::find_if(result.begin(), result.end(),
                                  [](char c) { return !std::isspace(c); }));
        // 去掉尾部空白
        result.erase(std::find_if(result.rbegin(), result.rend(),
                                  [](char c) { return !std::isspace(c); }).base(),
                     result.end());
        return result;
    }

    if (method == "split") {
        std::string delimiter = args.empty() ? " "
            : std::get<std::string>(args[0]);

        auto result = std::make_shared<TacoArray>();
        std::string::size_type start = 0;
        std::string::size_type pos;

        while ((pos = str.find(delimiter, start)) != std::string::npos) {
            result->elements.push_back(str.substr(start, pos - start));
            start = pos + delimiter.size();
        }
        result->elements.push_back(str.substr(start));
        return result;
    }

    if (method == "getLines") {
        return dispatch_string_method(str, "split", {std::string("\n")},
                                     nullptr, eval);
    }

    if (method == "getWords") {
        auto result = std::make_shared<TacoArray>();
        std::istringstream iss(str);
        std::string word;
        while (iss >> word) {
            result->elements.push_back(word);
        }
        return result;
    }

    if (method == "getChars") {
        auto result = std::make_shared<TacoArray>();
        for (char c : str) {
            result->elements.push_back(std::string(1, c));
        }
        return result;
    }

    if (method == "replaceStr") {
        if (args.size() < 2) throw std::runtime_error("🌮 replaceStr() needs 2 arguments.");
        const auto& from = std::get<std::string>(args[0]);
        const auto& to   = std::get<std::string>(args[1]);

        std::string result = str;
        std::string::size_type pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
        return result;
    }

    if (method == "findStr") {
        if (args.empty()) throw std::runtime_error("🌮 findStr() needs an argument.");
        auto pos = str.find(std::get<std::string>(args[0]));
        return pos != std::string::npos
            ? TacoValue{static_cast<double>(pos)}
            : TacoValue{-1.0};
    }

    throw std::runtime_error("🌮 String has no method '" + method + "'.");
}
```

---

## 25.5 内置标准库：文件和系统

```cpp
// builtins.cpp（新增部分）

void register_builtins(Environment::Ptr env) {
    // 之前注册的 print, type, input, number, string...

    // ── 文件系统 ────────────────────────────────────────

    // cat：读取文件内容
    env->define("cat", std::make_shared<TacoNative>(TacoNative{
        "cat",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 cat() needs a path.");
            const auto& path = std::get<std::string>(args[0]);
            std::ifstream f(path);
            if (!f) throw std::runtime_error("🌮 Cannot read: " + path);
            std::ostringstream buf;
            buf << f.rdbuf();
            return buf.str();
        }
    }));

    // echo：写文件
    env->define("echo", std::make_shared<TacoNative>(TacoNative{
        "echo",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 echo() needs content and path.");
            const auto& content = value_to_string(args[0]);
            const auto& path    = std::get<std::string>(args[1]);
            std::ofstream f(path);
            if (!f) throw std::runtime_error("🌮 Cannot write: " + path);
            f << content;
            return nullptr;
        }
    }));

    // ls：列出目录
    env->define("ls", std::make_shared<TacoNative>(TacoNative{
        "ls",
        [](std::vector<TacoValue> args) -> TacoValue {
            namespace fs = std::filesystem;
            std::string path = args.empty() ? "." : std::get<std::string>(args[0]);
            auto result = std::make_shared<TacoArray>();
            for (const auto& entry : fs::directory_iterator(path)) {
                result->elements.push_back(entry.path().filename().string());
            }
            return result;
        }
    }));

    // mkdir：创建目录
    env->define("mkdir", std::make_shared<TacoNative>(TacoNative{
        "mkdir",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 mkdir() needs a path.");
            namespace fs = std::filesystem;
            fs::create_directories(std::get<std::string>(args[0]));
            return nullptr;
        }
    }));

    // rm：删除文件
    env->define("rm", std::make_shared<TacoNative>(TacoNative{
        "rm",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 rm() needs a path.");
            namespace fs = std::filesystem;
            fs::remove(std::get<std::string>(args[0]));
            return nullptr;
        }
    }));

    // mv：移动/重命名
    env->define("mv", std::make_shared<TacoNative>(TacoNative{
        "mv",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 mv() needs src and dst.");
            namespace fs = std::filesystem;
            fs::rename(std::get<std::string>(args[0]),
                       std::get<std::string>(args[1]));
            return nullptr;
        }
    }));

    // cp：复制
    env->define("cp", std::make_shared<TacoNative>(TacoNative{
        "cp",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 cp() needs src and dst.");
            namespace fs = std::filesystem;
            fs::copy(std::get<std::string>(args[0]),
                     std::get<std::string>(args[1]));
            return nullptr;
        }
    }));

    // pwd：当前目录
    env->define("pwd", std::make_shared<TacoNative>(TacoNative{
        "pwd",
        [](std::vector<TacoValue>) -> TacoValue {
            return std::filesystem::current_path().string();
        }
    }));

    // exists：文件是否存在
    env->define("exists", std::make_shared<TacoNative>(TacoNative{
        "exists",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 exists() needs a path.");
            return std::filesystem::exists(std::get<std::string>(args[0]));
        }
    }));

    // ── 系统 ────────────────────────────────────────────

    // exec：执行系统命令，返回输出
    env->define("exec", std::make_shared<TacoNative>(TacoNative{
        "exec",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 exec() needs a command.");
            const auto& cmd = std::get<std::string>(args[0]);

            // 用 popen 捕获命令输出
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) throw std::runtime_error("🌮 exec() failed.");

            std::string result;
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                result += buffer;
            }
            pclose(pipe);
            return result;
        }
    }));

    // env：读取环境变量
    env->define("env", std::make_shared<TacoNative>(TacoNative{
        "env",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 env() needs a key.");
            const char* val = std::getenv(std::get<std::string>(args[0]).c_str());
            return val ? std::string(val) : TacoValue{nullptr};
        }
    }));

    // ── 网络（基础版，v7 会完善）──────────────────────────

    env->define("fetchUrl", std::make_shared<TacoNative>(TacoNative{
        "fetchUrl",
        [](std::vector<TacoValue> args) -> TacoValue {
            // 占位：v7 会用 cpp-httplib 实现
            throw std::runtime_error("🌮 fetchUrl() is available in v7.");
            return nullptr;
        }
    }));

    // ── random ──────────────────────────────────────────

    static std::mt19937 rng(std::random_device{}());

    env->define("random.pick", std::make_shared<TacoNative>(TacoNative{
        "random.pick",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty() || !std::holds_alternative<std::shared_ptr<TacoArray>>(args[0]))
                throw std::runtime_error("🌮 random.pick() needs an array.");
            auto& arr = std::get<std::shared_ptr<TacoArray>>(args[0]);
            if (arr->elements.empty()) return nullptr;
            std::uniform_int_distribution<int> dist(0, arr->elements.size() - 1);
            return arr->elements[dist(rng)];
        }
    }));

    env->define("random.flip", std::make_shared<TacoNative>(TacoNative{
        "random.flip",
        [](std::vector<TacoValue>) -> TacoValue {
            std::bernoulli_distribution dist(0.5);
            return dist(rng);
        }
    }));

    env->define("random.int", std::make_shared<TacoNative>(TacoNative{
        "random.int",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 random.int() needs min and max.");
            int lo = static_cast<int>(std::get<double>(args[0]));
            int hi = static_cast<int>(std::get<double>(args[1]));
            std::uniform_int_distribution<int> dist(lo, hi);
            return static_cast<double>(dist(rng));
        }
    }));
}
```

---

## 25.6 测试：运行完整的 Taco 脚本

创建 `test_v4.taco`：

```taco
// pipeline 风格处理数据
var scores = [88, 45, 92, 33, 76, 100, 55, 87];

print("通过的分数：");
scores
    .filter { s in s >= 60 }
    .sortBy { s in s }
    .each { s in print(s) };

print("平均分（通过）：");
var passed = scores.filter { s in s >= 60 };
print(passed.avg());

// 字符串处理
var log = "ERROR: something failed\nINFO: started\nERROR: another error\nINFO: done";
print("错误行：");
log.getLines()
   .filter { line in line.startsWith("ERROR") }
   .each { line in print(line) };

// map 操作
var person = {"name": "Miguel", "age": 12, "city": "Oaxaca"};
print(person.getKeys());
print(person.getValues());

// 文件操作
var files = ls(".");
print("当前目录文件数：" + string(files.len()));
files.filter { f in f.endsWith(".taco") }.each { f in print(f) };

// 系统命令
var git_status = exec("git status --short 2>/dev/null");
if (git_status.len() > 0) {
    print("Git 状态：");
    print(git_status);
}

// groupBy 示例
var words = ["apple", "banana", "avocado", "blueberry", "apricot"];
var grouped = words.groupBy { w in w.getChars()[0] };
print(grouped);
```

### 运行结果

```
通过的分数：
76
87
88
92
100
平均分（通过）：
88.6
错误行：
ERROR: something failed
ERROR: another error
当前目录文件数：5
test_v4.taco
test_v3.taco
...
```

---

## 25.7 这个版本的局限性

**`array` 是引用类型，但字面量每次创建新对象**

```taco
var a = [1, 2, 3];
var b = a;       // 共享
var c = [1, 2, 3];  // 新对象，和 a 不共享
```

这是正确的行为，但在 AST 节点里，数组字面量每次求值都创建新的 `TacoArray`，所以 `c` 和 `a` 不共享，符合预期。

**没有展开运算符 `...`**

```taco
var merged = [...a, ...b];  // 还不支持
```

展开运算符需要在语法分析器里特殊处理，v4 暂时不加。

**可选链 `?.` 和默认值 `??` 还没实现**

```taco
var city = user?.address?.city ?? "unknown";  // 还不支持
```

这些是语法糖，可以在解析器里加入，留给读者作为练习。

**没有 OOP（class、struct）**

class 和 struct 的实现需要在值系统里加入对象类型，以及方法查找机制。这在综合收尾章节里处理。

---

## 小结

v4 是 Taco 最接近"完整脚本语言"的一步。

**array 和 map** 用 STL 容器实现，用 `shared_ptr` 包装成引用类型。所有 pipeline 方法（`filter`、`map`、`each`、`reduce`）在方法分派函数里实现，内部用 STL 算法（`std::find_if`、`std::transform`、`std::sort` 等）。

**字符串方法** 用 STL 算法和标准库字符串操作实现，方法名遵循 v+n 的命名规范（`getLines`、`trimSpace`、`findStr` 等）。

**内置标准库** 包含文件系统操作（`cat`、`ls`、`mkdir`、`rm`、`mv`、`cp`、`pwd`、`exists`）、系统操作（`exec`、`env`）、随机数（`random.pick`、`random.flip`、`random.int`）。每个内置函数都是一个 `TacoNative`（用 `std::function` 包装的 lambda）。

**Lambda 在这里的核心作用**：每个 pipeline 方法里，调用 Taco 闭包的操作是通过 C++ lambda 包装后传给 STL 算法的。C++ lambda 捕获了 `eval` 引用，让 Taco 的 `{ x in x * 2 }` 可以在 C++ 的 `std::transform` 里被调用。

---

第五部分到这里结束。第六部分进入模板，学完之后 v5 会用模板重构解释器内部，让值类型和容器更通用、更高效。
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
# 第二十七章：类模板

---

上一章讲了函数模板——用模板参数让函数适用于多种类型。类模板是同样的思想用于类：定义一个"与类型无关"的类，让编译器根据使用时的具体类型生成对应的版本。

`std::vector<int>`、`std::map<std::string, int>`、`std::optional<double>`——STL 里所有的容器和工具类都是类模板。这一章解释它们背后的机制，并动手实现几个。

---

## 27.1 类模板的写法

### 基本语法

```cpp
template<typename T>
class Box {
public:
    Box(T value) : value_(value) {}

    T get() const { return value_; }
    void set(T value) { value_ = value; }

    bool operator==(const Box<T>& other) const {
        return value_ == other.value_;
    }

private:
    T value_;
};
```

使用类模板时，**必须显式指定类型**（C++17 之前；C++17 引入了类模板参数推导，后面会提到）：

```cpp
Box<int> int_box(42);
Box<std::string> str_box("hello");

int n = int_box.get();        // 42
std::string s = str_box.get(); // "hello"

int_box.set(100);

Box<int> another(100);
std::cout << (int_box == another) << "\n";  // 1（true）
```

### 类模板的实例化

和函数模板一样，类模板在编译时实例化。`Box<int>` 和 `Box<std::string>` 是两个完全不同的类，它们之间没有继承关系——只是碰巧从同一个模板生成出来。

```cpp
Box<int> int_box(42);
Box<double> dbl_box(3.14);

// 这两个是不同的类型，不能互相赋值
// int_box = dbl_box;  // 编译错误！
```

### 成员函数的定义

类模板的成员函数如果在类外定义，需要重复写模板参数：

```cpp
template<typename T>
class Box {
public:
    Box(T value);
    T get() const;
    void set(T value);
private:
    T value_;
};

// 类外定义：每个成员函数都要加 template<typename T>
template<typename T>
Box<T>::Box(T value) : value_(value) {}

template<typename T>
T Box<T>::get() const {
    return value_;
}

template<typename T>
void Box<T>::set(T value) {
    value_ = value;
}
```

`Box<T>::get` 表示"类型参数为 `T` 的 `Box` 类的 `get` 函数"。这个写法比较繁琐，所以通常把类模板的成员函数直接写在类体内——这是和普通类不同的习惯。

### C++17 类模板参数推导（CTAD）

C++17 引入了类模板参数推导（Class Template Argument Deduction），允许从构造函数参数推导模板参数：

```cpp
Box<int> b1(42);      // C++17 之前必须这样
Box b2(42);           // C++17 可以这样：编译器推导 T = int
Box b3(std::string("hello"));  // T = std::string
```

`std::vector{1, 2, 3}` 在 C++17 里也可以不写类型，编译器推导为 `std::vector<int>`。

不过显式写出类型参数依然是好习惯，特别是对复杂类型——清晰比省事更重要。

---

## 27.2 模板参数的默认值

和函数参数可以有默认值一样，模板参数也可以有默认值：

```cpp
// 第二个模板参数有默认值
template<typename Key, typename Value = std::string>
class Registry {
public:
    void insert(const Key& key, const Value& value) {
        data_[key] = value;
    }

    std::optional<Value> find(const Key& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::map<Key, Value> data_;
};

// 使用默认值：Value = std::string
Registry<std::string> r1;
r1.insert("name", "Miguel");
r1.insert("city", "Oaxaca");

// 显式指定两个参数
Registry<std::string, int> r2;
r2.insert("age", 12);
r2.insert("score", 100);
```

STL 里有很多模板参数默认值的例子：

```cpp
// std::vector 的真实定义（简化版）
template<typename T, typename Allocator = std::allocator<T>>
class vector { ... };

// std::map 的真实定义（简化版）
template<
    typename Key,
    typename Value,
    typename Compare = std::less<Key>,         // 默认升序比较
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class map { ... };
```

所以 `std::map<std::string, int>` 用了两个显式参数，Compare 和 Allocator 都用默认值。如果需要自定义排序：

```cpp
// 自定义比较：按字符串长度排序
auto cmp = [](const std::string& a, const std::string& b) {
    return a.size() < b.size();
};

std::map<std::string, int, decltype(cmp)> sorted_by_len(cmp);
sorted_by_len["hello"] = 1;
sorted_by_len["hi"] = 2;
sorted_by_len["hey"] = 3;
// 迭代顺序：hi, hey, hello（按长度升序）
```

---

## 27.3 成员函数模板

类模板的成员函数可以有**自己独立的**模板参数，和类的模板参数相互独立。

```cpp
template<typename T>
class Box {
public:
    Box(T value) : value_(value) {}
    T get() const { return value_; }

    // 成员函数模板：接受任意类型 U，转换成 Box<U>
    template<typename U>
    Box<U> convert_to() const {
        return Box<U>(static_cast<U>(value_));
    }

private:
    T value_;
};

Box<int> int_box(42);
Box<double> dbl_box = int_box.convert_to<double>();
Box<std::string> str_box = Box<double>(3.14).convert_to<std::string>();
// 错误！double 不能 static_cast 到 string
// 模板会编译失败——要求类型之间能互相转换
```

成员函数模板的另一个常见用途是**从不同类型构造**：

```cpp
template<typename T>
class SmartBox {
public:
    SmartBox(T value) : value_(value) {}

    // 允许从 Box<U> 构造 SmartBox<T>，只要 U 可以转换为 T
    template<typename U>
    SmartBox(const SmartBox<U>& other) : value_(other.get()) {}

    T get() const { return value_; }

private:
    T value_;
};

SmartBox<int> ibox(42);
SmartBox<double> dbox(ibox);  // int → double，OK
SmartBox<int> ibox2(dbox);    // double → int，精度损失，但编译通过
```

这种"转换构造函数模板"在智能指针里很常见：`std::shared_ptr<Base>` 可以从 `std::shared_ptr<Derived>` 构造（只要 `Derived` 继承自 `Base`）。

---

## 27.4 模板与继承

类模板可以继承，继承可以涉及模板参数，这里几种情况都要搞清楚。

### 普通类继承自类模板的实例

```cpp
template<typename T>
class Container {
public:
    virtual void add(T item) = 0;
    virtual T get(int index) const = 0;
    virtual int size() const = 0;
    virtual ~Container() = default;
};

// 普通类继承自具体实例
class IntList : public Container<int> {
public:
    void add(int item) override { data_.push_back(item); }
    int get(int index) const override { return data_[index]; }
    int size() const override { return data_.size(); }

private:
    std::vector<int> data_;
};

IntList list;
list.add(1);
list.add(2);
list.add(3);
std::cout << list.size() << "\n";  // 3
```

### 类模板继承自类模板

```cpp
template<typename T>
class Stack : public Container<T> {
public:
    void add(T item) override {
        data_.push_back(std::move(item));
    }

    T get(int index) const override {
        return data_[index];
    }

    int size() const override {
        return data_.size();
    }

    T pop() {
        T top = std::move(data_.back());
        data_.pop_back();
        return top;
    }

private:
    std::vector<T> data_;
};

Stack<std::string> str_stack;
str_stack.add("hello");
str_stack.add("world");
std::cout << str_stack.pop() << "\n";  // world

// 多态：通过 Container<std::string>* 操作 Stack<std::string>
Container<std::string>* c = &str_stack;
c->add("taco");
std::cout << c->size() << "\n";  // 2（因为 pop 了一个）
```

### 奇特的递归模板模式（CRTP）——了解即可

有一种叫 CRTP（Curiously Recurring Template Pattern）的高级技巧，让子类作为模板参数传给父类：

```cpp
// 父类以子类为模板参数
template<typename Derived>
class Printable {
public:
    void print() const {
        // static_cast 到子类，调用子类的方法
        static_cast<const Derived*>(this)->do_print();
    }
};

class MyClass : public Printable<MyClass> {
public:
    void do_print() const {
        std::cout << "I am MyClass\n";
    }
};

MyClass obj;
obj.print();  // 输出：I am MyClass
```

CRTP 让父类在不用虚函数的前提下调用子类的方法——是编译期多态的一种实现方式，没有虚函数表的开销。在性能敏感的场合（如数值库、嵌入式系统）会用到。这里只是让你知道有这个东西，Taco 项目里不会用到。

---

## 综合例子：Result 类型

Taco 在 v3 里用 `std::optional` 处理"可能没有值"的情况，但有时候需要区分"失败"和"为空"，需要一个携带错误信息的类型。来实现一个简化版的 `Result<T, E>`：

```cpp
// result.h
#pragma once
#include <variant>
#include <stdexcept>
#include <string>

template<typename T, typename E = std::string>
class Result {
public:
    // 工厂函数：构造成功的结果
    static Result ok(T value) {
        Result r;
        r.data_ = std::move(value);
        return r;
    }

    // 工厂函数：构造失败的结果
    static Result err(E error) {
        Result r;
        r.data_ = std::move(error);
        return r;
    }

    bool is_ok() const {
        return std::holds_alternative<T>(data_);
    }

    bool is_err() const {
        return std::holds_alternative<E>(data_);
    }

    // 提取值（调用前必须确认 is_ok()）
    T& value() {
        if (!is_ok()) throw std::runtime_error("Result is not ok");
        return std::get<T>(data_);
    }

    const T& value() const {
        if (!is_ok()) throw std::runtime_error("Result is not ok");
        return std::get<T>(data_);
    }

    // 提取错误（调用前必须确认 is_err()）
    E& error() {
        if (!is_err()) throw std::runtime_error("Result is not err");
        return std::get<E>(data_);
    }

    // 带默认值的提取：失败时返回默认值
    T value_or(T default_value) const {
        if (is_ok()) return std::get<T>(data_);
        return default_value;
    }

    // 链式操作：如果成功，对值应用函数；否则传播错误
    template<typename F>
    auto and_then(F func) const {
        using ReturnType = decltype(func(std::get<T>(data_)));
        if (is_ok()) {
            return func(std::get<T>(data_));
        }
        return ReturnType::err(std::get<E>(data_));
    }

private:
    std::variant<T, E> data_;
    Result() = default;
};
```

在 Taco 的求值器里，可以用这个类型来处理运行时错误：

```cpp
// 用 Result 处理除法（可能除以零）
Result<double> safe_divide(double a, double b) {
    if (b == 0.0) {
        return Result<double>::err("Division by zero");
    }
    return Result<double>::ok(a / b);
}

auto r1 = safe_divide(10.0, 2.0);
if (r1.is_ok()) {
    std::cout << r1.value() << "\n";  // 5
}

auto r2 = safe_divide(10.0, 0.0);
if (r2.is_err()) {
    std::cout << "Error: " << r2.error() << "\n";  // Error: Division by zero
}

// 带默认值
double result = safe_divide(10.0, 0.0).value_or(0.0);  // 0.0
```

这就是现代 C++ 里处理"可能失败的操作"的惯用方式——不用异常，不用裸裸的错误码，而是用类型系统编码成功或失败两种情况。Rust 语言的 `Result<T, E>` 是这个思想的更彻底实践。

---

## 小结

**类模板的语法**和函数模板一样，用 `template<typename T>` 前缀。类的成员变量、成员函数都可以用 `T`。成员函数如果在类外定义，每个都要加 `template<typename T>`，这很繁琐，所以通常把实现写在类体内。

**类模板的实例化**是显式的——必须写 `Box<int>` 而不是 `Box`（C++17 的 CTAD 可以推导，但显式写更清晰）。`Box<int>` 和 `Box<double>` 是完全不同的类型。

**模板参数默认值**：`template<typename K, typename V = std::string>` 让某些参数可以省略。STL 容器大量使用这个特性（`vector` 的 Allocator，`map` 的 Compare）。

**成员函数模板**：类模板的成员函数可以有独立的模板参数，与类的模板参数无关。常用于"从兼容类型转换"的构造函数。

**模板与继承**：普通类可以继承类模板的具体实例，类模板也可以继承另一个类模板。虚函数在模板类里照常工作，组合使用可以同时获得泛型和多态。

---

下一章讲模板进阶：特化、可变参数模板、`if constexpr`、`constexpr` 编译期计算——这些是理解 v5 项目代码的基础。
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
# 第二十九章：项目 v5——解释器内部模板化

---

第六部分学了三章模板：函数模板、类模板、特化与 `if constexpr`。现在用这些知识对 Taco 解释器做一次内部重构——v5 不增加新的语言功能，而是改进内部实现。

v5 做三件事：

1. **用 `std::variant` 重新设计 TacoValue**：v1 里已经用过 `variant`，但随着值类型增多，需要系统地重新设计
2. **用模板重构环境（Environment）**：让符号表可以存储不同类型的映射
3. **用模板实现通用的 AST 节点访问器（Visitor）**：用模板取代 `dynamic_cast` 链，实现干净的 Visitor 模式

重构之后，代码更干净，扩展更容易，性能略有提升。

---

## 29.1 用 std::variant 实现 Taco 值类型

### v4 的 TacoValue 回顾

v4 的 `TacoValue` 是这样的：

```cpp
// v4 的实现——已经可以工作，但有几个问题
struct TacoArray;
struct TacoMap;
struct TacoFunction;
struct TacoNative;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>
>;
```

这个设计工作得很好，但有几个摩擦点：
- 辅助函数（`is_truthy`、`value_to_string`、运算符）全部堆在 `value.cpp` 里，越来越难维护
- `std::get_if<T>` 的使用散落在各处，同样的模式重复很多次
- 需要一个统一的"访问"（visit）接口，对任意类型的值都能做某种操作

`std::visit` 是 `variant` 的标准配套工具，配合模板可以写出非常整洁的值处理代码。

### std::visit 的用法

`std::visit` 接收一个**访问者**（visitor）和一个 `variant`，用 `variant` 当前持有的实际类型调用访问者：

```cpp
#include <variant>

using Value = std::variant<int, double, std::string>;

Value v = 42;

// 访问者：一个有 operator() 的对象
struct Printer {
    void operator()(int n)                { std::cout << "int: " << n << "\n"; }
    void operator()(double d)             { std::cout << "double: " << d << "\n"; }
    void operator()(const std::string& s) { std::cout << "string: " << s << "\n"; }
};

std::visit(Printer{}, v);  // 输出：int: 42

v = 3.14;
std::visit(Printer{}, v);  // 输出：double: 3.14

v = std::string("hello");
std::visit(Printer{}, v);  // 输出：string: hello
```

访问者可以用一个重载了多个 `operator()` 的结构体，但写起来有点繁琐。C++17 里有一个更简洁的技巧：**overloaded**：

```cpp
// 把多个 lambda 合并成一个重载集合
template<typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;  // 把所有基类的 operator() 引入当前作用域
};

// C++17 的推导指南：让编译器推导模板参数
template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// 使用
Value v = 42;
std::visit(overloaded{
    [](int n)                { std::cout << "int: " << n << "\n"; },
    [](double d)             { std::cout << "double: " << d << "\n"; },
    [](const std::string& s) { std::cout << "string: " << s << "\n"; },
}, v);
```

`overloaded` 继承自所有传入的 lambda 类型，把它们的 `operator()` 全都引入，形成一个重载集。这是模板继承的实际应用。

### 重新设计 value.h

把 `overloaded` 工具和 TacoValue 的辅助函数整合进来：

```cpp
// value.h（v5 版本）
#pragma once
#include <variant>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <stdexcept>

// ── 前向声明 ──────────────────────────────────────────────────
struct TacoArray;
struct TacoMap;
struct TacoFunction;
struct TacoNative;
class  Environment;

// ── TacoValue：所有 Taco 值类型的 variant ─────────────────────
using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>
>;

// ── overloaded 工具 ───────────────────────────────────────────
// 把多个 lambda 合并成一个可以重载的访问者
template<typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// ── 辅助函数 ─────────────────────────────────────────────────
std::string  taco_to_string(const TacoValue& v);
bool         taco_is_truthy(const TacoValue& v);
bool         taco_equal(const TacoValue& a, const TacoValue& b);
TacoValue    taco_add(const TacoValue& a, const TacoValue& b);  // + 运算，支持数字和字符串拼接

// 类型检查辅助
template<typename T>
bool taco_is(const TacoValue& v) {
    return std::holds_alternative<T>(v);
}

// 安全提取（提取失败抛运行时错误）
template<typename T>
const T& taco_get(const TacoValue& v, const std::string& context = "") {
    if (auto* p = std::get_if<T>(&v)) {
        return *p;
    }
    throw std::runtime_error(
        context.empty()
            ? "Type error"
            : "Type error in " + context
    );
}

template<typename T>
T& taco_get_mut(TacoValue& v, const std::string& context = "") {
    if (auto* p = std::get_if<T>(&v)) {
        return *p;
    }
    throw std::runtime_error(
        context.empty()
            ? "Type error"
            : "Type error in " + context
    );
}
```

模板函数 `taco_is<T>` 和 `taco_get<T>` 比直接写 `holds_alternative` 和 `get_if` 清晰得多，而且集中了错误信息的格式。

### 辅助函数的实现

```cpp
// value.cpp（v5 版本）
#include "value.h"
#include "taco_array.h"
#include "taco_map.h"
#include "taco_function.h"

std::string taco_to_string(const TacoValue& v) {
    return std::visit(overloaded{
        [](double d) -> std::string {
            // 整数时不显示小数点
            if (d == static_cast<long long>(d) && !std::isinf(d))
                return std::to_string(static_cast<long long>(d));
            return std::to_string(d);
        },
        [](const std::string& s) -> std::string {
            return s;
        },
        [](bool b) -> std::string {
            return b ? "true" : "false";
        },
        [](std::nullptr_t) -> std::string {
            return "nil";
        },
        [](const std::shared_ptr<TacoArray>& arr) -> std::string {
            return arr->to_string();
        },
        [](const std::shared_ptr<TacoMap>& map) -> std::string {
            return map->to_string();
        },
        [](const std::shared_ptr<TacoFunction>& fn) -> std::string {
            return "<func " + fn->name + ">";
        },
        [](const std::shared_ptr<TacoNative>&) -> std::string {
            return "<native>";
        },
    }, v);
}

bool taco_is_truthy(const TacoValue& v) {
    return std::visit(overloaded{
        [](double d)                              { return d != 0.0; },
        [](const std::string& s)                 { return !s.empty(); },
        [](bool b)                               { return b; },
        [](std::nullptr_t)                       { return false; },
        [](const std::shared_ptr<TacoArray>& a)  { return true; },
        [](const std::shared_ptr<TacoMap>& m)    { return true; },
        [](const std::shared_ptr<TacoFunction>&) { return true; },
        [](const std::shared_ptr<TacoNative>&)   { return true; },
    }, v);
}
```

这种写法的好处是：**所有类型必须有对应的处理分支**。如果将来 `TacoValue` 加了新的类型，但 `overloaded` 里没有新分支，编译器会直接报错——强制你不能忘记处理新类型。这是 `variant` + `visit` 相比用继承 + 虚函数的一个优势：你不能漏掉某个类型。

---

## 29.2 用模板重构符号表与环境

### v4 的 Environment

v4 的环境是一个 `unordered_map<string, TacoValue>` 加上父环境的 `shared_ptr`：

```cpp
class Environment {
public:
    void define(const std::string& name, TacoValue value);
    TacoValue get(const std::string& name);
    void assign(const std::string& name, TacoValue value);

    std::shared_ptr<Environment> parent;

private:
    std::unordered_map<std::string, TacoValue> vars_;
};
```

这个设计是够用的，但有一个问题：`get` 找不到变量时怎么办？要么抛异常，要么返回 nil——两种方案都有缺点（抛异常开销大，返回 nil 会掩盖错误）。

v5 用模板改进 `get`，返回 `std::optional<TacoValue>`：

```cpp
// environment.h（v5 版本）
#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include "value.h"

class Environment {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr)
        : parent_(std::move(parent)) {}

    // 在当前作用域定义新变量
    void define(const std::string& name, TacoValue value) {
        vars_[name] = std::move(value);
    }

    // 查找变量：当前作用域找不到就往上找，找到返回值，找不到返回空
    std::optional<TacoValue> lookup(const std::string& name) const {
        auto it = vars_.find(name);
        if (it != vars_.end()) {
            return it->second;
        }
        if (parent_) {
            return parent_->lookup(name);
        }
        return std::nullopt;
    }

    // 获取变量（找不到抛错误）
    TacoValue get(const std::string& name) const {
        auto result = lookup(name);
        if (!result) {
            throw std::runtime_error("Undefined variable: " + name);
        }
        return *result;
    }

    // 赋值：找到变量的作用域，在那里修改
    bool assign(const std::string& name, TacoValue value) {
        auto it = vars_.find(name);
        if (it != vars_.end()) {
            it->second = std::move(value);
            return true;
        }
        if (parent_) {
            return parent_->assign(name, std::move(value));
        }
        return false;  // 变量不存在
    }

    // 创建子作用域
    std::shared_ptr<Environment> make_child() {
        return std::make_shared<Environment>(shared_from_this());
    }

    std::shared_ptr<Environment> parent() const { return parent_; }

private:
    std::unordered_map<std::string, TacoValue> vars_;
    std::shared_ptr<Environment> parent_;
};
```

`make_child()` 的实现用到了 `shared_from_this()`，这要求 `Environment` 继承自 `std::enable_shared_from_this<Environment>`：

```cpp
class Environment : public std::enable_shared_from_this<Environment> {
    // ... 同上
};
```

`enable_shared_from_this` 让对象可以安全地获取指向自己的 `shared_ptr`，不会创建第二套引用计数。这是在对象内部需要 `shared_ptr<this>` 时的标准做法。

### 模板化的注册接口

内置函数注册也可以用模板简化。v4 里每次注册都要手动包装 lambda：

```cpp
// v4 的手动注册方式
env->define("print", TacoValue{std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args) -> TacoValue {
        // ...
        return nullptr;
    }
)});
```

用模板写一个辅助函数，让注册更简洁：

```cpp
// 注册内置函数的辅助模板
template<typename Fn>
void register_builtin(Environment& env, const std::string& name, Fn fn) {
    env.define(name, TacoValue{
        std::make_shared<TacoNative>(std::function<TacoValue(std::vector<TacoValue>)>(fn))
    });
}

// 使用
void register_builtins(Environment& env) {
    register_builtin(env, "print", [](std::vector<TacoValue> args) -> TacoValue {
        for (auto& a : args) std::cout << taco_to_string(a);
        std::cout << "\n";
        return nullptr;
    });

    register_builtin(env, "type", [](std::vector<TacoValue> args) -> TacoValue {
        if (args.empty()) return std::string("nil");
        return std::visit(overloaded{
            [](double)                            -> TacoValue { return std::string("number"); },
            [](const std::string&)                -> TacoValue { return std::string("string"); },
            [](bool)                              -> TacoValue { return std::string("bool"); },
            [](std::nullptr_t)                    -> TacoValue { return std::string("nil"); },
            [](const std::shared_ptr<TacoArray>&) -> TacoValue { return std::string("array"); },
            [](const std::shared_ptr<TacoMap>&)   -> TacoValue { return std::string("map"); },
            [](const std::shared_ptr<TacoFunction>&) -> TacoValue { return std::string("func"); },
            [](const std::shared_ptr<TacoNative>&)   -> TacoValue { return std::string("func"); },
        }, args[0]);
    });

    register_builtin(env, "len", [](std::vector<TacoValue> args) -> TacoValue {
        if (args.empty()) throw std::runtime_error("len() requires an argument");
        return std::visit(overloaded{
            [](const std::string& s)              -> TacoValue { return (double)s.size(); },
            [](const std::shared_ptr<TacoArray>& a) -> TacoValue { return (double)a->elements.size(); },
            [](const std::shared_ptr<TacoMap>& m)   -> TacoValue { return (double)m->entries.size(); },
            [](const auto&) -> TacoValue {
                throw std::runtime_error("len() only works on string, array, and map");
                return nullptr;
            },
        }, args[0]);
    });
}
```

`register_builtin` 用 `Fn` 接收任意可调用类型（lambda、函数指针等），内部转换成 `std::function`。这比每次都手写 `make_shared<TacoNative>` 简洁很多。

---

## 29.3 用模板实现通用的 AST 节点访问器

### 访问者模式（Visitor Pattern）

v2 里用了一个简化版：让每个 AST 节点有 `evaluate()` 虚函数。这个设计有个问题：如果需要对同一棵 AST 做多种操作（求值、打印、类型检查、代码生成……），就要在每个节点类里加很多虚函数，节点类会越来越臃肿。

标准的访问者模式把操作从节点里分离出来：

- **节点类**只负责存储数据和调用 `accept(visitor)`
- **访问者类**实现对每种节点的具体操作

```
AST 节点：NumberLiteral, BinaryExpr, IfStmt, ...
  每个节点：accept(Visitor&)
  
访问者：Evaluator, Printer, TypeChecker, ...
  每个访问者：对每种节点实现 visit() 方法
```

这样，新增操作只需要写一个新的访问者类，不需要修改任何节点类。

### 用模板实现访问者基类

在不用模板的情况下，访问者模式需要手动列出所有节点类型：

```cpp
// 传统方式：为每种节点类型写一个 visit 函数
struct Visitor {
    virtual TacoValue visit(const NumberLiteralNode&) = 0;
    virtual TacoValue visit(const StringLiteralNode&) = 0;
    virtual TacoValue visit(const BinaryExprNode&) = 0;
    virtual TacoValue visit(const IfStmtNode&) = 0;
    // ... 每增加一种节点，这里就要加一行
};
```

用模板可以稍微改进——用 `if constexpr` 配合类型特征，让节点的 `accept` 函数更通用：

```cpp
// ast.h（v5 的新方式）
#pragma once
#include <memory>
#include <string>
#include "value.h"

// 前向声明访问者
class Evaluator;
class ASTPrinter;

// ── 基类 ──────────────────────────────────────────────────────
struct ASTNode {
    int line = 0;    // 出错时的行号
    virtual ~ASTNode() = default;

    // 求值：由子类实现
    virtual TacoValue evaluate(Evaluator& eval) const = 0;

    // 打印 AST（调试用）
    virtual std::string dump(int indent = 0) const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

// ── 表达式基类 ────────────────────────────────────────────────
struct Expr : ASTNode {};
using ExprPtr = std::unique_ptr<Expr>;

// ── 语句基类 ─────────────────────────────────────────────────
struct Stmt : ASTNode {};
using StmtPtr = std::unique_ptr<Stmt>;

// ── 字面量节点 ────────────────────────────────────────────────

struct NumberLiteralNode : Expr {
    double value;
    explicit NumberLiteralNode(double v, int ln = 0) : value(v) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Number(" + std::to_string(value) + ")";
    }
};

struct StringLiteralNode : Expr {
    std::string value;
    explicit StringLiteralNode(std::string v, int ln = 0) : value(std::move(v)) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "String(\"" + value + "\")";
    }
};

struct BoolLiteralNode : Expr {
    bool value;
    explicit BoolLiteralNode(bool v, int ln = 0) : value(v) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + std::string(value ? "True" : "False");
    }
};

struct NilLiteralNode : Expr {
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Nil";
    }
};

// ── 变量节点 ─────────────────────────────────────────────────

struct VariableNode : Expr {
    std::string name;
    explicit VariableNode(std::string n, int ln = 0) : name(std::move(n)) { line = ln; }
    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Var(" + name + ")";
    }
};

// ── 二元表达式 ────────────────────────────────────────────────

struct BinaryExprNode : Expr {
    ExprPtr left;
    std::string op;
    ExprPtr right;

    BinaryExprNode(ExprPtr l, std::string o, ExprPtr r, int ln = 0)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) { line = ln; }

    TacoValue evaluate(Evaluator& eval) const override;
    std::string dump(int indent = 0) const override {
        return std::string(indent, ' ') + "Binary(" + op + ")\n"
             + left->dump(indent + 2) + "\n"
             + right->dump(indent + 2);
    }
};
```

### 用 if constexpr 实现通用的 dump

为了演示 `if constexpr` 的实际应用，来写一个通用的缩进打印函数：

```cpp
// 通用的 AST 打印辅助：根据节点类型选择不同的打印方式
template<typename Node>
std::string dump_node(const Node& node, int indent) {
    std::string prefix(indent, ' ');

    if constexpr (std::is_same_v<Node, NumberLiteralNode>) {
        return prefix + "Number(" + std::to_string(node.value) + ")";
    } else if constexpr (std::is_same_v<Node, StringLiteralNode>) {
        return prefix + "String(\"" + node.value + "\")";
    } else if constexpr (std::is_same_v<Node, BinaryExprNode>) {
        return prefix + "Binary(" + node.op + ")\n"
             + node.left->dump(indent + 2) + "\n"
             + node.right->dump(indent + 2);
    } else {
        // 兜底：只打印类型名
        return prefix + "[Unknown Node]";
    }
}
```

这里 `if constexpr` 根据 `Node` 的类型在编译期决定打印格式——对 `NumberLiteralNode` 访问 `.value`，对 `BinaryExprNode` 访问 `.op`、`.left`、`.right`。如果用普通 `if`，所有分支都会被编译，但 `NumberLiteralNode` 没有 `.op` 成员，会编译失败。

---

## 29.4 模板在这里解决了什么问题

回顾一下 v5 里模板的三个应用，以及它们各自解决了什么问题：

**`overloaded` + `std::visit`**

解决了 `variant` 处理代码重复的问题。每次处理 `TacoValue` 都要 `if (holds_alternative<double>)...` 一长串，`visit` + `overloaded` 把所有类型的处理集中在一个地方，而且如果漏掉某种类型，编译器会报错。

**`taco_is<T>` 和 `taco_get<T>` 模板函数**

把散落在求值器各处的 `get_if`/`holds_alternative` 调用统一成一个接口，错误信息也更清晰。`context` 参数让错误定位更精准。

**`register_builtin<Fn>` 模板函数**

把每次注册内置函数都要写的 `make_shared<TacoNative>(function<...>(lambda))` 封装起来，调用端只需要写内置函数的名字和实现 lambda。新增内置函数从十行代码变成三行。

**`if constexpr` 在 `dump_node` 里**

让一个函数可以根据节点类型采用不同的打印逻辑，而且每个分支只访问该类型实际存在的成员，不需要基类预先声明所有可能用到的成员。

---

## 29.5 性能对比：重构前后的差异

v5 的重构对性能的影响主要体现在两个地方：

### 1. `std::visit` vs `dynamic_cast` 链

v1 里用过 `dynamic_cast` 来识别节点类型：

```cpp
// v1 的方式：O(n)，每次要试每种类型直到匹配
TacoValue evaluate(const Expr* e) {
    if (auto* n = dynamic_cast<const NumberLiteralNode*>(e)) return eval_number(n);
    if (auto* n = dynamic_cast<const BinaryExprNode*>(e))   return eval_binary(n);
    // ...
}
```

v2 改成了虚函数（查 vtable 一次，`O(1)`）。`std::variant` + `std::visit` 也是 `O(1)` 的——`variant` 内部存了一个标记（通常是整数），`visit` 根据这个标记做一次跳转（类似 switch，但更安全）。

两者性能相近，但 `variant` 的好处是**不需要基类**：`TacoValue` 里的 `double`、`std::string` 不是类，不需要继承任何东西，内存布局也更紧凑。

### 2. 内存布局

用继承实现的多态值类型，每个值都需要：
- 堆上一个对象（`new`）
- 一个 vtable 指针（通常 8 字节）
- 引用计数（如果用 `shared_ptr`）

`std::variant` 的内存布局：
- 一块栈上的内存，大小等于最大成员类型的大小
- 一个标记整数（通常 1-4 字节）
- 没有堆分配，没有 vtable，没有引用计数

对于像 `double` 和 `std::string` 这样的基本类型，`variant` 直接存在栈上，比 `shared_ptr` 包装的堆对象快很多（省去了分配和引用计数的开销）。

用 `<chrono>` 做一个简单的对比测试：

```cpp
#include <chrono>
#include <iostream>
#include <variant>
#include <string>

using TacoValue = std::variant<double, std::string, bool, std::nullptr_t>;

// 求和：用 variant 存数字
double sum_variant(int n) {
    TacoValue acc = 0.0;
    for (int i = 0; i < n; ++i) {
        std::get<double>(acc) += i;
    }
    return std::get<double>(acc);
}

int main() {
    const int N = 10'000'000;

    auto t1 = std::chrono::high_resolution_clock::now();
    double result = sum_variant(N);
    auto t2 = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "Sum: " << result << "\n";
    std::cout << "Time: " << ms << " ms\n";

    return 0;
}
```

在 Release 模式（`-O2`）下，`variant` 里的 `double` 几乎和裸 `double` 一样快——编译器能优化掉大部分 `variant` 的开销。

### 真正的性能瓶颈在哪里

测量之后会发现，Taco v5 的性能瓶颈不在 `variant`，而在：

1. **字符串操作**：每次字符串拼接都创建新的 `std::string`，对于 `"Hello, " + name + "!"` 这类插值，中间有多次临时字符串
2. **`unordered_map` 的变量查找**：每次变量读写都要做哈希和比较
3. **AST 节点的虚函数调用**：虚函数调用有 vtable 查找和间接跳转的开销

这些才是真正值得优化的地方。字节码虚拟机（把 AST 编译成指令数组，然后解释执行）可以消除虚函数调用的开销，但那是更高级的话题，留给第三十七章讨论。

---

## 测试 v5

用一段稍复杂的 Taco 脚本测试 v5 的核心功能——重点验证类型处理和错误信息：

```taco
// test_v5.taco

// 基本类型和 type()
var n = 42;
var s = "hello";
var b = true;
var arr = [1, 2, 3];
var m = {"key": "value"};

print(type(n));    // number
print(type(s));    // string
print(type(b));    // bool
print(type(arr));  // array
print(type(m));    // map

// 类型安全的运算
func safe_add(a, b) {
    if (type(a) != type(b)) {
        return "type mismatch";
    }
    return a + b;
}

print(safe_add(1, 2));       // 3
print(safe_add("a", "b"));   // ab
print(safe_add(1, "b"));     // type mismatch

// len() 对不同类型的行为
print(len("hello"));         // 5
print(len([1, 2, 3, 4]));    // 4
print(len({"a": 1, "b": 2})); // 2

// variant 值的完整生命周期：创建、传递、返回
func make_pair(k, v) {
    return {"key": k, "value": v};
}

var pair = make_pair("name", "Miguel");
print(pair["key"]);    // name
print(pair["value"]);  // Miguel

// 闭包 + 类型
func counter(start) {
    var count = start;
    return {
        inc:  func() { count = count + 1; return count; },
        dec:  func() { count = count - 1; return count; },
        get:  func() { return count; },
    };
}

var c = counter(10);
print(c["inc"]());  // 11
print(c["inc"]());  // 12
print(c["dec"]());  // 11
print(c["get"]());  // 11
```

### 预期输出

```
number
string
bool
array
map
3
ab
type mismatch
5
4
2
name
Miguel
11
12
11
11
```

---

## 小结

v5 是一次内部重构，没有新的语言特性，但代码质量有明显提升。

**`overloaded` + `std::visit`** 是 `std::variant` 的标准配套用法。`overloaded` 本身只有几行代码（模板继承 + using），但它让 `visit` 可以直接接收 lambda 列表，不需要手写结构体。所有类型必须有对应的处理分支，漏掉会在编译时报错——比虚函数更安全。

**`taco_is<T>` 和 `taco_get<T>`** 把类型检查和提取操作封装成模板函数，错误信息集中，调用端干净。

**`register_builtin<Fn>`** 用模板接收任意可调用类型，让注册内置函数的代码从繁琐的样板代码变成一行。

**`if constexpr`** 在需要对不同类型做不同操作、但又不想为每种类型写单独函数的场合非常有用。它是模板里的"条件编译"，没有被选中的分支根本不会编译。

**性能上**，`variant` 里的基本类型（`double`、`bool`）直接存在栈上，没有堆分配，在 Release 模式下编译器能充分优化。真正的性能瓶颈在字符串操作和变量查找，不在类型分发本身。

---

第六部分到这里结束。第七部分进入多线程，学完之后 v6 会用线程实现 REPL，让 Taco 有一个交互式的游乐场。
# 第三十章：并发基础

---

第七部分进入**多线程**（multithreading）。

并发是现代程序的基本需求——响应用户输入的同时处理后台任务，同时服务多个网络连接，利用多核 CPU 加速计算。C++ 从 C++11 开始提供了标准的多线程支持，不再依赖平台特定的 API（pthread、Windows Thread 等）。

这一章讲最基础的部分：线程是什么，怎么创建，怎么管理，什么时候会出问题。

---

## 30.1 进程与线程的区别

在讲 `std::thread` 之前，先把概念搞清楚。

**进程**（process）是操作系统资源分配的基本单位。每个进程有自己独立的：
- 虚拟地址空间（内存）
- 文件描述符
- 信号处理
- 用户权限

进程之间默认是隔离的——一个进程崩溃不会影响另一个进程，一个进程不能直接访问另一个进程的内存。

**线程**（thread）是进程内的执行单元，同一进程内的线程共享：
- 内存（堆、全局变量、代码段）
- 文件描述符
- 进程的所有资源

每个线程有自己私有的：
- 栈（每个线程的局部变量）
- 寄存器状态（程序计数器、栈指针等）
- 线程局部存储（`thread_local` 变量）

```
进程：一栋楼
  线程：楼里不同的人
  共享：楼道、电梯、厨房（内存、文件）
  私有：自己的房间（栈）
```

线程共享内存的好处是通信成本低——一个线程修改一个全局变量，另一个线程立刻能看到。但这也是危险所在：两个线程同时修改同一块内存，结果是未定义的。这就是**竞态条件**（race condition），下面会详细讲。

在 Python 里，虽然有 `threading` 模块，但由于 GIL（全局解释器锁），Python 线程无法真正并行地执行 Python 代码（I/O 密集型任务除外）。C++ 的线程是真正的操作系统线程，可以在多核 CPU 上并行运行。

---

## 30.2 std::thread：创建和管理线程

### 创建线程

```cpp
#include <thread>
#include <iostream>

// 最简单的线程：传入一个函数
void hello() {
    std::cout << "Hello from thread!\n";
}

int main() {
    std::thread t(hello);  // 创建线程，立刻开始执行 hello()

    std::cout << "Hello from main!\n";

    t.join();  // 等待线程 t 完成
    return 0;
}
```

输出可能是：
```
Hello from main!
Hello from thread!
```
也可能是：
```
Hello from thread!
Hello from main!
```
也可能两行混在一起（`std::cout` 不是线程安全的，这里暂时忽略）。线程调度由操作系统决定，你无法控制哪个线程先运行。

### 传递参数

```cpp
void print_n(int n, const std::string& msg) {
    for (int i = 0; i < n; ++i) {
        std::cout << msg << "\n";
    }
}

// 参数紧跟在函数后面传入
std::thread t(print_n, 3, "hello");
t.join();
```

**注意**：传给 `std::thread` 的参数会被**拷贝**（或移动）到线程内部。如果想传引用，必须用 `std::ref()` 包装：

```cpp
void increment(int& n) {
    n += 1;
}

int x = 0;
std::thread t(increment, std::ref(x));  // 用 std::ref 传引用
t.join();
std::cout << x << "\n";  // 1
```

不用 `std::ref` 的话，`increment` 会收到 `x` 的一个拷贝，修改不会反映到 `x` 上——而且对于接收非 const 引用的函数，不用 `std::ref` 会导致编译错误（因为 `thread` 构造函数试图把右值绑定到左值引用）。

### 用 Lambda 创建线程

更常见的做法是用 Lambda，代码更直接：

```cpp
int result = 0;

std::thread t([&result]() {
    // 这里可以访问外部变量（通过引用捕获）
    result = 42;
});

t.join();
std::cout << result << "\n";  // 42
```

**危险**：如果 Lambda 通过引用捕获了外部变量，而线程在那些变量销毁后还在运行，就会出现悬空引用：

```cpp
std::thread make_bad_thread() {
    int local = 42;

    // 错误：Lambda 捕获了 local 的引用
    // make_bad_thread 返回后，local 销毁了，但线程可能还在运行
    return std::thread([&local]() {
        std::cout << local << "\n";  // 未定义行为！local 已经销毁
    });
}
```

如果要在 Lambda 里使用外部数据，要么值捕获（`[=]`），要么确保数据的生命周期比线程长。

### 传递可移动对象

`std::thread` 只能移动，不能拷贝：

```cpp
std::thread t1(hello);

// std::thread t2 = t1;  // 错误！thread 不可拷贝

std::thread t2 = std::move(t1);  // 可以：把线程所有权从 t1 移到 t2
// t1 现在是空的（joinable() 返回 false）
t2.join();
```

线程对象可以放进容器里：

```cpp
std::vector<std::thread> threads;

for (int i = 0; i < 4; ++i) {
    threads.emplace_back([i]() {
        std::cout << "Thread " << i << "\n";
    });
}

for (auto& t : threads) {
    t.join();
}
```

---

## 30.3 线程的生命周期：join 与 detach

线程创建后，必须在销毁之前调用 `join()` 或 `detach()`，否则程序会调用 `std::terminate()`（通常是崩溃）。

### join()

`join()` 让当前线程等待目标线程完成：

```cpp
std::thread t(some_function);
// 主线程继续做自己的事
do_other_work();
// 等待 t 完成，之后才继续
t.join();
// t 完成后，t.joinable() 返回 false
```

`join()` 是同步的——调用方会阻塞，直到目标线程退出。

### detach()

`detach()` 让线程在后台独立运行，不再与 `thread` 对象关联：

```cpp
std::thread t(background_task);
t.detach();  // t 和后台线程脱钩

// t 销毁了，但后台线程仍在运行
// 程序结束时，后台线程会被强制终止
```

detach 之后的线程叫**守护线程**（daemon thread）。它在后台独立运行，程序结束时会被强制终止（不管线程是否完成）。

**一般情况下，优先用 join 而不是 detach**。detach 的线程很难调试——如果它访问了已销毁的资源，就是未定义行为，而且错误可能很难复现。

### 用 RAII 包装线程

如果一个函数里创建了线程，但在 `join()` 之前抛出了异常，线程对象会在析构时因为没有 join/detach 而触发 `terminate()`。用 RAII 来保证线程总是被 join：

```cpp
class ThreadGuard {
public:
    explicit ThreadGuard(std::thread& t) : t_(t) {}

    ~ThreadGuard() {
        if (t_.joinable()) {
            t_.join();  // 析构时确保 join
        }
    }

    // 不可拷贝、不可移动
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

private:
    std::thread& t_;
};

void risky_function() {
    std::thread t(some_work);
    ThreadGuard guard(t);  // RAII 保证 t 一定被 join

    do_something_that_might_throw();
    // 即使这里抛出异常，guard 的析构函数也会调用 t.join()
}
```

C++20 引入了 `std::jthread`（joining thread），它在析构时自动 join，还支持协作式停止（cooperative cancellation）。如果用 C++20，优先用 `jthread` 而不是裸的 `thread`。

---

## 30.4 竞态条件：什么时候出问题

竞态条件是多线程编程里最常见、最难调试的问题。

### 最简单的例子

```cpp
#include <thread>
#include <iostream>

int counter = 0;  // 全局变量，两个线程都会访问

void increment() {
    for (int i = 0; i < 100000; ++i) {
        counter++;  // 看起来是原子操作，实际上不是！
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    // 期望：200000
    // 实际：每次运行结果不同，通常小于 200000
    std::cout << counter << "\n";
    return 0;
}
```

运行结果会是一个不确定的值，通常小于 200000。

原因是 `counter++` 不是原子操作——它其实分三步：

```
1. 从内存读取 counter 的值到寄存器（load）
2. 寄存器里的值加 1（add）
3. 把寄存器里的值写回内存（store）
```

当两个线程交替执行时：

```
线程 1：load counter → 100
线程 2：load counter → 100  （线程 1 还没写回！）
线程 1：add → 101
线程 1：store 101 → counter
线程 2：add → 101           （基于旧的值 100！）
线程 2：store 101 → counter  （把线程 1 的更新覆盖了！）

结果：counter = 101，但应该是 102
```

这就是**竞态条件**（race condition）——多个线程同时访问同一块内存，而且至少有一个是写操作，导致结果取决于线程的执行顺序（"谁快谁赢"）。

### 为什么竞态条件难以发现

竞态条件只在特定的调度顺序下出现，而这个顺序是不确定的：
- 在开发机上运行正确，在生产服务器上崩溃
- 加了打印语句之后好了（因为 `cout` 改变了时序）
- 在调试器里单步运行永远不出问题（因为调试器让线程串行化了）

这类 bug 叫 **Heisenbug**（海森堡 bug）——观察它就改变了它的行为。

### 数据竞争（Data Race）

C++ 标准把"多个线程同时访问同一块内存，且至少有一个是写操作"定义为**数据竞争**（data race）。数据竞争是**未定义行为**——不只是"结果不正确"，而是编译器可以假设数据竞争不会发生，然后做出各种破坏性的优化。

实际上这意味着：存在数据竞争的程序，什么结果都可能发生，包括随机崩溃、静默数据损坏、甚至看起来工作但隐藏着错误。

解决数据竞争有三种主要手段：
- **互斥锁**（mutex）：让同一时间只有一个线程能访问共享数据（下一章）
- **原子操作**（atomic）：让简单的读写操作变成原子的（第三十二章）
- **避免共享**：每个线程只用自己的数据，线程间通过消息传递而不是共享内存通信

### 用 AddressSanitizer 检测竞态

`-fsanitize=thread`（ThreadSanitizer，TSan）是 GCC 和 Clang 提供的线程安全检测工具：

```bash
g++ -std=c++17 -fsanitize=thread -g main.cpp -o main
./main
```

TSan 会在运行时检测数据竞争，并打印详细报告：

```
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x... by thread T2:
    #0 increment() main.cpp:8
    ...
  Previous write of size 4 at 0x... by thread T1:
    #0 increment() main.cpp:8
    ...
```

在开发阶段开启 TSan 是个好习惯——它能在程序跑起来之前就发现竞态条件，比用眼睛看代码可靠得多。

---

## 一个更贴近实际的例子

来看一个 Taco 解释器里会遇到的场景：多个线程共享同一个 `Environment`（变量环境），分别读写变量。

```cpp
// 错误示例：不加保护地共享 Environment
#include <thread>
#include <unordered_map>
#include <string>
#include <iostream>

struct Environment {
    std::unordered_map<std::string, double> vars;

    void set(const std::string& name, double value) {
        vars[name] = value;  // 不安全：多线程同时写会崩溃
    }

    double get(const std::string& name) {
        return vars.at(name);  // 不安全：读写同时发生会返回损坏的数据
    }
};

Environment shared_env;

void worker(int id) {
    for (int i = 0; i < 1000; ++i) {
        std::string key = "var" + std::to_string(id);
        shared_env.set(key, i);  // 多个线程同时调用 set，会崩溃
    }
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);

    t1.join();
    t2.join();
    t3.join();

    // 大概率在运行过程中崩溃，原因：
    // unordered_map 在插入时可能 rehash（重新分配内存），
    // 如果另一个线程同时在读，就访问了已经释放的内存
    std::cout << "Done\n";
    return 0;
}
```

`unordered_map` 不是线程安全的。多个线程同时写，或者一个写一个读，都会导致崩溃。

修复方法是加锁——下一章的内容。

---

## 线程的开销

创建线程是有开销的。一个典型的线程创建耗时在微秒级别（比函数调用慢几个数量级）。原因包括：
- 向操作系统申请栈内存（通常 1-8 MB）
- 操作系统内核的初始化
- 调度器的介入

对于需要大量短小任务的场景，不应该为每个任务都创建一个线程，而应该用**线程池**（thread pool）——预先创建固定数量的线程，把任务分配给空闲的线程。这是第三十三章里 REPL 的实现方式之一。

---

## 小结

**进程和线程**：进程是独立的资源容器，线程是进程内的执行单元。同一进程内的线程共享内存，通信成本低，但需要同步。

**`std::thread`**：构造时传入函数和参数，立刻开始执行。引用参数需要用 `std::ref` 包装。线程不可拷贝，只能移动。

**join 和 detach**：创建的线程必须在销毁前 join 或 detach。`join()` 等待线程完成，`detach()` 让线程在后台独立运行。优先用 RAII 包装确保 join，或者用 C++20 的 `jthread`。

**竞态条件**：多个线程同时访问同一内存，且至少有一个写操作，导致结果不确定。`counter++` 看起来是一步，实际上是三步（load-add-store），中间可以被其他线程打断。竞态条件是未定义行为，开发时用 ThreadSanitizer（`-fsanitize=thread`）检测。

---

下一章讲解决竞态条件的主要工具：互斥锁（mutex）和条件变量（condition_variable）。
# 第三十一章：同步原语

---

上一章看到了竞态条件：多个线程同时访问共享数据，结果不确定。这一章讲解决方案：**同步原语**（synchronization primitives）——让多个线程对共享数据的访问有序进行的工具。

C++ 标准库提供了三类主要的同步工具：
- `std::mutex`：互斥锁，最基础的同步工具
- `std::lock_guard` / `std::unique_lock`：RAII 风格的锁管理
- `std::condition_variable`：线程间通知和等待

---

## 31.1 std::mutex 与 std::lock_guard

### 互斥锁的基本概念

**互斥**（mutual exclusion）：同一时间只有一个线程能访问某段代码。互斥锁（mutex）是实现互斥的工具。

想象一个公共厕所只有一个坑位——门上有一个锁。进去的人先锁门，出来时解锁，下一个人才能进去。mutex 就是这个门锁，被保护的代码是坑位，每次只有一个人能进去。

```cpp
#include <mutex>
#include <thread>
#include <iostream>

int counter = 0;
std::mutex mtx;  // 保护 counter 的互斥锁

void increment() {
    for (int i = 0; i < 100000; ++i) {
        mtx.lock();    // 上锁：其他线程在这里阻塞等待
        counter++;     // 临界区：同一时间只有一个线程在这里
        mtx.unlock();  // 解锁：允许其他线程进入
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    std::cout << counter << "\n";  // 总是 200000
    return 0;
}
```

现在结果总是正确的 200000。`lock()` 和 `unlock()` 之间的代码叫**临界区**（critical section）——同一时间只有一个线程能在里面。

### 为什么不应该手动 lock/unlock

上面的代码有一个隐患：如果临界区里抛出了异常，`unlock()` 永远不会被调用，互斥锁保持锁定状态，其他线程永远等待——这叫**死锁**（deadlock）。

解决方案是 RAII：`std::lock_guard`。

### std::lock_guard

`std::lock_guard` 在构造时加锁，在析构时解锁，不管是正常退出还是异常退出：

```cpp
#include <mutex>

int counter = 0;
std::mutex mtx;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> guard(mtx);
        // 构造时自动 lock()
        counter++;
        // 析构时自动 unlock()，离开作用域就解锁
    }
}
```

`lock_guard` 简单、高效、安全。**在绝大多数情况下，优先用 `lock_guard` 而不是手动 lock/unlock。**

注意 `lock_guard` 的作用域就是锁的持有范围——锁会在 `guard` 析构时释放（离开 for 循环体）。可以用 `{}` 控制锁的粒度：

```cpp
void process_data() {
    // 做一些不需要锁的工作
    auto local_data = prepare_data();

    {
        // 只在这个块内持有锁
        std::lock_guard<std::mutex> guard(mtx);
        shared_data = local_data;
    }
    // 锁已释放，可以继续做其他工作

    post_process();  // 这里不需要锁
}
```

缩短持有锁的时间，减少其他线程等待的时间，是提高并发性能的基本原则。

### C++17 的模板参数推导

C++17 里，`lock_guard` 的模板参数可以省略（类模板参数推导）：

```cpp
// C++17 之前
std::lock_guard<std::mutex> guard(mtx);

// C++17 之后
std::lock_guard guard(mtx);  // 编译器推导出 std::mutex
```

本书用 C++17，所以后面都省略 `<std::mutex>`。

---

## 31.2 std::unique_lock：更灵活的锁

`lock_guard` 简单但不够灵活：它一创建就加锁，一析构就解锁，期间无法手动控制。`std::unique_lock` 提供了更多控制：

### 延迟加锁

```cpp
std::mutex mtx;

// 创建但先不加锁
std::unique_lock<std::mutex> lock(mtx, std::defer_lock);

// 做一些准备工作
auto data = prepare_data();

// 手动加锁
lock.lock();
shared_data = data;
lock.unlock();  // 手动解锁

// 继续做其他事
post_process(data);
```

### 尝试加锁

```cpp
std::unique_lock lock(mtx, std::try_to_lock);

if (lock.owns_lock()) {
    // 成功拿到锁
    process_shared_data();
} else {
    // 没拿到锁，做其他事
    process_local_data();
}
```

### 超时加锁

```cpp
#include <chrono>

std::timed_mutex timed_mtx;  // 支持超时的 mutex

std::unique_lock lock(timed_mtx, std::chrono::milliseconds(100));

if (lock.owns_lock()) {
    // 100ms 内拿到锁
} else {
    // 超时，没拿到锁
}
```

### 配合条件变量

`unique_lock` 最重要的应用是配合 `condition_variable`——`lock_guard` 不能和条件变量配合，必须用 `unique_lock`（下一节会讲）。

### lock_guard vs unique_lock

| 特性 | lock_guard | unique_lock |
|------|-----------|-------------|
| 构造时加锁 | 总是 | 可选 |
| 析构时解锁 | 总是 | 总是（如果持有锁） |
| 手动 lock/unlock | 不支持 | 支持 |
| 移动语义 | 不支持 | 支持 |
| 配合条件变量 | 不支持 | 支持 |
| 开销 | 极低 | 略高（存了额外状态） |

**规则**：不需要灵活性时用 `lock_guard`；需要中途解锁、延迟加锁、或配合条件变量时用 `unique_lock`。

---

## 31.3 std::condition_variable：线程间通信

互斥锁解决了"多个线程不能同时访问共享数据"的问题。但有时候线程需要**等待某个条件成立**再继续——比如"等待队列非空"、"等待结果计算完成"。

用忙等待（busy-wait）浪费 CPU：

```cpp
// 错误做法：忙等待，一直占用 CPU
while (queue.empty()) {
    // 啥也不做，一直检查
}
process(queue.front());
```

`std::condition_variable` 提供了正确的方式：让线程挂起，直到被通知条件成立。

### 基本用法

```cpp
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <iostream>

std::mutex mtx;
std::condition_variable cv;
std::queue<int> task_queue;
bool done = false;

// 生产者：往队列里放任务
void producer() {
    for (int i = 0; i < 10; ++i) {
        {
            std::lock_guard lock(mtx);
            task_queue.push(i);
            std::cout << "Produced: " << i << "\n";
        }
        cv.notify_one();  // 通知一个等待的线程：队列有新数据了
    }

    {
        std::lock_guard lock(mtx);
        done = true;
    }
    cv.notify_all();  // 通知所有等待的线程：生产完毕
}

// 消费者：从队列里取任务
void consumer(int id) {
    while (true) {
        std::unique_lock lock(mtx);

        // wait：释放锁，挂起线程，直到 cv.notify 唤醒
        // 被唤醒后重新加锁，然后检查条件
        cv.wait(lock, [](){ return !task_queue.empty() || done; });
        //                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        //                   这是唤醒后的检查条件（防止虚假唤醒）

        if (task_queue.empty()) {
            break;  // 队列空且 done，退出
        }

        int task = task_queue.front();
        task_queue.pop();
        lock.unlock();  // 处理任务前先释放锁

        std::cout << "Consumer " << id << " processed: " << task << "\n";
    }
}

int main() {
    std::thread prod(producer);
    std::thread con1(consumer, 1);
    std::thread con2(consumer, 2);

    prod.join();
    con1.join();
    con2.join();
    return 0;
}
```

### wait 的两种形式

```cpp
// 形式 1：等待通知（可能虚假唤醒）
cv.wait(lock);

// 形式 2：等待通知，且条件满足（推荐）
cv.wait(lock, predicate);
// 等价于：
while (!predicate()) {
    cv.wait(lock);
}
```

**虚假唤醒**（spurious wakeup）：操作系统可能在没有 `notify` 的情况下唤醒等待的线程（这是 POSIX 标准允许的行为）。所以 `wait` 之后必须重新检查条件——这就是带 predicate 的 `wait` 存在的原因。带 predicate 的 `wait` 内部会自动重试，直到条件成立。

### wait_for 和 wait_until

有时候需要等待一段时间，超时就不等了：

```cpp
// 等待最多 100ms
auto status = cv.wait_for(lock, std::chrono::milliseconds(100),
                          []{ return condition_met; });

if (status) {
    // 条件满足
} else {
    // 超时
}
```

### notify_one vs notify_all

- `notify_one()`：唤醒**一个**等待的线程（哪个由操作系统决定）
- `notify_all()`：唤醒**所有**等待的线程

`notify_one` 更高效，但只适合"任意一个线程能处理这个任务"的情况。如果条件变化后所有线程都需要重新检查，用 `notify_all`。

---

## 31.4 死锁：如何产生，如何避免

**死锁**（deadlock）是多个线程互相等待对方释放资源，所有线程都无法继续的状态。

### 最简单的死锁

```cpp
std::mutex mtx1, mtx2;

void thread1_func() {
    std::lock_guard lock1(mtx1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));  // 让另一个线程先锁 mtx2
    std::lock_guard lock2(mtx2);  // 等待 mtx2，但 mtx2 被 thread2 持有
}

void thread2_func() {
    std::lock_guard lock2(mtx2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));  // 让另一个线程先锁 mtx1
    std::lock_guard lock1(mtx1);  // 等待 mtx1，但 mtx1 被 thread1 持有
}

// 结果：两个线程永远等待对方，程序挂死
```

死锁的四个必要条件（Coffman 条件，全部满足才会死锁）：
1. **互斥**：至少一个资源不能共享
2. **占有并等待**：持有一些资源，同时等待其他资源
3. **不可抢占**：已持有的资源只能主动释放，不能被强制夺走
4. **循环等待**：存在线程的循环等待链（A 等 B，B 等 C，C 等 A）

### 避免死锁的几种方法

**方法 1：统一加锁顺序**

如果所有线程都按相同的顺序加锁，就不会出现循环等待：

```cpp
void thread1_func() {
    // 总是先锁 mtx1，再锁 mtx2
    std::lock_guard lock1(mtx1);
    std::lock_guard lock2(mtx2);
}

void thread2_func() {
    // 和 thread1 一样的顺序
    std::lock_guard lock1(mtx1);
    std::lock_guard lock2(mtx2);
}
```

**方法 2：用 std::lock 同时加多个锁**

`std::lock` 能同时加多个锁，内部使用避免死锁的算法（会尝试不同的加锁顺序，直到成功）：

```cpp
void safe_func() {
    // 同时加两个锁，std::lock 保证不会死锁
    std::lock(mtx1, mtx2);

    // lock 加锁后，用 adopt_lock 告诉 lock_guard 锁已经加好了（不要再加）
    std::lock_guard lock1(mtx1, std::adopt_lock);
    std::lock_guard lock2(mtx2, std::adopt_lock);

    // ... 操作共享数据 ...
}
```

C++17 的 `std::scoped_lock` 更优雅，把上面两步合成一步：

```cpp
void safe_func() {
    std::scoped_lock lock(mtx1, mtx2);  // 同时加多个锁，自动避免死锁
    // ...
}
```

`scoped_lock` 是 C++17 引入的，优先使用它。

**方法 3：减少锁的数量**

锁越少，死锁的可能性越低。如果多个共享资源总是一起被访问，用一个锁保护它们：

```cpp
// 不好：两把锁，可能死锁
std::mutex name_mtx, age_mtx;
std::string name;
int age;

// 好：一把锁
struct UserData {
    std::string name;
    int age;
};
std::mutex user_mtx;
UserData user;
```

**方法 4：尽量缩短锁的持有时间，避免嵌套锁**

持有锁的时间越短，发生死锁的窗口越小。嵌套锁（持有一个锁时尝试获取另一个锁）是死锁的温床，尽量避免。

---

## 把这一章用到 Taco 里：线程安全的环境

上一章的 `Environment` 不是线程安全的。给它加锁，让多个线程可以安全地读写变量：

```cpp
// thread_safe_environment.h
#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <shared_mutex>  // C++17：读写锁
#include <memory>
#include "value.h"

class ThreadSafeEnvironment {
public:
    explicit ThreadSafeEnvironment(
        std::shared_ptr<ThreadSafeEnvironment> parent = nullptr
    ) : parent_(std::move(parent)) {}

    void define(const std::string& name, TacoValue value) {
        std::unique_lock lock(mtx_);  // 写操作：独占锁
        vars_[name] = std::move(value);
    }

    std::optional<TacoValue> lookup(const std::string& name) const {
        std::shared_lock lock(mtx_);  // 读操作：共享锁（允许多个读同时进行）
        auto it = vars_.find(name);
        if (it != vars_.end()) {
            return it->second;
        }
        if (parent_) {
            return parent_->lookup(name);
        }
        return std::nullopt;
    }

    TacoValue get(const std::string& name) const {
        auto result = lookup(name);
        if (!result) {
            throw std::runtime_error("Undefined variable: " + name);
        }
        return *result;
    }

    bool assign(const std::string& name, TacoValue value) {
        {
            std::unique_lock lock(mtx_);
            auto it = vars_.find(name);
            if (it != vars_.end()) {
                it->second = std::move(value);
                return true;
            }
        }
        // 没找到，尝试父作用域
        if (parent_) {
            return parent_->assign(name, value);
        }
        return false;
    }

private:
    mutable std::shared_mutex mtx_;  // mutable：允许在 const 函数里加锁
    std::unordered_map<std::string, TacoValue> vars_;
    std::shared_ptr<ThreadSafeEnvironment> parent_;
};
```

`std::shared_mutex`（C++17）是**读写锁**：
- 读操作用 `std::shared_lock`：多个线程可以同时读
- 写操作用 `std::unique_lock`：只有一个线程能写，写时其他读写都阻塞

`mutable std::shared_mutex mtx_`：`mutable` 允许在 `const` 成员函数里修改这个成员（加锁操作不改变"逻辑上的状态"，但需要修改锁对象）。这是 `mutable` 的标准用途之一。

**注意**：这里是为了演示 mutex 的用法。在 Taco v6 的实际设计里，每个线程有自己的 `Environment`（不共享），线程间通过 channel 通信，所以并不真的需要线程安全的 Environment。

---

## 小结

**`std::mutex`**：互斥锁，`lock()` 加锁，`unlock()` 解锁。同一时间只有一个线程持有锁，其他线程在 `lock()` 处阻塞等待。

**`std::lock_guard`**：RAII 风格，构造时加锁，析构时解锁。简单、安全，不可移动。大多数场景的首选。

**`std::unique_lock`**：更灵活的锁，支持延迟加锁、手动解锁、超时加锁、可移动。配合条件变量时必须用它。

**`std::condition_variable`**：让线程等待某个条件成立，而不是忙等待。`wait(lock, pred)` 释放锁并挂起，被 `notify_one` 或 `notify_all` 唤醒后重新加锁并检查条件。记住用带 predicate 的 `wait` 防止虚假唤醒。

**死锁**：多个线程循环等待对方释放锁。避免方法：统一加锁顺序、用 `std::scoped_lock` 同时加多个锁、减少嵌套锁。

**`std::shared_mutex`**：读写锁，允许多个读同时进行，写时独占。`shared_lock` 用于读，`unique_lock` 用于写。

---

下一章讲原子操作（`std::atomic`）和异步任务（`std::future`/`std::async`）——在不需要锁的情况下安全处理简单的共享状态，以及方便地获取异步操作的结果。
# 第三十二章：原子操作与异步

---

上一章用互斥锁解决了共享数据的竞态问题。互斥锁是通用解决方案，但对于简单的计数器、标志位这类场景，锁的开销有点重。这一章讲两个更轻量的工具：

- `std::atomic`：让简单的读写操作变成原子的，不需要锁
- `std::future` / `std::promise` / `std::async`：把异步计算的结果"打包"成一个值，在需要时取出

---

## 32.1 std::atomic：无锁编程入门

### 原子操作的概念

**原子操作**（atomic operation）是不可分割的操作——要么全部完成，要么完全没有发生，中间不会被其他线程打断。

第三十章的 `counter++` 不是原子的，因为它是 load-add-store 三步，中间可以被打断。`std::atomic<int>` 把这三步变成一个真正的原子操作：

```cpp
#include <atomic>
#include <thread>
#include <iostream>

std::atomic<int> counter = 0;  // 原子计数器

void increment() {
    for (int i = 0; i < 100000; ++i) {
        counter++;  // 原子操作：不需要锁，结果总是正确的
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    std::cout << counter << "\n";  // 总是 200000
    return 0;
}
```

### 支持的类型

`std::atomic<T>` 支持整数类型（`int`、`long`、`uint64_t` 等）、指针类型和 `bool`。C++20 扩展到了浮点数（`float`、`double`），但并不是所有平台都有硬件支持（某些平台会退化成锁实现）。

```cpp
std::atomic<int>      ai = 0;
std::atomic<bool>     ab = false;
std::atomic<long>     al = 0L;
std::atomic<uint64_t> au = 0ULL;

// 自定义类型：要求可平凡拷贝（trivially copyable）
// 通常只有简单的 struct 才能用
struct Point { int x, y; };
std::atomic<Point> ap = {0, 0};  // 可能不支持 fetch_add 等操作
```

### 常用操作

```cpp
std::atomic<int> n = 0;

// 读取
int val = n.load();          // 明确的原子读
int val2 = n;                // 隐式转换，等价于 load()

// 写入
n.store(42);                 // 明确的原子写
n = 42;                      // 等价于 store()

// 读取并加法（返回旧值）
int old = n.fetch_add(1);    // 原子 n += 1，返回操作前的值
int old2 = n.fetch_sub(5);   // 原子 n -= 5，返回操作前的值
int old3 = n.fetch_and(0xFF); // 原子 n &= 0xFF
int old4 = n.fetch_or(0x1);   // 原子 n |= 0x1

// ++ 和 -- 操作符（返回新值）
n++;    // 原子 n += 1
n--;    // 原子 n -= 1
++n;    // 同上
--n;    // 同上

// 交换（返回旧值）
int old_val = n.exchange(100);  // 把 n 设为 100，返回旧值

// 比较并交换（CAS，compare-and-swap）
int expected = 100;
bool success = n.compare_exchange_strong(expected, 200);
// 如果 n == expected（100），则把 n 设为 200，返回 true
// 如果 n != expected，则把 expected 改成 n 的当前值，返回 false
```

**CAS（compare_exchange）** 是无锁编程的核心操作。它让"比较-修改"这两步变成原子的，是实现无锁数据结构的基础。

`compare_exchange_strong` 和 `compare_exchange_weak` 的区别：`weak` 版本可能在"应该成功"的时候失败（但效率更高），需要放在循环里重试；`strong` 版本保证如果当前值等于期望值就一定成功。

### atomic 的常见用法

**开关标志**：

```cpp
std::atomic<bool> running = true;

void worker() {
    while (running.load()) {
        do_work();
    }
}

// 在另一个线程里：
void stop() {
    running.store(false);  // 原子写，不需要锁
}
```

**一次性标志（只执行一次）**：

```cpp
std::atomic<bool> initialized = false;

void ensure_initialized() {
    bool expected = false;
    if (initialized.compare_exchange_strong(expected, true)) {
        // 只有一个线程能进这里（CAS 成功的那个）
        do_initialization();
    }
    // 其他线程 CAS 失败，跳过初始化
}
```

**无锁计数器**：

最经典的用法，前面已经看过了。原子计数器比加锁的计数器快很多，因为不涉及操作系统级别的锁调度。

### atomic 的开销

原子操作不是"免费"的——它们通常由特殊的 CPU 指令实现（如 x86 的 `LOCK XADD`、`CMPXCHG`），比普通读写慢，但比加锁快（不涉及操作系统调度，不需要让其他线程挂起）。

在典型的现代 CPU 上：
- 普通整数操作：~1 个时钟周期
- 原子操作（无竞争时）：~5-20 个时钟周期
- 互斥锁（无竞争时）：~20-100 个时钟周期
- 互斥锁（有竞争时）：可能是微秒级

对于简单的标志位和计数器，用 `atomic` 而不是 `mutex`。

---

## 32.2 std::future 与 std::promise

有时候需要在一个线程里启动一个计算，在另一个线程里等待结果——就像下外卖：点好外卖，去做别的事，外卖送到了再取。`std::future` 和 `std::promise` 实现了这个模式。

### 基本概念

- `std::promise<T>`：生产者端，用来设置值（或异常）
- `std::future<T>`：消费者端，用来等待和获取值

```cpp
#include <future>
#include <thread>
#include <iostream>

int main() {
    std::promise<int> prom;          // 创建 promise
    std::future<int> fut = prom.get_future();  // 从 promise 获取 future

    // 在另一个线程里计算结果，然后通过 promise 设置
    std::thread worker([&prom]() {
        // 模拟耗时计算
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int result = 42 * 2;
        prom.set_value(result);  // 设置结果，会唤醒等待 fut 的线程
    });

    // 主线程可以做其他事
    std::cout << "Doing other work...\n";

    // 需要结果时，等待并获取
    int result = fut.get();  // 阻塞直到结果可用
    std::cout << "Result: " << result << "\n";  // 84

    worker.join();
    return 0;
}
```

`future::get()` 的行为：
- 如果结果还没准备好，阻塞等待
- 如果结果已经准备好，立刻返回
- 如果 worker 线程抛出了异常，`get()` 会重新抛出那个异常
- `get()` 只能调用一次（调用后 future 进入无效状态）

### 通过 promise 传递异常

```cpp
std::promise<int> prom;
std::future<int> fut = prom.get_future();

std::thread worker([&prom]() {
    try {
        int result = risky_computation();
        prom.set_value(result);
    } catch (...) {
        // 把异常"打包"传给 future
        prom.set_exception(std::current_exception());
    }
});

try {
    int result = fut.get();  // 如果 worker 抛异常，这里重新抛出
} catch (const std::exception& e) {
    std::cout << "Exception: " << e.what() << "\n";
}

worker.join();
```

这是异步编程里处理错误的标准方式：生产者捕获异常，通过 `promise` 传递给消费者，消费者在 `get()` 时处理。

---

## 32.3 std::async：简单的异步任务

`std::promise` 需要手动管理线程，有点繁琐。`std::async` 是更高层的封装——把一个函数异步执行，返回一个 `future`：

```cpp
#include <future>
#include <iostream>

int heavy_computation(int n) {
    // 模拟耗时计算
    long long sum = 0;
    for (int i = 0; i < n; ++i) sum += i;
    return static_cast<int>(sum % 1000000);
}

int main() {
    // 异步执行：可能在新线程里运行，也可能延迟执行
    std::future<int> fut = std::async(std::launch::async, heavy_computation, 100000000);

    // 同时做其他事
    std::cout << "Computing...\n";
    do_other_work();

    // 等待结果
    int result = fut.get();
    std::cout << "Result: " << result << "\n";
    return 0;
}
```

`std::launch::async` 强制在新线程里异步运行。还有 `std::launch::deferred`，让任务延迟到 `get()` 调用时才执行（懒求值）：

```cpp
// 延迟执行：调用 get() 时才真正计算，在调用方的线程里执行
auto fut = std::async(std::launch::deferred, heavy_computation, 100000000);

// ... 做其他事 ...

int result = fut.get();  // 此时才开始计算，在主线程里
```

不指定 launch policy 时，行为由实现决定（通常等于 `async | deferred`，实现可以选择）：

```cpp
// 实现自己决定（不推荐，行为不确定）
auto fut = std::async(heavy_computation, n);
```

### 并行化多个任务

`std::async` 的典型用法是并行化多个独立的计算：

```cpp
// 串行：总耗时 = t1 + t2 + t3
auto r1 = compute_1();
auto r2 = compute_2();
auto r3 = compute_3();

// 并行：总耗时 ≈ max(t1, t2, t3)
auto fut1 = std::async(std::launch::async, compute_1);
auto fut2 = std::async(std::launch::async, compute_2);
auto fut3 = std::async(std::launch::async, compute_3);

auto r1 = fut1.get();
auto r2 = fut2.get();
auto r3 = fut3.get();
```

在 Taco 里，可以用 `async` 并行执行多个文件的词法分析：

```cpp
// 并行分析多个文件
std::vector<std::string> files = get_all_taco_files();
std::vector<std::future<std::vector<Token>>> futures;

for (const auto& file : files) {
    futures.push_back(std::async(std::launch::async,
        [](const std::string& path) {
            auto source = read_file(path);
            return tokenize(source);
        }, file));
}

// 收集所有结果
std::vector<std::vector<Token>> all_tokens;
for (auto& fut : futures) {
    all_tokens.push_back(fut.get());
}
```

### future 的局限性

`std::future` 有一个重要限制：**`get()` 只能调用一次**。调用后 `future` 变成无效状态。

如果需要多个线程都能获取同一个结果，用 `std::shared_future`：

```cpp
std::promise<int> prom;
std::shared_future<int> shared_fut = prom.get_future().share();

// 多个线程都可以调用 get()
std::thread t1([shared_fut]() { std::cout << shared_fut.get() << "\n"; });
std::thread t2([shared_fut]() { std::cout << shared_fut.get() << "\n"; });

prom.set_value(42);

t1.join();
t2.join();
```

---

## *More About 多线程*：内存模型与内存序

> 第一次读可以跳过。

### 为什么需要内存模型

到目前为止，我们默认了一个假设：一个线程修改了变量，另一个线程能"立刻"看到。这在单核 CPU 上是成立的，但在现代多核 CPU 上不一定——每个核有自己的缓存（L1/L2/L3），CPU 可能把写操作缓存在寄存器或 L1 缓存里，暂时不刷新到主内存。

另外，编译器和 CPU 都会对指令重排序（reordering）——只要不影响单线程的语义，就可以打乱执行顺序，提高流水线效率。

```cpp
// 写入 data 和 ready 的顺序可能被重排
int data = 0;
bool ready = false;

// 线程 1
void producer() {
    data = 42;       // 操作 A
    ready = true;    // 操作 B
    // CPU 可能把 B 排在 A 之前执行！
}

// 线程 2
void consumer() {
    while (!ready) {}  // 等待
    // 即使 ready 是 true，data 可能还没有被写入（仍然是 0）！
    use(data);
}
```

单线程程序里，`data = 42` 发生在 `ready = true` 之前，结果总是正确的。但多线程程序里，这个顺序对其他线程可能不可见。

### 内存序（Memory Order）

C++ 的原子操作可以指定**内存序**（memory order），控制重排序的范围：

```cpp
std::atomic<int>  data;
std::atomic<bool> ready;

// 线程 1（生产者）
data.store(42, std::memory_order_relaxed);  // 不保证顺序
ready.store(true, std::memory_order_release);  // release：保证这之前的所有写操作都对其他线程可见

// 线程 2（消费者）
while (!ready.load(std::memory_order_acquire)) {}  // acquire：保证这之后的读操作看到生产者的写操作
int val = data.load(std::memory_order_relaxed);  // 现在 val 一定是 42
```

六种内存序（从宽松到严格）：

```
memory_order_relaxed   → 只保证原子性，不保证顺序
memory_order_consume   → 依赖当前操作的操作保持顺序（C++20 基本废弃）
memory_order_acquire   → 之后的读写不能排到之前
memory_order_release   → 之前的读写不能排到之后
memory_order_acq_rel   → acquire + release
memory_order_seq_cst   → 顺序一致，所有原子操作全局有序（默认）
```

默认的 `memory_order_seq_cst`（顺序一致性）是最严格的，也是最慢的——它要求所有线程看到所有原子操作的顺序完全一致。

在大多数应用代码里，不需要手动指定内存序——默认的 `seq_cst` 就够了。手动优化内存序是高级主题，需要深入理解 CPU 内存模型，容易出错。只有性能分析表明内存序是瓶颈时，才值得考虑。

### happen-before 关系

内存模型的核心概念是 **happens-before**（先于发生）关系：如果操作 A happens-before 操作 B，那么 B 能看到 A 的结果。

`release-acquire` 建立了 happens-before：`release` 操作 happens-before 对应的 `acquire` 操作。这正是生产者-消费者模式的正确实现方式。

详细的内存模型是 C++ 里最复杂的话题之一，这里只是点到为止。实际项目里，绝大多数场景用 mutex 或者默认的 `seq_cst` 原子操作就够了。

---

## 小结

**`std::atomic<T>`**：让读写变成原子操作，不需要锁。支持整数和指针类型。`fetch_add`、`fetch_sub`、`compare_exchange_strong` 是最常用的操作。适合简单的计数器和标志位。

**`std::promise<T>` / `std::future<T>`**：生产者-消费者的值传递。`promise::set_value()` 设置结果，`future::get()` 等待并获取结果（只能调用一次）。异常也可以通过 `set_exception` 传递。

**`std::async`**：最简单的异步执行方式。`std::launch::async` 强制在新线程里运行，`std::launch::deferred` 延迟到 `get()` 时执行。返回 `future`，通过 `get()` 获取结果。适合并行化多个独立计算。

**内存序**（了解）：原子操作默认 `seq_cst`（顺序一致），最安全但最慢。`release`/`acquire` 对可以在不需要全局一致性的场景提高性能。日常编程用默认值即可。

---

下一章是第七部分的项目章节：给 Taco 实现 REPL（Read-Eval-Print Loop）和多线程支持（v6）。这一章会把前三章的知识综合用起来。
# 第三十三章：项目 v6——REPL 与并发安全

---

第七部分学了三章多线程：`std::thread`、同步原语（mutex、condition_variable）、原子操作和异步（atomic、future、async）。现在把这些知识用到 Taco 项目里——v6 的核心是给 Taco 实现一个完整的 **REPL**（Read-Eval-Print Loop，读取-求值-打印循环）。

v6 结束时，Taco 能做到这样：

```
🌮 Taco 0.1.0
   It works on my machine.
> var name = "Miguel";
> print("Hola, {name}!");
Hola, Miguel!
> 1 + 1
2
> func fib(n) { if (n <= 1) { return n; } return fib(n-1) + fib(n-2); }
> fib(10)
55
> 🌮 "Will my code run?";
🎱 Signs point to yes.
> exit
🌮 Fine.
```

表达式直接显示结果（不需要 `print`），历史记录可以用上下箭头翻，输入和求值在不同线程里跑，错误不崩溃。

---

## 33.1 什么是 REPL

REPL 是交互式语言环境的标配：Python 的 `python3`，Node.js 的 `node`，Haskell 的 `ghci`……都是 REPL。

一个最简单的 REPL 循环：

```
while (true) {
    print("> ")       // 打印提示符
    line = read()     // 读取一行用户输入
    result = eval(line)  // 解释执行
    print(result)     // 打印结果
}
```

这是"单线程同步 REPL"——读取时阻塞，求值时阻塞，任何一步卡住都会导致整个 REPL 无响应。

Taco v6 要做一个**双线程 REPL**：

- **I/O 线程**：负责读取用户输入，把输入放进队列
- **Eval 线程**：负责从队列取出输入，解释执行，把结果放进输出队列
- **主线程**：协调两个线程，处理退出

这样的好处是：未来可以在 Eval 线程里做超时检测（避免死循环卡死 REPL），或者支持后台任务（用户在写代码的同时，后台在跑某个长时间任务）。

---

## 33.2 REPL 的整体架构

```
┌─────────────────────────────────────────────────────┐
│                    主进程                            │
│                                                     │
│  ┌──────────────┐    Channel     ┌───────────────┐  │
│  │  I/O 线程    │ ─── input ──▶ │  Eval 线程    │  │
│  │              │               │               │  │
│  │ 读取 stdin   │ ◀── output ── │  解释执行     │  │
│  │ 打印结果     │    Channel    │  打印错误     │  │
│  └──────────────┘               └───────────────┘  │
│                                                     │
│  std::atomic<bool> running      全局停止标志         │
└─────────────────────────────────────────────────────┘
```

两个线程通过两个"channel"（用 `queue` + `mutex` + `condition_variable` 实现）通信：
- **input channel**：I/O 线程把用户输入的行放进去，Eval 线程取出来解释
- **output channel**：Eval 线程把结果字符串放进去，I/O 线程打印出来

---

## 33.3 实现线程安全的 Channel

Taco 语言里有 `channel` 这个概念（用于并发），在 C++ 实现里也用类似的数据结构——一个线程安全的阻塞队列：

```cpp
// channel.h
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <string>

template<typename T>
class Channel {
public:
    // 发送：放入数据，通知等待的接收者
    void send(T value) {
        {
            std::lock_guard lock(mtx_);
            if (closed_) return;
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    // 接收：等待并取出数据
    // 返回 nullopt 表示 channel 已关闭且为空
    std::optional<T> receive() {
        std::unique_lock lock(mtx_);
        cv_.wait(lock, [this]{
            return !queue_.empty() || closed_;
        });

        if (queue_.empty()) {
            return std::nullopt;  // closed 且空
        }

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    // 非阻塞接收：没有数据立刻返回 nullopt
    std::optional<T> try_receive() {
        std::lock_guard lock(mtx_);
        if (queue_.empty()) return std::nullopt;
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    // 关闭 channel：之后 send 无效，receive 会在队列清空后返回 nullopt
    void close() {
        {
            std::lock_guard lock(mtx_);
            closed_ = true;
        }
        cv_.notify_all();  // 通知所有等待的接收者
    }

    bool is_closed() const {
        std::lock_guard lock(mtx_);
        return closed_;
    }

    bool empty() const {
        std::lock_guard lock(mtx_);
        return queue_.empty();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool closed_ = false;
};

// 常用别名
using StringChannel = Channel<std::string>;
```

这个 `Channel` 实现和 Go 的 channel、Taco 语言里的 `channel` 的行为类似：
- `send` 不阻塞（这里选择的设计，也可以实现带缓冲大小的阻塞发送）
- `receive` 阻塞等待，直到有数据或 channel 关闭
- `close` 之后所有等待者都会被唤醒

---

## 33.4 多行输入的处理

REPL 需要处理多行输入——用户可能输入一个不完整的块：

```
> func fib(n) {
... if (n <= 1) { return n; }
... return fib(n-1) + fib(n-2);
... }
```

判断输入是否完整的方法是数花括号：如果 `{` 多于 `}`，说明还没结束；如果 `{` 和 `}` 数量相等，说明一个块结束了。

```cpp
// repl.h
#pragma once
#include <string>

// 判断输入是否完整（括号是否平衡）
struct InputState {
    int brace_depth = 0;  // { 的深度
    int paren_depth = 0;  // ( 的深度

    // 处理一行输入，返回是否完整
    bool process_line(const std::string& line) {
        for (char c : line) {
            if (c == '{') brace_depth++;
            else if (c == '}') brace_depth--;
            else if (c == '(') paren_depth++;
            else if (c == ')') paren_depth--;
        }
        return brace_depth <= 0 && paren_depth <= 0;
    }

    bool is_complete() const {
        return brace_depth <= 0 && paren_depth <= 0;
    }

    void reset() {
        brace_depth = 0;
        paren_depth = 0;
    }
};
```

---

## 33.5 历史记录：上下箭头

在终端里支持历史记录和行编辑（上下箭头、左右移动光标）需要操纵终端的原始模式（raw mode）。这是一个涉及平台特定 API 的功能。

在 Linux/macOS 上用 `termios`，在 Windows 上用 `GetConsoleMode`。一个跨平台的简化实现：

```cpp
// line_editor.h
#pragma once
#include <string>
#include <vector>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

class LineEditor {
public:
    // 读取一行，支持上下箭头翻历史
    std::string readline(const std::string& prompt) {
        std::cout << prompt << std::flush;

#ifdef _WIN32
        return readline_windows();
#else
        return readline_unix();
#endif
    }

    void add_history(const std::string& line) {
        if (!line.empty() && (history_.empty() || history_.back() != line)) {
            history_.push_back(line);
        }
    }

private:
    std::vector<std::string> history_;
    int history_pos_ = -1;

#ifndef _WIN32
    std::string readline_unix() {
        // 切换到 raw mode
        struct termios old_term, raw;
        tcgetattr(STDIN_FILENO, &old_term);
        raw = old_term;
        raw.c_lflag &= ~(ECHO | ICANON);  // 关闭回显和规范模式
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        std::string line;
        history_pos_ = history_.size();  // 从历史末尾开始

        while (true) {
            char c;
            if (read(STDIN_FILENO, &c, 1) != 1) break;

            if (c == '\r' || c == '\n') {
                // 回车：完成输入
                std::cout << "\n";
                break;
            } else if (c == 127 || c == 8) {
                // Backspace
                if (!line.empty()) {
                    line.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            } else if (c == 27) {
                // 转义序列（方向键等）
                char seq[3];
                if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
                if (read(STDIN_FILENO, &seq[1], 1) != 1) break;

                if (seq[0] == '[') {
                    if (seq[1] == 'A') {
                        // 上箭头：往前翻历史
                        if (history_pos_ > 0) {
                            history_pos_--;
                            replace_line(line, history_[history_pos_]);
                        }
                    } else if (seq[1] == 'B') {
                        // 下箭头：往后翻历史
                        if (history_pos_ < (int)history_.size() - 1) {
                            history_pos_++;
                            replace_line(line, history_[history_pos_]);
                        } else if (history_pos_ == (int)history_.size() - 1) {
                            history_pos_++;
                            replace_line(line, "");
                        }
                    }
                }
            } else if (c >= 32) {
                // 普通字符
                line += c;
                std::cout << c << std::flush;
            }
        }

        // 恢复终端设置
        tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
        return line;
    }

    void replace_line(std::string& current, const std::string& newline) {
        // 清除当前行，打印新内容
        for (size_t i = 0; i < current.size(); ++i)
            std::cout << "\b \b";
        current = newline;
        std::cout << current << std::flush;
    }
#endif
};
```

实际项目里通常直接用 `linenoise`（一个轻量级的行编辑库，200 行 C 代码）或者 `readline`（GNU 的完整历史编辑库）。本书为了保持自包含，用简化的实现。CMakeLists.txt 里可以用 FetchContent 引入 linenoise：

```cmake
# 更简单的方案：引入 linenoise
FetchContent_Declare(
    linenoise
    GIT_REPOSITORY https://github.com/antirez/linenoise.git
    GIT_TAG master
)
FetchContent_MakeAvailable(linenoise)
target_link_libraries(taco PRIVATE linenoise)
```

---

## 33.6 用线程分离输入与求值

现在把 I/O 线程和 Eval 线程搭起来：

```cpp
// repl.cpp
#include "repl.h"
#include "channel.h"
#include "line_editor.h"
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "value.h"
#include <thread>
#include <atomic>
#include <iostream>
#include <string>

// REPL 的欢迎信息
void print_welcome() {
    std::cout << "🌮 Taco 0.1.0\n";
    std::cout << "   It works on my machine.\n";
}

// ── 求值线程 ───────────────────────────────────────────────────

struct EvalResult {
    bool has_value;
    std::string output;   // 要打印的内容
    bool is_error;
};

void eval_thread_func(
    Channel<std::string>& input_chan,
    Channel<EvalResult>& output_chan,
    std::atomic<bool>& running
) {
    // 每个 REPL 会话有自己的求值器和全局环境
    Evaluator eval;
    eval.init_global_env();

    while (running) {
        auto line_opt = input_chan.receive();
        if (!line_opt) break;  // channel 关闭

        const std::string& source = *line_opt;

        // 检查退出命令
        if (source == "exit" || source == "quit" || source == "🌮 Fine.") {
            running.store(false);
            output_chan.send({"🌮 Fine.", false});
            output_chan.close();
            break;
        }

        // 空行跳过
        if (source.empty()) {
            output_chan.send({"", false});
            continue;
        }

        try {
            // 词法 → 语法 → 求值
            auto tokens = tokenize(source);
            auto stmts = parse(tokens);

            TacoValue last_val;
            bool has_last = false;

            for (const auto& stmt : stmts) {
                last_val = eval.execute(*stmt);
                has_last = true;
            }

            // 如果最后一个是表达式（不是语句），自动打印结果
            // 判断方法：最后没有 ; 或者是纯表达式
            if (has_last && !std::holds_alternative<std::nullptr_t>(last_val)) {
                output_chan.send({taco_to_string(last_val), false});
            } else {
                output_chan.send({"", false});
            }

        } catch (const TacoRuntimeError& e) {
            output_chan.send({"🌮 " + std::string(e.what()), true});
        } catch (const std::exception& e) {
            output_chan.send({"🌮 Error: " + std::string(e.what()), true});
        }
    }
}

// ── I/O 线程 ───────────────────────────────────────────────────

void io_thread_func(
    Channel<std::string>& input_chan,
    Channel<EvalResult>& output_chan,
    std::atomic<bool>& running
) {
    LineEditor editor;
    InputState input_state;
    std::string accumulated;  // 多行输入的累积

    while (running) {
        // 打印提示符
        std::string prompt = accumulated.empty() ? "> " : "... ";

        std::string line = editor.readline(prompt);

        // 先检查输出队列，打印之前的结果
        // （这里做了简化：实际上 IO 和 output 是同步的）

        if (!running) break;

        // 处理多行输入
        if (!accumulated.empty()) {
            accumulated += "\n" + line;
        } else {
            accumulated = line;
        }

        // 检查输入是否完整
        input_state.process_line(line);

        if (input_state.is_complete()) {
            // 完整了，发送给 eval 线程
            std::string to_send = accumulated;
            accumulated.clear();
            input_state.reset();

            if (!to_send.empty()) {
                editor.add_history(to_send);
                input_chan.send(to_send);

                // 等待 eval 结果
                auto result_opt = output_chan.receive();
                if (!result_opt) break;

                if (!result_opt->output.empty()) {
                    if (result_opt->is_error) {
                        std::cerr << result_opt->output << "\n";
                    } else {
                        std::cout << result_opt->output << "\n";
                    }
                }

                if (!running) break;
            }
        }
    }

    input_chan.close();
}

// ── REPL 主函数 ────────────────────────────────────────────────

void run_repl() {
    print_welcome();

    Channel<std::string> input_chan;
    Channel<EvalResult>  output_chan;
    std::atomic<bool>    running{true};

    // 启动 Eval 线程
    std::thread eval_thread(eval_thread_func,
        std::ref(input_chan),
        std::ref(output_chan),
        std::ref(running));

    // I/O 在主线程里跑（因为终端 stdin 通常只能在主线程读）
    io_thread_func(input_chan, output_chan, running);

    // 等待 eval 线程退出
    if (eval_thread.joinable()) {
        eval_thread.join();
    }
}
```

---

## 33.7 保护解释器的共享状态

Taco v6 的设计里，每个会话有**独立的** `Evaluator` 和 `Environment`——I/O 线程和 Eval 线程之间只通过 `Channel` 传递字符串。这是刻意的设计选择：

**为什么不共享 Evaluator？**

如果两个线程共享同一个 `Evaluator`（或者 `Environment`），每次读写变量都需要加锁，锁的粒度很难掌握——加太粗，并发优势消失；加太细，死锁风险增加。

**通过消息传递而不是共享内存**，是并发编程里减少错误的最有效思路之一（这也是 Go、Erlang 语言的核心思想）。Taco 语言本身的 `channel` 设计也来自这里。

但有一个全局状态必须处理：**打印输出**（`std::cout`）。`std::cout` 不是线程安全的——如果 eval 线程和 I/O 线程同时写 `cout`，输出会乱掉。

解决方案是把所有输出都通过 `output_chan` 发回 I/O 线程统一打印：

```cpp
// 在求值器里，所有 print() 的输出不直接写 cout，
// 而是通过 output_chan 发回 I/O 线程

// 这需要在 Evaluator 里持有一个输出 channel 的引用
class Evaluator {
public:
    explicit Evaluator(Channel<EvalResult>* output = nullptr)
        : output_chan_(output) {}

    void repl_print(const std::string& msg) {
        if (output_chan_) {
            output_chan_->send({msg, false});
        } else {
            std::cout << msg << "\n";
        }
    }

private:
    Channel<EvalResult>* output_chan_;
};
```

当 `print()` 内置函数被调用时，调用 `eval.repl_print()` 而不是直接 `std::cout`。

---

## 33.8 清晰的错误提示，不崩溃

REPL 里的错误不应该让整个程序崩溃，而是打印错误信息然后继续：

```cpp
// TacoRuntimeError：Taco 运行时错误，带行号
class TacoRuntimeError : public std::runtime_error {
public:
    TacoRuntimeError(const std::string& msg, int line = 0)
        : std::runtime_error(format_error(msg, line)), line_(line) {}

    int line() const { return line_; }

private:
    int line_;

    static std::string format_error(const std::string& msg, int line) {
        if (line > 0) {
            return "line " + std::to_string(line) + ": " + msg;
        }
        return msg;
    }
};

// 在词法分析器里：
// throw TacoRuntimeError("Unterminated string", current_line_);

// 在求值器里：
// throw TacoRuntimeError("Division by zero", node.line);
```

REPL 的错误输出格式：

```
> var x = 10 / 0;
🌮 line 1: Division by zero

> print(naem);
🌮 line 1: 'naem' is not defined. Did you mean 'name'?

> {{{
... }
... }
... }
> print("ok")  // 括号平衡了，执行
ok
```

### 变量名提示（Did you mean?）

这是一个让错误体验好很多的小功能——当变量名找不到时，搜索所有已定义的变量，找最接近的（用编辑距离算法）：

```cpp
// 计算两个字符串的编辑距离（Levenshtein distance）
int edit_distance(const std::string& a, const std::string& b) {
    int m = a.size(), n = b.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    for (int i = 0; i <= m; ++i) dp[i][0] = i;
    for (int j = 0; j <= n; ++j) dp[0][j] = j;

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (a[i-1] == b[j-1]) {
                dp[i][j] = dp[i-1][j-1];
            } else {
                dp[i][j] = 1 + std::min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
            }
        }
    }
    return dp[m][n];
}

// 在 Environment::get() 里，找不到变量时给建议
TacoValue Environment::get(const std::string& name) const {
    auto result = lookup(name);
    if (!result) {
        // 找最接近的变量名
        std::string best_match;
        int best_dist = INT_MAX;

        for (const auto& [var_name, _] : all_variables()) {
            int dist = edit_distance(name, var_name);
            if (dist < best_dist && dist <= 3) {  // 编辑距离不超过 3
                best_dist = dist;
                best_match = var_name;
            }
        }

        std::string msg = "'" + name + "' is not defined.";
        if (!best_match.empty()) {
            msg += " Did you mean '" + best_match + "'?";
        }
        throw TacoRuntimeError(msg);
    }
    return *result;
}
```

---

## 33.9 让 REPL 变成游乐场：细节打磨

### 自动显示表达式结果

在 REPL 里，如果输入的是一个**表达式**（不是声明或语句），应该自动打印结果，不需要写 `print()`：

```
> 1 + 1
2
> "hello" + " world"
hello world
> [1, 2, 3].len()
3
```

判断方法：输入如果没有以 `;` 结尾，或者整行是一个表达式，就尝试用表达式模式解析，如果成功就打印结果。

```cpp
// 在 eval_thread_func 里，针对 REPL 的特殊处理
bool try_as_expression = !source.ends_with(";");

if (try_as_expression) {
    try {
        // 先尝试作为表达式解析
        auto tokens = tokenize(source + ";");  // 加分号让解析器接受
        auto expr = parse_as_expression(tokens);  // 只解析一个表达式
        if (expr) {
            auto val = eval.evaluate(*expr);
            if (!taco_is<std::nullptr_t>(val)) {
                output_chan.send({taco_to_string(val), false});
                return;  // 成功，不需要再作为语句解析
            }
        }
    } catch (...) {
        // 解析失败，回退到语句模式
    }
}

// 作为语句解析和执行
auto tokens = tokenize(source);
auto stmts = parse(tokens);
for (const auto& stmt : stmts) {
    eval.execute(*stmt);
}
```

### 彩蛋：🌮 Magic 8 Ball 在 REPL 里

🌮 在 REPL 里有特殊的显示效果——先显示"正在占卜..."，停顿一秒，再显示答案：

```cpp
// 词法分析器识别 🌮（Unicode U+1F32E，UTF-8: F0 9F 8C AE）
// 已经在 v0 实现，这里只是把 REPL 的显示效果加上

const std::vector<std::string> MAGIC8_ANSWERS = {
    "It is certain.", "Without a doubt.", "Yes, definitely.",
    "You may rely on it.", "Most likely.", "Outlook good.",
    "Yes.", "Signs point to yes.", "As I see it, yes.",
    "It is decidedly so.", "Reply hazy, try again.",
    "Ask again later.", "Cannot predict now.",
    "Concentrate and ask again.", "Better not tell you now.",
    "Don't count on it.", "My reply is no.", "Very doubtful.",
    "Outlook not so good.", "My sources say no."
};

TacoValue eval_magic8(/* optional question */) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, MAGIC8_ANSWERS.size() - 1);

    // 在 REPL 里：停顿效果
    if (in_repl_) {
        std::cout << "..." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        std::cout << "\r🎱 " << MAGIC8_ANSWERS[dist(rng)] << "\n";
        return nullptr;
    }
    return std::string("🎱 " + MAGIC8_ANSWERS[dist(rng)]);
}
```

### 完整的 REPL 会话示例

```
🌮 Taco 0.1.0
   It works on my machine.
> var x = 10;
> x + 20
30
> func greet(name) { print("Hola, {name}!"); }
> greet("Miguel")
Hola, Miguel!
> 🌮 "Will my code compile?";
...
🎱 Signs point to yes.
> var nums = [1, 2, 3, 4, 5];
> nums.filter { n in n % 2 == 0 }.map { n in n * n }
[4, 16]
> func fib(n) {
...   if (n <= 1) { return n; }
...   return fib(n-1) + fib(n-2);
... }
> fib(10)
55
> naem
🌮 line 1: 'naem' is not defined. Did you mean 'name'?
> exit
🌮 Fine.
```

---

## 33.10 CMakeLists.txt 的更新

v6 增加了线程支持，需要链接线程库：

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找到线程库（pthread on Linux/macOS）
find_package(Threads REQUIRED)

add_executable(taco
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/evaluator.cpp
    src/environment.cpp
    src/value.cpp
    src/error.cpp
    src/builtin.cpp
    src/repl.cpp
    src/channel.cpp    # 如果 channel 有实现文件
    src/line_editor.cpp
)

# 链接线程库
target_link_libraries(taco PRIVATE Threads::Threads)

# 可选：引入 linenoise
# FetchContent_Declare(linenoise ...)
# target_link_libraries(taco PRIVATE linenoise)
```

`Threads::Threads` 是 CMake 的跨平台线程目标，在 Linux/macOS 上会链接 `pthread`，在 Windows 上链接对应的线程 API。

---

## 33.11 这个版本的局限性

**单用户 REPL**

v6 只支持一个用户在一个 REPL 会话里。真实的语言服务器（比如 Jupyter Kernel）需要支持多个客户端同时连接，每个客户端有独立的环境，这需要更复杂的多线程设计。

**没有超时机制**

如果用户写了一个死循环：

```taco
> while (true) {}
```

REPL 会一直卡住，无法中断。实现超时需要在 eval 线程里设置一个定时器，超时后抛异常——这需要用 `std::async` 或者 `std::stop_token`（C++20），留给读者尝试。

**历史记录不持久化**

退出 REPL 后，历史记录就消失了。持久化需要在退出时把历史写到 `~/.taco_history`，下次启动时读取。用 `<filesystem>` 和 `<fstream>` 可以实现，留给读者练习。

**print() 的线程安全**

现在把 `print()` 的输出通过 `output_chan` 发回 I/O 线程，但如果 eval 线程里有多个并发的 Taco `thread`（Taco 语言的并发特性），它们同时调用 `print()`，输出顺序还是不确定的。完整的解决方案需要一个统一的输出调度器。

---

## 小结

v6 是 Taco 从"脚本解释器"进化成"交互式语言环境"的关键一步。

**双线程架构**：I/O 线程负责用户交互（读输入、打印结果），Eval 线程负责解释执行。两者通过 `Channel`（线程安全队列）通信，不共享内存（除了 `atomic<bool>` 的停止信号）。

**Channel 实现**：`std::queue` + `std::mutex` + `std::condition_variable` 构成一个阻塞队列，`send` 放入并通知，`receive` 等待并取出，`close` 关闭并通知所有等待者。

**多行输入**：数花括号和圆括号的深度，深度归零时表示一个完整的代码块，才发送给 eval 线程。

**错误不崩溃**：所有错误都用 `try-catch` 捕获，转成友好的错误信息打印，REPL 继续运行。`Did you mean?` 通过编辑距离找最接近的变量名。

**设计原则**：通过消息传递而不是共享内存来通信，让并发代码简单、安全。这不只是 v6 的设计原则，也是 Taco 语言的 `channel` 设计背后的思想。

---

第七部分到这里结束。第八部分进入网络编程，学完之后 v7 会给 Taco 加上 `fetchUrl()` 和 HTTP 请求支持。
# 第三十四章：网络编程基础

---

第七部分结束时，Taco 有了 REPL 和并发支持。第八部分只有三章，目标明确：给 Taco 加上网络能力，让它能发 HTTP 请求、调用 API。

这一章打基础。网络编程横跨操作系统、协议、I/O 模型三个层面，哪个都不简单。把这三个层面弄清楚，后面用库的时候才能知道库在帮你做什么，出了问题才知道往哪里查。

---

## 34.1 TCP/UDP、客户端/服务器模型

### 网络通信的层次

网络通信是分层的。从应用程序的视角看，最常打交道的是两层：

**传输层**：TCP 和 UDP。负责把数据从一台机器送到另一台机器的某个进程。

**应用层**：HTTP、WebSocket、gRPC……这些协议建立在 TCP（偶尔 UDP）之上，定义了数据的格式和交互方式。

写 C++ 网络代码，大多数时候是在和传输层打交道——直接操作 socket——然后在 socket 之上实现或使用应用层协议。

### TCP：可靠的字节流

TCP（Transmission Control Protocol，传输控制协议）是互联网最常用的传输协议。它的核心承诺是：

- **可靠**：数据一定到达，不丢包（丢了会重传）
- **有序**：数据按发送顺序到达
- **字节流**：没有消息边界，是一个连续的字节流

"字节流"这一点经常让新手困惑。TCP 不保证每次 `send` 的数据在对端一次 `recv` 就能收到——你 `send` 了 1000 字节，对端可能一次收到 500 字节，再收到 500 字节。所以应用层协议通常需要自己定义消息边界（比如 HTTP 用空行和 Content-Length）。

### UDP：不可靠的数据报

UDP（User Datagram Protocol）是另一种传输协议。它的特点：

- **不可靠**：包可能丢失，丢了不重传
- **有边界**：每次发送是一个独立的数据报，收到也是独立的
- **速度快**：没有连接建立、重传、拥塞控制的开销

UDP 适合对延迟敏感、能容忍少量丢包的场景：在线游戏、实时视频、DNS 查询。HTTP 用 TCP，所以 Taco 的网络功能只需要关心 TCP。

（HTTP/3 基于 QUIC，而 QUIC 运行在 UDP 上——但这是另一个话题，不在这里展开。）

### 客户端/服务器模型

几乎所有网络应用都是客户端/服务器（client/server）模型：

```
客户端                    服务器
  |                          |
  |----  建立连接  ---------->|
  |                          |
  |----  发送请求  ---------->|
  |                          |
  |<---  返回响应  -----------|
  |                          |
  |----  关闭连接  ---------->|
```

服务器的角色：
- **监听**（listen）：在某个端口等待连接
- **接受**（accept）：接受客户端的连接请求，创建一个新 socket 专门和这个客户端通信
- **处理**：读取请求，计算结果，发送响应

客户端的角色：
- **连接**（connect）：主动发起连接，指定服务器地址和端口
- **发送**（send）：发送请求
- **接收**（recv）：读取响应

Taco 的 `fetchUrl()` 是一个 HTTP 客户端——它主动连接到远程服务器，发送 HTTP 请求，读取响应。不需要实现服务器端。

### 端口号

端口号（port）是一个 16 位整数（0–65535），用来区分同一台机器上的不同服务。常见的端口号：

| 端口 | 服务 |
|------|------|
| 80 | HTTP |
| 443 | HTTPS |
| 22 | SSH |
| 3306 | MySQL |
| 5432 | PostgreSQL |

端口 0–1023 是"知名端口"，需要管理员权限才能使用。写服务器时通常选 1024 以上的端口。

---

## 34.2 用原生 socket API 写一个最简单的例子

在使用任何网络库之前，先看一下原生 socket API 长什么样。这层 API 来自 POSIX（Unix 系统的标准接口），在 Linux 和 macOS 上直接可用，Windows 上有 Winsock（基本相同，但有一些差异）。

理解原生 API 的价值在于：所有网络库（Asio、cpp-httplib 等）最终都是对这套 API 的封装。看到封装层时，你知道底下发生了什么。

### TCP 服务器（最简版）

```cpp
// server.cpp
#include <sys/socket.h>   // socket, bind, listen, accept, send, recv
#include <netinet/in.h>   // sockaddr_in
#include <arpa/inet.h>    // inet_addr, htons
#include <unistd.h>       // close
#include <cstring>        // memset
#include <cstdio>         // printf

int main() {
    // 1. 创建 socket
    // AF_INET：IPv4
    // SOCK_STREAM：TCP（流式）
    // 0：协议自动选择
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // 2. 设置 socket 选项
    // SO_REUSEADDR：允许重用地址，避免"Address already in use"
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 绑定地址和端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
    addr.sin_port = htons(8080);        // htons：主机字节序转网络字节序

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    // 4. 开始监听，最多 5 个排队连接
    listen(server_fd, 5);
    printf("Listening on port 8080...\n");

    // 5. 接受连接（阻塞，直到有客户端连进来）
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        return 1;
    }
    printf("Client connected.\n");

    // 6. 读取数据
    char buf[1024];
    int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        printf("Received: %s\n", buf);
    }

    // 7. 发送响应
    const char* response = "Hello from server!\n";
    send(client_fd, response, strlen(response), 0);

    // 8. 关闭连接
    close(client_fd);
    close(server_fd);
    return 0;
}
```

### TCP 客户端（最简版）

```cpp
// client.cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

int main() {
    // 1. 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // 2. 设置要连接的服务器地址
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    // 连接本机（127.0.0.1）
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // 3. 连接服务器（阻塞，直到连接成功或失败）
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }
    printf("Connected to server.\n");

    // 4. 发送数据
    const char* msg = "Hello from client!";
    send(sock, msg, strlen(msg), 0);

    // 5. 接收响应
    char buf[1024];
    int n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        printf("Server says: %s", buf);
    }

    // 6. 关闭
    close(sock);
    return 0;
}
```

### 运行这个例子

先开一个终端运行服务器，再开另一个运行客户端：

```bash
# 终端 1：编译并运行服务器
g++ -o server server.cpp && ./server
# Listening on port 8080...

# 终端 2：编译并运行客户端
g++ -o client client.cpp && ./client
# Connected to server.
# Server says: Hello from server!

# 终端 1 服务器输出：
# Client connected.
# Received: Hello from client!
```

也可以用 `nc`（netcat）代替客户端快速测试：

```bash
echo "hello" | nc 127.0.0.1 8080
```

### socket API 的核心函数梳理

| 函数 | 作用 |
|------|------|
| `socket(domain, type, protocol)` | 创建 socket，返回文件描述符 |
| `bind(fd, addr, addrlen)` | 绑定地址（服务器用） |
| `listen(fd, backlog)` | 开始监听（服务器用） |
| `accept(fd, addr, addrlen)` | 接受连接（服务器用），阻塞直到有连接 |
| `connect(fd, addr, addrlen)` | 发起连接（客户端用） |
| `send(fd, buf, len, flags)` | 发送数据 |
| `recv(fd, buf, len, flags)` | 接收数据，阻塞直到有数据到达 |
| `close(fd)` | 关闭 socket |

### 字节序

网络通信约定用**大端字节序**（big-endian，也叫网络字节序）。x86/ARM 机器通常是小端字节序（little-endian）。所以端口号和 IP 地址在写入 `sockaddr_in` 之前，需要转换字节序：

```cpp
htons(8080)   // host to network short（16位）
htonl(addr)   // host to network long（32位）
ntohs(port)   // network to host short
ntohl(addr)   // network to host long
```

`inet_pton` 把点分十进制的 IP 字符串（`"192.168.1.1"`）转成网络字节序的整数，顺便处理了字节序问题。

---

### 原生 API 的问题

上面的代码能跑，但有明显的问题：

**每次只能处理一个客户端**。`accept` 阻塞，`recv` 阻塞，处理完一个才能处理下一个。真实的服务器需要同时处理成百上千个连接。

**错误处理繁琐**。每个函数都返回 -1 表示失败，需要查 `errno` 才知道具体错误。

**不跨平台**。Windows 用 Winsock，头文件和部分函数名都不同。

**资源管理手动**。`close(fd)` 必须手动调用，忘了就泄漏文件描述符。

这就是为什么实际开发里几乎不直接用这套 API，而是用封装好的库。下一章介绍两个：Asio 和 cpp-httplib。

---

## 34.3 同步 I/O vs 异步 I/O 的概念

理解 I/O 模型是网络编程的关键。`recv` 调用会**阻塞**线程，直到有数据到达——在这段等待时间里，线程什么都做不了。这是**同步阻塞 I/O**。

### 几种 I/O 模型

**同步阻塞 I/O（Synchronous Blocking I/O）**

```
线程:  [  发起 recv  ] [........等待........] [  处理数据  ]
时间:  ─────────────────────────────────────────────────────>
数据:                                         [数据到达]
```

最简单，代码直接。问题是线程在等待时被占用，不能做其他事。要同时处理多个连接，就需要多个线程——每个连接一个线程。

**同步非阻塞 I/O（Synchronous Non-Blocking I/O）**

把 socket 设为非阻塞模式，`recv` 立刻返回：如果没有数据，返回 `EAGAIN`；如果有数据，返回数据。程序需要不断地轮询（poll）每个 socket：

```cpp
// 把 socket 设为非阻塞
fcntl(sock, F_SETFL, O_NONBLOCK);

// 轮询
while (true) {
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n < 0 && errno == EAGAIN) {
        // 暂时没数据，做别的事或者继续循环
        continue;
    }
    if (n > 0) {
        // 处理数据
    }
}
```

避免了线程阻塞，但忙等（busy-wait）浪费 CPU。实际中配合 `select`/`poll`/`epoll` 使用，让内核帮你监视多个 socket，只在有事件时唤醒。

**I/O 多路复用（I/O Multiplexing）**

用 `select`、`poll`（跨平台）或者 `epoll`（Linux）、`kqueue`（macOS）同时监视多个 socket，哪个有事件就处理哪个：

```cpp
// epoll 示例（Linux）
int epfd = epoll_create1(0);

// 把 server_fd 加入监视
epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = server_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

// 等待事件
epoll_event events[64];
int n = epoll_wait(epfd, events, 64, -1);  // 阻塞直到有事件
for (int i = 0; i < n; i++) {
    // 处理 events[i].data.fd 上的事件
}
```

一个线程就能高效管理数千个连接，这是 Nginx、Redis 等高性能服务的核心技术。

**异步 I/O（Asynchronous I/O）**

发起 I/O 操作，不等结果，注册一个回调（callback）或者通过 `std::future` 等机制。操作完成时，系统通知程序处理结果：

```cpp
// 概念示意（不是真实 API）
async_recv(sock, buf, [](int bytes_read) {
    // 这里是数据到达时的回调
    process(buf, bytes_read);
});

// 主线程可以做别的事情
do_other_work();
```

Asio 库（下一章）就是基于这个模型的——用 `async_read`、`async_write` 发起异步 I/O，在回调里处理结果。

### 如何选择

对于 Taco 的 `fetchUrl()`，情况很简单：

- `fetchUrl` 是一个**同步**的调用——Taco 脚本调用它，等它返回结果，然后继续执行
- 发起一次 HTTP 请求，等响应，返回结果
- 不需要同时处理多个连接

所以最简单的**同步阻塞**模型就够了。这也是 `cpp-httplib` 的工作方式——底层用同步 socket，API 非常简单：

```cpp
httplib::Client cli("api.github.com");
auto res = cli.Get("/users/torvalds");
// 这里 res 里已经有响应了
```

如果将来 Taco 要实现并发网络请求（比如同时发多个 `fetchUrl`，用 `thread` 并行），那时候才需要考虑异步或多线程方案。

---

### 一个更真实的问题：字节流的拆包

用 TCP 发 HTTP 请求，会碰到字节流的问题。HTTP 响应的格式是：

```
HTTP/1.1 200 OK\r\n
Content-Type: application/json\r\n
Content-Length: 42\r\n
\r\n
{"login":"torvalds","id":1024025,...}
```

头部以 `\r\n\r\n` 结束，body 长度由 `Content-Length` 给出。程序需要：

1. 不断 `recv`，直到看到 `\r\n\r\n`，这时候头部读完了
2. 解析头部，找到 `Content-Length`
3. 再 `recv` 指定字节数，读完 body

如果手动实现，大概是这样：

```cpp
std::string response;
char buf[4096];

// 读头部
while (true) {
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n <= 0) break;
    response.append(buf, n);
    // 找到头部结束标记
    if (response.find("\r\n\r\n") != std::string::npos) break;
}

// 解析 Content-Length
auto pos = response.find("Content-Length: ");
int content_length = 0;
if (pos != std::string::npos) {
    content_length = std::stoi(response.substr(pos + 16));
}

// 计算还需要读多少 body
auto header_end = response.find("\r\n\r\n") + 4;
int body_received = response.size() - header_end;
int remaining = content_length - body_received;

// 继续读 body
while (remaining > 0) {
    int n = recv(sock, buf, std::min(remaining, (int)sizeof(buf)), 0);
    if (n <= 0) break;
    response.append(buf, n);
    remaining -= n;
}
```

这只是 HTTP/1.1 的一个简化版本，还没处理 chunked transfer encoding、Keep-Alive、重定向……这就是为什么没人手写 HTTP 客户端，都用库。

---

## 小结

**TCP 和 UDP** 是两种传输层协议。TCP 可靠有序，是 HTTP 的基础；UDP 速度快但不可靠，适合实时应用。

**客户端/服务器模型**：服务器监听端口，接受连接；客户端主动发起连接，发送请求，接收响应。`fetchUrl()` 是一个纯客户端实现。

**原生 socket API**：`socket` → `connect` → `send`/`recv` → `close` 是客户端的基本流程。服务器多一个 `bind` → `listen` → `accept`。端口号和 IP 地址需要处理字节序。

**I/O 模型**：
- 同步阻塞：最简单，线程在等待时阻塞
- 同步非阻塞 + I/O 多路复用（epoll）：一个线程管理多个连接，高性能服务器的基础
- 异步 I/O：发起操作后不等结果，通过回调或 future 处理

对于 Taco 的 `fetchUrl()`，同步阻塞就够了。下一章介绍 Asio（异步 I/O 库）和 cpp-httplib（同步 HTTP 客户端），并选择适合 Taco 的方案。
# 第三十五章：现代 C++ 网络库

---

上一章看了原生 socket API，知道了底层是什么样的。这一章用两个库把网络编程变得简单：

- **Asio**（独立版，不依赖 Boost）：现代 C++ 风格，基于异步 I/O，功能完整，性能强
- **cpp-httplib**：单头文件，同步风格，专注 HTTP，极其简单

这两个库服务于不同的场景。弄清楚各自的定位，才能在项目里做出正确选择。

---

## 35.1 Asio（独立版）：现代 C++ 风格的网络库

Asio（Asynchronous I/O）最初是 Boost 的一部分（`boost::asio`），现在有独立版本（`asio`，不依赖 Boost）。它是 C++ 网络编程的工业级解决方案，C++ 网络标准库提案（Networking TS）很大程度上就是以 Asio 为基础的。

### Asio 的核心概念

Asio 的设计围绕几个核心概念：

**io_context（I/O 上下文）**

`asio::io_context` 是 Asio 的引擎。所有异步操作都注册在它上面，调用 `io_context.run()` 开始事件循环，驱动所有异步操作执行：

```cpp
asio::io_context io;
// 注册异步操作...
io.run();  // 运行事件循环，直到所有操作完成
```

可以把它想成一个任务队列：所有"等网络"的工作都丢给它，它负责调度，有结果了调用你的回调。

**executor**

executor 决定回调在哪里执行（哪个线程）。默认情况下，所有回调都在调用 `io.run()` 的线程执行。如果多个线程都调用 `io.run()`，Asio 会自动把任务分配到空闲线程。

**completion handler（完成处理程序）**

异步操作完成时调用的函数。可以是普通函数、lambda、`std::bind` 的绑定。从 C++20 开始，也可以是协程（coroutine）。

### 引入 Asio

在 CMakeLists.txt 里通过 FetchContent 引入独立版 Asio：

```cmake
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Asio 是 header-only，只需要 include 路径
target_include_directories(taco PRIVATE ${asio_SOURCE_DIR}/asio/include)

# 独立版 Asio 需要定义这个宏，禁用 Boost 依赖
target_compile_definitions(taco PRIVATE ASIO_STANDALONE)

# Linux 上需要链接 pthread
if (UNIX)
    target_link_libraries(taco PRIVATE pthread)
endif()
```

Asio 独立版是 header-only，不需要编译成库，只需要 include。

### Asio 的同步接口

Asio 支持同步和异步两套接口。先看同步的，比较直观：

```cpp
#include <asio.hpp>
#include <iostream>

int main() {
    asio::io_context io;

    // 创建 TCP socket
    asio::ip::tcp::socket sock(io);

    // 解析域名（DNS 查询）
    asio::ip::tcp::resolver resolver(io);
    // resolve 返回端点列表（一个域名可能对应多个 IP）
    auto endpoints = resolver.resolve("example.com", "80");

    // 连接（尝试端点列表中的每一个，直到成功）
    asio::connect(sock, endpoints);

    // 发送 HTTP GET 请求
    std::string request =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";
    asio::write(sock, asio::buffer(request));

    // 读取响应（直到连接关闭）
    asio::streambuf response;
    asio::error_code ec;
    asio::read(sock, response, asio::transfer_all(), ec);
    // ec == asio::error::eof 表示服务器关闭了连接，这是正常的

    std::cout << &response;
    return 0;
}
```

对比原生 socket API，改进是显著的：

- `resolver.resolve("example.com", "80")` 替代了手动的 `getaddrinfo` + 结构体填充
- `asio::connect` 替代了手动遍历地址列表 + `::connect`
- `asio::write` 和 `asio::read` 处理了部分写/部分读的问题（原生 `send`/`recv` 可能不一次写完/读完）
- RAII：`sock` 离开作用域自动关闭，不需要手动 `close`

### Asio 的异步接口

异步接口是 Asio 的精髓。核心函数都有对应的 `async_` 版本：

```cpp
#include <asio.hpp>
#include <iostream>
#include <memory>

// 把客户端状态封装在一个共享对象里
struct Session : std::enable_shared_from_this<Session> {
    asio::ip::tcp::socket sock;
    asio::streambuf buf;

    Session(asio::io_context& io) : sock(io) {}

    void start(asio::ip::tcp::resolver::results_type endpoints) {
        // 异步连接：连接完成后调用 on_connect
        asio::async_connect(
            sock,
            endpoints,
            [self = shared_from_this()](
                asio::error_code ec,
                asio::ip::tcp::endpoint
            ) {
                if (!ec) self->on_connect();
            }
        );
    }

    void on_connect() {
        // 连接成功，异步发送请求
        static const std::string request =
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n"
            "\r\n";

        asio::async_write(
            sock,
            asio::buffer(request),
            [self = shared_from_this()](asio::error_code ec, size_t) {
                if (!ec) self->on_write();
            }
        );
    }

    void on_write() {
        // 请求发出，异步读取响应
        asio::async_read(
            sock,
            buf,
            asio::transfer_all(),
            [self = shared_from_this()](asio::error_code ec, size_t) {
                // EOF 是正常结束
                if (!ec || ec == asio::error::eof) {
                    std::cout << &self->buf;
                }
            }
        );
    }
};

int main() {
    asio::io_context io;

    asio::ip::tcp::resolver resolver(io);
    auto endpoints = resolver.resolve("example.com", "80");

    auto session = std::make_shared<Session>(io);
    session->start(endpoints);

    io.run();  // 运行事件循环，直到所有回调执行完
    return 0;
}
```

这段代码比同步版本长很多，但它的关键优势是：`io.run()` 运行期间，线程不会阻塞在任何一个 I/O 操作上——它在等网络的时候，可以同时处理其他的异步操作。如果有 1000 个并发请求，同步版本需要 1000 个线程，异步版本只需要几个线程（甚至一个）。

注意 `shared_from_this`：异步回调会在将来某个时刻执行，届时 `Session` 对象必须还活着。用 `shared_ptr` + `weak_from_this`/`shared_from_this` 来保证对象在回调执行完之前不被销毁，这是异步代码里管理对象生命周期的常用模式。

### C++20 协程：让异步代码看起来像同步代码

Asio 从 1.18 版本开始支持 C++20 协程（coroutine）。协程让异步代码的写法和同步代码一样直观：

```cpp
// 需要 C++20 和支持协程的 Asio 版本
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>

asio::awaitable<void> fetch(asio::io_context& io) {
    asio::ip::tcp::resolver resolver(io);
    auto endpoints = co_await resolver.async_resolve(
        "example.com", "80", asio::use_awaitable
    );

    asio::ip::tcp::socket sock(io);
    co_await asio::async_connect(sock, endpoints, asio::use_awaitable);

    std::string request = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    co_await asio::async_write(sock, asio::buffer(request), asio::use_awaitable);

    asio::streambuf buf;
    asio::error_code ec;
    co_await asio::async_read(sock, buf, asio::transfer_all(), asio::use_awaitable);

    std::cout << &buf;
}

int main() {
    asio::io_context io;
    asio::co_spawn(io, fetch(io), asio::detached);
    io.run();
}
```

`co_await` 暂停协程，把控制权还给事件循环；I/O 完成后，协程从暂停处恢复继续执行。代码结构和同步代码一模一样，但底层是完全异步的。这是现代 C++ 异步编程的方向。

---

## 35.2 用 Asio 写一个 TCP 客户端

把上面的知识整合成一个完整的、带错误处理的同步 TCP 客户端：

```cpp
// tcp_client.cpp
#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <string>
#include <stdexcept>

// 同步 TCP 客户端：连接到服务器，发送数据，接收响应
std::string tcp_request(const std::string& host,
                         const std::string& port,
                         const std::string& data) {
    asio::io_context io;

    // 解析主机名（可能触发 DNS 查询）
    asio::ip::tcp::resolver resolver(io);
    asio::error_code ec;
    auto endpoints = resolver.resolve(host, port, ec);
    if (ec) {
        throw std::runtime_error("DNS resolve failed: " + ec.message());
    }

    // 创建 socket 并连接
    asio::ip::tcp::socket sock(io);
    asio::connect(sock, endpoints, ec);
    if (ec) {
        throw std::runtime_error("Connect failed: " + ec.message());
    }

    // 发送数据
    asio::write(sock, asio::buffer(data), ec);
    if (ec) {
        throw std::runtime_error("Write failed: " + ec.message());
    }

    // 关闭发送方向（告诉服务器"我发完了"）
    sock.shutdown(asio::ip::tcp::socket::shutdown_send, ec);

    // 读取响应（直到 EOF）
    asio::streambuf response;
    asio::read(sock, response, asio::transfer_all(), ec);
    // 服务器关闭连接会触发 EOF，这是正常的
    if (ec && ec != asio::error::eof) {
        throw std::runtime_error("Read failed: " + ec.message());
    }

    // 把 streambuf 转成 string
    return std::string(
        std::istreambuf_iterator<char>(&response),
        std::istreambuf_iterator<char>()
    );
}

int main() {
    try {
        // 发一个原始的 HTTP/1.0 请求（HTTP/1.0 默认 Connection: close，简单）
        std::string request =
            "GET / HTTP/1.0\r\n"
            "Host: example.com\r\n"
            "\r\n";

        std::string response = tcp_request("example.com", "80", request);
        std::cout << response.substr(0, 500) << "...\n";  // 只打印前 500 字节
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
```

这个封装已经相当实用。但如果要处理真实的 HTTP：状态码、响应头、chunked 编码、HTTPS……还是很繁琐。这就是 cpp-httplib 的用武之地。

---

## 35.3 cpp-httplib：几行代码发一个 HTTP 请求

`cpp-httplib` 是一个单头文件的 C++ HTTP 库，支持 HTTP/1.1 和 HTTPS（需要 OpenSSL）。它的设计哲学是**极简**：一个头文件，几行代码，搞定 HTTP。

GitHub：https://github.com/yhirose/cpp-httplib

### 引入 cpp-httplib

方式一：直接下载 `httplib.h` 放进项目。

方式二：CMake FetchContent：

```cmake
include(FetchContent)

FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.0
)
FetchContent_MakeAvailable(cpp_httplib)

target_link_libraries(taco PRIVATE httplib::httplib)
```

如果需要 HTTPS 支持，还要链接 OpenSSL：

```cmake
find_package(OpenSSL REQUIRED)
target_link_libraries(taco PRIVATE httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(taco PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
```

### 发 GET 请求

```cpp
#define CPPHTTPLIB_OPENSSL_SUPPORT  // 启用 HTTPS
#include "httplib.h"
#include <iostream>

int main() {
    // HTTPS 客户端
    httplib::SSLClient cli("api.github.com");
    cli.set_connection_timeout(10);  // 10 秒超时

    // 发 GET 请求
    auto res = cli.Get("/users/torvalds");

    if (!res) {
        // 网络错误
        std::cerr << "Error: " << httplib::to_string(res.error()) << "\n";
        return 1;
    }

    if (res->status == 200) {
        std::cout << "Status: " << res->status << "\n";
        std::cout << "Body: " << res->body << "\n";
    } else {
        std::cerr << "HTTP error: " << res->status << "\n";
    }
    return 0;
}
```

`res` 是一个 `std::optional<httplib::Response>`，用 `!res` 检查网络错误，用 `res->status` 拿状态码，用 `res->body` 拿响应体。就这么简单。

### 发 POST 请求

```cpp
httplib::SSLClient cli("httpbin.org");

// POST JSON 数据
std::string body = R"({"name": "Miguel", "age": 12})";

auto res = cli.Post(
    "/post",                        // 路径
    body,                           // 请求体
    "application/json"              // Content-Type
);

if (res && res->status == 200) {
    std::cout << res->body << "\n";
}
```

### 设置请求头

```cpp
httplib::Headers headers = {
    {"Authorization", "Bearer your_token_here"},
    {"User-Agent", "Taco/0.1"}
};

auto res = cli.Get("/api/data", headers);
```

### 完整的错误处理

```cpp
#include "httplib.h"
#include <iostream>
#include <stdexcept>

std::string fetch_url(const std::string& url) {
    // 解析 URL：分离主机名和路径
    // 简化：假设 URL 格式为 https://host/path
    std::string host, path;
    if (url.substr(0, 8) == "https://") {
        auto rest = url.substr(8);
        auto slash = rest.find('/');
        if (slash == std::string::npos) {
            host = rest;
            path = "/";
        } else {
            host = rest.substr(0, slash);
            path = rest.substr(slash);
        }
    } else if (url.substr(0, 7) == "http://") {
        auto rest = url.substr(7);
        auto slash = rest.find('/');
        host = (slash == std::string::npos) ? rest : rest.substr(0, slash);
        path = (slash == std::string::npos) ? "/" : rest.substr(slash);
    } else {
        throw std::runtime_error("Unsupported URL scheme: " + url);
    }

    // 根据协议选择客户端
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    bool is_https = (url.substr(0, 8) == "https://");
#else
    bool is_https = false;
#endif

    httplib::Result res;
    if (is_https) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        httplib::SSLClient cli(host);
        cli.set_connection_timeout(30);
        cli.set_follow_location(true);  // 跟随重定向
        res = cli.Get(path);
#endif
    } else {
        httplib::Client cli(host);
        cli.set_connection_timeout(30);
        cli.set_follow_location(true);
        res = cli.Get(path);
    }

    if (!res) {
        throw std::runtime_error(
            "Network error: " + httplib::to_string(res.error())
        );
    }

    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error(
            "HTTP error: " + std::to_string(res->status)
        );
    }

    return res->body;
}
```

这个函数就是 v7 里 `fetchUrl()` 的 C++ 核心实现。

### 写一个简单的 HTTP 服务器

cpp-httplib 也支持服务器端，语法同样简洁：

```cpp
#include "httplib.h"
#include <iostream>

int main() {
    httplib::Server svr;

    // GET /hello
    svr.Get("/hello", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("Hello, World!", "text/plain");
    });

    // GET /greet?name=Miguel
    svr.Get("/greet", [](const httplib::Request& req, httplib::Response& res) {
        auto name = req.get_param_value("name");
        if (name.empty()) name = "World";
        res.set_content("Hello, " + name + "!", "text/plain");
    });

    // POST /echo
    svr.Post("/echo", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(req.body, "text/plain");
    });

    std::cout << "Server running on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
```

用 curl 测试：

```bash
curl http://localhost:8080/hello
# Hello, World!

curl "http://localhost:8080/greet?name=Miguel"
# Hello, Miguel!

curl -X POST -d "some data" http://localhost:8080/echo
# some data
```

Taco 的 v7 只需要客户端功能，但服务器功能这里也展示一下，以备将来使用。

---

## 35.4 网络生态概览：什么场景用什么库

C++ 的网络生态比较分散，不同场景有不同的选择：

### 低层次、高性能、异步：Asio / Boost.Asio

适合：需要精细控制 I/O 模型、高并发服务器、自定义协议、长连接。

学习曲线陡，但功能最强，性能最好。工业界广泛使用（Asio 本身是很多高级框架的基础层）。

```cpp
// Asio 核心：io_context + 异步操作 + 回调/协程
asio::io_context io;
asio::ip::tcp::acceptor acceptor(io, {asio::ip::tcp::v4(), 8080});
// async_accept, async_read, async_write...
```

### HTTP 客户端/服务器，快速开发：cpp-httplib

适合：需要做 HTTP 请求、写简单的 REST API 服务器、脚本工具、原型开发。

单头文件，零依赖（HTTPS 需要 OpenSSL），API 极简。性能够用，不适合高并发场景（内部每个连接一个线程）。

```cpp
httplib::Client cli("api.example.com");
auto res = cli.Get("/data");
```

### gRPC：微服务 RPC

适合：服务间通信、需要强类型接口定义、跨语言 RPC。

基于 Protobuf，性能强，但配置和学习成本高。

### libcurl：成熟的 HTTP 客户端（C 库，有 C++ 封装）

适合：需要支持更多协议（FTP、SFTP、SMTP…）、需要经过时间检验的实现。

C 接口有点繁琐，但有 curlpp 等 C++ 封装。

### cpr（C++ Requests）：C++ 版的 Python requests

适合：喜欢 Python requests 风格、快速写 HTTP 请求。

底层用 libcurl，API 更现代：

```cpp
#include <cpr/cpr.h>

auto r = cpr::Get(cpr::Url{"https://api.github.com/users/torvalds"},
                  cpr::Header{{"User-Agent", "Taco/0.1"}});
std::cout << r.text;  // 响应 body
```

### 选择建议

| 场景 | 推荐 |
|------|------|
| Taco 的 `fetchUrl()`（HTTP 客户端） | cpp-httplib |
| 高并发 TCP 服务器 | Asio |
| 微服务 RPC | gRPC |
| 需要更多协议支持 | libcurl / cpr |
| 快速原型，Python requests 风格 | cpr |

对于 Taco 的 v7，选择 **cpp-httplib**：零配置（单头文件），API 简单，直接对应 `fetchUrl()` 的同步语义，HTTPS 支持好。

---

### 关于 HTTPS 和 SSL/TLS

现代 API 几乎都强制 HTTPS。HTTPS = HTTP + TLS（Transport Layer Security）。TLS 在 TCP 之上加了一层加密和身份验证。

要用 cpp-httplib 发 HTTPS 请求，需要：

1. 安装 OpenSSL（大多数系统自带，或用包管理器安装）
2. 编译时定义 `CPPHTTPLIB_OPENSSL_SUPPORT`
3. 链接 `OpenSSL::SSL` 和 `OpenSSL::Crypto`

在 CMakeLists.txt 里：

```cmake
find_package(OpenSSL REQUIRED)
target_link_libraries(taco PRIVATE httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(taco PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
```

如果没有 OpenSSL，cpp-httplib 仍然可以用，只是只支持 HTTP，不支持 HTTPS。

---

## 小结

**Asio** 是 C++ 网络编程的工业级库。核心是 `io_context`（事件循环）+ 异步操作（`async_read`、`async_write`、`async_connect`）+ 完成处理程序（回调或 C++20 协程）。同步接口也有，适合简单场景。

**cpp-httplib** 是单头文件的 HTTP 库。`httplib::Client` 做 HTTP，`httplib::SSLClient` 做 HTTPS，`httplib::Server` 实现服务器。API 极简，几行代码完成一次 HTTP 请求。适合 Taco 的 `fetchUrl()` 场景。

**选择依据**：需要高并发、低层控制、自定义协议 → Asio；需要快速做 HTTP 请求或简单 REST 服务器 → cpp-httplib；生产环境微服务 → gRPC；需要多协议支持 → libcurl/cpr。

下一章是第八部分的项目章节：用 cpp-httplib 在 Taco 里实现 `fetchUrl()` 和 `postData()`，完成 v7。
# 第三十六章：项目 v7——内置 fetchUrl() 与网络支持

---

v6 结束时，Taco 有了 REPL 和并发支持。v7 加上最后一块：网络。

Taco 脚本能发 HTTP 请求，能调用真实的 API，就从一个本地脚本工具变成了可以和外部世界交互的东西。

v7 完成后，Taco 能跑这样的脚本：

```taco
// 调用 GitHub API
var user = fetchUrl("https://api.github.com/users/torvalds");
print(user);

// 查天气
var weather = fetchUrl("https://wttr.in/Beijing?format=3");
print(weather);

// POST 数据
var response = postData("https://httpbin.org/post",
                        "{'name': 'Miguel', 'age': 12}");
print(response);
```

---

## 36.1 在 Taco 里实现 `fetchUrl()`

### 设计决策

**同步还是异步？**

Taco 脚本是顺序执行的——一行运行完再运行下一行。`fetchUrl()` 应该是一个同步调用：Taco 脚本调用它，等它返回，然后继续。这和 Python 的 `urllib.request.urlopen()` 行为一致。

如果需要并发网络请求（同时发多个），Taco 已经有 `thread`，可以这样写：

```taco
var t1 = thread { var r1 = fetchUrl("https://api1.example.com"); };
var t2 = thread { var r2 = fetchUrl("https://api2.example.com"); };
t1.join();
t2.join();
```

所以 `fetchUrl()` 本身保持同步，并发交给 `thread`。

**返回什么？**

返回原始的响应体字符串。如果是 JSON，Taco 脚本可以用 `parseJson()` 解析（v7 一起实现）。如果 HTTP 请求失败（网络错误、非 200 状态码），Taco 按照它的错误处理哲学：打印清晰的错误信息，然后崩溃（`runtime_error`）。

### 更新 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(taco)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# ... 其他依赖 ...

# cpp-httplib
FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.0
)
FetchContent_MakeAvailable(cpp_httplib)

# nlohmann/json（用于 parseJson）
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(taco PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
)

# HTTPS 支持
find_package(OpenSSL QUIET)
if (OpenSSL_FOUND)
    target_link_libraries(taco PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(taco PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    message(STATUS "HTTPS support enabled (OpenSSL found)")
else()
    message(STATUS "HTTPS support disabled (OpenSSL not found)")
endif()
```

用 `find_package(OpenSSL QUIET)` 而不是 `REQUIRED`——如果没有 OpenSSL，HTTP 请求仍然可以工作，只是不支持 HTTPS。这样在没有 OpenSSL 的机器上也能编译。

---

## 36.2 用 cpp-httplib 发 HTTP 请求

### 核心实现：url_fetch 函数

在 `builtin.cpp` 里实现核心的网络请求函数：

```cpp
// builtin_net.cpp
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <nlohmann/json.hpp>
#include <string>
#include <stdexcept>
#include <tuple>

namespace taco {

// 解析 URL，返回 (scheme, host, path, port)
struct ParsedUrl {
    std::string scheme;  // "http" or "https"
    std::string host;
    std::string path;
    int port;
};

ParsedUrl parse_url(const std::string& url) {
    ParsedUrl result;

    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        throw std::runtime_error("Invalid URL (missing scheme): " + url);
    }

    result.scheme = url.substr(0, scheme_end);
    if (result.scheme != "http" && result.scheme != "https") {
        throw std::runtime_error("Unsupported scheme: " + result.scheme);
    }

    result.port = (result.scheme == "https") ? 443 : 80;

    std::string rest = url.substr(scheme_end + 3);  // 跳过 "://"

    // 提取 host（可能包含端口号）和 path
    size_t slash = rest.find('/');
    std::string host_part = (slash == std::string::npos) ? rest : rest.substr(0, slash);
    result.path = (slash == std::string::npos) ? "/" : rest.substr(slash);

    // 检查是否有显式端口号 host:port
    size_t colon = host_part.find(':');
    if (colon != std::string::npos) {
        result.host = host_part.substr(0, colon);
        result.port = std::stoi(host_part.substr(colon + 1));
    } else {
        result.host = host_part;
    }

    return result;
}

// HTTP GET 请求
// 返回响应 body
// 失败时抛出 std::runtime_error
std::string do_get(const std::string& url,
                   const httplib::Headers& headers = {}) {
    auto parsed = parse_url(url);

    httplib::Result res;

    if (parsed.scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        httplib::SSLClient cli(parsed.host, parsed.port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(60);
        cli.set_follow_location(true);
        // 验证证书（生产中应该开启）
        cli.enable_server_certificate_verification(true);
        res = headers.empty()
            ? cli.Get(parsed.path)
            : cli.Get(parsed.path, headers);
#else
        throw std::runtime_error(
            "HTTPS not supported (compile with OpenSSL)"
        );
#endif
    } else {
        httplib::Client cli(parsed.host, parsed.port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(60);
        cli.set_follow_location(true);
        res = headers.empty()
            ? cli.Get(parsed.path)
            : cli.Get(parsed.path, headers);
    }

    // 检查网络层错误（连接超时、DNS 失败等）
    if (!res) {
        throw std::runtime_error(
            "fetchUrl failed: " + httplib::to_string(res.error())
            + " (url: " + url + ")"
        );
    }

    // 检查 HTTP 状态码
    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error(
            "fetchUrl: HTTP " + std::to_string(res->status)
            + " (url: " + url + ")"
        );
    }

    return res->body;
}

// HTTP POST 请求
std::string do_post(const std::string& url,
                    const std::string& body,
                    const std::string& content_type = "application/json") {
    auto parsed = parse_url(url);

    httplib::Result res;

    if (parsed.scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        httplib::SSLClient cli(parsed.host, parsed.port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(60);
        cli.set_follow_location(true);
        res = cli.Post(parsed.path, body, content_type);
#else
        throw std::runtime_error("HTTPS not supported");
#endif
    } else {
        httplib::Client cli(parsed.host, parsed.port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(60);
        cli.set_follow_location(true);
        res = cli.Post(parsed.path, body, content_type);
    }

    if (!res) {
        throw std::runtime_error(
            "postData failed: " + httplib::to_string(res.error())
        );
    }

    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error(
            "postData: HTTP " + std::to_string(res->status)
        );
    }

    return res->body;
}

}  // namespace taco
```

### 注册为 Taco 内置函数

在 `builtin.cpp` 的 `register_builtins` 函数里加入网络相关的内置函数：

```cpp
// builtin.cpp（节选，加入 v7 网络部分）
#include "builtin_net.cpp"

void register_builtins(Environment& env) {
    // ... 之前的内置函数 ...

    // fetchUrl(url)  -> string
    env.define("fetchUrl", TacoValue(std::make_shared<TacoNative>(
        [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
            if (args.size() != 1 || !args[0].is_string()) {
                throw std::runtime_error(
                    "fetchUrl() expects one string argument (URL)"
                );
            }
            std::string result = taco::do_get(args[0].as_string());
            return TacoValue(result);
        }
    )));

    // postData(url, body)  -> string
    // postData(url, body, contentType)  -> string
    env.define("postData", TacoValue(std::make_shared<TacoNative>(
        [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
            if (args.size() < 2 || !args[0].is_string() || !args[1].is_string()) {
                throw std::runtime_error(
                    "postData() expects (url: string, body: string)"
                );
            }
            std::string url = args[0].as_string();
            std::string body = args[1].as_string();
            std::string content_type = "application/json";
            if (args.size() >= 3 && args[2].is_string()) {
                content_type = args[2].as_string();
            }
            std::string result = taco::do_post(url, body, content_type);
            return TacoValue(result);
        }
    )));

    // fetchWithHeaders(url, headersMap) -> string
    // 支持传请求头（比如 Authorization）
    env.define("fetchWithHeaders", TacoValue(std::make_shared<TacoNative>(
        [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
            if (args.size() != 2 || !args[0].is_string() || !args[1].is_map()) {
                throw std::runtime_error(
                    "fetchWithHeaders() expects (url: string, headers: map)"
                );
            }
            std::string url = args[0].as_string();

            // 把 Taco map 转成 httplib::Headers
            httplib::Headers headers;
            for (const auto& [k, v] : args[1].as_map()) {
                if (v.is_string()) {
                    headers.insert({k, v.as_string()});
                }
            }
            std::string result = taco::do_get(url, headers);
            return TacoValue(result);
        }
    )));
}
```

---

## 36.3 解析 JSON 响应：nlohmann/json

大多数 API 返回 JSON。Taco 需要能把 JSON 字符串解析成 Taco 的 map 和 array。

### nlohmann/json 简介

nlohmann/json 是 C++ 里最流行的 JSON 库，单头文件，语法优雅：

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 解析
json j = json::parse(R"({"name": "Miguel", "age": 12, "scores": [88, 92]})");

// 访问
std::string name = j["name"];  // "Miguel"
int age = j["age"];            // 12
auto scores = j["scores"];     // json array

// 遍历数组
for (auto& s : scores) {
    std::cout << s.get<int>() << "\n";
}

// 遍历对象
for (auto& [key, val] : j.items()) {
    std::cout << key << ": " << val << "\n";
}

// 检查类型
j["name"].is_string();   // true
j["age"].is_number();    // true
j["scores"].is_array();  // true
```

### 把 JSON 转成 TacoValue

需要一个递归函数，把 `nlohmann::json` 对象转成 `TacoValue`：

```cpp
// builtin_net.cpp（续）
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 把 nlohmann::json 转成 TacoValue（递归）
TacoValue json_to_taco(const json& j) {
    if (j.is_null()) {
        return TacoValue();  // nil
    }
    if (j.is_boolean()) {
        return TacoValue(j.get<bool>());
    }
    if (j.is_number_integer()) {
        return TacoValue(static_cast<double>(j.get<int64_t>()));
    }
    if (j.is_number_float()) {
        return TacoValue(j.get<double>());
    }
    if (j.is_string()) {
        return TacoValue(j.get<std::string>());
    }
    if (j.is_array()) {
        // JSON array -> TacoArray
        auto arr = std::make_shared<TacoArray>();
        for (const auto& elem : j) {
            arr->push_back(json_to_taco(elem));
        }
        return TacoValue(arr);
    }
    if (j.is_object()) {
        // JSON object -> TacoMap
        auto map = std::make_shared<TacoMap>();
        for (const auto& [key, val] : j.items()) {
            (*map)[key] = json_to_taco(val);
        }
        return TacoValue(map);
    }
    // 其他类型（不应该出现）
    return TacoValue(j.dump());  // 转成字符串兜底
}

// 把 TacoValue 转成 JSON 字符串
json taco_to_json(const TacoValue& v) {
    if (v.is_nil()) return nullptr;
    if (v.is_bool()) return v.as_bool();
    if (v.is_number()) return v.as_number();
    if (v.is_string()) return v.get_string();
    if (v.is_array()) {
        json arr = json::array();
        for (const auto& elem : v.as_array_ref()) {
            arr.push_back(taco_to_json(elem));
        }
        return arr;
    }
    if (v.is_map()) {
        json obj = json::object();
        for (const auto& [k, val] : v.as_map()) {
            obj[k] = taco_to_json(val);
        }
        return obj;
    }
    return v.to_string();  // 其他类型转字符串
}
```

### 注册 parseJson 和 toJson

```cpp
// 注册 parseJson 和 toJson

// parseJson(str) -> map/array/any
env.define("parseJson", TacoValue(std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
        if (args.size() != 1 || !args[0].is_string()) {
            throw std::runtime_error(
                "parseJson() expects one string argument"
            );
        }
        try {
            json j = json::parse(args[0].as_string());
            return json_to_taco(j);
        } catch (const json::parse_error& e) {
            throw std::runtime_error(
                std::string("parseJson: invalid JSON: ") + e.what()
            );
        }
    }
)));

// toJson(value) -> string
env.define("toJson", TacoValue(std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
        if (args.size() != 1) {
            throw std::runtime_error("toJson() expects one argument");
        }
        json j = taco_to_json(args[0]);
        return TacoValue(j.dump(2));  // 2 个空格缩进的 pretty-print
    }
)));

// toJsonCompact(value) -> string（不带缩进）
env.define("toJsonCompact", TacoValue(std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
        if (args.size() != 1) {
            throw std::runtime_error("toJsonCompact() expects one argument");
        }
        json j = taco_to_json(args[0]);
        return TacoValue(j.dump());
    }
)));
```

---

## 36.4 实现 `postData()`

`postData()` 在 36.2 里已经实现了。这里看一下它在 Taco 脚本里的完整用法：

```taco
// 发送 JSON POST 请求
var payload = {"name": "Miguel", "age": 12};
var json_str = toJson(payload);

var response_str = postData("https://httpbin.org/post", json_str);
var response = parseJson(response_str);

// httpbin.org/post 会把你发的数据原样返回
print(response["json"]["name"]);  // Miguel
print(response["json"]["age"]);   // 12
```

### 处理不同的 Content-Type

```taco
// 发 form 数据
var form_data = "username=Miguel&password=secret";
var resp = postData("https://httpbin.org/post",
                    form_data,
                    "application/x-www-form-urlencoded");
print(resp);
```

```cpp
// C++ 实现里，content_type 参数直接传给 httplib
res = cli.Post(parsed.path, body, content_type);
```

---

## 36.5 测试：用 Taco 脚本调用真实 API

### 测试 1：GitHub 用户信息

```taco
// github_user.taco
// 获取 GitHub 用户信息

func printUser(username) {
    var url = "https://api.github.com/users/{username}";
    var raw = fetchUrl(url);
    var user = parseJson(raw);

    print("=== GitHub User: {username} ===");
    print("Name:      " + (user["name"] ?? "N/A"));
    print("Company:   " + (user["company"] ?? "N/A"));
    print("Location:  " + (user["location"] ?? "N/A"));
    print("Bio:       " + (user["bio"] ?? "N/A"));
    print("Repos:     " + string(user["public_repos"]));
    print("Followers: " + string(user["followers"]));
    print("Following: " + string(user["following"]));
    print("Created:   " + user["created_at"]);
}

printUser("torvalds");
```

运行：

```
=== GitHub User: torvalds ===
Name:      Linus Torvalds
Company:   Linux Foundation
Location:  Portland, OR
Bio:       N/A
Repos:     8
Followers: 245000+
Following: 0
Created:   2011-09-04T19:48:04Z
```

### 测试 2：天气查询

```taco
// weather.taco
// wttr.in 提供简单的天气 API

var cities = ["Beijing", "Tokyo", "New York", "London", "Mexico City"];

cities.each { city in
    var url = "https://wttr.in/{city}?format=3";
    var weather = fetchUrl(url);
    print(weather.trimSpace());
};
```

运行：

```
Beijing: ⛅️  +28°C
Tokyo: 🌧  +22°C
New York: ☀️  +31°C
London: 🌦  +18°C
Mexico City: ⛅️  +22°C
```

### 测试 3：并发请求（用 thread）

```taco
// concurrent_fetch.taco
// 同时发多个请求，用 channel 收集结果

var ch = channel();
var urls = [
    "https://api.github.com/users/torvalds",
    "https://api.github.com/users/gvanrossum",
    "https://api.github.com/users/antirez"
];

// 为每个 URL 启动一个线程
var threads = urls.map { url in
    thread {
        var raw = fetchUrl(url);
        var user = parseJson(raw);
        ch.send(user["login"] + ": " + string(user["followers"]) + " followers");
    }
};

// 等待所有线程完成，收集结果
var results = [];
threads.each { t in
    var result = ch.receive();
    results.push(result);
    t.join();
};

results.sortBy { r in r }.each { r in print(r) };
```

运行：

```
antirez: 23000+ followers
gvanrossum: 28000+ followers
torvalds: 245000+ followers
```

注意：这里 `fetchUrl` 是同步的，但多个线程各自调用自己的 `fetchUrl`，所以三个请求实际上是并发发出的（每个线程阻塞在自己的 HTTP 请求上，互不影响）。

### 测试 4：Magic 8 Ball 🌮 问一个网络问题

```taco
// v7 的彩蛋整合
var answer = 🌮;

var url = "https://httpbin.org/get?question=" + answer;
var resp = fetchUrl(url);
var data = parseJson(resp);

print("The ball says: " + answer);
print("Server echoed: " + data["args"]["question"]);
```

运行：

```
The ball says: 🎱 It is certain.
Server echoed: 🎱 It is certain.
```

Magic 8 Ball 的答案可以穿越网络。

### 测试 5：一个实用脚本——查 IP 归属地

```taco
// ipinfo.taco
// 查询当前机器的公网 IP 和归属地

var ip_info = parseJson(fetchUrl("https://ipinfo.io/json"));

print("IP:       " + ip_info["ip"]);
print("City:     " + ip_info["city"]);
print("Region:   " + ip_info["region"]);
print("Country:  " + ip_info["country"]);
print("Org:      " + ip_info["org"]);
print("Timezone: " + ip_info["timezone"]);
```

运行：

```
IP:       203.xxx.xxx.xxx
City:     Singapore
Region:   Central Singapore
Country:  SG
Org:      AS9876 SingNet Pte Ltd
Timezone: Asia/Singapore
```

---

## 36.6 错误处理：网络请求失败怎么办

网络请求失败的情况有很多：DNS 解析失败、连接超时、服务器返回 4xx/5xx……

Taco 的哲学是**直接崩溃，打印清晰的错误信息**。在 C++ 实现里，`do_get` 和 `do_post` 在失败时抛出 `std::runtime_error`，求值器（evaluator）捕获它，打印 Taco 风格的错误信息：

```
🌮 line 3: fetchUrl failed: Connection refused
   var data = fetchUrl("http://localhost:9999/api");
              ^^^^^^^^^
```

在 Taco 脚本里，目前没有 try/catch 机制（Taco 不设计异常处理，出错就崩）。如果想要更优雅的错误处理，可以用 `fetchUrl` 的返回值约定一个特殊值（比如空字符串表示失败），但这会让 API 变得不一致。

更好的方案是用 `Result` enum（Taco 有关联值枚举）：

```taco
// 未来版本的想法（当前 v7 不实现）
var result = tryFetch("https://api.example.com");
switch result {
    case Result.Ok(data) { print(data); }
    case Result.Err(msg) { print("Failed: " + msg); }
}
```

这留给读者作为练习：在 C++ 层，让 `fetchUrl` 返回 `TacoResult` 而不是直接抛出异常。

---

## 36.7 v7 完整架构

v7 结束，Taco 的完整组件图：

```
taco/
  src/
    main.cpp              # 程序入口（脚本模式 / REPL 模式）
    token.h / token.cpp   # Token 定义和工具函数
    lexer.h / lexer.cpp   # 词法分析器（v0）
    ast.h                 # AST 节点定义（v1）
    parser.h / parser.cpp # 递归下降解析器（v1）
    value.h / value.cpp   # TacoValue（所有运行时类型）（v3 → v5 variant 重构）
    environment.h         # 作用域链（v3）
    evaluator.h           # 求值器主体
    evaluator.cpp         # 所有 AST 节点的 evaluate() 实现
    builtin.cpp           # 内置函数注册（v3 开始）
    builtin_io.cpp        # 文件、系统内置（v4）
    builtin_net.cpp       # 网络内置：fetchUrl、postData、parseJson（v7）
    repl.cpp              # REPL 实现（v6）
    error.h               # 错误报告
```

每一层的演化轨迹：

| 文件 | 引入版本 | 主要演化 |
|------|----------|----------|
| token.h | v0 | 稳定 |
| lexer.cpp | v0 | v1 加字符串插值 token |
| ast.h | v1 | v2 加控制流节点；v3 加函数/闭包节点 |
| parser.cpp | v1 | 每版加新语法 |
| value.h | v1（简单） | v3 加函数；v5 用 variant 重构 |
| evaluator.cpp | v1 | 每版加新 evaluate 逻辑 |
| builtin.cpp | v3 | v4 加 array/map 方法；v7 加网络 |
| repl.cpp | v6 | 新增 |
| builtin_net.cpp | v7 | 新增 |

---

## 小结

v7 是 Taco 的最后一次进化，也是最简单的一次——因为网络库帮我们做了所有繁琐的事情。

**`fetchUrl()` 和 `postData()`** 内部调用 `do_get` 和 `do_post`，用 cpp-httplib 发 HTTP/HTTPS 请求，返回响应 body 字符串。参数校验在 C++ lambda 里完成，失败时抛出 `std::runtime_error`，由求值器统一处理成 Taco 风格的错误输出。

**`parseJson()` 和 `toJson()`** 用 nlohmann/json 实现。`json_to_taco` 递归地把 JSON 对象转成 TacoValue（null → nil，boolean → bool，number → double，string → string，array → TacoArray，object → TacoMap），`taco_to_json` 做反向转换。

**并发网络请求**：`fetchUrl` 本身是同步的，并发交给 Taco 的 `thread`。每个线程有自己的 httplib Client 实例，不存在共享状态的问题，天然线程安全。

**URL 解析**：手写了一个简单的 `parse_url`，覆盖 http 和 https，支持显式端口号（`host:port`）。生产中应该用更健壮的 URL 解析库（比如 Ada URL parser），这里保持简单。

---

第八部分到这里结束。Taco 从一个只能切 Token 的 v0，进化成了一个能运行脚本、有 REPL、支持并发、能发网络请求的 v7。第九部分是综合收尾：回顾整个 Taco 项目，讲讲工程实践，展望接下来的路。
# 第三十七章：Taco 完整版

---

第九部分是全书的收尾。不再引入新的 C++ 知识点，而是回头看：我们建了什么，学了什么，还能往哪里走。

这一章回顾 Taco 的七次进化，看整个解释器的完整架构，然后实现一个一直缺席的功能：**模块系统**（`import`）。

---

## 37.1 回顾七次进化

从第六章的 v0 到第三十六章的 v7，Taco 用了全书的篇幅一步一步构建出来。每一步都有具体的动机，而不是为了演示 C++ 特性而演示。

### v0：词法分析器（第六章）

**问题**：源代码是一个字符串，无法直接处理。

**解决**：词法分析器（Lexer）把源代码切成 Token 列表。

**用到的 C++**：结构体、枚举、`std::string`、`std::vector`、基本函数。

```taco
// v0 能做的事
// 输入：var x = 10 + 20;
// 输出：[Var, Identifier("x"), Assign, Number(10), Plus, Number(20), Semicolon]
```

**局限**：只能切 Token，不知道 Token 之间的关系，不能执行任何代码。

### v1：语法分析器与 AST（第十一章）

**问题**：Token 列表是线性的，无法表达表达式的结构（`10 + 20 * 3` 中乘法优先于加法）。

**解决**：递归下降解析器把 Token 列表解析成抽象语法树（AST）。

**用到的 C++**：类、继承、`std::unique_ptr`、多态（虚函数雏形）。

```
// AST 的树形结构
BinaryExpr(Plus)
  ├── NumberLiteral(10)
  └── BinaryExpr(Multiply)
        ├── NumberLiteral(20)
        └── NumberLiteral(3)
```

**局限**：能构建 AST，能对基本表达式求值，但没有控制流、没有变量。

### v2：求值器与控制流（第十五章）

**问题**：解释器只能算表达式，不能跑程序（没有 if/while/for）。

**解决**：用继承体系设计完整的 AST 节点层次，用虚函数实现 `evaluate()`，实现 if/elseif/else、while、for（C 风格和 range 风格）、switch。

**用到的 C++**：继承、虚函数、多态、`dynamic_cast`、RAII 雏形。

```taco
// v2 能运行：
if (x > 10) {
    print("big");
} else {
    print("small");
}
for i in range(0, 5) { print(i); }
```

**局限**：有控制流了，但没有函数，程序无法抽象和复用。

### v3：函数、闭包与环境（第二十章）

**问题**：没有函数，所有逻辑都是平铺的，无法复用。

**解决**：实现 Environment（作用域链）、用户定义函数（`TacoFunction`）、闭包（函数捕获定义时的环境）、递归、命名参数、多返回值。

**用到的 C++**：`shared_ptr`（共享所有权管理作用域链）、移动语义、RAII（`ScopeGuard` 管理函数调用栈）、`std::function`（内置函数）。

```taco
// v3 能运行：
func makeCounter() {
    var count = 0;
    return func() { count = count + 1; return count; };
}
var c = makeCounter();
print(c());  // 1
print(c());  // 2
```

**局限**：没有内置数据结构（array、map），字符串方法缺失，无法处理集合数据。

### v4：array、map 与标准库（第二十五章）

**问题**：没有集合类型，Taco 写不了大部分实用脚本。

**解决**：用 `std::vector` 实现 TacoArray，用 `std::map` 实现 TacoMap，实现 pipeline 方法（`filter`、`map`、`each`、`reduce`），实现内置标准库（文件、系统、随机数）。

**用到的 C++**：STL 容器（`vector`、`map`、`unordered_map`）、算法（`transform`、`sort`、`find_if`）、Lambda 表达式、`std::function`。

```taco
// v4 能运行：
var nums = [1, 2, 3, 4, 5];
nums.filter { n in n % 2 == 0 }.map { n in n * n }.each { n in print(n) };
// 4 16
```

**局限**：值类型系统是一个大的 `std::variant` 手写版本，类型检查分散，性能有优化空间。

### v5：解释器模板化（第二十九章）

**问题**：v4 的值类型（`TacoValue`）是手写的 tagged union，类型检查代码散落在各处，容易出错，难以扩展。

**解决**：用 `std::variant` 重构 TacoValue，用 `std::visit` + 模板实现类型分派，用类模板重构 Environment，用 Visitor 模式实现 AST 节点遍历。

**用到的 C++**：`std::variant`、`std::visit`、函数模板、类模板、模板特化、`if constexpr`。

```cpp
// v5 之后的值类型
using TacoVariant = std::variant<
    std::monostate,                          // nil
    bool,                                    // bool
    double,                                  // number
    std::string,                             // string
    std::shared_ptr<TacoArray>,              // array
    std::shared_ptr<TacoMap>,               // map
    std::shared_ptr<TacoFunction>,           // function
    std::shared_ptr<TacoNative>             // native function
>;
```

**局限**：没有 REPL，没有并发，只能以文件模式运行脚本。

### v6：REPL 与并发（第三十三章）

**问题**：Taco 只能运行脚本文件，没有交互式环境；没有并发支持。

**解决**：实现 REPL（Read-Eval-Print Loop），用线程分离输入和求值，用 `std::condition_variable` 实现 channel，用 `std::mutex` 保护共享状态，实现 Taco 的 `thread`、`channel`、`mutex` 关键字。

**用到的 C++**：`std::thread`、`std::mutex`、`std::lock_guard`、`std::condition_variable`、`std::atomic`、`std::future`/`std::promise`。

```taco
// v6 能运行：
var ch = channel();
var t = thread { ch.send(42); };
var val = ch.receive();
print(val);  // 42
t.join();
```

**局限**：没有网络支持，不能和外部世界交互。

### v7：网络支持（第三十六章）

**问题**：Taco 是一个孤立的本地工具，不能调用 API、不能发 HTTP 请求。

**解决**：用 cpp-httplib 实现 `fetchUrl()` 和 `postData()`，用 nlohmann/json 实现 `parseJson()` 和 `toJson()`。

**用到的 C++**：第三方库集成（FetchContent）、cmake find_package、库封装设计。

```taco
// v7 能运行：
var user = parseJson(fetchUrl("https://api.github.com/users/torvalds"));
print("Name: " + user["name"]);
```

---

## 37.2 最终版本的完整架构图

```
┌─────────────────────────────────────────────────────┐
│                   main.cpp                          │
│  解析命令行参数：taco script.taco 或 taco（REPL）    │
└───────────────┬─────────────────────┬───────────────┘
                │                     │
         脚本模式                   REPL 模式
                │                     │
                ▼                     ▼
┌───────────────────────────────────────────────────────────┐
│                     前端：词法 + 语法                      │
│                                                           │
│  源代码字符串                                              │
│       │                                                   │
│       ▼                                                   │
│  Lexer（lexer.cpp）                                        │
│  源代码 → Token 列表                                       │
│       │                                                   │
│       ▼                                                   │
│  Parser（parser.cpp）                                      │
│  Token 列表 → AST（unique_ptr 树）                         │
└───────────────────────┬───────────────────────────────────┘
                        │
                        ▼
┌───────────────────────────────────────────────────────────┐
│                     后端：求值器                           │
│                                                           │
│  Evaluator（evaluator.cpp）                               │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  AST 节点 → evaluate() → TacoValue                  │  │
│  │                                                     │  │
│  │  Environment（environment.h）                        │  │
│  │  作用域链：shared_ptr<Env> → parent                  │  │
│  │                                                     │  │
│  │  TacoValue（value.h，std::variant）                  │  │
│  │  nil | bool | double | string | array |             │  │
│  │  map | TacoFunction | TacoNative                    │  │
│  └─────────────────────────────────────────────────────┘  │
└───────────────────────┬───────────────────────────────────┘
                        │
                        ▼
┌───────────────────────────────────────────────────────────┐
│                     内置库                                │
│                                                           │
│  builtin.cpp      核心内置（print, input, type, ...）     │
│  builtin_io.cpp   文件系统（cat, ls, mkdir, ...）         │
│  builtin_net.cpp  网络（fetchUrl, postData, parseJson）   │
│                                                           │
│  第三方库：                                               │
│    cpp-httplib    HTTP 客户端/服务器                       │
│    nlohmann/json  JSON 解析                               │
└───────────────────────────────────────────────────────────┘
```

### 数据流

一个 Taco 程序的完整执行过程：

```
源代码: var x = 10 + 20; print(x);
   │
   ▼ Lexer
Token: [Var, Id("x"), Assign, Num(10), Plus, Num(20), Semicolon,
        Id("print"), LParen, Id("x"), RParen, Semicolon]
   │
   ▼ Parser
AST:
  Program
  ├── VarDecl("x", BinaryExpr(Plus, Num(10), Num(20)))
  └── ExprStmt(CallExpr(Id("print"), [Id("x")]))
   │
   ▼ Evaluator
执行 VarDecl：
  计算 BinaryExpr(Plus, 10, 20) → TacoValue(30.0)
  在环境里定义 "x" = 30.0
执行 ExprStmt：
  查找 "print" → TacoNative(print_fn)
  查找 "x" → 30.0
  调用 print_fn([30.0]) → 输出 "30"
   │
   ▼ 输出
30
```

---

## 37.3 模块系统的实现：`import`

模块系统一直是个缺口。Taco 支持 `import utils;` 和 `from io import readFile;` 这样的语法（设计文档里有），但之前没有实现。现在补上。

### 设计决策

Taco 的模块系统非常简单：**一个 `.taco` 文件就是一个模块**，`import` 就是把那个文件求值一遍，把它的顶层定义导入当前作用域。

这和 Python 早期的 `import` 机制类似，也是很多脚本语言（Lua、早期 Ruby）的做法。不用 namespace，不用符号表分离，简单直接。

### 模块查找顺序

`import utils;` 会按以下顺序查找 `utils.taco`：

1. 当前脚本所在目录
2. 环境变量 `TACO_PATH` 指定的目录列表
3. Taco 安装目录的 `lib/` 下（标准库）

### 词法分析：新 Token

v7 的词法分析器已经识别 `import` 和 `from`（在 token.h 里定义了 `Import` 和 `From`），这里不需要改词法分析器。

### 语法分析：解析 import 语句

```cpp
// parser.cpp：解析 import 语句

// import utils;
// import utils as u;
// from io import readFile;
// from io import readFile, writeFile;

ExprPtr Parser::parse_import_stmt() {
    // import ...
    expect(TokenType::Import, "Expected 'import'.");

    auto node = std::make_unique<ImportStmt>();

    if (current().type == TokenType::Identifier) {
        node->module_name = advance().value;

        // import utils as u
        if (current().type == TokenType::Identifier
            && current().value == "as") {
            advance();  // 消费 "as"
            node->alias = expect(TokenType::Identifier,
                                 "Expected alias name after 'as'.").value;
        }
    }

    expect(TokenType::Semicolon, "Expected ';' after import.");
    return node;
}

// from io import readFile;
// from io import readFile, writeFile;
ExprPtr Parser::parse_from_import_stmt() {
    // from ...
    expect(TokenType::From, "Expected 'from'.");

    auto node = std::make_unique<FromImportStmt>();
    node->module_name = expect(TokenType::Identifier,
                               "Expected module name.").value;

    // import ...
    if (current().type != TokenType::Import) {
        throw ParseError("Expected 'import' after module name.", current());
    }
    advance();

    // 读取导入的名字列表
    do {
        node->names.push_back(
            expect(TokenType::Identifier, "Expected name to import.").value
        );
        if (current().type == TokenType::Comma) advance();
        else break;
    } while (true);

    expect(TokenType::Semicolon, "Expected ';' after import.");
    return node;
}
```

### AST 节点

```cpp
// ast.h（新增）

struct ImportStmt : Stmt {
    std::string module_name;    // "utils"
    std::string alias;          // "u"（可选）

    TacoValue evaluate(Evaluator& eval) override;
};

struct FromImportStmt : Stmt {
    std::string module_name;           // "io"
    std::vector<std::string> names;    // ["readFile", "writeFile"]

    TacoValue evaluate(Evaluator& eval) override;
};
```

### 求值：加载模块

```cpp
// evaluator.cpp

// 模块缓存：避免同一个模块被 import 多次重复执行
static std::unordered_map<std::string, std::shared_ptr<Environment>>
    module_cache;

// 查找模块文件
std::string find_module_file(const std::string& module_name,
                             const std::string& current_dir) {
    // 1. 当前目录
    std::string local = current_dir + "/" + module_name + ".taco";
    if (std::filesystem::exists(local)) return local;

    // 2. TACO_PATH 环境变量
    const char* taco_path = std::getenv("TACO_PATH");
    if (taco_path) {
        std::string path_str(taco_path);
        std::stringstream ss(path_str);
        std::string dir;
        // TACO_PATH 用冒号分隔（Unix 风格）
        while (std::getline(ss, dir, ':')) {
            std::string candidate = dir + "/" + module_name + ".taco";
            if (std::filesystem::exists(candidate)) return candidate;
        }
    }

    // 3. 标准库目录（可执行文件旁边的 lib/）
    // 这里简化，留给读者实现

    throw std::runtime_error(
        "Module not found: '" + module_name
        + "'. (Searched in: " + current_dir + ")"
    );
}

// 加载并执行模块，返回模块的环境
std::shared_ptr<Environment> load_module(const std::string& module_name,
                                          const std::string& current_dir,
                                          Evaluator& eval) {
    std::string filepath = find_module_file(module_name, current_dir);

    // 检查缓存
    auto it = module_cache.find(filepath);
    if (it != module_cache.end()) {
        return it->second;
    }

    // 读取文件
    std::ifstream f(filepath);
    if (!f) {
        throw std::runtime_error("Cannot open module file: " + filepath);
    }
    std::string source((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());

    // 在独立的环境里执行模块
    // 模块的父环境是全局环境（可以使用内置函数），但和当前脚本隔离
    auto module_env = std::make_shared<Environment>(eval.global_env());

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto program = parser.parse();

    // 用模块的目录作为当前目录（模块内的相对路径基于模块自身位置）
    std::string module_dir = std::filesystem::path(filepath).parent_path();
    Evaluator module_eval(module_env, module_dir);
    module_eval.evaluate(*program);

    // 缓存模块环境
    module_cache[filepath] = module_env;
    return module_env;
}

// import utils;
TacoValue ImportStmt::evaluate(Evaluator& eval) {
    auto module_env = load_module(module_name,
                                  eval.current_dir(),
                                  eval);

    // 把模块的所有顶层定义导入当前环境
    std::string prefix = alias.empty() ? "" : alias + ".";

    for (const auto& [name, value] : module_env->bindings()) {
        // 跳过以 _ 开头的"私有"定义
        if (!name.empty() && name[0] == '_') continue;

        std::string import_name = alias.empty() ? name : (alias + "." + name);
        eval.current_env()->define(import_name, value);
    }

    return TacoValue();  // import 语句不返回值
}

// from io import readFile;
TacoValue FromImportStmt::evaluate(Evaluator& eval) {
    auto module_env = load_module(module_name,
                                  eval.current_dir(),
                                  eval);

    for (const auto& name : names) {
        auto val = module_env->get(name);
        if (!val.has_value()) {
            throw std::runtime_error(
                "Module '" + module_name
                + "' has no export named '" + name + "'"
            );
        }
        eval.current_env()->define(name, *val);
    }

    return TacoValue();
}
```

### 测试模块系统

```taco
// math_utils.taco（模块文件）

// 私有辅助函数（以 _ 开头，import 时不导出）
func _clamp(x, lo, hi) {
    if (x < lo) { return lo; }
    if (x > hi) { return hi; }
    return x;
}

func square(x) { return x * x; }
func cube(x) { return x * x * x; }
func abs(x) { return x >= 0 ? x : -x; }

func factorial(n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

func isPrime(n) {
    if (n < 2) { return false; }
    for i in range(2, n) {
        if (n % i == 0) { return false; }
    }
    return true;
}

var PI = 3.14159265358979;
var E  = 2.71828182845905;
```

```taco
// main.taco

import math_utils;

print(math_utils.square(5));      // 25
print(math_utils.factorial(10));  // 3628800
print(math_utils.PI);             // 3.14159...
print(math_utils.isPrime(17));    // true

// _clamp 不会被导入
// print(math_utils._clamp(5, 0, 10));  // 报错：not defined
```

```taco
// 用 from 导入特定名字

from math_utils import square, PI;

print(square(7));  // 49
print(PI);         // 3.14159...
```

```taco
// 用别名

import math_utils as m;

print(m.square(4));   // 16
print(m.cube(3));     // 27
```

---

## 37.4 还能怎么扩展：垃圾回收、字节码、标准库

v7 是这本书的终点，但 Taco 作为一个解释器还有很多可以改进的方向。这里简要介绍，为有兴趣继续的读者指路。

### 垃圾回收（Garbage Collection）

Taco 目前用 `shared_ptr` 管理内存。`shared_ptr` 基于引用计数，有一个著名的问题：**循环引用**导致内存泄漏。

```taco
// 这在 Taco 里会产生循环引用
var a = {};
var b = {};
a["other"] = b;
b["other"] = a;
// a 和 b 的引用计数都不会降到 0，永远不释放
```

用 `weak_ptr` 可以打破一些循环，但不能解决所有情况。真实的解释器（Python、Ruby、Lua）用专门的垃圾回收器。

**最简单的 GC：标记-清除（Mark-Sweep）**

1. 从根集合（全局变量、调用栈上的值）出发
2. 遍历所有可达对象，标记为"存活"
3. 清除所有未标记的对象

实现标记-清除 GC 需要把所有堆对象放在一个全局列表里统一管理，放弃 `shared_ptr`，改用裸指针。工程量不小，但思路清晰。

**更高效的 GC：分代回收（Generational GC）**

大多数对象"活得很短"（局部变量、临时值）。分代回收把对象分成年轻代和老年代，优先回收年轻代，减少 GC 停顿时间。这是 Python、Java 的做法。

### 字节码虚拟机（Bytecode VM）

Taco 目前是**树遍历解释器**（tree-walking interpreter）：每次执行都遍历 AST 树，每个节点调用 `evaluate()`。这很简单，但有性能问题：

- AST 节点分散在堆上，缓存不友好
- 每个节点求值都有虚函数调用的开销
- 无法做很多编译期优化

改进方案：**编译成字节码**，用字节码虚拟机（VM）执行。

```
源代码 → Lexer → Parser → AST → 字节码编译器 → 字节码 → VM
```

字节码是一种紧凑的、为解释器设计的指令集：

```
// var x = 10 + 20; print(x);
PUSH_CONST  10     // 把 10 压栈
PUSH_CONST  20     // 把 20 压栈
ADD                // 弹出两个，相加，结果压栈
SET_LOCAL   0      // 把栈顶存到局部变量 0（x）
GET_LOCAL   0      // 把局部变量 0 压栈
CALL_NATIVE print  // 调用内置 print
```

字节码 VM 的优势：

- 字节码是连续内存，CPU 缓存友好
- 避免了虚函数调用的间接层
- 可以做很多编译期优化（常量折叠、死代码消除）

这是 CPython、Lua、Ruby（YARV）的实现方式。

### JIT 编译（Just-In-Time Compilation）

更进一步：在运行时把热点字节码编译成机器码。这是 V8（JavaScript）、PyPy（Python）、LuaJIT（Lua）的做法。实现复杂度高，但性能可以接近原生代码。

对于 Taco 这样的小解释器，JIT 不是必要的，但了解它的存在很重要。

### 标准库扩展

Taco 的标准库目前非常有限。可以参考 Python 或者 Lua 的标准库设计，添加：

- `taco.math`：更多数学函数（sin、cos、log、sqrt……）
- `taco.string`：更丰富的字符串操作
- `taco.io`：文件读写的更多模式（append、binary）
- `taco.net`：更完整的网络支持（WebSocket、UDP）
- `taco.db`：SQLite 接口

每个标准库模块都是一个 `.taco` 文件（用 `from taco.math import sqrt;` 导入），或者一个 C++ 内置函数集合。

### 类型系统

Taco 是动态类型的，类型错误在运行时才发现。可以加一个**可选的类型标注**：

```taco
// 想象中的 Taco 类型标注（目前不支持）
func add(x: number, y: number) -> number {
    return x + y;
}
```

加类型标注不一定要做全量静态类型检查，也可以是运行时检查（在函数调用时验证参数类型）。

---

## 小结

这一章是 Taco 的完整总结。

七次进化对应七组 C++ 知识：词法器（基础语法）→ AST（类与继承）→ 求值器（多态）→ 闭包（智能指针、RAII）→ 标准库（STL、Lambda）→ 模板化（variant、模板）→ REPL（多线程）→ 网络（第三方库）。

模块系统让 Taco 的代码可以分文件组织，`import`/`from import` 语法和 Python 风格一致，实现简单：找到 `.taco` 文件，在独立环境里执行，把顶层定义导入当前作用域，用文件路径做缓存键避免重复执行。

Taco 还有很多可以探索的方向：GC 替换 `shared_ptr` 解决循环引用，字节码 VM 提升性能，类型标注增强安全性，标准库扩展实用性。这些都是真实语言实现里的核心问题，有兴趣的读者可以从 Crafting Interpreters（Robert Nystrom）开始深入。

---

下一章讲 VSCode 插件：给 Taco 加上语法高亮和基础的 Language Server，让开发体验更好。
# 第三十八章：VSCode 插件与 Language Server

---

Taco 可以运行了，但写 Taco 脚本的体验还不够好——所有关键字都是同一个颜色，没有自动补全，写错了没有提示。

这一章给 Taco 加上编辑器支持：先做语法高亮（不需要 C++，只是配置文件），然后实现一个基础的 Language Server（需要 C++，让编辑器能和解释器通信），最后打包发布到 VSCode Marketplace。

---

## 38.1 LSP 是什么

在 LSP 出现之前，每个编辑器（VSCode、Vim、Emacs、Sublime Text……）和每种语言都需要单独集成。N 个编辑器 × M 种语言 = N×M 个集成点，每个都要独立实现。

**Language Server Protocol（LSP）** 是 Microsoft 在 2016 年提出的协议，把语言智能和编辑器解耦：

```
┌─────────┐   LSP（JSON-RPC）   ┌────────────────┐
│  编辑器  │ <─────────────────> │ Language Server │
│(Client) │                     │   (Server)     │
└─────────┘                     └────────────────┘
```

- **编辑器（LSP Client）**：VSCode、Vim、Emacs 等，发送用户操作（光标移动、打字、保存）作为请求
- **Language Server**：独立进程，理解某种语言，回答编辑器的问题（这个词是什么、这里有没有错误、补全建议是什么）

有了 LSP，一个 Language Server 可以为所有支持 LSP 的编辑器服务。现在已经有几百种语言的 Language Server（clangd 为 C++，rust-analyzer 为 Rust，pyright 为 Python……）。

### LSP 通信格式

LSP 用 JSON-RPC 2.0 通过 stdin/stdout 通信（或 TCP socket）。每条消息有一个 header 和 body：

```
Content-Length: 89\r\n
\r\n
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"processId":12345,...}}
```

编辑器发送请求，Language Server 发送响应：

```json
// 编辑器 → Server：初始化
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
        "processId": 12345,
        "rootUri": "file:///home/user/project",
        "capabilities": { ... }
    }
}

// Server → 编辑器：初始化响应
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "capabilities": {
            "textDocumentSync": 1,
            "completionProvider": {},
            "hoverProvider": true
        }
    }
}
```

常用的 LSP 方法：

| 方法 | 方向 | 含义 |
|------|------|------|
| `initialize` | Client → Server | 初始化，交换能力列表 |
| `textDocument/didOpen` | Client → Server | 用户打开了文件 |
| `textDocument/didChange` | Client → Server | 用户修改了文件 |
| `textDocument/completion` | Client → Server | 请求自动补全 |
| `textDocument/hover` | Client → Server | 鼠标悬停，请求信息 |
| `textDocument/publishDiagnostics` | Server → Client | 推送错误/警告 |

---

## 38.2 语法高亮：TextMate grammar

语法高亮不需要 Language Server，只需要一个 **TextMate grammar** 文件（`.tmLanguage.json`）。这是一个正则表达式规则集，定义哪些词应该用什么颜色显示。

### VSCode 插件结构

```
taco-language/
  package.json          # 插件配置（名字、版本、贡献点）
  language-configuration.json  # 括号匹配、注释、缩进规则
  syntaxes/
    taco.tmLanguage.json  # TextMate 语法文件
  server/
    src/
      server.cpp        # Language Server 实现
    CMakeLists.txt
  client/
    src/
      extension.ts      # VSCode 插件入口（TypeScript）
  package-lock.json
```

### package.json

```json
{
    "name": "taco-language",
    "displayName": "Taco Language",
    "description": "Language support for the Taco scripting language",
    "version": "0.1.0",
    "publisher": "your-name",
    "engines": {
        "vscode": "^1.85.0"
    },
    "categories": ["Programming Languages"],
    "contributes": {
        "languages": [
            {
                "id": "taco",
                "aliases": ["Taco", "taco"],
                "extensions": [".taco"],
                "configuration": "./language-configuration.json"
            }
        ],
        "grammars": [
            {
                "language": "taco",
                "scopeName": "source.taco",
                "path": "./syntaxes/taco.tmLanguage.json"
            }
        ]
    },
    "main": "./client/out/extension.js",
    "scripts": {
        "compile": "tsc -p ./client/tsconfig.json",
        "vscode:prepublish": "npm run compile"
    },
    "dependencies": {
        "vscode-languageclient": "^9.0.1"
    },
    "devDependencies": {
        "@types/vscode": "^1.85.0",
        "typescript": "^5.3.3"
    }
}
```

### language-configuration.json

```json
{
    "comments": {
        "lineComment": "//"
    },
    "brackets": [
        ["{", "}"],
        ["[", "]"],
        ["(", ")"]
    ],
    "autoClosingPairs": [
        { "open": "{", "close": "}" },
        { "open": "[", "close": "]" },
        { "open": "(", "close": ")" },
        { "open": "\"", "close": "\"" }
    ],
    "surroundingPairs": [
        ["{", "}"],
        ["[", "]"],
        ["(", ")"],
        ["\"", "\""]
    ],
    "indentationRules": {
        "increaseIndentPattern": "^.*\\{\\s*$",
        "decreaseIndentPattern": "^\\s*\\}"
    }
}
```

### taco.tmLanguage.json

TextMate grammar 用正则表达式匹配代码里的不同部分，给每部分打上 scope（范围标签），编辑器主题根据 scope 决定颜色。

```json
{
    "$schema": "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json",
    "name": "Taco",
    "scopeName": "source.taco",
    "patterns": [
        { "include": "#comments" },
        { "include": "#strings" },
        { "include": "#keywords" },
        { "include": "#numbers" },
        { "include": "#operators" },
        { "include": "#functions" },
        { "include": "#variables" },
        { "include": "#builtins" }
    ],
    "repository": {
        "comments": {
            "name": "comment.line.double-slash.taco",
            "match": "//.*$"
        },
        "strings": {
            "name": "string.quoted.double.taco",
            "begin": "\"",
            "end": "\"",
            "patterns": [
                {
                    "name": "constant.character.escape.taco",
                    "match": "\\\\[nrt\\\\\"']"
                },
                {
                    "name": "meta.embedded.expression.taco",
                    "begin": "\\{",
                    "end": "\\}",
                    "patterns": [{ "include": "$self" }]
                }
            ]
        },
        "keywords": {
            "name": "keyword.control.taco",
            "match": "\\b(var|func|if|elseif|else|while|for|in|return|class|struct|enum|extends|self|super|switch|case|default|import|from|thread|channel|mutex|true|false|nil)\\b"
        },
        "numbers": {
            "name": "constant.numeric.taco",
            "match": "\\b([0-9][0-9_]*\\.?[0-9_]*)\\b"
        },
        "operators": {
            "patterns": [
                {
                    "name": "keyword.operator.comparison.taco",
                    "match": "(==|!=|>=|<=|>|<)"
                },
                {
                    "name": "keyword.operator.logical.taco",
                    "match": "(&&|\\|\\||!)"
                },
                {
                    "name": "keyword.operator.arithmetic.taco",
                    "match": "(\\+|-|\\*|/|%|\\^)"
                },
                {
                    "name": "keyword.operator.assignment.taco",
                    "match": "="
                },
                {
                    "name": "keyword.operator.other.taco",
                    "match": "(\\?\\.|\\?\\?|\\.\\.\\.)"
                }
            ]
        },
        "functions": {
            "name": "entity.name.function.taco",
            "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*\\()"
        },
        "builtins": {
            "name": "support.function.builtin.taco",
            "match": "\\b(print|input|type|number|string|bool|cat|echo|ls|mkdir|rm|mv|cp|pwd|cd|exists|exec|env|fetchUrl|postData|parseJson|toJson|range)\\b"
        },
        "variables": {
            "name": "variable.other.taco",
            "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b"
        }
    }
}
```

这个 grammar 实现了：
- 注释（`//`）：灰色
- 字符串（双引号）：绿色，支持字符串插值（`{expr}` 内部也有高亮）
- 关键字（`var`、`func`、`if`……）：蓝色
- 数字：橙色
- 运算符：高亮
- 函数调用：函数名高亮
- 内置函数（`print`、`fetchUrl`……）：特殊颜色

---

## 38.3 实现基础 Language Server

语法高亮不需要 Language Server，只是静态规则。但自动补全、错误提示、悬停信息需要真正理解代码——需要 Language Server。

### Language Server 的架构

Taco 的 Language Server 是一个独立的 C++ 可执行文件，通过 stdin/stdout 和 VSCode 通信。VSCode 插件（TypeScript）启动这个进程，然后通过 LSP 协议和它交互。

```
VSCode
  └── extension.ts（LSP Client）
        │  stdin/stdout
        ▼
  taco-lsp（C++ Language Server）
        └── 复用 Taco 的 Lexer、Parser
```

Language Server 内部复用 Taco 的 Lexer 和 Parser——这就是把解释器拆成独立模块的好处。

### LSP 消息解析

LSP 消息通过 stdin 逐条读取，格式是 header + JSON body：

```cpp
// lsp_server.cpp

#include <iostream>
#include <string>
#include <sstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 读取一条 LSP 消息（阻塞直到读到完整消息）
json read_message() {
    // 读取 header 行
    int content_length = -1;
    std::string line;

    while (std::getline(std::cin, line)) {
        // 移除 \r（Windows 换行符）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            // 空行：header 结束，读取 body
            break;
        }

        // 解析 Content-Length
        const std::string prefix = "Content-Length: ";
        if (line.substr(0, prefix.size()) == prefix) {
            content_length = std::stoi(line.substr(prefix.size()));
        }
    }

    if (content_length < 0) {
        throw std::runtime_error("Missing Content-Length header");
    }

    // 读取 body
    std::string body(content_length, '\0');
    std::cin.read(body.data(), content_length);

    return json::parse(body);
}

// 发送一条 LSP 消息
void send_message(const json& msg) {
    std::string body = msg.dump();
    std::cout << "Content-Length: " << body.size() << "\r\n";
    std::cout << "\r\n";
    std::cout << body;
    std::cout.flush();
}

// 发送响应
void send_response(const json& id, const json& result) {
    send_message({
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    });
}

// 发送错误响应
void send_error(const json& id, int code, const std::string& message) {
    send_message({
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", {{"code", code}, {"message", message}}}
    });
}

// 发送通知（没有 id，Server 主动发给 Client）
void send_notification(const std::string& method, const json& params) {
    send_message({
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    });
}
```

### 文档管理

Language Server 需要维护当前打开的所有文档的内容：

```cpp
// 存储所有打开的文档内容
std::unordered_map<std::string, std::string> open_documents;

void handle_did_open(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    auto text = params["textDocument"]["text"].get<std::string>();
    open_documents[uri] = text;

    // 打开时立刻做一次诊断
    publish_diagnostics(uri, text);
}

void handle_did_change(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    // contentChanges 是增量更新列表，这里取全量（simpler）
    auto changes = params["contentChanges"];
    if (!changes.empty()) {
        open_documents[uri] = changes.back()["text"].get<std::string>();
    }

    // 每次改动后重新做诊断
    publish_diagnostics(uri, open_documents[uri]);
}
```

### 错误诊断（Diagnostics）

Diagnostics 是 LSP 里最有价值的功能：实时显示代码错误。

Taco 的词法分析器和语法分析器在遇到错误时会抛出异常。Language Server 捕获这些异常，把错误位置和信息转成 LSP Diagnostic，推送给编辑器：

```cpp
// Taco 错误结构（在 error.h 里定义）
struct TacoError {
    std::string message;
    int line;    // 1-indexed
    int column;  // 0-indexed
};

// 把 Taco 错误转成 LSP Diagnostic
json make_diagnostic(const TacoError& err, int severity = 1) {
    // severity: 1=Error, 2=Warning, 3=Information, 4=Hint
    return {
        {"range", {
            {"start", {{"line", err.line - 1}, {"character", err.column}}},
            {"end",   {{"line", err.line - 1}, {"character", err.column + 1}}}
        }},
        {"severity", severity},
        {"source", "taco"},
        {"message", err.message}
    };
}

void publish_diagnostics(const std::string& uri, const std::string& source) {
    json diagnostics = json::array();

    try {
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.tokenize();  // 可能抛出 LexError

        // 语法分析
        Parser parser(tokens);
        auto ast = parser.parse();      // 可能抛出 ParseError

        // 也可以做更深的语义分析，比如检查未定义变量
        // 但那需要一个不执行、只分析的 pass，暂时不实现

    } catch (const LexError& e) {
        diagnostics.push_back(make_diagnostic({e.message(), e.line(), e.column()}));
    } catch (const ParseError& e) {
        diagnostics.push_back(make_diagnostic({e.message(), e.line(), e.column()}));
    } catch (...) {
        // 其他错误忽略
    }

    send_notification("textDocument/publishDiagnostics", {
        {"uri", uri},
        {"diagnostics", diagnostics}
    });
}
```

这样，用户一写错了（比如漏了括号、拼错了关键字），编辑器里立刻出现红色波浪线。

### 自动补全（Completion）

自动补全返回一个候选项列表。对于 Taco，最简单的补全策略是：返回所有关键字和内置函数。

```cpp
// 所有 Taco 关键字
static const std::vector<std::string> TACO_KEYWORDS = {
    "var", "func", "if", "elseif", "else", "while", "for", "in",
    "return", "class", "struct", "enum", "extends", "self", "super",
    "switch", "case", "default", "import", "from", "thread", "channel",
    "mutex", "true", "false", "nil"
};

// 所有内置函数
static const std::vector<std::string> TACO_BUILTINS = {
    "print", "input", "type", "number", "string", "bool",
    "cat", "echo", "ls", "mkdir", "rm", "mv", "cp", "pwd", "cd", "exists",
    "exec", "env", "fetchUrl", "postData", "parseJson", "toJson",
    "range", "random"
};

json handle_completion(const json& params) {
    json items = json::array();

    // 关键字补全
    for (const auto& kw : TACO_KEYWORDS) {
        items.push_back({
            {"label", kw},
            {"kind", 14},          // 14 = Keyword
            {"detail", "keyword"}
        });
    }

    // 内置函数补全
    for (const auto& fn : TACO_BUILTINS) {
        items.push_back({
            {"label", fn},
            {"kind", 3},           // 3 = Function
            {"detail", "built-in function"}
        });
    }

    // 更高级：扫描当前文档，找用户定义的变量和函数名
    // ... （从 open_documents 里取当前文件内容，跑 lexer，提取标识符）

    return items;
}
```

### 悬停信息（Hover）

鼠标悬停在某个词上时，显示文档或类型信息：

```cpp
// 内置函数的文档字符串
static const std::unordered_map<std::string, std::string> BUILTIN_DOCS = {
    {"print",     "print(value)\n\nPrint value to stdout."},
    {"fetchUrl",  "fetchUrl(url: string) -> string\n\nSend HTTP GET request. Returns response body."},
    {"postData",  "postData(url: string, body: string, contentType?: string) -> string\n\nSend HTTP POST request."},
    {"parseJson", "parseJson(str: string) -> any\n\nParse JSON string into Taco value (map, array, etc.)."},
    {"range",     "range(start: number, end: number) -> array\n\nGenerate integer sequence [start, end)."},
    // ... 其他内置函数 ...
};

json handle_hover(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    auto line = params["position"]["line"].get<int>();
    auto character = params["position"]["character"].get<int>();

    // 从文档里取当前行
    auto& source = open_documents[uri];
    std::istringstream ss(source);
    std::string current_line;
    for (int i = 0; i <= line; i++) {
        std::getline(ss, current_line);
    }

    // 找光标位置的词
    // 向左找词的开始，向右找词的结束
    int start = character;
    while (start > 0 && (std::isalnum(current_line[start-1]) || current_line[start-1] == '_')) {
        start--;
    }
    int end = character;
    while (end < (int)current_line.size()
           && (std::isalnum(current_line[end]) || current_line[end] == '_')) {
        end++;
    }

    std::string word = current_line.substr(start, end - start);

    auto it = BUILTIN_DOCS.find(word);
    if (it != BUILTIN_DOCS.end()) {
        return {
            {"contents", {
                {"kind", "markdown"},
                {"value", "```taco\n" + it->second + "\n```"}
            }}
        };
    }

    return nullptr;  // 没有信息，不显示悬停框
}
```

### 主循环

把所有处理程序组装起来：

```cpp
int main() {
    // 禁用 stdout 缓冲（LSP 要求立即发送）
    std::cout.setf(std::ios::unitbuf);

    while (true) {
        json msg;
        try {
            msg = read_message();
        } catch (const std::exception& e) {
            // stdin 关闭（编辑器退出），退出 server
            break;
        }

        std::string method = msg.value("method", "");
        json id = msg.contains("id") ? msg["id"] : json(nullptr);
        json params = msg.value("params", json(nullptr));

        if (method == "initialize") {
            // 声明 server 的能力
            send_response(id, {
                {"capabilities", {
                    {"textDocumentSync", 1},          // 全量同步
                    {"completionProvider", json::object()},
                    {"hoverProvider", true}
                }},
                {"serverInfo", {
                    {"name", "taco-lsp"},
                    {"version", "0.1.0"}
                }}
            });

        } else if (method == "initialized") {
            // 通知：不需要回复

        } else if (method == "textDocument/didOpen") {
            handle_did_open(params);

        } else if (method == "textDocument/didChange") {
            handle_did_change(params);

        } else if (method == "textDocument/didClose") {
            auto uri = params["textDocument"]["uri"].get<std::string>();
            open_documents.erase(uri);

        } else if (method == "textDocument/completion") {
            send_response(id, handle_completion(params));

        } else if (method == "textDocument/hover") {
            auto result = handle_hover(params);
            send_response(id, result.is_null() ? json(nullptr) : result);

        } else if (method == "shutdown") {
            send_response(id, nullptr);

        } else if (method == "exit") {
            break;

        } else if (!id.is_null()) {
            // 未知请求：返回错误
            send_error(id, -32601, "Method not found: " + method);
        }
        // 未知通知（没有 id）：忽略
    }

    return 0;
}
```

---

## 38.4 自动补全与错误提示

把 Language Server 和 VSCode 插件连起来，需要写一个 TypeScript 的 client 端：

```typescript
// client/src/extension.ts
import * as path from 'path';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
    // Language Server 可执行文件的路径
    const serverPath = context.asAbsolutePath(
        path.join('server', 'build', 'taco-lsp')
    );

    const serverOptions: ServerOptions = {
        command: serverPath,
        transport: TransportKind.stdio   // 用 stdin/stdout 通信
    };

    const clientOptions: LanguageClientOptions = {
        // 只对 .taco 文件激活
        documentSelector: [{ scheme: 'file', language: 'taco' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.taco')
        }
    };

    client = new LanguageClient(
        'taco-language-server',
        'Taco Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
    console.log('Taco Language Server started.');
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) return undefined;
    return client.stop();
}
```

当用户打开 `.taco` 文件时，VSCode 自动启动 `taco-lsp` 进程，通过 stdin/stdout 建立 LSP 连接。之后：

- 每次文件内容变化 → 触发诊断 → 错误实时显示在编辑器里
- 按 `Ctrl+Space` → 请求补全 → 出现关键字和内置函数列表
- 鼠标悬停在内置函数上 → 显示文档字符串

效果大致如下：

```
// 写错了 elseif 的拼写：
if (x > 10) {
    print("big");
} elsief (x > 5) {       // ← 红色波浪线："Unexpected token 'elsief'"
    print("medium");
}

// 输入 fetch 后按 Ctrl+Space：
fetch                    // → 补全建议：fetchUrl
```

---

## 38.5 发布到 VSCode Marketplace

### 安装发布工具

```bash
npm install -g @vscode/vsce
```

### 打包插件

```bash
cd taco-language
npm install
npm run compile
# 编译 Language Server
cd server && cmake -B build && cmake --build build && cd ..
# 打包成 .vsix 文件
vsce package
# 生成 taco-language-0.1.0.vsix
```

`.vsix` 是 VSCode 插件的包格式，可以直接在 VSCode 里安装（Extensions → Install from VSIX）。

### 发布到 Marketplace

1. 在 https://marketplace.visualstudio.com 创建账号
2. 在 Azure DevOps 创建 Personal Access Token（PAT），权限选 Marketplace → Manage
3. 登录：`vsce login your-publisher-name`
4. 发布：`vsce publish`

发布后，任何人都可以在 VSCode 的扩展市场搜索 "Taco" 并安装。

### 在 package.json 里补充元信息

发布前完善插件的元信息：

```json
{
    "name": "taco-language",
    "displayName": "Taco Language",
    "description": "Syntax highlighting and language support for Taco (.taco) scripting language",
    "version": "0.1.0",
    "publisher": "your-publisher-name",
    "license": "MIT",
    "repository": {
        "type": "git",
        "url": "https://github.com/your-name/taco"
    },
    "bugs": {
        "url": "https://github.com/your-name/taco/issues"
    },
    "icon": "images/taco-icon.png",
    "keywords": ["taco", "scripting", "language"],
    "categories": ["Programming Languages"]
}
```

---

## 小结

**TextMate grammar**（`taco.tmLanguage.json`）是语法高亮的核心，用正则表达式规则把代码里的不同元素打上 scope 标签，主题根据 scope 决定颜色。不需要 C++，纯配置文件。

**Language Server Protocol（LSP）** 把语言智能和编辑器解耦。Language Server 是一个独立进程，通过 stdin/stdout 用 JSON-RPC 和编辑器通信。主要功能：

- `textDocument/publishDiagnostics`：推送错误信息（实时红线）
- `textDocument/completion`：提供自动补全列表
- `textDocument/hover`：悬停显示文档

Taco 的 Language Server 用 C++ 实现，内部复用 Lexer 和 Parser 来做语法错误检测，这体现了把解释器模块化的价值。

**VSCode 插件**：TypeScript 写的 client 负责启动 Language Server 进程，把 LSP 消息转发给 VSCode 的 Extension API。用户无感知，打开 `.taco` 文件就自动生效。

整个工具链：写一次 Language Server（C++），所有支持 LSP 的编辑器（VSCode、Neovim、Emacs、Helix……）都能用。这是现代语言工具链的标准做法。
# 第三十九章：C++ 生态与工程实践

---

Taco 的功能已经完整。这一章讲的是**工程**：怎么保证代码质量，怎么调试，怎么发布。

这些技能在任何 C++ 项目里都用得到，不只是 Taco。

---

## 39.1 常用第三方库概览：nlohmann/json、spdlog、Catch2

C++ 的标准库相比 Python 要精简得多，很多实用功能要靠第三方库。下面是写 C++ 项目时经常会用到的几个。

### nlohmann/json：JSON 处理

Taco 的 v7 已经用过了。几个常用模式：

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 解析
json j = json::parse(R"({"name": "Miguel", "scores": [88, 92, 76]})");

// 访问（带默认值）
std::string name = j.value("name", "unknown");
int first_score = j["scores"][0];

// 构建
json resp;
resp["status"] = "ok";
resp["data"]["user"] = "Miguel";
resp["data"]["scores"] = {88, 92, 76};
std::cout << resp.dump(2);  // pretty-print

// 序列化/反序列化自定义类型（需要实现 to_json/from_json）
struct Config {
    std::string host;
    int port;
};

void to_json(json& j, const Config& c) {
    j = {{"host", c.host}, {"port", c.port}};
}

void from_json(const json& j, Config& c) {
    j.at("host").get_to(c.host);
    j.at("port").get_to(c.port);
}

// 使用：
Config cfg{"localhost", 8080};
json j_cfg = cfg;                   // 自动调用 to_json
Config restored = j_cfg;           // 自动调用 from_json
```

### spdlog：日志库

`std::cout` 的问题：没有日志级别（debug/info/warn/error），多线程写会乱，没有时间戳，性能不好。spdlog 解决这些问题：

```cmake
# CMakeLists.txt
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(taco PRIVATE spdlog::spdlog)
```

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

int main() {
    // 默认 logger（输出到终端，带颜色）
    spdlog::info("Taco interpreter starting...");
    spdlog::debug("Parsing file: {}", filename);
    spdlog::warn("Deprecated feature used at line {}", line);
    spdlog::error("Failed to load module: {}", module_name);

    // 设置全局日志级别
    spdlog::set_level(spdlog::level::debug);  // 调试时开 debug 级别

    // 自定义 logger（同时输出到终端和文件）
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "taco.log", true  // true = 每次启动清空日志文件
    );

    auto logger = std::make_shared<spdlog::logger>(
        "taco",
        spdlog::sinks_init_list{console_sink, file_sink}
    );
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);

    spdlog::info("Logger initialized.");
}
```

在 Taco 里，调试模式（`taco --debug script.taco`）可以打开 debug 级别日志，输出词法分析、语法分析、求值的详细过程，帮助排查脚本问题。

### fmtlib：格式化字符串

fmtlib 是 C++20 `<format>` 的前身，也是 spdlog 格式化的基础。`{}` 占位符，支持各种格式选项：

```cpp
#include <fmt/format.h>
#include <fmt/ranges.h>   // 用于格式化容器

// 基本用法
std::string s = fmt::format("Hello, {}! You are {} years old.", "Miguel", 12);

// 格式选项
fmt::print("{:>10}\n", "right");     // 右对齐，宽度 10
fmt::print("{:0>5}\n", 42);         // 左填零，宽度 5：00042
fmt::print("{:.2f}\n", 3.14159);    // 2 位小数：3.14
fmt::print("{:#x}\n", 255);         // 16进制：0xff

// 格式化容器（需要 fmt/ranges.h）
std::vector<int> v = {1, 2, 3, 4, 5};
fmt::print("{}\n", v);              // [1, 2, 3, 4, 5]
```

### Catch2：单元测试框架

下一节详细介绍。

---

## 39.2 测试：用 Catch2 给 Taco 写单元测试

软件没有测试，就没有信心改动它。Taco 是一个相当复杂的系统，单元测试可以保证每个组件在修改后仍然正确工作。

### 引入 Catch2

```cmake
# CMakeLists.txt
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

# 测试可执行文件（和主程序分开）
add_executable(taco_tests
    tests/test_lexer.cpp
    tests/test_parser.cpp
    tests/test_evaluator.cpp
    # 共享 taco 的源文件（除了 main.cpp）
    src/lexer.cpp
    src/parser.cpp
    src/evaluator.cpp
    src/value.cpp
    src/environment.cpp
    src/builtin.cpp
)
target_link_libraries(taco_tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(taco_tests PRIVATE src)

# 让 CMake 的 test 命令运行 Catch2 测试
include(CTest)
include(Catch)
catch_discover_tests(taco_tests)
```

### 测试词法分析器

```cpp
// tests/test_lexer.cpp
#include <catch2/catch_test_macros.hpp>
#include "lexer.h"
#include "token.h"

TEST_CASE("Lexer tokenizes numbers", "[lexer]") {
    SECTION("integer") {
        Lexer lex("42");
        auto tokens = lex.tokenize();
        REQUIRE(tokens.size() == 2);  // 42, EOF
        REQUIRE(tokens[0].type == TokenType::Number);
        REQUIRE(tokens[0].value == "42");
    }

    SECTION("float") {
        Lexer lex("3.14");
        auto tokens = lex.tokenize();
        REQUIRE(tokens[0].type == TokenType::Number);
        REQUIRE(tokens[0].value == "3.14");
    }

    SECTION("underscore separator") {
        Lexer lex("1_000_000");
        auto tokens = lex.tokenize();
        // 词法分析器应该去掉下划线，得到 "1000000"
        REQUIRE(tokens[0].value == "1000000");
    }
}

TEST_CASE("Lexer tokenizes strings", "[lexer]") {
    SECTION("basic string") {
        Lexer lex(R"("hello world")");
        auto tokens = lex.tokenize();
        REQUIRE(tokens[0].type == TokenType::String);
        REQUIRE(tokens[0].value == "hello world");
    }

    SECTION("string with escape") {
        Lexer lex(R"("line1\nline2")");
        auto tokens = lex.tokenize();
        REQUIRE(tokens[0].value == "line1\nline2");
    }
}

TEST_CASE("Lexer tokenizes keywords", "[lexer]") {
    Lexer lex("var func if while return");
    auto tokens = lex.tokenize();
    REQUIRE(tokens[0].type == TokenType::Var);
    REQUIRE(tokens[1].type == TokenType::Func);
    REQUIRE(tokens[2].type == TokenType::If);
    REQUIRE(tokens[3].type == TokenType::While);
    REQUIRE(tokens[4].type == TokenType::Return);
}

TEST_CASE("Lexer handles line numbers", "[lexer]") {
    Lexer lex("var x = 1;\nvar y = 2;");
    auto tokens = lex.tokenize();
    // x 在第 1 行，y 在第 2 行
    REQUIRE(tokens[1].line == 1);  // x
    // 找到 y 所在的 token
    auto y_token = std::find_if(tokens.begin(), tokens.end(),
        [](const Token& t) { return t.value == "y"; });
    REQUIRE(y_token != tokens.end());
    REQUIRE(y_token->line == 2);
}

TEST_CASE("Lexer rejects invalid input", "[lexer]") {
    SECTION("unterminated string") {
        Lexer lex(R"("unclosed)");
        REQUIRE_THROWS_AS(lex.tokenize(), LexError);
    }

    SECTION("invalid character") {
        Lexer lex("@invalid");
        REQUIRE_THROWS_AS(lex.tokenize(), LexError);
    }
}
```

### 测试求值器

求值器测试最有价值：直接验证 Taco 脚本的执行结果。

```cpp
// tests/test_evaluator.cpp
#include <catch2/catch_test_macros.hpp>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "environment.h"
#include "builtin.h"

// 辅助函数：运行一段 Taco 代码，返回最后一个表达式的值
TacoValue run(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto ast = parser.parse();

    auto global = std::make_shared<Environment>();
    register_builtins(*global);

    Evaluator eval(global, ".");
    return eval.evaluate(*ast);
}

// 辅助函数：运行代码，捕获 print 的输出
std::string capture_output(const std::string& source) {
    // 替换 print 的实现，把输出写入字符串
    std::string output;
    // ... （通过依赖注入或全局状态捕获输出）
    return output;
}

TEST_CASE("Arithmetic expressions", "[evaluator]") {
    REQUIRE(run("1 + 2").as_number() == 3.0);
    REQUIRE(run("10 - 3").as_number() == 7.0);
    REQUIRE(run("4 * 5").as_number() == 20.0);
    REQUIRE(run("10 / 4").as_number() == 2.5);
    REQUIRE(run("10 % 3").as_number() == 1.0);
    REQUIRE(run("2 ^ 10").as_number() == 1024.0);
}

TEST_CASE("Comparison operators", "[evaluator]") {
    REQUIRE(run("1 < 2").as_bool() == true);
    REQUIRE(run("2 > 1").as_bool() == true);
    REQUIRE(run("1 == 1").as_bool() == true);
    REQUIRE(run("1 != 2").as_bool() == true);
    REQUIRE(run("2 >= 2").as_bool() == true);
    REQUIRE(run("1 <= 2").as_bool() == true);
}

TEST_CASE("String operations", "[evaluator]") {
    REQUIRE(run(R"("hello" + " world")").as_string() == "hello world");

    SECTION("string interpolation") {
        REQUIRE(run(R"(var name = "Miguel"; "Hola, {name}!")").as_string()
                == "Hola, Miguel!");
    }
}

TEST_CASE("Variable declaration and assignment", "[evaluator]") {
    REQUIRE(run("var x = 42; x").as_number() == 42.0);
    REQUIRE(run("var x = 1; x = 2; x").as_number() == 2.0);
}

TEST_CASE("If expression", "[evaluator]") {
    REQUIRE(run("var x = 10; if (x > 5) { 1 } else { 0 }").as_number() == 1.0);
    REQUIRE(run("var x = 3; if (x > 5) { 1 } else { 0 }").as_number() == 0.0);
}

TEST_CASE("While loop", "[evaluator]") {
    std::string code = R"(
        var i = 0;
        var sum = 0;
        while (i < 5) {
            sum = sum + i;
            i = i + 1;
        }
        sum
    )";
    REQUIRE(run(code).as_number() == 10.0);  // 0+1+2+3+4 = 10
}

TEST_CASE("Function definition and call", "[evaluator]") {
    SECTION("basic function") {
        std::string code = R"(
            func add(a, b) { return a + b; }
            add(3, 4)
        )";
        REQUIRE(run(code).as_number() == 7.0);
    }

    SECTION("recursion") {
        std::string code = R"(
            func fib(n) {
                if (n <= 1) { return n; }
                return fib(n - 1) + fib(n - 2);
            }
            fib(10)
        )";
        REQUIRE(run(code).as_number() == 55.0);
    }

    SECTION("closure") {
        std::string code = R"(
            func makeCounter() {
                var count = 0;
                return func() {
                    count = count + 1;
                    return count;
                };
            }
            var c = makeCounter();
            c(); c(); c()
        )";
        REQUIRE(run(code).as_number() == 3.0);
    }
}

TEST_CASE("Array operations", "[evaluator]") {
    SECTION("basic access") {
        REQUIRE(run("[1, 2, 3][0]").as_number() == 1.0);
        REQUIRE(run("[1, 2, 3][2]").as_number() == 3.0);
    }

    SECTION("len") {
        REQUIRE(run("[1, 2, 3].len()").as_number() == 3.0);
    }

    SECTION("push and pop") {
        std::string code = R"(
            var arr = [1, 2, 3];
            arr.push(4);
            arr.len()
        )";
        REQUIRE(run(code).as_number() == 4.0);
    }

    SECTION("filter") {
        std::string code = R"(
            [1, 2, 3, 4, 5].filter { x in x % 2 == 0 }.len()
        )";
        REQUIRE(run(code).as_number() == 2.0);
    }

    SECTION("reduce") {
        std::string code = R"(
            [1, 2, 3, 4, 5].reduce { (acc, x) in acc + x }
        )";
        REQUIRE(run(code).as_number() == 15.0);
    }
}

TEST_CASE("Map operations", "[evaluator]") {
    SECTION("access by key") {
        REQUIRE(run(R"({"x": 1, "y": 2}["x"])").as_number() == 1.0);
    }

    SECTION("dot access") {
        REQUIRE(run(R"(var m = {"name": "Miguel"}; m.name)").as_string()
                == "Miguel");
    }

    SECTION("getKeys") {
        std::string code = R"({"a": 1, "b": 2}.getKeys().len())";
        REQUIRE(run(code).as_number() == 2.0);
    }
}

TEST_CASE("Error handling", "[evaluator]") {
    REQUIRE_THROWS(run("undefined_var"));
    REQUIRE_THROWS(run("1 + \"string\""));  // 类型错误
    REQUIRE_THROWS(run("[1, 2, 3][10]"));   // 越界
}
```

### 运行测试

```bash
# 编译并运行测试
cmake -B build && cmake --build build
cd build && ctest --verbose

# 或者直接运行测试可执行文件
./build/taco_tests

# 只运行特定标签的测试
./build/taco_tests [lexer]
./build/taco_tests [evaluator]

# 输出更详细的信息
./build/taco_tests -v

# 测试失败时显示更多上下文
./build/taco_tests --show-catchup
```

Catch2 的输出：

```
===============================================================================
All tests passed (47 assertions in 18 test cases)
```

或者（有失败时）：

```
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
taco_tests is a Catch2 v3.5.4 host application.
Run with -? for options

───────────────────────────────────────────────────────────────────────────────
tests/test_evaluator.cpp:89: FAILED:
  REQUIRE( run(code).as_number() == 55.0 )
with expansion:
  54.0 == 55.0

===============================================================================
test cases:  18 | 17 passed | 1 failed
assertions:  47 | 46 passed | 1 failed
```

---

## 39.3 调试工具：gdb、valgrind、AddressSanitizer

C++ 有三类常见的运行时问题：**逻辑错误**（输出结果不对）、**内存错误**（越界、use-after-free）、**内存泄漏**。三类问题用不同的工具。

### gdb：调试逻辑错误

gdb 是 Linux 下的调试器，可以设置断点、单步执行、查看变量值。

编译时加 `-g` 生成调试符号（CMake 的 `Debug` 模式自动加）：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

基本 gdb 使用：

```bash
gdb ./build/taco

# 在 gdb 里：
(gdb) run script.taco       # 运行
(gdb) break evaluator.cpp:120  # 在第 120 行设断点
(gdb) break Evaluator::evaluate  # 在函数入口设断点
(gdb) continue              # 继续运行到下一个断点
(gdb) next                  # 单步（不进入函数）
(gdb) step                  # 单步（进入函数）
(gdb) print current_env     # 打印变量
(gdb) info locals           # 打印所有局部变量
(gdb) backtrace             # 查看调用栈
(gdb) quit                  # 退出
```

程序崩溃时，gdb 会停在崩溃点，可以查看调用栈和变量：

```bash
# 调试 core dump
gdb ./build/taco core
(gdb) backtrace
```

VSCode 有 C/C++ 插件，可以用图形界面调试，不需要记 gdb 命令。配置 `.vscode/launch.json`：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Taco",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/taco",
            "args": ["${workspaceFolder}/test.taco"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "MIMode": "gdb"
        }
    ]
}
```

### AddressSanitizer：内存错误

AddressSanitizer（ASan）是编译器内置的内存检查工具，能检测：

- 越界访问（buffer overflow）
- use-after-free（访问已释放的内存）
- use-after-scope（访问已出作用域的变量）
- double free

性能开销约 2x，适合在测试环境里开启：

```cmake
# CMakeLists.txt（Debug 模式下开启 ASan）
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(taco PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(taco PRIVATE -fsanitize=address)

    target_compile_options(taco_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(taco_tests PRIVATE -fsanitize=address)
endif()
```

当有内存错误时，ASan 打印详细的错误报告：

```
=================================================================
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000000110
READ of size 8 at 0x602000000110 thread T0
    #0 0x55555557a3c0 in TacoValue::as_number() src/value.h:45
    #1 0x55555557c1f0 in BinaryExpr::evaluate(Evaluator&) src/evaluator.cpp:89
    ...
0x602000000110 is located 0 bytes inside of 32-byte region [0x602000000110,0x602000000130)
freed by thread T0 here:
    #0 0x7ffff7c12b20 in operator delete(void*) ...
    #1 0x55555557b8a0 in ~TacoValue() src/value.h:30
    ...
```

### valgrind：内存泄漏

valgrind 检测内存泄漏和内存错误，比 ASan 慢（10x 以上），但不需要重新编译：

```bash
valgrind --leak-check=full --show-leak-kinds=all ./build/taco script.taco

# 输出（无泄漏时）：
# ==12345== LEAK SUMMARY:
# ==12345==    definitely lost: 0 bytes in 0 blocks
# ==12345==    indirectly lost: 0 bytes in 0 blocks
# ==12345==      possibly lost: 0 bytes in 0 blocks
# ==12345==    still reachable: 1,024 bytes in 3 blocks  ← 正常（全局对象）
```

`definitely lost`：真实的内存泄漏，一定要修。`still reachable`：程序退出时仍可访问的内存，通常是全局对象，不用担心。

### UndefinedBehaviorSanitizer

UBSan 检测 C++ 里的未定义行为：整数溢出、空指针解引用、越界数组访问……

```cmake
target_compile_options(taco PRIVATE -fsanitize=undefined)
target_link_options(taco PRIVATE -fsanitize=undefined)
```

可以同时开启 ASan 和 UBSan：

```cmake
target_compile_options(taco PRIVATE -fsanitize=address,undefined)
target_link_options(taco PRIVATE -fsanitize=address,undefined)
```

---

## 39.4 代码风格：clang-format、clang-tidy

代码风格统一可以减少 code review 时的噪音，让团队专注于逻辑而不是格式。

### clang-format：自动格式化

clang-format 根据配置文件自动格式化 C++ 代码：

```yaml
# .clang-format（放在项目根目录）
BasedOnStyle: LLVM       # 基于 LLVM 风格（可选：Google、Mozilla、WebKit、Chromium）
IndentWidth: 4           # 4 空格缩进
ColumnLimit: 100         # 每行最多 100 字符
PointerAlignment: Left   # int* ptr 而不是 int *ptr
AllowShortFunctionsOnASingleLine: Empty    # 空函数允许单行：{}
AllowShortIfStatementsOnASingleLine: Never # if 不允许单行
SortIncludes: CaseSensitive  # 自动排序 #include
```

使用：

```bash
# 格式化单个文件（修改原文件）
clang-format -i src/evaluator.cpp

# 格式化所有 .cpp 和 .h 文件
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# 只检查（不修改），输出差异（用于 CI）
clang-format --dry-run --Werror src/evaluator.cpp
```

在 VSCode 里安装 C/C++ 插件后，保存时自动格式化（设置 `"editor.formatOnSave": true`）。

### clang-tidy：静态分析

clang-tidy 是一个 linter，检测代码里的潜在问题：

```yaml
# .clang-tidy（放在项目根目录）
Checks: >
    clang-diagnostic-*,
    clang-analyzer-*,
    cppcoreguidelines-*,
    modernize-*,
    performance-*,
    readability-*,
    -modernize-use-trailing-return-type,
    -cppcoreguidelines-avoid-magic-numbers,
    -readability-magic-numbers

WarningsAsErrors: ""
HeaderFilterRegex: "src/.*"
```

```bash
# 分析单个文件（需要 compile_commands.json）
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy src/evaluator.cpp -p build/

# 自动修复部分问题
clang-tidy src/evaluator.cpp -p build/ --fix
```

clang-tidy 的常见检查：

- `modernize-use-auto`：建议用 `auto`（`std::vector<int>::iterator it` → `auto it`）
- `modernize-use-nullptr`：用 `nullptr` 替换 `NULL` 或 `0`
- `modernize-use-override`：重写虚函数时加 `override`
- `performance-unnecessary-copy-initialization`：不必要的拷贝
- `cppcoreguidelines-avoid-c-arrays`：用 `std::array` 替代 C 数组
- `readability-identifier-naming`：命名规范检查

### 在 CI 里强制执行

```yaml
# .github/workflows/lint.yml
name: Lint
on: [push, pull_request]
jobs:
  clang-format:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Run clang-format
        run: |
          find src -name "*.cpp" -o -name "*.h" | \
          xargs clang-format --dry-run --Werror
```

---

## 39.5 发布到 GitHub：README、CI、二进制发布

### README.md

一个好的 README 是项目的门面。Taco 的 README 应该包含：

```markdown
# 🌮 Taco

> A simple, spicy scripting language.

## Quick Start

```bash
# 安装
brew install taco          # macOS
apt install taco           # Ubuntu

# 运行脚本
taco hello.taco

# REPL
taco
```

## Hello, World

```taco
print("¡Hola, mundo!");
```

## Features

- Dynamic typing, clean syntax
- First-class functions and closures
- Array and map with pipeline methods
- Built-in HTTP support: fetchUrl(), postData()
- Concurrent: thread, channel, mutex
- Interactive REPL

## Building from Source

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/taco --version
```

Requires: CMake 3.20+, C++17 compiler, OpenSSL (optional, for HTTPS).

## Language Reference

[Taco Language Spec](docs/spec.md)

## License

MIT
```

### GitHub Actions CI

```yaml
# .github/workflows/ci.yml
name: CI
on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build-and-test:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      - name: Install OpenSSL (Ubuntu)
        if: runner.os == 'Linux'
        run: sudo apt-get install -y libssl-dev

      - name: Install OpenSSL (macOS)
        if: runner.os == 'macOS'
        run: brew install openssl

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release

      - name: Test
        run: |
          cd build
          ctest --config Release --output-on-failure

      - name: Run integration test
        run: |
          echo 'print("hello from CI");' > /tmp/test.taco
          ./build/taco /tmp/test.taco
```

### 二进制发布

用 GitHub Actions 在 tag 触发时自动构建并发布二进制：

```yaml
# .github/workflows/release.yml
name: Release
on:
  push:
    tags:
      - 'v*'   # 触发条件：推送 v0.1.0 这样的 tag

jobs:
  build:
    strategy:
      matrix:
        include:
          - os: ubuntu-latest
            artifact: taco-linux-x86_64
          - os: macos-latest
            artifact: taco-macos-x86_64
          - os: windows-latest
            artifact: taco-windows-x86_64.exe

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      - name: Build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build --config Release

      - name: Rename binary
        run: |
          # Linux/macOS
          cp build/taco ${{ matrix.artifact }}   # 或者 .exe 版本

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: ${{ matrix.artifact }}
          path: ${{ matrix.artifact }}

  release:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - uses: actions/download-artifact@v4

      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            taco-linux-x86_64/taco-linux-x86_64
            taco-macos-x86_64/taco-macos-x86_64
            taco-windows-x86_64.exe/taco-windows-x86_64.exe
          generate_release_notes: true
```

发布流程：

```bash
# 打 tag 触发自动发布
git tag v0.1.0
git push origin v0.1.0

# GitHub Actions 自动：
# 1. 在 Linux、macOS、Windows 上构建
# 2. 运行测试
# 3. 创建 GitHub Release，上传三个平台的二进制文件
```

---

## 小结

**第三方库**：nlohmann/json 处理 JSON，spdlog 处理日志，fmtlib 处理格式化字符串，Catch2 做单元测试。这四个库几乎在所有 C++ 项目里都会用到。

**Catch2 单元测试**：`TEST_CASE` + `SECTION` 组织测试，`REQUIRE` 断言结果，`REQUIRE_THROWS_AS` 断言异常。Taco 的测试覆盖 Lexer、Parser、Evaluator 三层，每个语言特性有对应的测试用例。

**调试工具三件套**：
- gdb：调试逻辑错误，断点、单步、查看变量
- AddressSanitizer：编译时插桩，检测内存越界、use-after-free
- valgrind：运行时检测内存泄漏

**代码质量工具**：
- clang-format：自动格式化，`.clang-format` 配置风格
- clang-tidy：静态分析，发现潜在问题，建议现代 C++ 写法

**发布工程**：GitHub Actions 实现 CI（每次 push 自动构建测试）和 CD（打 tag 自动发布多平台二进制）。好的 README 是项目的入口，包含快速开始、特性列表、构建说明。
# 第四十章：接下来去哪里

---

这是全书的最后一章。

不讲新的 C++ 特性，不加新的 Taco 功能。回头看走过的路，然后往前看：C++ 还有哪些没讲到的重要内容，Taco 还能往哪里走，这门语言的未来是什么样的。

---

## 40.1 没讲到的重要内容

这本书覆盖了现代 C++ 的核心，但 C++ 是一门极其宽广的语言，还有很多重要的东西没有触及。

### 异常处理（Exception Handling）

Taco 的错误处理哲学是"直接崩溃"，所以 C++ 的异常机制在 Taco 内部只是作为内部错误传播的工具，没有专门讲。但在真实项目里，异常是主流的错误处理机制之一。

```cpp
// 抛出异常
void parse_number(const std::string& s) {
    try {
        size_t pos;
        double v = std::stod(s, &pos);
        if (pos != s.size()) {
            throw std::invalid_argument("not a number: " + s);
        }
    } catch (const std::invalid_argument& e) {
        // 处理特定类型的异常
        std::cerr << "Error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        // 处理所有标准异常
        std::cerr << "Unexpected error: " << e.what() << "\n";
    } catch (...) {
        // 处理所有异常（不推荐，除非是最外层的兜底）
        std::cerr << "Unknown error.\n";
    }
}
```

C++ 异常的争议：有些代码库（Google、游戏引擎）禁用异常，用错误码或者 `std::expected`（C++23）替代。理由是：异常会让控制流变得隐式，难以推理；在嵌入式、实时系统里，异常的开销不可接受。

哪种方式更好，至今仍有争论。了解两种方式，根据项目需求做选择。

**推荐阅读**：《A Tour of C++》（Bjarne Stroustrup）第 3 章；《Effective C++》条款 8-9（Scott Meyers）。

### 协程（Coroutine，C++20）

C++20 加入了语言级别的协程支持。`co_await`、`co_yield`、`co_return` 三个关键字，让异步代码写起来像同步代码。

第三十五章介绍 Asio 时提到了协程，但没有深讲。C++20 协程是一个相当复杂的特性——标准只提供了底层机制（promise type、awaitable concept），没有提供高层抽象，需要库来封装（Asio、cppcoro、libcopp）。

```cpp
// C++20 协程（需要 Asio 1.20+ 或其他协程库）
asio::awaitable<std::string> fetch_and_parse(std::string url) {
    auto body = co_await async_fetch(url);     // 暂停，等待 HTTP 响应
    auto json = co_await async_parse(body);    // 暂停，等待 JSON 解析
    co_return json["result"].get<std::string>();
}
```

**推荐阅读**：《C++20: The Complete Guide》（Nicolai Josuttis）；David Mazières 的《My tutorial and take on C++20 coroutines》。

### 模块（Module，C++20）

C++20 引入了语言级别的模块系统，解决头文件的种种问题：

- **编译速度**：头文件每次被 include 都重新编译；模块只编译一次
- **宏污染**：头文件里的宏会泄漏到所有 include 它的地方；模块不会
- **循环依赖**：头文件容易形成循环依赖；模块有明确的导入关系

```cpp
// 定义一个模块（token.cppm）
export module taco.token;

import std;  // 导入标准库

export enum class TokenType { ... };  // export 关键字：对外可见
export struct Token { ... };

// 内部实现（不 export 的不对外可见）
static std::string escape_string(const std::string& s) { ... }
```

```cpp
// 使用模块
import taco.token;    // 导入，不是 #include

Token t;
```

模块在 C++20 编译器里已经可用，但工具链支持（CMake、构建系统、IDE）还在完善中。

**推荐**：等编译器和工具链的支持成熟后迁移（大概 2025-2026 年是比较好的时机）。

### `std::expected`（C++23）

`std::expected<T, E>` 是一个"要么有值，要么有错误"的类型，是异常的一个替代方案：

```cpp
// C++23
std::expected<double, std::string> parse_number(const std::string& s) {
    try {
        return std::stod(s);
    } catch (...) {
        return std::unexpected("not a number: " + s);
    }
}

// 使用
auto result = parse_number("42.0");
if (result) {
    std::cout << "Parsed: " << *result << "\n";
} else {
    std::cerr << "Error: " << result.error() << "\n";
}
```

比异常更显式（函数签名里写明了可能出错），比错误码更方便（可以用 monadic 操作链式处理）：

```cpp
auto final = parse_number(input)
    .and_then([](double v) -> std::expected<int, std::string> {
        if (v < 0) return std::unexpected("negative number");
        return static_cast<int>(v);
    })
    .transform([](int n) { return n * 2; });
```

### ranges（C++20）

第二十二章提到了 ranges，但只是预览。`std::ranges` 提供了更好的算法接口，以及 views（惰性求值的视图）：

```cpp
#include <ranges>
#include <vector>
#include <iostream>

std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// 传统算法（三步走）
std::vector<int> temp;
std::copy_if(nums.begin(), nums.end(), std::back_inserter(temp),
             [](int n) { return n % 2 == 0; });
std::vector<int> result;
std::transform(temp.begin(), temp.end(), std::back_inserter(result),
               [](int n) { return n * n; });

// ranges（pipeline 风格，惰性求值）
auto result2 = nums
    | std::views::filter([](int n) { return n % 2 == 0; })
    | std::views::transform([](int n) { return n * n; });

for (int n : result2) std::cout << n << " ";
// 4 16 36 64 100
```

注意：`result2` 不是一个容器，而是一个惰性视图（view）——遍历时才真正计算，不创建中间容器。这和 Taco 的 `.filter { }.map { }` 在语义上接近，但 Taco 的版本是严格求值的（每步都创建新 array）。

---

## 40.2 Taco 的下一步：垃圾回收、字节码虚拟机、JIT

上一章简要介绍了 Taco 可以扩展的方向。这里多说一点关于实现路径的细节，供有兴趣继续做的读者参考。

### 从 shared_ptr 迁移到 GC

Taco 目前用 `shared_ptr` 管理值对象。要替换成 GC，大致步骤：

**Step 1**：把所有堆分配的 Taco 对象（TacoArray、TacoMap、TacoFunction、Environment）注册到一个全局的 GC 堆里：

```cpp
class GcHeap {
public:
    // 分配一个新的 GC 对象
    template<typename T, typename... Args>
    T* alloc(Args&&... args) {
        auto* obj = new T(std::forward<Args>(args)...);
        all_objects_.insert(obj);
        maybe_collect();
        return obj;
    }

    void collect();  // 触发 mark-sweep

private:
    std::unordered_set<GcObject*> all_objects_;
    std::unordered_set<GcObject*> roots_;  // 根对象集合（全局变量、栈上的值）
};
```

**Step 2**：TacoValue 改用裸指针（不是 `shared_ptr`），由 GcHeap 统一管理：

```cpp
// 之前：TacoValue 持有 shared_ptr<TacoArray>
// 之后：TacoValue 持有 TacoArray*（GC 管理的裸指针）
```

**Step 3**：实现 mark-sweep：

```cpp
void GcHeap::collect() {
    // Mark phase：从根集合出发，标记所有可达对象
    for (GcObject* root : roots_) {
        mark(root);
    }

    // Sweep phase：回收所有未标记的对象
    for (auto it = all_objects_.begin(); it != all_objects_.end(); ) {
        if (!(*it)->is_marked()) {
            delete *it;
            it = all_objects_.erase(it);
        } else {
            (*it)->unmark();  // 清除标记，为下次 GC 准备
            ++it;
        }
    }
}
```

这是 Lua 5.0 之前的做法。Lua 5.1 开始用增量 GC（interleave mark-sweep 和程序执行，减少停顿），Lua 5.4 加了分代 GC。

**推荐阅读**：《Crafting Interpreters》（Robert Nystrom）第 26 章：Garbage Collection。这本书可能是实现解释器最好的入门书，免费在线阅读。

### 字节码虚拟机

把 Taco 改成字节码 VM 是一个很大的工程，但思路清晰：

**Step 1**：定义字节码指令集：

```cpp
enum class OpCode : uint8_t {
    // 常量
    LOAD_CONST,    // 从常量池加载
    LOAD_NIL,
    LOAD_TRUE,
    LOAD_FALSE,

    // 局部变量
    LOAD_LOCAL,    // 从局部变量槽加载
    STORE_LOCAL,

    // 全局变量
    LOAD_GLOBAL,   // 从全局环境加载
    STORE_GLOBAL,

    // 算术
    ADD, SUB, MUL, DIV, MOD, POW,
    NEG,           // 取反

    // 比较
    EQ, NEQ, LT, LE, GT, GE,

    // 控制流
    JUMP,          // 无条件跳转
    JUMP_IF_FALSE, // 条件跳转
    LOOP,          // 回跳（用于循环）

    // 函数
    CALL,          // 调用函数
    RETURN,

    // 其他
    PRINT,
    POP,           // 丢弃栈顶
};
```

**Step 2**：写编译器（AST → 字节码）：

```cpp
class Compiler {
public:
    Chunk compile(const Program& ast);

private:
    void compile_expr(const Expr& expr);
    void compile_stmt(const Stmt& stmt);

    void emit(OpCode op);
    void emit(OpCode op, uint8_t arg);
    int emit_jump(OpCode op);     // 发射跳转指令，返回位置用于回填
    void patch_jump(int offset);  // 回填跳转目标

    Chunk current_chunk_;
};
```

**Step 3**：写 VM（执行字节码）：

```cpp
class VM {
public:
    InterpretResult run(const Chunk& chunk);

private:
    const uint8_t* ip_;          // 指令指针（instruction pointer）
    std::vector<TacoValue> stack_;  // 操作数栈
    std::vector<TacoValue> globals_; // 全局变量

    TacoValue pop() { auto v = stack_.back(); stack_.pop_back(); return v; }
    void push(TacoValue v) { stack_.push_back(std::move(v)); }
};

InterpretResult VM::run(const Chunk& chunk) {
    ip_ = chunk.code.data();

    for (;;) {
        OpCode op = static_cast<OpCode>(*ip_++);

        switch (op) {
        case OpCode::LOAD_CONST: {
            uint8_t idx = *ip_++;
            push(chunk.constants[idx]);
            break;
        }
        case OpCode::ADD: {
            auto b = pop();
            auto a = pop();
            push(TacoValue(a.as_number() + b.as_number()));
            break;
        }
        case OpCode::JUMP_IF_FALSE: {
            uint16_t offset = (ip_[0] << 8) | ip_[1];
            ip_ += 2;
            if (!pop().as_bool()) ip_ += offset;
            break;
        }
        case OpCode::RETURN:
            return InterpretResult::OK;
        // ...
        }
    }
}
```

**推荐阅读**：《Crafting Interpreters》第二部分（第 14-30 章）完整实现了一个字节码 VM，是目前最好的实践参考。

---

## 40.3 网络编程深入：Boost.Asio、gRPC

第八部分只是网络编程的入门。如果想深入：

### Boost.Asio 与高并发服务器

独立 Asio 是 Boost.Asio 的子集。Boost.Asio 功能更完整：

```cpp
// 一个用 Asio 协程实现的 echo 服务器
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/awaitable.hpp>
using namespace boost::asio;

awaitable<void> handle_client(ip::tcp::socket socket) {
    char buf[1024];
    for (;;) {
        // 读数据（暂停协程，等数据到达）
        size_t n = co_await socket.async_read_some(
            buffer(buf), use_awaitable
        );
        // 回写（echo）
        co_await async_write(socket, buffer(buf, n), use_awaitable);
    }
}

awaitable<void> listener(io_context& io) {
    ip::tcp::acceptor acceptor(io, {ip::tcp::v4(), 8080});
    for (;;) {
        auto socket = co_await acceptor.async_accept(use_awaitable);
        // 为每个连接启动一个协程
        co_spawn(io, handle_client(std::move(socket)), detached);
    }
}

int main() {
    io_context io;
    co_spawn(io, listener(io), detached);
    io.run();  // 单线程处理所有连接！
}
```

这个 echo 服务器用单线程处理所有客户端连接（协程切换，不是线程切换），可以轻松处理数万个并发连接。

### gRPC：微服务 RPC

gRPC 是 Google 开发的 RPC 框架，基于 HTTP/2 和 Protobuf：

```protobuf
// taco_service.proto
syntax = "proto3";

service TacoRunner {
    rpc Run(RunRequest) returns (RunResponse);
    rpc RunStream(RunRequest) returns (stream OutputLine);
}

message RunRequest {
    string code = 1;
}

message RunResponse {
    bool success = 1;
    string output = 2;
    string error = 3;
}

message OutputLine {
    string line = 1;
}
```

生成 C++ 代码后，可以把 Taco 解释器包装成一个网络服务——别人发来 Taco 代码，服务端执行并返回结果。这就是云 IDE（比如 Replit）的基本思路。

---

## 40.4 值得读的书与资源

把这本书读完，是一个起点，不是终点。下面是继续深入的资源。

### C++ 书籍

**入门到进阶**

- **《A Tour of C++》**（Bjarne Stroustrup）——C++ 之父写的概览，简洁，现代，适合有基础的读者。第三版覆盖 C++20。

- **《C++ Primer》**（Lippman 等）——最系统的 C++ 入门书，适合想彻底学清楚每个细节的读者。

**进阶**

- **《Effective C++》+《More Effective C++》**（Scott Meyers）——55 条实践建议，每条都有原理解释。经典，虽然部分条款已经被现代 C++ 取代，但思维方式仍然有价值。

- **《Effective Modern C++》**（Scott Meyers）——专门针对 C++11/14 的 42 条建议，覆盖 `auto`、move semantics、lambda、智能指针、并发。

- **《C++ Templates: The Complete Guide》**（Vandevoorde、Josuttis）——模板的圣经。

**高性能**

- **《C++ High Performance》**（Björn Andrist、Viktor Sehr）——写出高性能 C++ 代码的方法：缓存友好性、SIMD、并行算法、profile 工具。

### 编译器与解释器

- **《Crafting Interpreters》**（Robert Nystrom）——免费在线阅读（craftinginterpreters.com）。实现两个 Lox 解释器：一个树遍历（Java），一个字节码 VM（C）。是 Taco 最直接的进阶路线。

- **《Writing a Compiler in Go》**（Thorsten Ball）——相同作者的系列（《Writing An Interpreter In Go》是前传），Go 语言，很清晰。

- **《Engineering a Compiler》**（Cooper、Torczon）——编译原理教材，系统、严谨，比 龙书（CLRS）更现代。

### 并发

- **《C++ Concurrency in Action》**（Anthony Williams）——C++ 并发编程的权威参考。作者是 C++ 并发标准的参与者。

- **《The Art of Multiprocessor Programming》**（Herlihy、Shavit）——并发数据结构的理论基础，从锁到无锁，严谨但不枯燥。

### 网络编程

- **《UNIX Network Programming》**（W. Richard Stevens）——网络编程的圣经，用 C 写，讲的是 socket API 和网络编程的每个细节。仍然是最好的底层参考。

- **《Boost.Asio C++ Network Programming》**——Asio 的实践书籍。

### 在线资源

- **cppreference.com**——C++ 标准库的权威参考，每个函数都有说明、示例和异常规范。书签收好。

- **Compiler Explorer（godbolt.org）**——把 C++ 代码编译成汇编，实时看，对理解编译器优化极有帮助。

- **C++ Weekly（Jason Turner，YouTube）**——每周一个 C++ 小技巧或特性讲解，几分钟一集，质量高。

- **cppcon 演讲**——YouTube 上有大量 CppCon 的录播，很多都是顶级 C++ 工程师的分享。

---

## 40.5 C++ 的未来：C++23 及之后

C++ 标准每三年更新一次：C++11 → C++14 → C++17 → C++20 → C++23 → C++26……

### C++23 已经发布的主要内容

**`std::expected<T, E>`**：函数式错误处理，前面介绍过。

**`std::print` 和 `std::println`**：基于 fmtlib 的格式化输出，终于进入标准：

```cpp
#include <print>
std::println("Hello, {}!", name);  // 不需要 "\n"
std::print("{:>10.2f}", 3.14159);  // 格式化选项
```

**`std::mdspan`**：多维数组视图，高性能数值计算的基础：

```cpp
// 把一维数组当多维数组用，零开销
float data[6] = {1, 2, 3, 4, 5, 6};
auto matrix = std::mdspan(data, 2, 3);  // 2x3 矩阵
matrix[1, 2] = 42.0f;  // 访问 row=1, col=2
```

**`std::flat_map` 和 `std::flat_set`**：基于排序 vector 的有序映射和集合，比 `std::map` 和 `std::set` 有更好的缓存性能（连续内存），但插入较慢：

```cpp
std::flat_map<std::string, int> m;
m["hello"] = 1;
// 内部是两个排序 vector：keys 和 values，对应位置
```

**Ranges 扩展**：更多 range 算法和 views，`std::views::zip`、`std::views::slide`、`std::ranges::fold_left`……

### C++26（进行中）

C++26 的一些重要提案：

**反射（Reflection）**：在编译期检查和操作类型的结构。可以不用手写序列化代码——直接反射出类的所有字段，自动生成 JSON 序列化/反序列化：

```cpp
// 未来（C++26 reflection 的想象写法）
struct Point { int x, y; };

// 自动生成：不需要手写 to_json/from_json
json j = reflect_to_json(Point{1, 2});  // {"x": 1, "y": 2}
```

**执行器（Executors / senders/receivers）**：统一的异步执行模型，可能最终合并 Asio、线程池、协程等不同的并发方式。这是 C++ 社区争论最激烈的提案之一，进展缓慢但很重要。

**Contracts**：前置条件、后置条件、不变量的语言级别支持：

```cpp
// 想象的 contracts 语法
double sqrt(double x)
    pre (x >= 0.0)     // 前置条件
    post(result >= 0.0)  // 后置条件
{
    return std::sqrt(x);
}
```

### C++ 的地位

C++ 不会消失。它占据的领域——系统软件、游戏引擎、高频交易、嵌入式、科学计算——对性能和控制有极高要求，Rust 在部分场景里是竞争者，但 C++ 庞大的代码库、工具链、生态系统的惯性让它仍然是这些领域的主流。

每次有人说"C++ 要被 XXX 替代了"，C++ 又发布了一个新标准，变得更现代、更安全、更好用。

---

## 最后

这本书建立了一个框架：从零实现一门语言的解释器。过程里学到的每一个 C++ 特性，都有真实的动机——不是"学这个因为重要"，而是"遇到了这个问题，这个特性解决了它"。

Taco 的七次进化对应的知识路径：

```
基础语法 → 类与继承 → 多态 → 智能指针/RAII → STL → 模板 → 并发 → 网络
    ↕           ↕         ↕          ↕             ↕        ↕       ↕       ↕
  词法器  →   AST  →  求值器  →   闭包/作用域 → 标准库 → 重构  → REPL  → HTTP
```

每条横线上面是 C++ 知识，下面是 Taco 的对应进化。这个对应关系不是偶然的——语言特性存在是因为有真实的工程问题，了解问题才能理解特性。

C++ 是一门工具，目的是解决问题。工具越熟，解决问题的方式越多，越优雅，越高效。

---

🌮

```taco
// 写在最后
var 答案 = 🌮 "Will I become a good C++ programmer?";
print(答案);
// 🎱 It is certain.
```
