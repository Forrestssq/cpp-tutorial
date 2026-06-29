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
