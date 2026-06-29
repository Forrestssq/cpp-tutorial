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
