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
