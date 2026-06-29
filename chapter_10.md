# 第十章：运算符重载

---

运算符重载（operator overloading）让自定义类型可以使用 `+`、`==`、`<<` 这样的运算符，让代码读起来像操作内置类型一样自然。

Python 里也有运算符重载，通过 `__add__`、`__eq__`、`__str__` 等魔法方法实现。C++ 的机制不同，但思路相同。

---

## 10.1 什么是运算符重载

当写 `a + b` 时，编译器实际上在调用一个函数。对于内置类型（`int`、`double`），这个函数是语言内置的。对于自定义类型，可以定义这个函数的行为——这就是运算符重载。

```cpp
// 这两行等价
Token result = t1 + t2;
Token result = operator+(t1, t2);  // 编译器看到的实际调用
```

运算符重载有两种写法：**成员函数**和**非成员函数**。

---

## 10.2 常用运算符重载

### == 和 != ：相等比较

在 Taco 里，比较两个 Token 是否相同很常见（比如检查当前 Token 是不是某个关键字）：

```cpp
// 成员函数写法
class Token {
public:
    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }

    bool operator!=(const Token& other) const {
        return !(*this == other);  // 复用 == 的逻辑
    }
};

Token t1{TokenType::Var, "var", 1, 1};
Token t2{TokenType::Var, "var", 1, 1};
Token t3{TokenType::Identifier, "x", 1, 5};

if (t1 == t2) { ... }  // true
if (t1 != t3) { ... }  // true
```

也可以只比较类型，不比较值：

```cpp
// 更常用的写法：直接和 TokenType 比较
bool operator==(TokenType t) const {
    return type == t;
}

// 使用
if (current_token == TokenType::Var) { ... }
```

---

### << ：输出运算符

重载 `<<` 让对象可以直接用 `std::cout` 输出：

```cpp
// 非成员函数写法（推荐）
// 因为左边是 std::ostream，不是 Token，不能写成 Token 的成员函数
std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "[" << token_type_to_string(token.type) << "] "
       << "\"" << token.value << "\""
       << " (line " << token.line << ")";
    return os;  // 返回 os 支持链式调用
}

// 使用
Token t{TokenType::Number, "42", 1, 5};
std::cout << t << "\n";  // [NUMBER] "42" (line 1)

// 链式调用
std::cout << "Token: " << t << ", next: " << t2 << "\n";
```

`<<` 运算符必须返回 `std::ostream&`，才能支持链式调用（`cout << a << b << c`）。

---

### + ：加法（字符串拼接的例子）

假设要实现一个简单的值类型，支持字符串拼接：

```cpp
class TacoString {
public:
    explicit TacoString(std::string s) : m_value(std::move(s)) {}

    // 成员函数写法
    TacoString operator+(const TacoString& other) const {
        return TacoString(m_value + other.m_value);
    }

    std::string value() const { return m_value; }

private:
    std::string m_value;
};

TacoString a("Hello, ");
TacoString b("World!");
TacoString c = a + b;
std::cout << c.value() << "\n";  // Hello, World!
```

---

### [] ：下标运算符

让对象支持 `obj[index]` 的访问方式：

```cpp
class TokenStream {
public:
    // 只读版本（const）
    const Token& operator[](int index) const {
        return m_tokens[index];
    }

    // 可写版本（非 const）
    Token& operator[](int index) {
        return m_tokens[index];
    }

    int size() const { return static_cast<int>(m_tokens.size()); }

private:
    std::vector<Token> m_tokens;
};

TokenStream stream;
// ...
Token first = stream[0];  // 使用 [] 运算符
```

---

### 布尔转换：operator bool

让对象可以在 `if` 条件里使用：

```cpp
class LexerResult {
public:
    explicit operator bool() const {
        return m_success;
    }

    bool m_success;
    std::vector<Token> m_tokens;
    std::string m_error;
};

LexerResult result = run_lexer("var x = 10;");
if (result) {  // 使用 operator bool
    // 成功
} else {
    std::cerr << result.m_error << "\n";
}
```

注意 `explicit` 关键字：它防止 `LexerResult` 在不期望的地方被隐式转换成 `bool`（比如用来做算术运算）。

---

## 10.3 什么时候该重载，什么时候不该

运算符重载可以让代码更简洁，但滥用会让代码更难懂。

**应该重载的情况**：

- 运算符的语义对于这个类型来说很自然（`+` 对字符串，`==` 对值类型）
- 代码读起来更清晰，而不是更神秘
- 标准库或常见约定期望这个类型支持某个运算符（比如放入 `std::map` 需要 `<`，`std::cout` 输出需要 `<<`）

**不应该重载的情况**：

- 运算符的含义不明显（`&` 重载成"查找"，`*` 重载成"执行"）
- 只是为了让代码看起来酷，但实际上增加了理解负担
- 有更清晰的命名函数可以代替

一个常见的反例是把 `+` 重载成完全不相关的操作：

```cpp
// 糟糕的设计
Lexer operator+(const Lexer& a, const std::string& extra_source) {
    // 把 extra_source 追加到 a 的源代码？
    // 这让人完全看不懂
}
```

**Python 对比**：

Python 的魔法方法和 C++ 的运算符重载思路相同：

```python
class Token:
    def __eq__(self, other):
        return self.type == other.type and self.value == other.value

    def __repr__(self):
        return f'[{self.type}] "{self.value}"'
```

```cpp
// C++ 对应
bool operator==(const Token& other) const { ... }
std::ostream& operator<<(std::ostream& os, const Token& t) { ... }
```

`__repr__` 在 C++ 里对应 `<<` 运算符（没有完全对应的，但 `<<` 是最接近的输出接口）。

---

### Taco 项目里的运算符重载

在 Taco 解释器里，目前最有用的是：

```cpp
// Token 的相等比较：检查 Token 类型
bool Token::operator==(TokenType t) const {
    return type == t;
}

// Token 的输出：调试时打印 Token 列表
std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "[" << token_type_to_string(token.type) << "] \""
       << token.value << "\"";
    return os;
}
```

用这两个，`main.cpp` 的调试输出可以改成：

```cpp
for (const auto& token : tokens) {
    std::cout << token << "\n";  // 直接用 << 输出
}

// 检查当前 token 是不是 var 关键字
if (tokens[0] == TokenType::Var) { ... }
```

---

## 小结

**运算符重载**让自定义类型可以使用 C++ 的运算符，让代码读起来更自然。

常用的重载：`==`/`!=` 用于相等比较，`<<` 用于输出，`[]` 用于下标访问，`bool` 转换用于条件判断。

**成员函数**还是**非成员函数**的选择：如果运算符的左操作数是这个类型，用成员函数；如果左操作数是其他类型（比如 `<<` 的左边是 `ostream`），用非成员函数。

**什么时候重载**：运算符语义自然、代码更清晰时才重载。不要为了"酷"而重载，不明显的运算符只会让人困惑。

---

下一章是第二部分的项目章节：用类来重构词法分析器，并实现语法分析器和 AST（v1）。
