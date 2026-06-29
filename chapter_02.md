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

为了掩饰CMake的多文件项目的编译方法，我们先来构建一个词法分析器（lexer）。

这个函数用来实现分词的。比如`var x = 10;`这个语句，lexer用来把这个句子拆开成这样：

```
var
x
=
10
```

每一行都是一个Token。

然后识别出各个Token的类型（Type）：

```
var --> "Identifier"
x   --> "Identifier"
=   --> "Equals"
10  --> "Number"
```

也就是说，我们需要一个储存所有"类型（`TokenType`）"的`enum`，以及一个`struct`，用来储存原始的值和他对应的`TokenType`。

我们先来构建一个极其简略的lexer。这个项目放在：

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
    Equals,
    EndOfFile,
};

struct Token {
    TokenType type;       // 这个词的类型
    std::string value;    // 这个词的原始文本（string类型）
};

std::vector<Token> tokenize(const std::string& source); // 这个函数用来实现分词，输入string的地址，返回一个Token类型的动态数组
// 注意这里只是header，具体的函数在.cpp里面实现
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

运行的结果是：

```bash
forrest@debian:~/test/taco$ ./build/taco
Taco 0.1.0
Tokens 0
```

Tokens为0是因为我们的函数故意返回空列表。

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
