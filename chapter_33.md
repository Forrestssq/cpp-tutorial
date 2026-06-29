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
