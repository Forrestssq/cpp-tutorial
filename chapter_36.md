# 第三十六章：项目 v7——内置 fetchUrl() 与网络支持

---

v6 结束时，Taco 有了 REPL 和并发支持。v7 加上最后一块：网络。

Taco 脚本能发 HTTP 请求，能调用真实的 API，就从一个本地脚本工具变成了可以和外部世界交互的东西。

v7 完成后，Taco 能跑这样的脚本：

```taco
// 调用 GitHub API
var user = fetchUrl("https://api.github.com/users/torvalds");
print(user);

// 查天气
var weather = fetchUrl("https://wttr.in/Beijing?format=3");
print(weather);

// POST 数据
var response = postData("https://httpbin.org/post",
                        "{'name': 'Miguel', 'age': 12}");
print(response);
```

---

## 36.1 在 Taco 里实现 `fetchUrl()`

### 设计决策

**同步还是异步？**

Taco 脚本是顺序执行的——一行运行完再运行下一行。`fetchUrl()` 应该是一个同步调用：Taco 脚本调用它，等它返回，然后继续。这和 Python 的 `urllib.request.urlopen()` 行为一致。

如果需要并发网络请求（同时发多个），Taco 已经有 `thread`，可以这样写：

```taco
var t1 = thread { var r1 = fetchUrl("https://api1.example.com"); };
var t2 = thread { var r2 = fetchUrl("https://api2.example.com"); };
t1.join();
t2.join();
```

所以 `fetchUrl()` 本身保持同步，并发交给 `thread`。

**返回什么？**

返回原始的响应体字符串。如果是 JSON，Taco 脚本可以用 `parseJson()` 解析（v7 一起实现）。如果 HTTP 请求失败（网络错误、非 200 状态码），Taco 按照它的错误处理哲学：打印清晰的错误信息，然后崩溃（`runtime_error`）。

### 更新 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(taco)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# ... 其他依赖 ...

# cpp-httplib
FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.0
)
FetchContent_MakeAvailable(cpp_httplib)

# nlohmann/json（用于 parseJson）
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(taco PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
)

# HTTPS 支持
find_package(OpenSSL QUIET)
if (OpenSSL_FOUND)
    target_link_libraries(taco PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(taco PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    message(STATUS "HTTPS support enabled (OpenSSL found)")
else()
    message(STATUS "HTTPS support disabled (OpenSSL not found)")
endif()
```

用 `find_package(OpenSSL QUIET)` 而不是 `REQUIRED`——如果没有 OpenSSL，HTTP 请求仍然可以工作，只是不支持 HTTPS。这样在没有 OpenSSL 的机器上也能编译。

---

## 36.2 用 cpp-httplib 发 HTTP 请求

### 核心实现：url_fetch 函数

在 `builtin.cpp` 里实现核心的网络请求函数：

```cpp
// builtin_net.cpp
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <nlohmann/json.hpp>
#include <string>
#include <stdexcept>
#include <tuple>

namespace taco {

// 解析 URL，返回 (scheme, host, path, port)
struct ParsedUrl {
    std::string scheme;  // "http" or "https"
    std::string host;
    std::string path;
    int port;
};

ParsedUrl parse_url(const std::string& url) {
    ParsedUrl result;

    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        throw std::runtime_error("Invalid URL (missing scheme): " + url);
    }

    result.scheme = url.substr(0, scheme_end);
    if (result.scheme != "http" && result.scheme != "https") {
        throw std::runtime_error("Unsupported scheme: " + result.scheme);
    }

    result.port = (result.scheme == "https") ? 443 : 80;

    std::string rest = url.substr(scheme_end + 3);  // 跳过 "://"

    // 提取 host（可能包含端口号）和 path
    size_t slash = rest.find('/');
    std::string host_part = (slash == std::string::npos) ? rest : rest.substr(0, slash);
    result.path = (slash == std::string::npos) ? "/" : rest.substr(slash);

    // 检查是否有显式端口号 host:port
    size_t colon = host_part.find(':');
    if (colon != std::string::npos) {
        result.host = host_part.substr(0, colon);
        result.port = std::stoi(host_part.substr(colon + 1));
    } else {
        result.host = host_part;
    }

    return result;
}

// HTTP GET 请求
// 返回响应 body
// 失败时抛出 std::runtime_error
std::string do_get(const std::string& url,
                   const httplib::Headers& headers = {}) {
    auto parsed = parse_url(url);

    httplib::Result res;

    if (parsed.scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        httplib::SSLClient cli(parsed.host, parsed.port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(60);
        cli.set_follow_location(true);
        // 验证证书（生产中应该开启）
        cli.enable_server_certificate_verification(true);
        res = headers.empty()
            ? cli.Get(parsed.path)
            : cli.Get(parsed.path, headers);
#else
        throw std::runtime_error(
            "HTTPS not supported (compile with OpenSSL)"
        );
#endif
    } else {
        httplib::Client cli(parsed.host, parsed.port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(60);
        cli.set_follow_location(true);
        res = headers.empty()
            ? cli.Get(parsed.path)
            : cli.Get(parsed.path, headers);
    }

    // 检查网络层错误（连接超时、DNS 失败等）
    if (!res) {
        throw std::runtime_error(
            "fetchUrl failed: " + httplib::to_string(res.error())
            + " (url: " + url + ")"
        );
    }

    // 检查 HTTP 状态码
    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error(
            "fetchUrl: HTTP " + std::to_string(res->status)
            + " (url: " + url + ")"
        );
    }

    return res->body;
}

// HTTP POST 请求
std::string do_post(const std::string& url,
                    const std::string& body,
                    const std::string& content_type = "application/json") {
    auto parsed = parse_url(url);

    httplib::Result res;

    if (parsed.scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        httplib::SSLClient cli(parsed.host, parsed.port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(60);
        cli.set_follow_location(true);
        res = cli.Post(parsed.path, body, content_type);
#else
        throw std::runtime_error("HTTPS not supported");
#endif
    } else {
        httplib::Client cli(parsed.host, parsed.port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(60);
        cli.set_follow_location(true);
        res = cli.Post(parsed.path, body, content_type);
    }

    if (!res) {
        throw std::runtime_error(
            "postData failed: " + httplib::to_string(res.error())
        );
    }

    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error(
            "postData: HTTP " + std::to_string(res->status)
        );
    }

    return res->body;
}

}  // namespace taco
```

### 注册为 Taco 内置函数

在 `builtin.cpp` 的 `register_builtins` 函数里加入网络相关的内置函数：

```cpp
// builtin.cpp（节选，加入 v7 网络部分）
#include "builtin_net.cpp"

void register_builtins(Environment& env) {
    // ... 之前的内置函数 ...

    // fetchUrl(url)  -> string
    env.define("fetchUrl", TacoValue(std::make_shared<TacoNative>(
        [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
            if (args.size() != 1 || !args[0].is_string()) {
                throw std::runtime_error(
                    "fetchUrl() expects one string argument (URL)"
                );
            }
            std::string result = taco::do_get(args[0].as_string());
            return TacoValue(result);
        }
    )));

    // postData(url, body)  -> string
    // postData(url, body, contentType)  -> string
    env.define("postData", TacoValue(std::make_shared<TacoNative>(
        [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
            if (args.size() < 2 || !args[0].is_string() || !args[1].is_string()) {
                throw std::runtime_error(
                    "postData() expects (url: string, body: string)"
                );
            }
            std::string url = args[0].as_string();
            std::string body = args[1].as_string();
            std::string content_type = "application/json";
            if (args.size() >= 3 && args[2].is_string()) {
                content_type = args[2].as_string();
            }
            std::string result = taco::do_post(url, body, content_type);
            return TacoValue(result);
        }
    )));

    // fetchWithHeaders(url, headersMap) -> string
    // 支持传请求头（比如 Authorization）
    env.define("fetchWithHeaders", TacoValue(std::make_shared<TacoNative>(
        [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
            if (args.size() != 2 || !args[0].is_string() || !args[1].is_map()) {
                throw std::runtime_error(
                    "fetchWithHeaders() expects (url: string, headers: map)"
                );
            }
            std::string url = args[0].as_string();

            // 把 Taco map 转成 httplib::Headers
            httplib::Headers headers;
            for (const auto& [k, v] : args[1].as_map()) {
                if (v.is_string()) {
                    headers.insert({k, v.as_string()});
                }
            }
            std::string result = taco::do_get(url, headers);
            return TacoValue(result);
        }
    )));
}
```

---

## 36.3 解析 JSON 响应：nlohmann/json

大多数 API 返回 JSON。Taco 需要能把 JSON 字符串解析成 Taco 的 map 和 array。

### nlohmann/json 简介

nlohmann/json 是 C++ 里最流行的 JSON 库，单头文件，语法优雅：

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 解析
json j = json::parse(R"({"name": "Miguel", "age": 12, "scores": [88, 92]})");

// 访问
std::string name = j["name"];  // "Miguel"
int age = j["age"];            // 12
auto scores = j["scores"];     // json array

// 遍历数组
for (auto& s : scores) {
    std::cout << s.get<int>() << "\n";
}

// 遍历对象
for (auto& [key, val] : j.items()) {
    std::cout << key << ": " << val << "\n";
}

// 检查类型
j["name"].is_string();   // true
j["age"].is_number();    // true
j["scores"].is_array();  // true
```

### 把 JSON 转成 TacoValue

需要一个递归函数，把 `nlohmann::json` 对象转成 `TacoValue`：

```cpp
// builtin_net.cpp（续）
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 把 nlohmann::json 转成 TacoValue（递归）
TacoValue json_to_taco(const json& j) {
    if (j.is_null()) {
        return TacoValue();  // nil
    }
    if (j.is_boolean()) {
        return TacoValue(j.get<bool>());
    }
    if (j.is_number_integer()) {
        return TacoValue(static_cast<double>(j.get<int64_t>()));
    }
    if (j.is_number_float()) {
        return TacoValue(j.get<double>());
    }
    if (j.is_string()) {
        return TacoValue(j.get<std::string>());
    }
    if (j.is_array()) {
        // JSON array -> TacoArray
        auto arr = std::make_shared<TacoArray>();
        for (const auto& elem : j) {
            arr->push_back(json_to_taco(elem));
        }
        return TacoValue(arr);
    }
    if (j.is_object()) {
        // JSON object -> TacoMap
        auto map = std::make_shared<TacoMap>();
        for (const auto& [key, val] : j.items()) {
            (*map)[key] = json_to_taco(val);
        }
        return TacoValue(map);
    }
    // 其他类型（不应该出现）
    return TacoValue(j.dump());  // 转成字符串兜底
}

// 把 TacoValue 转成 JSON 字符串
json taco_to_json(const TacoValue& v) {
    if (v.is_nil()) return nullptr;
    if (v.is_bool()) return v.as_bool();
    if (v.is_number()) return v.as_number();
    if (v.is_string()) return v.get_string();
    if (v.is_array()) {
        json arr = json::array();
        for (const auto& elem : v.as_array_ref()) {
            arr.push_back(taco_to_json(elem));
        }
        return arr;
    }
    if (v.is_map()) {
        json obj = json::object();
        for (const auto& [k, val] : v.as_map()) {
            obj[k] = taco_to_json(val);
        }
        return obj;
    }
    return v.to_string();  // 其他类型转字符串
}
```

### 注册 parseJson 和 toJson

```cpp
// 注册 parseJson 和 toJson

// parseJson(str) -> map/array/any
env.define("parseJson", TacoValue(std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
        if (args.size() != 1 || !args[0].is_string()) {
            throw std::runtime_error(
                "parseJson() expects one string argument"
            );
        }
        try {
            json j = json::parse(args[0].as_string());
            return json_to_taco(j);
        } catch (const json::parse_error& e) {
            throw std::runtime_error(
                std::string("parseJson: invalid JSON: ") + e.what()
            );
        }
    }
)));

// toJson(value) -> string
env.define("toJson", TacoValue(std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
        if (args.size() != 1) {
            throw std::runtime_error("toJson() expects one argument");
        }
        json j = taco_to_json(args[0]);
        return TacoValue(j.dump(2));  // 2 个空格缩进的 pretty-print
    }
)));

// toJsonCompact(value) -> string（不带缩进）
env.define("toJsonCompact", TacoValue(std::make_shared<TacoNative>(
    [](std::vector<TacoValue> args, Evaluator&) -> TacoValue {
        if (args.size() != 1) {
            throw std::runtime_error("toJsonCompact() expects one argument");
        }
        json j = taco_to_json(args[0]);
        return TacoValue(j.dump());
    }
)));
```

---

## 36.4 实现 `postData()`

`postData()` 在 36.2 里已经实现了。这里看一下它在 Taco 脚本里的完整用法：

```taco
// 发送 JSON POST 请求
var payload = {"name": "Miguel", "age": 12};
var json_str = toJson(payload);

var response_str = postData("https://httpbin.org/post", json_str);
var response = parseJson(response_str);

// httpbin.org/post 会把你发的数据原样返回
print(response["json"]["name"]);  // Miguel
print(response["json"]["age"]);   // 12
```

### 处理不同的 Content-Type

```taco
// 发 form 数据
var form_data = "username=Miguel&password=secret";
var resp = postData("https://httpbin.org/post",
                    form_data,
                    "application/x-www-form-urlencoded");
print(resp);
```

```cpp
// C++ 实现里，content_type 参数直接传给 httplib
res = cli.Post(parsed.path, body, content_type);
```

---

## 36.5 测试：用 Taco 脚本调用真实 API

### 测试 1：GitHub 用户信息

```taco
// github_user.taco
// 获取 GitHub 用户信息

func printUser(username) {
    var url = "https://api.github.com/users/{username}";
    var raw = fetchUrl(url);
    var user = parseJson(raw);

    print("=== GitHub User: {username} ===");
    print("Name:      " + (user["name"] ?? "N/A"));
    print("Company:   " + (user["company"] ?? "N/A"));
    print("Location:  " + (user["location"] ?? "N/A"));
    print("Bio:       " + (user["bio"] ?? "N/A"));
    print("Repos:     " + string(user["public_repos"]));
    print("Followers: " + string(user["followers"]));
    print("Following: " + string(user["following"]));
    print("Created:   " + user["created_at"]);
}

printUser("torvalds");
```

运行：

```
=== GitHub User: torvalds ===
Name:      Linus Torvalds
Company:   Linux Foundation
Location:  Portland, OR
Bio:       N/A
Repos:     8
Followers: 245000+
Following: 0
Created:   2011-09-04T19:48:04Z
```

### 测试 2：天气查询

```taco
// weather.taco
// wttr.in 提供简单的天气 API

var cities = ["Beijing", "Tokyo", "New York", "London", "Mexico City"];

cities.each { city in
    var url = "https://wttr.in/{city}?format=3";
    var weather = fetchUrl(url);
    print(weather.trimSpace());
};
```

运行：

```
Beijing: ⛅️  +28°C
Tokyo: 🌧  +22°C
New York: ☀️  +31°C
London: 🌦  +18°C
Mexico City: ⛅️  +22°C
```

### 测试 3：并发请求（用 thread）

```taco
// concurrent_fetch.taco
// 同时发多个请求，用 channel 收集结果

var ch = channel();
var urls = [
    "https://api.github.com/users/torvalds",
    "https://api.github.com/users/gvanrossum",
    "https://api.github.com/users/antirez"
];

// 为每个 URL 启动一个线程
var threads = urls.map { url in
    thread {
        var raw = fetchUrl(url);
        var user = parseJson(raw);
        ch.send(user["login"] + ": " + string(user["followers"]) + " followers");
    }
};

// 等待所有线程完成，收集结果
var results = [];
threads.each { t in
    var result = ch.receive();
    results.push(result);
    t.join();
};

results.sortBy { r in r }.each { r in print(r) };
```

运行：

```
antirez: 23000+ followers
gvanrossum: 28000+ followers
torvalds: 245000+ followers
```

注意：这里 `fetchUrl` 是同步的，但多个线程各自调用自己的 `fetchUrl`，所以三个请求实际上是并发发出的（每个线程阻塞在自己的 HTTP 请求上，互不影响）。

### 测试 4：Magic 8 Ball 🌮 问一个网络问题

```taco
// v7 的彩蛋整合
var answer = 🌮;

var url = "https://httpbin.org/get?question=" + answer;
var resp = fetchUrl(url);
var data = parseJson(resp);

print("The ball says: " + answer);
print("Server echoed: " + data["args"]["question"]);
```

运行：

```
The ball says: 🎱 It is certain.
Server echoed: 🎱 It is certain.
```

Magic 8 Ball 的答案可以穿越网络。

### 测试 5：一个实用脚本——查 IP 归属地

```taco
// ipinfo.taco
// 查询当前机器的公网 IP 和归属地

var ip_info = parseJson(fetchUrl("https://ipinfo.io/json"));

print("IP:       " + ip_info["ip"]);
print("City:     " + ip_info["city"]);
print("Region:   " + ip_info["region"]);
print("Country:  " + ip_info["country"]);
print("Org:      " + ip_info["org"]);
print("Timezone: " + ip_info["timezone"]);
```

运行：

```
IP:       203.xxx.xxx.xxx
City:     Singapore
Region:   Central Singapore
Country:  SG
Org:      AS9876 SingNet Pte Ltd
Timezone: Asia/Singapore
```

---

## 36.6 错误处理：网络请求失败怎么办

网络请求失败的情况有很多：DNS 解析失败、连接超时、服务器返回 4xx/5xx……

Taco 的哲学是**直接崩溃，打印清晰的错误信息**。在 C++ 实现里，`do_get` 和 `do_post` 在失败时抛出 `std::runtime_error`，求值器（evaluator）捕获它，打印 Taco 风格的错误信息：

```
🌮 line 3: fetchUrl failed: Connection refused
   var data = fetchUrl("http://localhost:9999/api");
              ^^^^^^^^^
```

在 Taco 脚本里，目前没有 try/catch 机制（Taco 不设计异常处理，出错就崩）。如果想要更优雅的错误处理，可以用 `fetchUrl` 的返回值约定一个特殊值（比如空字符串表示失败），但这会让 API 变得不一致。

更好的方案是用 `Result` enum（Taco 有关联值枚举）：

```taco
// 未来版本的想法（当前 v7 不实现）
var result = tryFetch("https://api.example.com");
switch result {
    case Result.Ok(data) { print(data); }
    case Result.Err(msg) { print("Failed: " + msg); }
}
```

这留给读者作为练习：在 C++ 层，让 `fetchUrl` 返回 `TacoResult` 而不是直接抛出异常。

---

## 36.7 v7 完整架构

v7 结束，Taco 的完整组件图：

```
taco/
  src/
    main.cpp              # 程序入口（脚本模式 / REPL 模式）
    token.h / token.cpp   # Token 定义和工具函数
    lexer.h / lexer.cpp   # 词法分析器（v0）
    ast.h                 # AST 节点定义（v1）
    parser.h / parser.cpp # 递归下降解析器（v1）
    value.h / value.cpp   # TacoValue（所有运行时类型）（v3 → v5 variant 重构）
    environment.h         # 作用域链（v3）
    evaluator.h           # 求值器主体
    evaluator.cpp         # 所有 AST 节点的 evaluate() 实现
    builtin.cpp           # 内置函数注册（v3 开始）
    builtin_io.cpp        # 文件、系统内置（v4）
    builtin_net.cpp       # 网络内置：fetchUrl、postData、parseJson（v7）
    repl.cpp              # REPL 实现（v6）
    error.h               # 错误报告
```

每一层的演化轨迹：

| 文件 | 引入版本 | 主要演化 |
|------|----------|----------|
| token.h | v0 | 稳定 |
| lexer.cpp | v0 | v1 加字符串插值 token |
| ast.h | v1 | v2 加控制流节点；v3 加函数/闭包节点 |
| parser.cpp | v1 | 每版加新语法 |
| value.h | v1（简单） | v3 加函数；v5 用 variant 重构 |
| evaluator.cpp | v1 | 每版加新 evaluate 逻辑 |
| builtin.cpp | v3 | v4 加 array/map 方法；v7 加网络 |
| repl.cpp | v6 | 新增 |
| builtin_net.cpp | v7 | 新增 |

---

## 小结

v7 是 Taco 的最后一次进化，也是最简单的一次——因为网络库帮我们做了所有繁琐的事情。

**`fetchUrl()` 和 `postData()`** 内部调用 `do_get` 和 `do_post`，用 cpp-httplib 发 HTTP/HTTPS 请求，返回响应 body 字符串。参数校验在 C++ lambda 里完成，失败时抛出 `std::runtime_error`，由求值器统一处理成 Taco 风格的错误输出。

**`parseJson()` 和 `toJson()`** 用 nlohmann/json 实现。`json_to_taco` 递归地把 JSON 对象转成 TacoValue（null → nil，boolean → bool，number → double，string → string，array → TacoArray，object → TacoMap），`taco_to_json` 做反向转换。

**并发网络请求**：`fetchUrl` 本身是同步的，并发交给 Taco 的 `thread`。每个线程有自己的 httplib Client 实例，不存在共享状态的问题，天然线程安全。

**URL 解析**：手写了一个简单的 `parse_url`，覆盖 http 和 https，支持显式端口号（`host:port`）。生产中应该用更健壮的 URL 解析库（比如 Ada URL parser），这里保持简单。

---

第八部分到这里结束。Taco 从一个只能切 Token 的 v0，进化成了一个能运行脚本、有 REPL、支持并发、能发网络请求的 v7。第九部分是综合收尾：回顾整个 Taco 项目，讲讲工程实践，展望接下来的路。
