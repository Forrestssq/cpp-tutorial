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
