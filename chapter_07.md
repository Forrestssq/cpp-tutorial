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

### 在类外定义成员函数

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

> `Tips`:
> 成员函数在底层其实就是一个普通函数，只不过编译器帮你把"调用这个函数的对象"作为第一个参数（`this`）传进去。这就是为什么 Python 里需要显式写 `self`，而 C++ 里 `this` 是隐式的。看下面的代码对比：

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

> 详细的讲解见后面的 More about `static` 章节。

```cpp
class Logger {
public:
    static Logger& get_instance() { // 这行的static 表示：类的 static 成员，所有的实例共用它
        static Logger instance;     // 这行的static 表示：局部静态变量，它只在第一次调用时构造
        return instance;
    }
    
    void log(const std::string& message) { // 补上这个成员函数 
	    std::cout << "[LOG] " << message << "\n"; 
	}
private:
    Logger() {}   // 构造函数设为 private，外部无法直接 new 一个
};

Logger::get_instance().log("hello");
```

这种写法依赖的正是 `static` 成员函数不需要对象就能调用的特性，是个常见的实际应用场景，但属于进阶用法。

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

### `struct` 和 `class` 的区别

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


## 7.2 More About `static` 与 单例

前面讲的 `static` 都是"类的 `static` 成员"这一种用法，但 `static` 这个关键字在 C++ 里其实还有别的含义，单例模式里用到的正是另外一种——局部静态变量（local static variable）。它和在 `C` 中的含义是一样的。

先脱离类，单独看一下局部静态变量是什么。

```cpp
void counter_func() {
    static int count = 0;   // 只在第一次执行到这一行时初始化
    count++;
    std::cout << count << "\n";
}

counter_func();   // 输出 1
counter_func();   // 输出 2
counter_func();   // 输出 3
```

普通的局部变量，每次调用函数都会重新创建一份，函数一返回就销毁，下次调用又是全新的一份——这也是为什么普通局部变量没法用来"记住"上次调用时的状态。但加上 `static` 之后，情况变了：这个变量**只在第一次执行到这行代码时初始化一次**，之后无论这个函数被调用多少次，都直接复用同一块内存，不会重新初始化，函数返回时也不会销毁它，要一直等到整个程序结束才会销毁。它的生命周期跟全局变量一样长，但作用域依然只在函数内部可见，外面访问不到——相当于"生命周期是全局的，可见范围是局部的"。

理解了这一点，回头再看单例模式那段代码：

```cpp
class Logger {
public:
    static Logger& get_instance() { // 这行的static 表示：类的 static 成员，所有的实例共用它
        static Logger instance;     // 这行的static 表示：局部静态变量，它只在第一次调用时构造
        return instance;
    }
    
    void log(const std::string& message) { // 补上这个成员函数 
	    std::cout << "[LOG] " << message << "\n"; 
	}
private:
    Logger() {}   // 构造函数设为 private，外部无法直接 new 一个
};

Logger::get_instance().log("hello");
```

看 `private:` 后面的这个空函数 `Logger() {}` 。这个函数名和这个 `class` 名是一样的，所以它是我们显式地初始化 `class` ！

> `Tips`:
> 希望你仍然记得"构造函数"章节里面，我们提到过，如果对一个 `class` 没有写用来初始化的构造函数（就是专门用来给 `class` 初始化的函数， 复杂一点的长这样： `Counter(std::string name) m_counter_name(name) {};`），那么 `C++` 会自动补上空的构造函数：

```cpp
class Logger { 
public: 
	void log(const std::string& message) { 
	// 省略...
	}
private:
	// 省略...
}

// 上面的代码将补齐成下面这样：
class Logger {
public:
	Logger() {}; // 加上了这行
	void log(const std::string& message) {
	// 省略...
	}
private:
	// 省略...
}
```

> 但是如果我们写了一个构造函数，那么 `C++` 就不会为我们在 `public:` 下自动生成这个空的构造函数。

我们知道，在 `public:` 下的内容在外部是可以被调用的，所以构造函数默认放在 `public:` 下，这样我们才可以在外面调用它来初始化一个实例。

那么，如果我们突发奇想地把构造函数放在 `private:` 下，外面就不能调用它了；也就是说，在外部无法初始化这个 `class` 的实例。

又注意到，我们可以构造一个 `static` 成员变量或函数，而这个变量或函数不需要实例化就可以存在。

把两个合起来，再看一遍这个`class`：

```cpp
class Logger {
public:
    static Logger& get_instance() { // 这行的static 表示：类的 static 成员，所有的实例共用它
        static Logger instance;     // 这行的static 表示：局部静态变量，它只在第一次调用时构造
        // 这里是在初始化 Logger 的实例。因为这一行代码写在 `Logger` 类**自己的成员函数**里面，
        // 而 `private` 规则允许类的成员函数访问本类的 `private` 成员，
        // 所以可以访问 `private` 下的构造函数，进而实例化。
        // FYI，实例化一个 class 的语法就是 `class名 实例名;` ，
        // 然后 `C++` 会自动调用 构造函数 Logger() 来初始化实例。
        return instance;
    }
    
    void log(const std::string& message) { // 补上这个成员函数 
	    std::cout << "[LOG] " << message << "\n"; 
	}
private:
    Logger() {}   // 构造函数设为 private，外部无法直接 new 一个
};

Logger::get_instance().log("hello");
```

在外部无法实例化 `Logger` 对象（因为无法访问构造函数），但可以调用 `Logger::get_instance()`；而 `get_instance()` 这个函数本来有权限访问同一个 `class` 下的构造函数，所以它能在内部造出对象，再把这个对象交给外部用。 

 `get_instance()` 之所以能保证"只造一次"，靠的是第二个`static`：`static Logger instance;`——这里的 `static` 已经不是"类的 static 成员"这种用法了，而是表示另一种含义：局部静态变量：
 
```cpp 
static Logger& get_instance() { 
	static Logger instance; // 只在第一次调用 get_instance() 时，这一行才会真正执行构造 
	return instance; // 之后每次调用，这一行直接跳过，instance 还是原来那个
} 
```

所以不管外部代码调用 `Logger::get_instance()` 多少次，`instance` 自始至终只会被构造那一次。

后续每次调用都只是把同一个对象的引用再返回一遍，全程序里实实在在存在的 `Logger` 对象永远只有这一个——这正是"单例"这个名字想表达的：单，独此一份的意思。 

`get_instance()` 是一个 `static` 成员函数，不需要先有对象就能调用。

它内部的 `static Logger instance;` 是一个局部静态变量——`instance` 只会在第一次调用 `get_instance()` 时被真正构造出来，之后不管再调用 `get_instance()` 多少次，返回的都是同一个 `instance`，不会重复构造。

又因为 `Logger` 的构造函数是 `private` 的，外部代码没办法绕开 `get_instance()` 自己创建另一个 `Logger` 对象，这就保证了全局范围内 `Logger` 只可能存在这一份实例——这正是"单例"这个名字的由来。这种写法有个专门的名字，叫 Meyer's Singleton（以提出者 Scott Meyers 命名），是现代 C++ 里最常见、最推荐的单例实现方式。

值得一提的是，C++11 标准专门为局部静态变量的初始化加了一条保证：**如果多个线程同时第一次调用到这行代码，标准保证只有一个线程会真正执行构造，其他线程会等待这次构造完成，不会出现重复构造或构造到一半被另一个线程读到的情况**。这条保证俗称"magic statics"（神奇静态量），是 C++11 才正式写进标准的行为。这也是为什么 Meyer's Singleton 在现代 C++ 里特别受欢迎——在它之前，想写一个线程安全的单例，得自己手动加锁、做双重检查锁定（double-checked locking）之类相当容易出错的操作，而局部静态变量的写法把这件事完全交给编译器和语言标准来保证，代码反而更短、更不容易出 bug。

>补充一句，与C一样，`static` 在 C++ 里其实还有有一种用法：写在全局作用域的变量或函数前面（比如 `static int x = 0;` 写在文件最外层，不在任何类或函数里），作用是把这个变量或函数的链接属性限制成"内部链接"（internal linkage），意思是它只在当前这一个源文件里可见，不会被其他 `.cpp` 文件看到，用来避免不同源文件之间因为重名而冲突。

## 7.3 `lexer` 项目 Part 1：大框架

我们回到 `Lexer` 到设计中来吧。下面是作者的得意之作，保证逻辑清晰地从头开始把这个 `lexer` 分词器不跳步地一步一步写出来。

我们先要明确 `Lexer` 需要什么数据。

### 数据

回看一下在 `token.h` 中定义的 `struct` ：

```cpp
// token.h
// Token 结构体
struct Token {
    TokenType   type;
    std::string value;   // 原始文本
    int         line;    // 行号（从 1 开始）
    int         column;  // 列号（从 1 开始）
};
```

还记得我们在分析一段代码（比如 `var x = 10`）的时候，需要把它拆成一个一个的 `Token` 吗（比如 `type: IDENTIFIER, value: "x"` ）这样。为了实现这样的拆分，我们需要 `lexer` ：

1. 给人看的：记录下分析到了哪一个词（需要行号+列号）；
2. 给程序用的：记录下当前分析到哪个词了（只需一个变量即可）；
3. 源代码。

> 讲解：
> 假设我们有下面的代码需要 `lexer` 来分析：

```cpp
var x = 10;
var y = 20;
var z = x + y;
```

> 上面是人看到的。但是对于程序来说，它看到的是：

```cpp
var x = 10;\nvar y = 10;\nvar z = x + y;
```

> 也就是说，程序没有"行号""列号"这两个概念：因为都是字符编码，都是在"一行"里面的。
> 
> 行号列号只是在报错的时候给人看的，在程序内部只需要一个 `pos` 记录下当前分析到的位置即可。

所以我们可以写出如下的代码：

```cpp
// lexer.h
#include <string>

class Lexer {
public:

private:
    std::string m_source; // 用来放源代码的
    int m_pos;            // 记录程序分析到的位置
    int m_line;           // 记录行号（给人看）
    int m_column;         // 记录列号（给人看）
};

```

然后我们需要让 `Lexer` 接受源代码（显然是 `std::string` 类型）：

```cpp
// lexer.h
#include <string>

class Lexer {
public:
    Lexer(std::string source) : m_source(source) {}
private:
    std::string m_source; // 用来放源代码的
    int m_pos;            // 记录程序分析到的位置
    int m_line;           // 记录行号（给人看）
    int m_column;         // 记录列号（给人看）
};

```

希望我们就此熟悉了 `class` 的构造函数（constructor）的写法。ps：不要在构造函数后面加 `;` 哦。

不知是否还记得，这样的构造函数是不安全的，因为 `C++` 可能会隐式地进行类型转换，从而产生意外。记得总是在构造函数前面加上 `explicit` 关键字，限定只能通过 `Lexer(源代码)` 这样的方式来实例化。

```cpp
// lexer.h
#include <string>

class Lexer {
public:
    explicit Lexer(std::string source) : m_source(source) {}
private:
    std::string m_source; // 用来放源代码的
    int m_pos;            // 记录程序分析到的位置
    int m_line;           // 记录行号（给人看）
    int m_column;         // 记录列号（给人看）
};
```

还没好！我们还需要初始化这三个 `m_` 开头的参数（因为在一开始的时候， `m_pos = 0`， `m_line = 1`， `m_column = 1` 这些是可以确定的，如果对line和column为1不为0感到惊讶的话，请回忆我们人类在数行数的时候是从"第一行"开始数的还是"第零行"开始数的）。

要么在构造函数里面初始化：

```cpp
// lexer.h
#include <string>

class Lexer {
public:
    explicit Lexer(std::string source) 
        : m_source(source)
        , m_pos(0)
        , m_line(1)
        , m_column(1)
    {}
private:
    std::string m_source; // 用来放源代码的
    int m_pos;            // 记录程序分析到的位置
    int m_line;           // 记录行号（给人看）
    int m_column;         // 记录列号（给人看）
};

```

要么在它们各自的后面直接初始化：

```cpp
// lexer.h
#include <string>

class Lexer {
public:
    explicit Lexer(std::string source) : m_source(source) {}
private:
    std::string m_source;     // 用来放源代码的
    int m_pos = 0;            // 记录程序分析到的位置
    int m_line = 1;           // 记录行号（给人看）
    int m_column = 1;         // 记录列号（给人看）
};
```

这里更推荐第二种，因为构造函数中最好是初始化传入的值。

### 处理数据

数据定义好了，接着是写处理数据所需要的函数。

我们想要 `Lexer` 实现的是：在实例化的时候，传入 `source_code` （ `std::string` ）。在之后，对这个实例调用一个函数（命名为 `tokenize()` ）的时候，他可以返回一个 `vector` （一个存放 `Token` 的列表） ，里面存放了分析好的信息。如下：

```cpp
std::string example_code = "var x = 10";

Lexer code1(example_code);
std::vector<Token> tokens = code1.tokenize()
```

然后一个一个分行打印出来，结果是这样的（TokenType + code + 行号）：

```bash
[VAR] "var" (line 1) 
[IDENTIFIER] "x" (line 1) 
[ASSIGN] "=" (line 1) 
[NUMBER] "10" (line 1) 
[EOF] "" (line 1)
```

我们先把这个函数放到 `public:` 里面：

```cpp
class Lexer {
public:
	explicit Lexer(std::string source);
	std::vector<Token> tokenize(); // 返回一个放 Token 的列表
private:
	std::string m_source;
	int m_pos = 0;
	int m_line = 1;
	int m_column = 1;
}
```

那么我们怎么实现这个函数呢？

首先当然是将 `m_source` 中的源代码的各个 `Token` 拆封后对应成 `TokenType` 。

为了减少翻页的苦痛，下面贴出来部分的 `enum class TokenType` 代码（显然是不完整的）：

```cpp
// token.h

enum class TokenType {
    // 字面量
    Number,       // 42, 3.14, 1_000_000
    String,       // "hello"
    True,         // true
    False,        // false
    Nil,          // nil

    // 标识符和关键字
    Identifier,   // x, name, greet
    Var,          // var
    Func,         // func
    If,           // if
    Elseif,       // elseif
    Else,         // else
    
    EndOfFile,
 };
```

源代码有三种情况，一个是**系统保留的关键字**，比如 `var`、`nil`、 `true` 等等。这样关键字的数量很少，我们不妨直接写好对应的关键字表，比如让程序看到 `var` 就对应成  `TokenType` 中的 `Var` ，看到 `func` 就对应成 `Func` 。

二是**用户定义标识符**，比如函数名，变量名。它的数量无限、由用户自由命名，没法预先列举。

三是**运算符和符号**，比如 `=`、`+`、`(`、`;`。这些不是由字母组成的单词，而是固定的符号，直接一对一识别即可（比如看到 `+` 就直接返回 `TokenType::Plus`），不需要经过"查关键字表"这一步。

因此，程序的判断方法是：如果这个 `Token` 不是运算符或者符号的话，那么先假设这个 `Token` 是标识符，然后去关键字表中查。如果没有查到，那么就是标识符；如果查到了，那么将它的类型改为相应的 `TokenType` 关键字。

那么用什么来储存这个一一对应的关键字表呢？我们很容易联想到 `python` 的字典是一个很好的选择。

`C++` 中的字典的用法与 `std::vector<T>` 类似，如下：

```cpp
#include <unordered_map> // 导入库

std::unordered_map<T, U> KEYWORDS; // 声明。T 和 U 是键值对的类型，其中 `T` 类型是 `key` , `U` 类型是 `value` 。字典的名称一般大写。

KEYWORDS = {
	{"var", TokenType::Var},  // 键值对用 `{}` 包裹，键值用 `,` 隔开
	{"func", TokenType::Func},
	{"for", TokenType::For},
	// 省略...
}; //记得在结尾加上分号
```

有三种调用方法：

1. `find()`

```cpp
auto it = KEYWORDS.find("var");
```

`find("var")` 做的事情就是去表里找有没有 key 叫 "var" 的**那一行**。

那么就有两种可能：

找到了：`it` 会指向表里面的那一行（ `{"var", TokenType::var}` ），然后我们可以用 `it->second` 拿到对应的value（即 `TokenType::Var` ）。

注意，`KEYWORDS` 本身是一个 `unordered_map`（哈希表容器），它实现了迭代功能，内部存的是一个个 `pair` 这种 struct。而 `std::pair` 的简化定义如下：

```cpp
template<typename T1, typename T2>
struct pair {
    T1 first;
    T2 second;
};
```

它是一个数组！

所以 `KEYWORDS.find("var")` 返回的 `it` 是一个迭代器，它指向 `KEYWORDS` 这个 `unordered_map` 里的某一个 `pair`。每个 叫做`pair` 的 `struct` 都有 `first`（key）和 `second`（value）两个成员，所以我们可以用 `it->first` 取出 key（`"var"`），用 `it->second` 取出 value（`TokenType::Var`）。

> 迭代器在后面章节会讲，简单说，迭代器就是一个"指向容器中某个元素的东西"。它用起来很像指针，可以用 `->` 访问它指向的内容，因为 `it->second` 等价于 `(*it).second` ，而在第一步 `(*it)` 解引用的时候，就会找到迭代器指向的元素，在这里它指向的是struct，也就是说， `(*it)` 的时候已经是一个 `struct` 了，那么后面对 `struct` 取出成员好理解了。

如果没找到，那么 `it` 就会等于 `KEYWORDS.end()` 。`end()` 是一个特殊的迭代器，指向"末尾之后"这个位置；那个位置没有真实的 `pair`，所以不能对它解引用（`*it` 或 `it->second`），否则是未定义行为。

所以我们使用前**必须先判断** `it != KEYWORDS.end()`，确认没到末尾，然后才能安全地用 `it->second`。这也是为什么下面的 `it != KEYWORDS.end()` 判断永远要写在取值之前。

因此，我们这样用：

```cpp
auto it = KEYWORDS.find("var");
TokenType type;

if (it != KEYWORDS.end()) {
	std::cout << "Find!";
	type = it->second;
} else {
	std::cout << "Not found!";
	type = TokenType::Identifier; // 如果在 KEYWORDS 的表中没有找到，
	// 那么它就是 `Token::Identifier` 
}
```

我们也可以用三元运算符来简化上面的程序：

```cpp
auto it = KEYWORDS.find("var");

TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::Identifier;
```

2. `[]` 运算符直接取值

```cpp
TokenType type = KEYWORDS["var"];
```

注意，如果 `KEYWORDS` 是 `const` 的（我们的场景就是），**不能用 `[]`**！因为 `[]` 在 key 不存在时会自动插入一个新的键值对，这个"插入"操作违反了 `const` 的承诺，编译器会直接报错。所以在 Lexer 里必须用 `find()`，不能用 `[]`。

3. `count()` 只想知道存不存在

```cpp
if (KEYWORDS.count("var") > 0) {
	// 存在，然后处理
}
```

`count()` 对 `unordered_map` 来说只会返回 0 或 1（因为 key 不会重复），但它拿不到对应的 value，只能配合 `[]` 或 `find()` 再查一次，所以在 Lexer 场景里不如直接用 `find()` 一步到位。

---

好的回到 `Lexer` 来。我们需要在 `class` 中放一个字典，这个字典应该是 `private` 的（不需要对外开放），并且，所有的实例都用共用这一个字典。那么在字典前面加上 `static` 关键字吧，让所有的成员共用一份。

另外，这个字典不应该被修改，所以加上 `const` 关键字。

因此，我们在 `lexer.h` 中就应该这样声明（还记得 `static` 的部分需要放到 `class` 外面去定义吗）：

```cpp
#include "token.h"
#include <string>
#include <unordered_map>

class Lexer {
public:
	explicit Lexer(std::string source) : m_source(source) {}
private:
	static const std::unordered_map<std::string, TokenType> KEYWORDS;
	std::string m_source;
	int m_pos = 0;
	int m_line = 1;
	int m_column = 1;
};
```

然后我们在 `lexer.cpp` 中给 `KEYWORDS` 做真正的定义：

> 下面的文件在 `codeSources/v0-lexer-unordered_map.cpp` 中

```cpp
// lexer.cpp

#include "lexer.h"

// 注意哦，对于 `class` 中的 `static` 成员变量，必须要完整地写出变量的类型，不能用 `auto` 。另外，不要忘了在 `KEYWORDS` 前加上 `Lexer::` 
const std::unordered_map<std::string, TokenType> Lexer::KEYWORDS = {
    {"true", TokenType::True},
    {"false", TokenType::False},
    {"nil", TokenType::Nil},
    {"var", TokenType::Var},
    {"func", TokenType::Func},
    {"if", TokenType::If},
    {"elseif", TokenType::Elseif},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"for", TokenType::For},
    {"in", TokenType::In},
    {"return", TokenType::Return},
    {"class", TokenType::Class},
    {"struct", TokenType::Struct},
    {"enum", TokenType::Enum},
    {"extends", TokenType::Extends},
    {"self", TokenType::Self},
    {"super", TokenType::Super},
    {"switch", TokenType::Switch},
    {"case", TokenType::Case},
    {"default", TokenType::Default},
    {"import", TokenType::Import},
    {"from", TokenType::From},
    {"thread", TokenType::Thread},
    {"channel", TokenType::Channel},
};
```

最讨人厌的部分已经完成了，下面就是有趣的编写分词逻辑的环节了。我们不妨先拿几张纸，看看自己会怎么设计这个分词逻辑，随后进入 Part 2

## 7.4 `lexer` 项目 Part 2：分词逻辑的实现


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
	if (is_at_end()) return '\0';
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
    
    if (std::isalpha(current()) || current == '_') {
	    throw std::runtime_err(
	    "🌮 line " + std::to_string(start_line) + 
	    ": Invalid number literal, unexpected character '" + 
	    current() + "' after number."
	    )
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
                // 对于未知的转义字符，我们选择原样保留，
                // 因为 Taco 是一门脚本语言，不想要设计的那么严格
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
    // 不能用 `make_token()` 来，因为这个函数的行号和列号是不对的。
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

## 7.4 测试：把 `var x = 10;` 切成 Token 列表

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

## 7.5 这个版本的局限性

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
