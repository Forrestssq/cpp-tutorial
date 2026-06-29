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
