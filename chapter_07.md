# 第七章：类的基础

---

从这一章开始进入第二部分：类与对象。

类是 C++ 最核心的特性之一，也是和 C 差距最大的地方。Python 里已经有类的概念，所以这里不从零解释"什么是封装"，而是直接对比：C++ 的类和 Python 的类有什么相同，有什么不同，为什么会有这些不同。

---
## 7.1 `class` 入门

 `class` 本质上是在解决一个问题：当数据和操作这些数据的函数关系紧密的时候，把它们硬拆开管理很别扭。从这个问题出发，一步步建立完整的概念。
 
### 没有 `class` 之前的世界

假设要表示一个"计数器"，纯 C 风格的写法是数据和函数分开：

```cpp
struct Counter {
    int count;
};

void counter_increment(Counter* c) {
    c->count++;
}

int counter_get(const Counter* c) {
    return c->count;
}
```

我们会发现这样的写法很松散。函数 `counter_increment` 和 结构体 `Counter` 之间唯一的联系是程序员脑子里的约定，编译器不知道这个函数"属于"这个结构体。而且 `count` 完全暴露在外，任何代码都能直接改 `c.count = -999`，没有任何约束。

### 解决这种问题的方法是，`class` 把函数"绑"进数据里

```cpp
class Counter {
public:
    void increment() {
        m_count++;
    }
    int get() const {
        return m_count;
    }
private:
    int m_count = 0;
};
```

用起来是这样：

```cpp
Counter c;
c.increment();
c.increment();
std::cout << c.get();  // 2
```

这里的关键变化是， `increment()` 这个函数现在"长在" `Counter` 里面，调用时不用再手动传 `Counter*` 进去—— `m_count++` 直接就能访问到当前这个对象的成员。这背后其实有一个隐藏的参数，叫 `this`，下面会讲到。

### 访问控制：`public` / `private` / `protected`

```cpp
class BankAccount {
public:
    void deposit(double amount) {
        m_balance += amount;
    }
private:
    double m_balance = 0;
};

BankAccount acc;
acc.deposit(100);     // 可以，deposit 是 public
acc.m_balance += 100; // 编译错误！m_balance 是 private
```

`public` 顾名思义，表示的是可以供外部调用，比如上面例子中的 `deposit()` 函数。相反的， `private` 的部分则只能在 `class` 内部来调用，比如 `m_balance` ，如果在 `class` 外调用 `acc.m_balance++;` 的话，会报错。

为什么要把数据藏起来（`private`）？因为一旦数据可以被外部随便改，类自己就没法保证任何"不变量"。比如如果允许外部直接写 `m_balance = -100`，账户余额就可能变成负数，而这种校验逻辑本该写在 `deposit` / `withdraw` 里。`private` 强迫所有修改都必须经过类自己提供的接口，逻辑才有地方可控。

`class` 默认所有成员是 `private`，所以这里显式写了 `public:` 把 `increment` 和 `get` 暴露出去，`m_count` 留在 `private` 里不让外部直接碰。如果换成 `struct`，唯一的区别是默认访问权限变成 `public`（也就是说 `struct` 和 `class` 在功能上完全等价，只是 `struct` 默认是 `public` 的），习惯上数据为主、没什么行为的用 `struct`，需要封装的用 `class`。

`protected` 留到讲继承时才有意义（子类能访问父类的 `protected` 成员，外部不能），这里先知道有这个第三种权限即可。

### 声明和实现分开写

类内部直接写函数体（像上面 `increment`）叫"内联定义"。更常见、更工程化的做法是类里只写声明，函数体放到类外面，用 `类名::函数名` 表明归属：

```cpp
// 头文件 counter.h
class Counter {
public:
    void increment();
    int get() const;
private:
    int m_count = 0;
};

// 源文件 counter.cpp
void Counter::increment() {
    m_count++;
}

int Counter::get() const {
    return m_count;
}
```

`::` 叫作用域解析运算符，`Counter::increment` 的意思是"这个 `increment` 是 `Counter` 类的，不是某个全局函数"。头文件放接口（给别人看的"说明书"），源文件放实现（具体怎么干的），这是大型 C++ 项目的标准组织方式，编译时也能各自独立编译，加快构建速度。

### `this` 指针

每个非静态成员函数被调用时，编译器偷偷传进去一个指针，**指向调用它的那个对象实例**（也就是用这个类实例化出来的具体对象），这个指针就叫 `this`。看下面的例子：

```cpp
class Counter {
public:
    Counter& increment() {
        m_count++;
        return *this;   // 返回当前对象本身，支持链式调用
    }
private:
    int m_count = 0;
};

Counter c;
c.increment().increment().increment();  // 链式调用，c.count 变成 3
```

`this` 等价于 `&c` ，也就是说， `c.increment()` 返回的是 `*&c` ，即， `c` 这个实例。然后再对返回的 `*&c` 调用 `.increment()` ，从而实现链式调用。

`m_count++` 其实是 `this->m_count++` 的省略写法。

平时不需要显式写 `this`，但想返回对象自身（实现链式调用）或者在成员函数里区分同名的参数和成员变量时，必须显式用 `*this` 或 `this->`。

### 构造函数：对象怎么诞生

构造函数是一个和类同名、没有返回类型的特殊函数，对象创建的瞬间自动调用，专门用来做初始化。

```cpp
class Counter {
public:
    Counter(int start) {
        m_count = start;
    }
    int get() const { return m_count; }
private:
    int m_count;
};

Counter c(10);
std::cout << c.get();  // 10
```

更地道的写法是用"初始化列表"而不是在函数体里赋值：

```cpp
Counter(int start) : m_count(start) {}
```

这是构造函数的"初始化列表"语法，完整名字叫 **成员初始化列表**（member initializer list），专属于构造函数，作用是在构造函数执行函数体之前，直接初始化成员变量。

它的基本语法结构如下：

```cpp
构造函数名(参数列表) : 成员1(初始值1), 成员2(初始值2), ... {
    // 函数体
}
```

拆开看每个部分：

```cpp
Counter(int start) : m_count(start) {}
//  ①        ②     ③      ④       ⑤
```

- ① 构造函数名，必须和类名完全相同
- ② 参数列表，跟普通函数的参数列表写法一样
- ③ 一个冒号 `:`，标志着初始化列表的开始
- ④ 初始化列表本身，格式是 `成员变量名(用来初始化它的值)`
- ⑤ 函数体，用花括号包起来，这里是空的，但完全可以写别的逻辑

> `Counter(int start)` 这个就是 `函数名(需要在实例化的时候传入的参数名)` 。

多个成员变量的情况

如果有多个成员变量要初始化，中间用逗号分隔：

```cpp
class Point {
public:
    Point(int x, int y) : m_x(x), m_y(y) {}
private:
    int m_x;
    int m_y;
};
```

写法是 `m_x(x), m_y(y)`——每一项都是"成员变量名 + 括号 + 用来初始化它的表达式"，多项之间用逗号隔开，最后跟着函数体 `{}`。

值得注意的是，它的初始化值不一定来自参数，也可以是字面量、表达式，或者干脆不写值用默认构造：

```cpp
class Example {
public:
    Example(int x) 
        : m_x(x)           // 用参数初始化
        , m_y(0)           // 用字面量初始化
        , m_z(x * 2)       // 用表达式初始化
        , m_name()         // 用默认构造（std::string 默认构造成空字符串）
    {}
private:
    int m_x;
    int m_y;
    int m_z;
    std::string m_name;
};
```

如果不写任何构造函数，编译器会偷偷生成一个什么都不做的"默认构造函数"；一旦我们写了任意一个构造函数，这个自动生成的默认构造函数就不再存在，除非我们写的构造函数长这样： `Counter() = default;` ，这个的意思就是让编译器自己去生成那个默认的构造函数。

### `explicit`

`explicit` 是英语单词本身的意思——"明确的、显式的"，作为 C++ 关键字，用来**禁止构造函数被用来做隐式类型转换**，强制要求调用者必须明确地、显式地写出构造调用。

先看没有 explicit 时会发生什么

```cpp
class Counter {
public:
    Counter(int start) : m_count(start) {}   // 没加 explicit
private:
    int m_count;
};
```

这时候，一个单参数构造函数会让编译器认为"`int` 可以自动变成 `Counter`"，于是允许这样写：

```cpp
Counter c2 = 10;
```

注意这行代码字面上看，等号右边是个 `int`（`10`），左边却要一个 `Counter` 类型的变量。按理说类型不匹配应该报错，但编译器在背后偷偷做了一步转换：把 `10` 当成参数，调用了 `Counter(int)` 这个构造函数，生成了一个临时的 `Counter` 对象，再用这个临时对象去初始化 `c2`。这整个过程你完全没有写出来"我要调用构造函数"，是编译器自己悄悄补上的，这就叫**隐式转换**。

同样的事也会发生在函数参数传递时：

```cpp
void f(Counter c) {}

f(10);   // 10 是 int，f 要的是 Counter，编译器偷偷把 10 转成 Counter 再传进去
```

隐式转换最大的风险是：**类型不匹配时，编译器不报错，而是悄悄帮你"圆"过去**，这经常会掩盖真正的 bug。举个更危险的例子：

```cpp
class Counter {
public:
    Counter(int start) : m_count(start) {}
private:
    int m_count;
};

void process(Counter c) {
    // 处理一个计数器
}

int main() {
    int user_id = 12345;
    process(user_id);   // 本意是想传一个 Counter，结果手滑传了个 int
                          // 编译器不报错！默默把 12345 转成了一个 Counter 对象
}
```

这里程序员的本意可能是函数签名写错了，或者调用时传错了变量，是一个明显的逻辑错误。但因为隐式转换的存在，编译器完全不会提醒你，代码照样能编译通过、能跑，错误被悄悄"吃掉"了，只有运行时行为不对劲，才会被发现，排查起来很麻烦。

为了解决这个问题，我们只需要在初始化列表前加上 `explicit` 即可：

```cpp
class Counter {
public:
    explicit Counter(int start) : m_count(start) {}   // 加上 explicit
private:
    int m_count;
};
```

加了 `explicit` 之后，这个构造函数就**不能再被用来做隐式转换**了：

```cpp
Counter c1(10);     // 可以——这是直接初始化，明确写出了"我要构造一个 Counter"
Counter c2 = 10;    // 错误！这是想让编译器隐式转换，被 explicit 挡住了
```

但如果你**明确地**写出构造调用，依然是可以的：

```cpp
Counter c2 = Counter(10);          // 可以，这里明确写了 Counter(10)，不是隐式转换
Counter c3 = static_cast<Counter>(10);  // 可以，static_cast 也是一种"明确表态"
```

函数参数那边同理：

```cpp
void f(Counter c) {}

f(10);            // 错误！想让 10 隐式转成 Counter，被挡住
f(Counter(10));   // 可以，明确写出了构造调用
```

只有**单参数**构造函数（或者所有参数除第一个外都有默认值，效果上等同于能用一个参数调用的构造函数）才会触发隐式转换问题，因为隐式转换的本质是"用一个值去换出另一个类型"，天然就是单值对单值的转换关系。所以如果构造函数需要两个或更多必填参数，本身就不存在"一个值自动变成这个类型"的歧义，就不需要 `explicit` 了。

```cpp
class Point {
public:
    Point(int x, int y) : m_x(x), m_y(y) {}   // 双参数，本身不会触发隐式转换
private:
    int m_x, m_y;
};

Point p = {1, 2};   // 这是列表初始化，跟 explicit 是另一回事，这里不展开
```

**什么时候反而不想加 explicit、就是想要这种隐式转换**：最常见的例子是 `std::string` 的构造函数：

```cpp
void greet(const std::string& name) {
    std::cout << "Hello, " << name << "\n";
}

greet("Alice");   // "Alice" 是 const char*，隐式转成了 std::string
```

如果 `std::string` 的构造函数也被标成 `explicit`，那么每次想用字符串字面量初始化 `std::string`、或者传给接受 `std::string` 的函数，都得手动写 `std::string("Alice")`，会非常繁琐。这种情况下隐式转换是有意为之、提升便利性的，所以标准库特意没给这个构造函数加 `explicit`。所以原则不是"所有单参数构造函数都必须加 explicit"，而是"默认倾向于加，除非你确实希望这种类型转换是丝滑、隐式发生的"。

### 析构函数：对象怎么消亡

```cpp
class Counter {
public:
    Counter() { std::cout << "构造\n"; }
    ~Counter() { std::cout << "析构\n"; }
};

{
    Counter c;   // 打印 "构造"
}                // 离开作用域，自动打印 "析构"
```

析构函数名字是 `~类名`，没有参数、没有返回类型，对象生命周期结束时（离开作用域、被 delete、所属容器被清空……）自动调用，专门用来做清理工作，比如释放申请的内存、关闭文件句柄。这正是 `C++` "`RAII`"（资源获取即初始化）这套设计哲学的核心：把资源的生命周期和对象的生命周期绑在一起，对象一创建资源就到位，对象一销毁资源自动释放，不需要手动记得清理，也不会因为忘记清理而泄漏。

### `const` 成员函数

函数名后面的 `const` 是对编译器的承诺："这个函数不会修改对象的状态"。意义在于：一个 `const` 对象（或者通过 `const&` 传进来的对象）只能调用标了 `const` 的成员函数，编译器会在编译期帮你拦住误操作。养成习惯——不修改成员变量的成员函数都加上 `const`，这是 `C++` 里很重要的约定：

```cpp
class Counter {
public:
    int get() const {     // 承诺不修改任何成员变量
        return m_count;
    }
    void increment() {    // 没有 const，会修改成员变量
        m_count++;
    }
private:
    int m_count = 0;
};

const Counter c;
c.get();          // 可以，get 是 const
c.increment();    // 错误！const 对象不能调用非 const 成员函数
```

### `static` 成员

先看普通成员变量是什么样的。

```cpp
class Counter {
public:
    Counter(int start) : m_count(start) {}
private:
    int m_count;
};

Counter a(1);
Counter b(2);
Counter c(3);
```

普通成员变量 `m_count`，**每个对象都有自己独立的一份**。`a` 的 `m_count` 是 1，`b` 的 `m_count` 是 2，`c` 的 `m_count` 是 3，三份内存完全独立，互不影响，改 `a.m_count` 不会影响 `b.m_count`。

接下来看加上 `static` 之后会发生什么变化。

```cpp
class Counter {
public:
    Counter() { s_instance_count++; }
private:
    static int s_instance_count;
};
```

加上 `static` 之后，情况完全不同：**不管创建多少个对象，`s_instance_count` 永远只有一份**，所有对象看到的、改动的，都是同一块内存。可以这样理解：普通成员变量是"每个对象各拿一份"，`static` 成员变量是"这个类的所有对象一起共用一份，谁也不单独拥有"。更准确地说，`static` 成员变量根本不属于任何一个对象，它属于**类本身**，哪怕一个对象都没创建，这个变量也已经存在了。

既然 `static` 成员变量不属于任何对象，它的初始化方式也跟普通成员变量不一样，需要单独再写一行。

```cpp
class Counter {
private:
    static int s_instance_count;   // 这里只是声明，告诉编译器"有这么个东西"
};

int Counter::s_instance_count = 0;   // 这里才是真正的定义，分配内存、给初始值
```

普通成员变量的内存，是跟着每个对象一起分配的——创建一个 `Counter` 对象，编译器就在那块内存里顺带留出 `m_count` 的位置。但 `static` 成员变量不属于任何对象，它的内存得**单独**分配，而且只能分配一次（因为全程序只有一份）。类定义本身（写在头文件里的那部分）只是描述"这个类长什么样"，并不会真正分配内存。所以类内的 `static int s_instance_count;` 只是个声明，类似于"提前打个招呼说有这么个变量"，真正在某个地方把内存开辟出来、给上初始值，必须在类外面单独写一行，格式是 `类型 类名::变量名 = 初始值;`，这一行通常放在对应的 `.cpp` 源文件里，确保整个程序只出现一次（否则又会撞上之前讲过的 ODR 单一定义规则）。

> 从 C++17 开始，如果在类内声明时加上 `inline` 关键字（`static inline int s_instance_count = 0;`），就可以直接在类内完成初始化，不需要再去类外写一行了，这是为了解决头文件里类的 static 成员初始化麻烦的问题专门引入的新写法。

定义好之后，用下面的方式访问 `static` 成员变量：

```cpp
Counter a, b, c;
std::cout << Counter::get_instance_count();   // 3
```

因为 `static` 成员变量不属于具体某个对象，所以更地道的访问方式是直接用**类名**加 `::`，不需要先有一个对象：

```cpp
std::cout << Counter::s_instance_count;   // 如果它是 public 的话，可以这样直接访问
```

也可以通过某个对象去访问（虽然不那么常见），效果是一样的，因为反正所有对象共享的都是同一份：

```cpp
std::cout << a.s_instance_count;   // 跟 Counter::s_instance_count 是同一个东西
```

变量讲完了，再看 `static` 成员函数。

```cpp
class Counter {
public:
    static int get_instance_count() { return s_instance_count; }
private:
    static int s_instance_count;
};
```

普通成员函数被调用时，前面讲过编译器会偷偷传一个 `this` 指针进去，指向具体是哪个对象在调用。但 `static` 成员函数**没有 `this`**——它压根不需要知道是哪个对象在调用它，因为它本来就不依赖任何具体对象。正因为没有 `this`，所以调用 `static` 成员函数完全不需要先创建对象：

```cpp
std::cout << Counter::get_instance_count();   // 不需要先有一个 Counter 对象，直接用类名调用
```

也正因为没有 `this`，`static` 成员函数**只能访问其他的 `static` 成员**（变量或函数），完全不能访问非 `static` 的成员，因为非 `static` 成员必须通过某个具体对象（也就是 `this`）才能找到，而 `static` 函数里根本没有这个东西可用：

```cpp
class Counter {
public:
    static int get_instance_count() { return s_instance_count; }   // 可以，访问的是 static 成员
    static void bad_example() {
        m_count++;   // 错误！m_count 是非 static 成员，static 函数里没有 this，不知道该改谁的 m_count
    }
private:
    int m_count = 0;             // 普通成员
    static int s_instance_count; // static 成员
};
```

总结一下：普通成员（变量和函数）描述的是"每一个对象自己的状态和行为"，依赖 `this` 才能知道"是哪个对象"。`static` 成员描述的是"这个类整体共享的状态和行为"，跟具体哪个对象无关，甚至可以在没有任何对象存在时就被使用。这也是为什么 `static` 成员函数没有 `this`——它服务的对象是"类"，不是某个实例。

有一点：局部类不能有 `static` 数据成员。

```cpp
#include <iostream>
#include <string>

int main(void) {
    class Counter {       // 这是一个局部类，定义在函数内部
    public:
        Counter() { m_counter++; }
        static int get_count_value() { return m_counter; }
    private:
        static int m_counter;   // 编译错误！局部类不允许有 static 成员变量
    };
    int Counter::m_counter = 0;
}
```

直接编译会报类似这样的错误：`error: a static data member of name 'm_counter' may not be defined in a local class` 。

原因在于：`static` 成员变量需要在类外单独写一行来定义、分配内存（比如 `int Counter::m_counter = 0;`），这一行本质上是在**全局（或命名空间）作用域**里，给这个变量分配一块独立于任何函数调用、贯穿整个程序生命周期的内存。但局部类本身定义在函数体内部，它的"生存范围"被限制在这个函数作用域里——一个只在函数内部"临时存在"的类型，却要拥有一个全局唯一、跨越整个程序生命周期的静态存储，这两者产生了根本性的冲突，C++ 语言规则直接禁止了这种组合，不允许局部类声明 `static` 数据成员。顺带一提，局部类的 `static` **成员函数**是可以有的，只是 `static` **数据成员**不行，因为**成员函数不需要类似的"类外定义来分配内存"这一步**：

```cpp
int main(void) {
    class Counter {
    public:
        static void say_hello() {   // 这个没问题，static 成员函数可以
            std::cout << "hello\n";
        }
    };
    Counter::say_hello();   // 可以正常调用
}
```

修正方式很直接，把这个类挪到 `main` 函数外面，变成一个普通的（非局部）类即可。

```cpp
#include <iostream>
#include <string>

class Counter {
public:
    Counter() { m_counter++; }
    static int get_count_value() { return m_counter; }
private:
    static int m_counter;
};

int Counter::m_counter = 0;   // 类外定义，这里没问题，因为 Counter 现在是全局作用域的类

int main(void) {
    Counter a, b, c;
    std::cout << Counter::get_count_value();   // 3
}
```

这样写就完全没问题了——`Counter` 现在是一个普通的、定义在全局作用域的类，`static` 成员变量可以正常在类外定义并拥有全局唯一的存储位置。

最后提一个 `static` 成员常见的实际用途：单例模式，让一个类全局只能存在一个实例。

```cpp
class Logger {
public:
    static Logger& get_instance() {
        static Logger instance;   // 注意：这里的 static 是另一种用法（局部静态变量），后面可以单独展开讲
        return instance;
    }
private:
    Logger() {}   // 构造函数设为 private，外部无法直接 new 一个
};

Logger::get_instance().log("hello");
```

这种写法依赖的正是 `static` 成员函数不需要对象就能调用的特性，是个常见的实际应用场景，但属于进阶用法，先了解 `static` 的基本概念，这类设计模式之后单独学也来得及。

### 串起来看一个完整的例子

```cpp
class Person {
public:
    explicit Person(std::string name, int age)
        : m_name(std::move(name))
        , m_age(age)
    {
        s_count++;
    }

    ~Person() {
        s_count--;
    }

    std::string get_name() const { return m_name; }
    int get_age() const { return m_age; }

    void birthday() {
        m_age++;
    }

    static int get_population() { return s_count; }

private:
    std::string m_name;
    int m_age;
    static int s_count;
};

int Person::s_count = 0;
```

这一个类涵盖了：访问控制（`private` 数据 + `public` 接口）、构造函数（带初始化列表和 `explicit`）、析构函数、普通成员函数、`const` 成员函数、`static` 成员变量和成员函数。这基本就是单个类能用到的全部基础工具了——拷贝构造/拷贝赋值（对象被复制时发生什么）、运算符重载、继承和虚函数，是在此之上的进阶话题，分别属于不同的主题，需要单独展开讲。

---

## 7.2 成员变量与成员函数

### 成员变量

成员变量（member variable）是类的数据部分，描述对象的状态：

```cpp
class Lexer {
    std::string m_source;  // 源代码
    int         m_pos;     // 当前位置
    int         m_line;    // 当前行号
};
```

命名约定：很多 C++ 项目用 `m_` 前缀表示成员变量（member），区别于局部变量和参数。这不是语言要求，但是一个有用的习惯——读代码时一眼就能看出哪些是成员变量。

成员变量可以在声明时给默认值（C++11 起）：

```cpp
class Lexer {
    std::string m_source;
    int m_pos    = 0;     // 默认值
    int m_line   = 1;     // 默认值
    int m_column = 1;     // 默认值
};
```

---

### 成员函数

成员函数（member function）是类的行为部分，定义对象能做什么：

```cpp
class Lexer {
public:
    // 成员函数：可以访问 m_source、m_pos 等成员变量
    char current() const {
        if (is_at_end()) return '\0';
        return m_source[m_pos];
    }

    bool is_at_end() const {
        return m_pos >= static_cast<int>(m_source.size());
    }
};
```

成员函数可以直接访问同一个类的成员变量，不需要传参数。这就是"封装"的核心——数据和操作数据的函数绑定在一起。

**在类外定义成员函数**

成员函数可以在类定义里声明，在类外实现：

```cpp
// lexer.h：只声明
class Lexer {
public:
    char current() const;
    bool is_at_end() const;
private:
    std::string m_source;
    int m_pos = 0;
};

// lexer.cpp：实现，用 Lexer:: 前缀表明属于 Lexer 类
char Lexer::current() const {
    if (is_at_end()) return '\0';
    return m_source[m_pos];
}

bool Lexer::is_at_end() const {
    return m_pos >= static_cast<int>(m_source.size());
}
```

这是大型项目里的标准做法：头文件只放接口（声明），源文件放实现。

---

## 7.3 访问控制：public、private、protected

C++ 用三个关键字控制成员的访问权限：

- `public`：任何地方都可以访问
- `private`：只有类自己的成员函数可以访问
- `protected`：类自己和子类可以访问（第十二章讲继承时再展开）

```cpp
class BankAccount {
public:
    // 公开接口：外部可以调用
    void deposit(double amount) {
        if (amount > 0) m_balance += amount;
    }

    double get_balance() const {
        return m_balance;
    }

private:
    // 私有数据：外部无法直接访问和修改
    double m_balance = 0.0;
    std::string m_owner;
};

BankAccount account;
account.deposit(100.0);          // 可以，deposit 是 public
double b = account.get_balance(); // 可以，get_balance 是 public
account.m_balance = 9999.0;      // 错误！m_balance 是 private
```

**为什么要 private？**

把数据设为 private，强迫外部代码通过公开接口操作对象，而不是直接修改数据。这样有几个好处：

- 可以在接口里做检查（`if (amount > 0)`），防止非法操作
- 内部实现可以改变，只要接口不变，外部代码不需要修改
- 读代码时，public 部分就是类的"说明书"，private 是实现细节

**struct 和 class 的区别**

C++ 里 `struct` 和 `class` 几乎完全相同，只有一个区别：`struct` 的成员默认是 `public`，`class` 的成员默认是 `private`。

```cpp
struct Point {
    double x;  // 默认 public
    double y;  // 默认 public
};

class Token {
    // 这里是 private！
    std::string value;
public:
    // 这里才是 public
    std::string get_value() const { return value; }
};
```

惯例上，`struct` 用于简单的数据容器（不需要封装），`class` 用于有复杂行为的对象（需要封装）。在 Taco 项目里，`Token` 用 `struct`（只是数据），`Lexer` 用 `class`（有复杂的内部逻辑）。

---

## 7.4 this 指针

在成员函数里，有一个隐式的指针叫 `this`，指向调用这个函数的对象本身：

```cpp
class Counter {
public:
    void increment() {
        this->count++;  // 等价于 count++
    }

    // 返回对象自身的引用，可以链式调用
    Counter& add(int n) {
        this->count += n;
        return *this;  // 解引用 this，返回对象本身
    }

    int get() const {
        return this->count;  // 等价于 count
    }

private:
    int count = 0;
};

Counter c;
c.add(5).add(3).add(2);  // 链式调用
std::cout << c.get();    // 10
```

大多数情况下不需要显式写 `this->`，编译器知道成员函数里的 `count` 就是 `this->count`。但有两种情况必须用 `this`：

**情况一：参数名和成员变量同名**

```cpp
class Token {
public:
    Token(std::string value) {
        this->value = value;  // this->value 是成员变量，value 是参数
    }
private:
    std::string value;
};
```

不过更好的做法是用初始化列表（下一章讲），或者给成员变量加 `m_` 前缀来避免歧义。

**情况二：返回对象自身**

```cpp
Counter& add(int n) {
    count += n;
    return *this;  // 必须用 this 才能返回对象自身的引用
}
```

---

### Taco 里的应用

在 Taco 项目里，`Lexer` 类用 `this` 的地方主要是构造函数。当前版本的 Lexer 用了成员变量初始化列表（下一章详细讲），所以 `this` 用得不多。但理解 `this` 的存在，有助于理解成员函数和普通函数的本质区别：

成员函数在底层其实就是一个普通函数，只不过编译器帮你把"调用这个函数的对象"作为第一个参数（`this`）传进去。这就是为什么 Python 里需要显式写 `self`，而 C++ 里 `this` 是隐式的。

```python
# Python：self 是显式参数
def to_string(self):
    return self.value
```

```cpp
// C++：this 是隐式指针
std::string to_string() const {
    return value;  // 等价于 this->value
}
```

---

## 7.5 回到项目：用 class 实现 Lexer

第六章定义了 `Token`，但只完成了一半：还需要一个东西，能读入源代码字符串，逐字符扫描，把它切成 Token 列表。这就是 `Lexer`。

`Token` 用 `struct` 就够了，因为它只是一捆数据，没有行为。`Lexer` 不一样：它有内部状态（读到第几个字符了、当前第几行第几列），还有一堆只在内部使用的辅助函数（`advance`、`peek`、`match`……），不希望外部代码随便碰这些细节，只暴露一个干净的接口——"给我源代码，还我 Token 列表"。这正是上面几节讲的封装：数据和操作数据的函数绑定在一起，用 `private` 藏起实现细节，只留 `public` 接口。所以 `Lexer` 用 `class` 来写。

按照 7.2 提到的惯例，`Lexer` 把声明和实现分开：`lexer.h` 只放接口，`lexer.cpp` 放具体实现。

### lexer.h

```cpp
#pragma once
#include "token.h"
#include <string>
#include <vector>
#include <unordered_map>

class Lexer {
public:
    // 构造函数：接收源代码字符串
    explicit Lexer(std::string source);

    // 核心接口：把源代码切成 Token 列表
    std::vector<Token> tokenize();

private:
    std::string m_source;   // 源代码
    int         m_pos;      // 当前读取位置
    int         m_line;     // 当前行号
    int         m_column;   // 当前列号

    // 关键字表：字符串 → TokenType
    static const std::unordered_map<std::string, TokenType> KEYWORDS;

    // 辅助函数
    char current() const;           // 当前字符
    char peek(int offset = 1) const;// 向前看 offset 个字符
    char advance();                 // 消费当前字符，前进一步
    bool is_at_end() const;         // 是否到达末尾
    bool match(char expected);      // 如果下一个字符匹配，消费并返回 true

    // 创建 Token
    Token make_token(TokenType type, const std::string& value) const;

    // 跳过空白和注释
    void skip_whitespace_and_comments();

    // 各类 Token 的读取函数
    Token read_number();
    Token read_string();
    Token read_identifier_or_keyword();
    Token read_taco_emoji();

    // 读取下一个 Token
    Token next_token();
};
```

---

### lexer.cpp

```cpp
#include "lexer.h"
#include <stdexcept>
#include <sstream>

// 关键字表：static 成员，所有 Lexer 实例共享一份
const std::unordered_map<std::string, TokenType> Lexer::KEYWORDS = {
    {"var",     TokenType::Var},
    {"func",    TokenType::Func},
    {"if",      TokenType::If},
    {"elseif",  TokenType::Elseif},
    {"else",    TokenType::Else},
    {"while",   TokenType::While},
    {"for",     TokenType::For},
    {"in",      TokenType::In},
    {"return",  TokenType::Return},
    {"true",    TokenType::True},
    {"false",   TokenType::False},
    {"nil",     TokenType::Nil},
    {"class",   TokenType::Class},
    {"struct",  TokenType::Struct},
    {"enum",    TokenType::Enum},
    {"extends", TokenType::Extends},
    {"self",    TokenType::Self},
    {"super",   TokenType::Super},
    {"switch",  TokenType::Switch},
    {"case",    TokenType::Case},
    {"default", TokenType::Default},
    {"import",  TokenType::Import},
    {"from",    TokenType::From},
    {"thread",  TokenType::Thread},
    {"channel", TokenType::Channel},
};

// 构造函数用的是"初始化列表"语法（: m_source(...), m_pos(0) ...）
// 这是 C++ 初始化成员变量的标准写法，比在函数体里逐个赋值更高效
// 下一章会专门讲这个语法，这里先照着写，能看懂"冒号后面是按成员变量顺序初始化"就够了
Lexer::Lexer(std::string source)
    : m_source(std::move(source))  // move 避免拷贝
    , m_pos(0)
    , m_line(1)
    , m_column(1)
{}

// ────────────────────────────────
// 辅助函数
// ────────────────────────────────

char Lexer::current() const {
    if (is_at_end()) return '\0';
    return m_source[m_pos];
}

char Lexer::peek(int offset) const {
    int idx = m_pos + offset;
    if (idx < 0 || idx >= static_cast<int>(m_source.size())) return '\0';
    return m_source[idx];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    return c;
}

bool Lexer::is_at_end() const {
    return m_pos >= static_cast<int>(m_source.size());
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (m_source[m_pos] != expected) return false;
    advance();
    return true;
}

Token Lexer::make_token(TokenType type, const std::string& value) const {
    return Token{type, value, m_line, m_column};
}

// ────────────────────────────────
// 跳过空白和注释
// ────────────────────────────────

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = current();

        // 跳过空白字符
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
            continue;
        }

        // 跳过单行注释 //
        if (c == '/' && peek() == '/') {
            while (!is_at_end() && current() != '\n') {
                advance();
            }
            continue;
        }

        break;
    }
}

// ────────────────────────────────
// 读取数字
// ────────────────────────────────

Token Lexer::read_number() {
    int start_line   = m_line;
    int start_column = m_column;
    std::string value;

    while (!is_at_end() && (std::isdigit(current()) || current() == '_')) {
        if (current() != '_') {   // 下划线只用于分隔，不加入值
            value += current();
        }
        advance();
    }

    // 小数部分
    if (current() == '.' && std::isdigit(peek())) {
        value += '.';
        advance();
        while (!is_at_end() && std::isdigit(current())) {
            value += advance();
        }
    }

    return Token{TokenType::Number, value, start_line, start_column};
}

// ────────────────────────────────
// 读取字符串
// ────────────────────────────────

Token Lexer::read_string() {
    int start_line   = m_line;
    int start_column = m_column;

    advance();  // 跳过开头的 "

    std::string value;
    while (!is_at_end() && current() != '"') {
        if (current() == '\\') {
            advance();  // 跳过反斜杠
            switch (current()) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                default:
                    value += '\\';
                    value += current();
            }
        } else {
            value += current();
        }
        advance();
    }

    if (is_at_end()) {
        // 字符串没有闭合
        throw std::runtime_error(
            "🌮 line " + std::to_string(start_line) +
            ": Unterminated string. Did you forget the closing \"?"
        );
    }

    advance();  // 跳过结尾的 "
    return Token{TokenType::String, value, start_line, start_column};
}

// ────────────────────────────────
// 读取标识符或关键字
// ────────────────────────────────

Token Lexer::read_identifier_or_keyword() {
    int start_line   = m_line;
    int start_column = m_column;
    std::string value;

    while (!is_at_end() && (std::isalnum(current()) || current() == '_')) {
        value += advance();
    }

    // 查关键字表，不在就是标识符
    auto it = KEYWORDS.find(value);
    TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::Identifier;

    return Token{type, value, start_line, start_column};
}

// ────────────────────────────────
// 读取 🌮 彩蛋
// ────────────────────────────────

Token Lexer::read_taco_emoji() {
    // 🌮 是 UTF-8 四字节序列：0xF0 0x9F 0x8C 0xAE
    // 调用时已经确认第一个字节是 0xF0，这里消费剩余三个字节
    int start_line   = m_line;
    int start_column = m_column;

    std::string value;
    value += advance();  // 0xF0
    value += advance();  // 0x9F
    value += advance();  // 0x8C
    value += advance();  // 0xAE

    return Token{TokenType::Taco, value, start_line, start_column};
}

// ────────────────────────────────
// 读取下一个 Token
// ────────────────────────────────

Token Lexer::next_token() {
    skip_whitespace_and_comments();

    if (is_at_end()) {
        return make_token(TokenType::EndOfFile, "");
    }

    int start_line   = m_line;
    int start_column = m_column;
    char c = current();

    // 数字
    if (std::isdigit(c)) {
        return read_number();
    }

    // 字符串
    if (c == '"') {
        return read_string();
    }

    // 标识符或关键字
    if (std::isalpha(c) || c == '_') {
        return read_identifier_or_keyword();
    }

    // 🌮 彩蛋（UTF-8 四字节，第一个字节是 0xF0）
    if (static_cast<unsigned char>(c) == 0xF0) {
        // 检查接下来三个字节是否匹配 🌮
        if (static_cast<unsigned char>(peek(1)) == 0x9F &&
            static_cast<unsigned char>(peek(2)) == 0x8C &&
            static_cast<unsigned char>(peek(3)) == 0xAE) {
            return read_taco_emoji();
        }
    }

    // 消费当前字符，处理单字符和双字符运算符
    advance();

    switch (c) {
        case '+': return Token{TokenType::Plus,         "+", start_line, start_column};
        case '-': return Token{TokenType::Minus,        "-", start_line, start_column};
        case '*': return Token{TokenType::Star,         "*", start_line, start_column};
        case '/': return Token{TokenType::Slash,        "/", start_line, start_column};
        case '%': return Token{TokenType::Percent,      "%", start_line, start_column};
        case '^': return Token{TokenType::Caret,        "^", start_line, start_column};
        case '?': return Token{TokenType::Question,     "?", start_line, start_column};
        case ':': return Token{TokenType::Colon,        ":", start_line, start_column};
        case ';': return Token{TokenType::Semicolon,    ";", start_line, start_column};
        case ',': return Token{TokenType::Comma,        ",", start_line, start_column};
        case '(': return Token{TokenType::LeftParen,    "(", start_line, start_column};
        case ')': return Token{TokenType::RightParen,   ")", start_line, start_column};
        case '{': return Token{TokenType::LeftBrace,    "{", start_line, start_column};
        case '}': return Token{TokenType::RightBrace,   "}", start_line, start_column};
        case '[': return Token{TokenType::LeftBracket,  "[", start_line, start_column};
        case ']': return Token{TokenType::RightBracket, "]", start_line, start_column};

        case '!':
            if (match('=')) return Token{TokenType::NotEqual,     "!=", start_line, start_column};
            return Token{TokenType::Not, "!", start_line, start_column};

        case '=':
            if (match('=')) return Token{TokenType::Equal,  "==", start_line, start_column};
            return Token{TokenType::Assign, "=", start_line, start_column};

        case '<':
            if (match('=')) return Token{TokenType::LessEqual,    "<=", start_line, start_column};
            return Token{TokenType::Less, "<", start_line, start_column};

        case '>':
            if (match('=')) return Token{TokenType::GreaterEqual, ">=", start_line, start_column};
            return Token{TokenType::Greater, ">", start_line, start_column};

        case '&':
            if (match('&')) return Token{TokenType::And, "&&", start_line, start_column};
            break;

        case '|':
            if (match('>')) return Token{TokenType::PipeArrow, "|>", start_line, start_column};
            break;

        case '.':
            if (current() == '.' && peek() == '.') {
                advance(); advance();
                return Token{TokenType::Ellipsis, "...", start_line, start_column};
            }
            return Token{TokenType::Dot, ".", start_line, start_column};
    }

    // 遇到无法识别的字符，报错
    throw std::runtime_error(
        "🌮 line " + std::to_string(start_line) +
        ": Unexpected character '" + c + "'."
    );
}

// ────────────────────────────────
// 核心接口
// ────────────────────────────────

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        Token t = next_token();
        tokens.push_back(t);
        if (t.type == TokenType::EndOfFile) break;
    }

    return tokens;
}
```

---

## 7.6 测试：把 `var x = 10;` 切成 Token 列表

### main.cpp

```cpp
#include <iostream>
#include "lexer.h"
#include "token.h"

int main() {
    std::string source = R"(
var x = 10 + 20;
var name = "Miguel";
var flag = true;

func greet(name) {
    print("Hola, " + name + "!");
}
)";

    Lexer lexer(source);

    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenize();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    // 打印每个 Token
    for (const auto& token : tokens) {
        std::cout
            << "[" << token_type_to_string(token.type) << "] "
            << "\"" << token.value << "\""
            << " (line " << token.line << ")\n";
    }

    return 0;
}
```

### 更新 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(taco VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_executable(taco
    src/main.cpp
    src/token.cpp
    src/lexer.cpp
)

target_include_directories(taco PRIVATE src)
```

### 编译运行

```bash
cmake -B build
cmake --build build
./build/taco
```

### 输出

```
[VAR] "var" (line 2)
[IDENTIFIER] "x" (line 2)
[ASSIGN] "=" (line 2)
[NUMBER] "10" (line 2)
[PLUS] "+" (line 2)
[NUMBER] "20" (line 2)
[SEMICOLON] ";" (line 2)
[VAR] "var" (line 3)
[IDENTIFIER] "name" (line 3)
[ASSIGN] "=" (line 3)
[STRING] "Miguel" (line 3)
[SEMICOLON] ";" (line 3)
[VAR] "var" (line 4)
[IDENTIFIER] "flag" (line 4)
[ASSIGN] "=" (line 4)
[TRUE] "true" (line 4)
[SEMICOLON] ";" (line 4)
[FUNC] "func" (line 6)
[IDENTIFIER] "greet" (line 6)
[LEFT_PAREN] "(" (line 6)
[IDENTIFIER] "name" (line 6)
[RIGHT_PAREN] ")" (line 6)
[LEFT_BRACE] "{" (line 6)
[IDENTIFIER] "print" (line 7)
[LEFT_PAREN] "(" (line 7)
[STRING] "Hola, " (line 7)
[PLUS] "+" (line 7)
[IDENTIFIER] "name" (line 7)
[PLUS] "+" (line 7)
[STRING] "!" (line 7)
[RIGHT_PAREN] ")" (line 7)
[SEMICOLON] ";" (line 7)
[RIGHT_BRACE] "}" (line 8)
[EOF] "" (line 9)
```

词法分析器正确地把源代码切成了 Token 列表。关键字 `var`、`func`、`true` 被识别为对应的类型，而不是标识符。字符串的内容（`"Miguel"`）去掉了引号，只保留内容本身。

---

## 7.7 这个版本的局限性

v0 能正确识别 Token，但有几个明显的局限性：

**不支持字符串插值**

```taco
var greeting = "Hola, {name}!";
```

`"Hola, {name}!"` 现在被当成一个普通字符串，不会把 `{name}` 识别成插值表达式。字符串插值需要在读取字符串时，把 `{...}` 里的内容单独切出来，作为一个表达式处理。这需要解析器的配合，v1 再处理。

**没有行列号精确追踪**

当前的行号追踪是粗糙的——只在遇到 `\n` 时更新行号。列号的追踪也不够准确。v1 引入 AST 之后，会用行列号来生成精确的错误信息。

**没有从文件读取**

现在源代码是硬编码在 `main.cpp` 里的字符串。下一步要从 `.taco` 文件读取，并处理命令行参数。

**没有彩蛋逻辑**

🌮 Token 现在只是被识别出来，但没有任何特殊行为。彩蛋逻辑（Magic 8 Ball、魔法海螺等）会在 v6 的 REPL 里实现——在那之前，🌮 只是一个普通的 Token。

---

### v0 项目小结

到这里，v0 完成了解释器的第一层：词法分析器。

**Token** 是词法分析的输出单元，包含类型和原始文本，用 `struct` 定义。`enum class` 比普通 `enum` 更安全，是定义 TokenType 的正确方式。

**Lexer** 用 `class` 定义，核心逻辑是 `next_token()`：跳过空白和注释，然后根据当前字符决定读哪种 Token。关键字通过查表（`unordered_map`）来识别，避免了一大堆 `if/else`。这也是本章学到的封装思想第一次在 Taco 项目里落地——`Lexer` 把扫描逻辑的所有细节都藏在 `private` 里，外部只看得到 `tokenize()` 这一个接口。

**错误处理** 现在很简单：遇到无法识别的字符就抛出异常，打印带行号的错误信息。这符合 Taco 的设计——出错直接崩溃，信息要清晰。

---

## 小结

这一章分两部分：先讲 C++ 类的基础语法，再回到 Taco 项目，把 v0 的最后一块拼图补上。

**成员变量**描述对象的状态，**成员函数**描述对象的行为。两者绑定在一起，这就是封装。

**访问控制**（`public`/`private`）让类可以暴露稳定的接口，隐藏实现细节。`struct` 默认 `public`，`class` 默认 `private`——根据是否需要封装来选择。

**`this` 指针**是每个成员函数里隐式存在的，指向当前对象。大多数时候不需要显式写，但返回对象自身时必须用 `*this`。

学完这些之后，`Lexer` 类就不再是一堆看不懂的语法了：`public`/`private` 划清了对外接口和内部细节的界限，类外定义（`Lexer::xxx`）把声明和实现分开，构造函数完成了初始化。v0 到这里就完整了——`Lexer` 的构造函数里用了"初始化列表"语法（`: m_source(...), m_pos(0) ...`），当时只是先照着写，下一章会正式讲清楚它到底是什么、为什么比函数体里赋值更好。

---

下一章讲构造函数和析构函数——对象是怎么诞生和消亡的，以及 C++ 里最重要的设计模式之一：RAII。
