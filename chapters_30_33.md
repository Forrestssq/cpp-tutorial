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
