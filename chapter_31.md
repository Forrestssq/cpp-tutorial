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
