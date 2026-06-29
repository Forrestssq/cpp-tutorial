# 第二十五章：项目 v4——array、map、enum 与标准库

---

第五部分学完了 STL 容器、迭代器、算法、Lambda。现在用这些知识做 v4 最重要的两件事：

1. **实现 Taco 的 array 和 map 类型**，以及对应的 pipeline 方法（`filter`、`map`、`each`、`reduce`）
2. **实现 Taco 的内置标准库**，包括字符串、文件、系统操作等

v4 结束时，Taco 能运行这样的代码：

```taco
// array 和 pipeline
var nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

nums
    .filter { n in n % 2 == 0 }
    .map { n in n * n }
    .each { n in print(n) };
// 4 16 36 64 100

// array 方法
var arr = [3, 1, 4, 1, 5, 9, 2, 6];
print(arr.sum());         // 31
print(arr.min());         // 1
print(arr.max());         // 9
print(arr.len());         // 8
print(arr.contains(5));   // true
print(arr.getFirst(3));   // [3, 1, 4]
print(arr.getLast(3));    // [2, 6]... 不对，是最后3个 [9, 2, 6]

// map（字典）
var person = {"name": "Miguel", "age": 12};
print(person["name"]);    // Miguel
print(person.name);       // Miguel（点语法）
person["city"] = "Oaxaca";
print(person.getKeys());  // ["name", "age", "city"]

// 字符串方法
var s = "Hello, World!";
print(s.len());           // 13
print(s.upper());         // HELLO, WORLD!
print(s.contains("World")); // true
print(s.split(", "));     // ["Hello", "World!"]
print(s.getChars());      // ["H", "e", "l", ...]

// 文件操作
var content = cat("test.taco");
print(content.getLines().len());

// 系统操作
var files = ls(".");
files.filter { f in f.endsWith(".taco") }.each { f in print(f) };
```

---

## 25.1 用 STL 容器实现 Taco 的 array 和 map

### 更新值类型

```cpp
// value.h（v4 版本）
#pragma once
#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

struct TacoFunction;
struct TacoNative;

// Taco 的 array：vector<TacoValue> 的 shared_ptr 包装
// 用 shared_ptr 是因为 array 是引用类型（赋值是共享，不是拷贝）
struct TacoArray;
struct TacoMap;

using TacoValue = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<TacoFunction>,
    std::shared_ptr<TacoNative>,
    std::shared_ptr<TacoArray>,
    std::shared_ptr<TacoMap>
>;

// 定义在 value.h 里（或 array.h/map.h 里）
struct TacoArray {
    std::vector<TacoValue> elements;

    TacoArray() = default;
    explicit TacoArray(std::vector<TacoValue> elems)
        : elements(std::move(elems)) {}
};

struct TacoMap {
    // 用 vector<pair> 保持插入顺序（unordered_map 不保证顺序）
    std::vector<std::pair<std::string, TacoValue>> entries;

    TacoMap() = default;

    TacoValue get(const std::string& key) const;
    void set(const std::string& key, TacoValue val);
    bool has(const std::string& key) const;
    void remove(const std::string& key);
};
```

为什么 `TacoArray` 和 `TacoMap` 用 `shared_ptr` 包装？

在 Taco 里，array 和 map 是**引用类型**（和 class 一样）：

```taco
var a = [1, 2, 3];
var b = a;        // b 和 a 指向同一个 array
b.push(4);
print(a.len());   // 4，a 也变了
```

用 `shared_ptr` 可以让多个变量指向同一个底层数组，赋值时只拷贝指针，不拷贝数组内容。

---

### TacoMap 的实现

```cpp
// value.cpp（部分）

TacoValue TacoMap::get(const std::string& key) const {
    for (const auto& [k, v] : entries) {
        if (k == key) return v;
    }
    throw std::runtime_error("🌮 Key '" + key + "' not found.");
}

void TacoMap::set(const std::string& key, TacoValue val) {
    for (auto& [k, v] : entries) {
        if (k == key) {
            v = std::move(val);
            return;
        }
    }
    entries.push_back({key, std::move(val)});
}

bool TacoMap::has(const std::string& key) const {
    for (const auto& [k, v] : entries) {
        if (k == key) return true;
    }
    return false;
}

void TacoMap::remove(const std::string& key) {
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
                       [&key](const auto& pair) { return pair.first == key; }),
        entries.end()
    );
}
```

这里用 `vector<pair>` 而不是 `unordered_map`，是因为 Taco 的 map 需要保持插入顺序（和 Python 3.7+ 的 `dict` 类似）。查找是 O(n)，对于小型字典完全够用。如果需要高性能，可以改成 `unordered_map`。

---

## 25.2 实现 enum

Taco 的 enum 是一组命名常量：

```taco
enum Direction {
    North,
    South,
    East,
    West
}

var d = Direction.North;
switch (d) {
    case Direction.North { print("going north"); }
    default { print("other"); }
}
```

在 C++ 里，Taco 的 enum 值用字符串表示：`Direction.North` 就是字符串 `"Direction.North"`。

```cpp
// ast.h 新增
struct EnumDecl : Expr {
    std::string              name;
    std::vector<std::string> variants;  // 枚举值的名字列表

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override { return "enum " + name; }
};
```

```cpp
// ast.cpp
TacoValue EnumDecl::evaluate(Evaluator& eval) const {
    // 把每个变体注册为字符串常量
    // Direction.North = "Direction.North"
    for (const auto& variant : variants) {
        std::string full_name = name + "." + variant;
        eval.current_env()->define(name + "." + variant,
                                   std::string(full_name));
    }
    return nullptr;
}
```

带关联值的 enum：

```taco
enum Result {
    Ok(value),
    Err(message)
}

var r = Result.Ok(42);
```

带关联值的枚举变体是一个函数，调用它返回一个包含类型标签和值的 map：

```cpp
// Result.Ok 是一个函数，调用后返回 {"__type": "Result.Ok", "value": 42}
TacoValue make_enum_variant(const std::string& tag, std::vector<TacoValue> args) {
    auto m = std::make_shared<TacoMap>();
    m->set("__type", std::string(tag));

    // 根据关联值数量绑定到对应的字段名
    // 简化实现：只有一个关联值时用 "value"
    if (!args.empty()) {
        m->set("value", args[0]);
    }
    return m;
}
```

---

## 25.3 实现 pipeline：filter、map、each、reduce

Pipeline 是 Taco 最有特色的功能。`arr.filter { x in x > 3 }` 这样的方法链，每个方法接受一个闭包，返回新的 array。

在 C++ 里，这些方法挂在 `TacoArray` 上（通过方法分派），或者在求值器里作为内置方法处理。

### 方法调用的 AST 节点

```cpp
// ast.h 新增
struct MethodCallExpr : Expr {
    ExprPtr              object;     // 调用方法的对象
    std::string          method;     // 方法名
    std::vector<ExprPtr> args;       // 普通参数
    ExprPtr              closure_arg; // 闭包参数（{ x in ... } 风格）

    TacoValue evaluate(Evaluator& eval) const override;
    std::string to_string() const override {
        return object->to_string() + "." + method + "(...)";
    }
};
```

### 方法调用的求值

```cpp
// ast.cpp
TacoValue MethodCallExpr::evaluate(Evaluator& eval) const {
    TacoValue obj = eval.evaluate(object.get());

    // 求值普通参数
    std::vector<TacoValue> arg_vals;
    for (const auto& arg : args) {
        arg_vals.push_back(eval.evaluate(arg.get()));
    }

    // 求值闭包参数（如果有）
    TacoValue closure_val = nullptr;
    if (closure_arg) {
        closure_val = eval.evaluate(closure_arg.get());
    }

    // 分派到对应类型的方法
    if (std::holds_alternative<std::string>(obj)) {
        return dispatch_string_method(
            std::get<std::string>(obj), method, arg_vals, closure_val, eval
        );
    }

    if (std::holds_alternative<std::shared_ptr<TacoArray>>(obj)) {
        return dispatch_array_method(
            std::get<std::shared_ptr<TacoArray>>(obj), method, arg_vals, closure_val, eval
        );
    }

    if (std::holds_alternative<std::shared_ptr<TacoMap>>(obj)) {
        return dispatch_map_method(
            std::get<std::shared_ptr<TacoMap>>(obj), method, arg_vals, closure_val, eval
        );
    }

    throw std::runtime_error(
        "🌮 '" + value_to_string(obj) + "' has no method '" + method + "'."
    );
}
```

### array 的方法实现

```cpp
// array_methods.cpp

// 调用一个 Taco 函数（闭包）
static TacoValue call_closure(Evaluator& eval, const TacoValue& fn,
                               std::vector<TacoValue> args) {
    if (std::holds_alternative<std::shared_ptr<TacoFunction>>(fn)) {
        auto& f = std::get<std::shared_ptr<TacoFunction>>(fn);
        return call_user_function(eval, f, args, {});
    }
    throw std::runtime_error("🌮 Expected a function.");
}

TacoValue dispatch_array_method(
    std::shared_ptr<TacoArray> arr,
    const std::string& method,
    const std::vector<TacoValue>& args,
    const TacoValue& closure,
    Evaluator& eval)
{
    // ── filter ──────────────────────────────────────────
    if (method == "filter") {
        auto result = std::make_shared<TacoArray>();
        for (const auto& elem : arr->elements) {
            TacoValue keep = call_closure(eval, closure, {elem});
            if (is_truthy(keep)) {
                result->elements.push_back(elem);
            }
        }
        return result;
    }

    // ── map ─────────────────────────────────────────────
    if (method == "map") {
        auto result = std::make_shared<TacoArray>();
        result->elements.reserve(arr->elements.size());
        std::transform(
            arr->elements.begin(), arr->elements.end(),
            std::back_inserter(result->elements),
            [&](const TacoValue& elem) {
                return call_closure(eval, closure, {elem});
            }
        );
        return result;
    }

    // ── each ────────────────────────────────────────────
    if (method == "each") {
        std::for_each(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                call_closure(eval, closure, {elem});
            }
        );
        return nullptr;
    }

    // ── reduce ──────────────────────────────────────────
    if (method == "reduce") {
        if (arr->elements.empty()) {
            return args.empty() ? TacoValue{nullptr} : args[0];
        }

        TacoValue acc = args.empty() ? arr->elements[0] : args[0];
        int start = args.empty() ? 1 : 0;

        for (int i = start; i < static_cast<int>(arr->elements.size()); i++) {
            acc = call_closure(eval, closure, {acc, arr->elements[i]});
        }
        return acc;
    }

    // ── find ────────────────────────────────────────────
    if (method == "find") {
        auto it = std::find_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                return is_truthy(call_closure(eval, closure, {elem}));
            }
        );
        return it != arr->elements.end() ? *it : TacoValue{nullptr};
    }

    // ── sortBy ──────────────────────────────────────────
    if (method == "sortBy") {
        auto result = std::make_shared<TacoArray>(*arr);  // 拷贝
        std::sort(
            result->elements.begin(), result->elements.end(),
            [&](const TacoValue& a, const TacoValue& b) {
                TacoValue ka = call_closure(eval, closure, {a});
                TacoValue kb = call_closure(eval, closure, {b});
                if (std::holds_alternative<double>(ka) &&
                    std::holds_alternative<double>(kb)) {
                    return std::get<double>(ka) < std::get<double>(kb);
                }
                if (std::holds_alternative<std::string>(ka) &&
                    std::holds_alternative<std::string>(kb)) {
                    return std::get<std::string>(ka) < std::get<std::string>(kb);
                }
                return false;
            }
        );
        return result;
    }

    // ── push ────────────────────────────────────────────
    if (method == "push") {
        if (args.empty()) throw std::runtime_error("🌮 push() needs an argument.");
        arr->elements.push_back(args[0]);
        return nullptr;
    }

    // ── pop ─────────────────────────────────────────────
    if (method == "pop") {
        if (arr->elements.empty()) throw std::runtime_error("🌮 pop() on empty array.");
        TacoValue last = arr->elements.back();
        arr->elements.pop_back();
        return last;
    }

    // ── len ─────────────────────────────────────────────
    if (method == "len") {
        return static_cast<double>(arr->elements.size());
    }

    // ── contains ────────────────────────────────────────
    if (method == "contains") {
        if (args.empty()) throw std::runtime_error("🌮 contains() needs an argument.");
        auto it = std::find_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) { return values_equal(elem, args[0]); }
        );
        return it != arr->elements.end();
    }

    // ── sum ─────────────────────────────────────────────
    if (method == "sum") {
        double total = std::accumulate(
            arr->elements.begin(), arr->elements.end(), 0.0,
            [](double acc, const TacoValue& v) {
                if (std::holds_alternative<double>(v))
                    return acc + std::get<double>(v);
                return acc;
            }
        );
        return total;
    }

    // ── avg ─────────────────────────────────────────────
    if (method == "avg") {
        if (arr->elements.empty()) return 0.0;
        TacoValue sum_val = dispatch_array_method(arr, "sum", {}, nullptr, eval);
        return std::get<double>(sum_val) / arr->elements.size();
    }

    // ── min / max ────────────────────────────────────────
    if (method == "min" || method == "max") {
        if (arr->elements.empty())
            throw std::runtime_error("🌮 min()/max() on empty array.");

        auto cmp = [](const TacoValue& a, const TacoValue& b) {
            if (std::holds_alternative<double>(a) &&
                std::holds_alternative<double>(b)) {
                return std::get<double>(a) < std::get<double>(b);
            }
            return false;
        };

        auto it = (method == "min")
            ? std::min_element(arr->elements.begin(), arr->elements.end(), cmp)
            : std::max_element(arr->elements.begin(), arr->elements.end(), cmp);

        return *it;
    }

    // ── getFirst / getLast ───────────────────────────────
    if (method == "getFirst") {
        if (arr->elements.empty()) return nullptr;
        if (args.empty()) return arr->elements.front();

        int n = static_cast<int>(std::get<double>(args[0]));
        int count = std::min(n, static_cast<int>(arr->elements.size()));
        auto result = std::make_shared<TacoArray>();
        result->elements.assign(arr->elements.begin(),
                                arr->elements.begin() + count);
        return result;
    }

    if (method == "getLast") {
        if (arr->elements.empty()) return nullptr;
        if (args.empty()) return arr->elements.back();

        int n = static_cast<int>(std::get<double>(args[0]));
        int count = std::min(n, static_cast<int>(arr->elements.size()));
        auto result = std::make_shared<TacoArray>();
        result->elements.assign(arr->elements.end() - count,
                                arr->elements.end());
        return result;
    }

    // ── findFirst / findLast ─────────────────────────────
    if (method == "findFirst") {
        if (arr->elements.empty()) return nullptr;
        return arr->elements.front();
    }

    if (method == "findLast") {
        if (arr->elements.empty()) return nullptr;
        return arr->elements.back();
    }

    // ── groupBy ──────────────────────────────────────────
    if (method == "groupBy") {
        auto result = std::make_shared<TacoMap>();
        for (const auto& elem : arr->elements) {
            TacoValue key_val = call_closure(eval, closure, {elem});
            std::string key = value_to_string(key_val);

            if (!result->has(key)) {
                result->set(key, std::make_shared<TacoArray>());
            }
            auto& group = std::get<std::shared_ptr<TacoArray>>(result->get(key));
            group->elements.push_back(elem);
        }
        return result;
    }

    // ── countBy ──────────────────────────────────────────
    if (method == "countBy") {
        int count = static_cast<int>(std::count_if(
            arr->elements.begin(), arr->elements.end(),
            [&](const TacoValue& elem) {
                return is_truthy(call_closure(eval, closure, {elem}));
            }
        ));
        return static_cast<double>(count);
    }

    throw std::runtime_error(
        "🌮 Array has no method '" + method + "'."
    );
}
```

---

## 25.4 字符串方法的实现

```cpp
// string_methods.cpp

TacoValue dispatch_string_method(
    const std::string& str,
    const std::string& method,
    const std::vector<TacoValue>& args,
    const TacoValue& closure,
    Evaluator& eval)
{
    if (method == "len") {
        return static_cast<double>(str.size());
    }

    if (method == "upper") {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    if (method == "lower") {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    if (method == "contains") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 contains() needs a string argument.");
        return str.find(std::get<std::string>(args[0])) != std::string::npos;
    }

    if (method == "startsWith") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 startsWith() needs a string argument.");
        const auto& prefix = std::get<std::string>(args[0]);
        return str.size() >= prefix.size() &&
               str.substr(0, prefix.size()) == prefix;
    }

    if (method == "endsWith") {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("🌮 endsWith() needs a string argument.");
        const auto& suffix = std::get<std::string>(args[0]);
        return str.size() >= suffix.size() &&
               str.substr(str.size() - suffix.size()) == suffix;
    }

    if (method == "trimSpace") {
        std::string result = str;
        // 去掉头部空白
        result.erase(result.begin(),
                     std::find_if(result.begin(), result.end(),
                                  [](char c) { return !std::isspace(c); }));
        // 去掉尾部空白
        result.erase(std::find_if(result.rbegin(), result.rend(),
                                  [](char c) { return !std::isspace(c); }).base(),
                     result.end());
        return result;
    }

    if (method == "split") {
        std::string delimiter = args.empty() ? " "
            : std::get<std::string>(args[0]);

        auto result = std::make_shared<TacoArray>();
        std::string::size_type start = 0;
        std::string::size_type pos;

        while ((pos = str.find(delimiter, start)) != std::string::npos) {
            result->elements.push_back(str.substr(start, pos - start));
            start = pos + delimiter.size();
        }
        result->elements.push_back(str.substr(start));
        return result;
    }

    if (method == "getLines") {
        return dispatch_string_method(str, "split", {std::string("\n")},
                                     nullptr, eval);
    }

    if (method == "getWords") {
        auto result = std::make_shared<TacoArray>();
        std::istringstream iss(str);
        std::string word;
        while (iss >> word) {
            result->elements.push_back(word);
        }
        return result;
    }

    if (method == "getChars") {
        auto result = std::make_shared<TacoArray>();
        for (char c : str) {
            result->elements.push_back(std::string(1, c));
        }
        return result;
    }

    if (method == "replaceStr") {
        if (args.size() < 2) throw std::runtime_error("🌮 replaceStr() needs 2 arguments.");
        const auto& from = std::get<std::string>(args[0]);
        const auto& to   = std::get<std::string>(args[1]);

        std::string result = str;
        std::string::size_type pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
        return result;
    }

    if (method == "findStr") {
        if (args.empty()) throw std::runtime_error("🌮 findStr() needs an argument.");
        auto pos = str.find(std::get<std::string>(args[0]));
        return pos != std::string::npos
            ? TacoValue{static_cast<double>(pos)}
            : TacoValue{-1.0};
    }

    throw std::runtime_error("🌮 String has no method '" + method + "'.");
}
```

---

## 25.5 内置标准库：文件和系统

```cpp
// builtins.cpp（新增部分）

void register_builtins(Environment::Ptr env) {
    // 之前注册的 print, type, input, number, string...

    // ── 文件系统 ────────────────────────────────────────

    // cat：读取文件内容
    env->define("cat", std::make_shared<TacoNative>(TacoNative{
        "cat",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 cat() needs a path.");
            const auto& path = std::get<std::string>(args[0]);
            std::ifstream f(path);
            if (!f) throw std::runtime_error("🌮 Cannot read: " + path);
            std::ostringstream buf;
            buf << f.rdbuf();
            return buf.str();
        }
    }));

    // echo：写文件
    env->define("echo", std::make_shared<TacoNative>(TacoNative{
        "echo",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 echo() needs content and path.");
            const auto& content = value_to_string(args[0]);
            const auto& path    = std::get<std::string>(args[1]);
            std::ofstream f(path);
            if (!f) throw std::runtime_error("🌮 Cannot write: " + path);
            f << content;
            return nullptr;
        }
    }));

    // ls：列出目录
    env->define("ls", std::make_shared<TacoNative>(TacoNative{
        "ls",
        [](std::vector<TacoValue> args) -> TacoValue {
            namespace fs = std::filesystem;
            std::string path = args.empty() ? "." : std::get<std::string>(args[0]);
            auto result = std::make_shared<TacoArray>();
            for (const auto& entry : fs::directory_iterator(path)) {
                result->elements.push_back(entry.path().filename().string());
            }
            return result;
        }
    }));

    // mkdir：创建目录
    env->define("mkdir", std::make_shared<TacoNative>(TacoNative{
        "mkdir",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 mkdir() needs a path.");
            namespace fs = std::filesystem;
            fs::create_directories(std::get<std::string>(args[0]));
            return nullptr;
        }
    }));

    // rm：删除文件
    env->define("rm", std::make_shared<TacoNative>(TacoNative{
        "rm",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 rm() needs a path.");
            namespace fs = std::filesystem;
            fs::remove(std::get<std::string>(args[0]));
            return nullptr;
        }
    }));

    // mv：移动/重命名
    env->define("mv", std::make_shared<TacoNative>(TacoNative{
        "mv",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 mv() needs src and dst.");
            namespace fs = std::filesystem;
            fs::rename(std::get<std::string>(args[0]),
                       std::get<std::string>(args[1]));
            return nullptr;
        }
    }));

    // cp：复制
    env->define("cp", std::make_shared<TacoNative>(TacoNative{
        "cp",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 cp() needs src and dst.");
            namespace fs = std::filesystem;
            fs::copy(std::get<std::string>(args[0]),
                     std::get<std::string>(args[1]));
            return nullptr;
        }
    }));

    // pwd：当前目录
    env->define("pwd", std::make_shared<TacoNative>(TacoNative{
        "pwd",
        [](std::vector<TacoValue>) -> TacoValue {
            return std::filesystem::current_path().string();
        }
    }));

    // exists：文件是否存在
    env->define("exists", std::make_shared<TacoNative>(TacoNative{
        "exists",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 exists() needs a path.");
            return std::filesystem::exists(std::get<std::string>(args[0]));
        }
    }));

    // ── 系统 ────────────────────────────────────────────

    // exec：执行系统命令，返回输出
    env->define("exec", std::make_shared<TacoNative>(TacoNative{
        "exec",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 exec() needs a command.");
            const auto& cmd = std::get<std::string>(args[0]);

            // 用 popen 捕获命令输出
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) throw std::runtime_error("🌮 exec() failed.");

            std::string result;
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                result += buffer;
            }
            pclose(pipe);
            return result;
        }
    }));

    // env：读取环境变量
    env->define("env", std::make_shared<TacoNative>(TacoNative{
        "env",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty()) throw std::runtime_error("🌮 env() needs a key.");
            const char* val = std::getenv(std::get<std::string>(args[0]).c_str());
            return val ? std::string(val) : TacoValue{nullptr};
        }
    }));

    // ── 网络（基础版，v7 会完善）──────────────────────────

    env->define("fetchUrl", std::make_shared<TacoNative>(TacoNative{
        "fetchUrl",
        [](std::vector<TacoValue> args) -> TacoValue {
            // 占位：v7 会用 cpp-httplib 实现
            throw std::runtime_error("🌮 fetchUrl() is available in v7.");
            return nullptr;
        }
    }));

    // ── random ──────────────────────────────────────────

    static std::mt19937 rng(std::random_device{}());

    env->define("random.pick", std::make_shared<TacoNative>(TacoNative{
        "random.pick",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.empty() || !std::holds_alternative<std::shared_ptr<TacoArray>>(args[0]))
                throw std::runtime_error("🌮 random.pick() needs an array.");
            auto& arr = std::get<std::shared_ptr<TacoArray>>(args[0]);
            if (arr->elements.empty()) return nullptr;
            std::uniform_int_distribution<int> dist(0, arr->elements.size() - 1);
            return arr->elements[dist(rng)];
        }
    }));

    env->define("random.flip", std::make_shared<TacoNative>(TacoNative{
        "random.flip",
        [](std::vector<TacoValue>) -> TacoValue {
            std::bernoulli_distribution dist(0.5);
            return dist(rng);
        }
    }));

    env->define("random.int", std::make_shared<TacoNative>(TacoNative{
        "random.int",
        [](std::vector<TacoValue> args) -> TacoValue {
            if (args.size() < 2) throw std::runtime_error("🌮 random.int() needs min and max.");
            int lo = static_cast<int>(std::get<double>(args[0]));
            int hi = static_cast<int>(std::get<double>(args[1]));
            std::uniform_int_distribution<int> dist(lo, hi);
            return static_cast<double>(dist(rng));
        }
    }));
}
```

---

## 25.6 测试：运行完整的 Taco 脚本

创建 `test_v4.taco`：

```taco
// pipeline 风格处理数据
var scores = [88, 45, 92, 33, 76, 100, 55, 87];

print("通过的分数：");
scores
    .filter { s in s >= 60 }
    .sortBy { s in s }
    .each { s in print(s) };

print("平均分（通过）：");
var passed = scores.filter { s in s >= 60 };
print(passed.avg());

// 字符串处理
var log = "ERROR: something failed\nINFO: started\nERROR: another error\nINFO: done";
print("错误行：");
log.getLines()
   .filter { line in line.startsWith("ERROR") }
   .each { line in print(line) };

// map 操作
var person = {"name": "Miguel", "age": 12, "city": "Oaxaca"};
print(person.getKeys());
print(person.getValues());

// 文件操作
var files = ls(".");
print("当前目录文件数：" + string(files.len()));
files.filter { f in f.endsWith(".taco") }.each { f in print(f) };

// 系统命令
var git_status = exec("git status --short 2>/dev/null");
if (git_status.len() > 0) {
    print("Git 状态：");
    print(git_status);
}

// groupBy 示例
var words = ["apple", "banana", "avocado", "blueberry", "apricot"];
var grouped = words.groupBy { w in w.getChars()[0] };
print(grouped);
```

### 运行结果

```
通过的分数：
76
87
88
92
100
平均分（通过）：
88.6
错误行：
ERROR: something failed
ERROR: another error
当前目录文件数：5
test_v4.taco
test_v3.taco
...
```

---

## 25.7 这个版本的局限性

**`array` 是引用类型，但字面量每次创建新对象**

```taco
var a = [1, 2, 3];
var b = a;       // 共享
var c = [1, 2, 3];  // 新对象，和 a 不共享
```

这是正确的行为，但在 AST 节点里，数组字面量每次求值都创建新的 `TacoArray`，所以 `c` 和 `a` 不共享，符合预期。

**没有展开运算符 `...`**

```taco
var merged = [...a, ...b];  // 还不支持
```

展开运算符需要在语法分析器里特殊处理，v4 暂时不加。

**可选链 `?.` 和默认值 `??` 还没实现**

```taco
var city = user?.address?.city ?? "unknown";  // 还不支持
```

这些是语法糖，可以在解析器里加入，留给读者作为练习。

**没有 OOP（class、struct）**

class 和 struct 的实现需要在值系统里加入对象类型，以及方法查找机制。这在综合收尾章节里处理。

---

## 小结

v4 是 Taco 最接近"完整脚本语言"的一步。

**array 和 map** 用 STL 容器实现，用 `shared_ptr` 包装成引用类型。所有 pipeline 方法（`filter`、`map`、`each`、`reduce`）在方法分派函数里实现，内部用 STL 算法（`std::find_if`、`std::transform`、`std::sort` 等）。

**字符串方法** 用 STL 算法和标准库字符串操作实现，方法名遵循 v+n 的命名规范（`getLines`、`trimSpace`、`findStr` 等）。

**内置标准库** 包含文件系统操作（`cat`、`ls`、`mkdir`、`rm`、`mv`、`cp`、`pwd`、`exists`）、系统操作（`exec`、`env`）、随机数（`random.pick`、`random.flip`、`random.int`）。每个内置函数都是一个 `TacoNative`（用 `std::function` 包装的 lambda）。

**Lambda 在这里的核心作用**：每个 pipeline 方法里，调用 Taco 闭包的操作是通过 C++ lambda 包装后传给 STL 算法的。C++ lambda 捕获了 `eval` 引用，让 Taco 的 `{ x in x * 2 }` 可以在 C++ 的 `std::transform` 里被调用。

---

第五部分到这里结束。第六部分进入模板，学完之后 v5 会用模板重构解释器内部，让值类型和容器更通用、更高效。
