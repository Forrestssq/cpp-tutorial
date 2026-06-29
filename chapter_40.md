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
