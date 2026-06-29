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
