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
