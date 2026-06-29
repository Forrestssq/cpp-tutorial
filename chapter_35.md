# 第三十五章：现代 C++ 网络库

---

上一章看了原生 socket API，知道了底层是什么样的。这一章用两个库把网络编程变得简单：

- **Asio**（独立版，不依赖 Boost）：现代 C++ 风格，基于异步 I/O，功能完整，性能强
- **cpp-httplib**：单头文件，同步风格，专注 HTTP，极其简单

这两个库服务于不同的场景。弄清楚各自的定位，才能在项目里做出正确选择。

---

## 35.1 Asio（独立版）：现代 C++ 风格的网络库

Asio（Asynchronous I/O）最初是 Boost 的一部分（`boost::asio`），现在有独立版本（`asio`，不依赖 Boost）。它是 C++ 网络编程的工业级解决方案，C++ 网络标准库提案（Networking TS）很大程度上就是以 Asio 为基础的。

### Asio 的核心概念

Asio 的设计围绕几个核心概念：

**io_context（I/O 上下文）**

`asio::io_context` 是 Asio 的引擎。所有异步操作都注册在它上面，调用 `io_context.run()` 开始事件循环，驱动所有异步操作执行：

```cpp
asio::io_context io;
// 注册异步操作...
io.run();  // 运行事件循环，直到所有操作完成
```

可以把它想成一个任务队列：所有"等网络"的工作都丢给它，它负责调度，有结果了调用你的回调。

**executor**

executor 决定回调在哪里执行（哪个线程）。默认情况下，所有回调都在调用 `io.run()` 的线程执行。如果多个线程都调用 `io.run()`，Asio 会自动把任务分配到空闲线程。

**completion handler（完成处理程序）**

异步操作完成时调用的函数。可以是普通函数、lambda、`std::bind` 的绑定。从 C++20 开始，也可以是协程（coroutine）。

### 引入 Asio

在 CMakeLists.txt 里通过 FetchContent 引入独立版 Asio：

```cmake
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Asio 是 header-only，只需要 include 路径
target_include_directories(taco PRIVATE ${asio_SOURCE_DIR}/asio/include)

# 独立版 Asio 需要定义这个宏，禁用 Boost 依赖
target_compile_definitions(taco PRIVATE ASIO_STANDALONE)

# Linux 上需要链接 pthread
if (UNIX)
    target_link_libraries(taco PRIVATE pthread)
endif()
```

Asio 独立版是 header-only，不需要编译成库，只需要 include。

### Asio 的同步接口

Asio 支持同步和异步两套接口。先看同步的，比较直观：

```cpp
#include <asio.hpp>
#include <iostream>

int main() {
    asio::io_context io;

    // 创建 TCP socket
    asio::ip::tcp::socket sock(io);

    // 解析域名（DNS 查询）
    asio::ip::tcp::resolver resolver(io);
    // resolve 返回端点列表（一个域名可能对应多个 IP）
    auto endpoints = resolver.resolve("example.com", "80");

    // 连接（尝试端点列表中的每一个，直到成功）
    asio::connect(sock, endpoints);

    // 发送 HTTP GET 请求
    std::string request =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";
    asio::write(sock, asio::buffer(request));

    // 读取响应（直到连接关闭）
    asio::streambuf response;
    asio::error_code ec;
    asio::read(sock, response, asio::transfer_all(), ec);
    // ec == asio::error::eof 表示服务器关闭了连接，这是正常的

    std::cout << &response;
    return 0;
}
```

对比原生 socket API，改进是显著的：

- `resolver.resolve("example.com", "80")` 替代了手动的 `getaddrinfo` + 结构体填充
- `asio::connect` 替代了手动遍历地址列表 + `::connect`
- `asio::write` 和 `asio::read` 处理了部分写/部分读的问题（原生 `send`/`recv` 可能不一次写完/读完）
- RAII：`sock` 离开作用域自动关闭，不需要手动 `close`

### Asio 的异步接口

异步接口是 Asio 的精髓。核心函数都有对应的 `async_` 版本：

```cpp
#include <asio.hpp>
#include <iostream>
#include <memory>

// 把客户端状态封装在一个共享对象里
struct Session : std::enable_shared_from_this<Session> {
    asio::ip::tcp::socket sock;
    asio::streambuf buf;

    Session(asio::io_context& io) : sock(io) {}

    void start(asio::ip::tcp::resolver::results_type endpoints) {
        // 异步连接：连接完成后调用 on_connect
        asio::async_connect(
            sock,
            endpoints,
            [self = shared_from_this()](
                asio::error_code ec,
                asio::ip::tcp::endpoint
            ) {
                if (!ec) self->on_connect();
            }
        );
    }

    void on_connect() {
        // 连接成功，异步发送请求
        static const std::string request =
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n"
            "\r\n";

        asio::async_write(
            sock,
            asio::buffer(request),
            [self = shared_from_this()](asio::error_code ec, size_t) {
                if (!ec) self->on_write();
            }
        );
    }

    void on_write() {
        // 请求发出，异步读取响应
        asio::async_read(
            sock,
            buf,
            asio::transfer_all(),
            [self = shared_from_this()](asio::error_code ec, size_t) {
                // EOF 是正常结束
                if (!ec || ec == asio::error::eof) {
                    std::cout << &self->buf;
                }
            }
        );
    }
};

int main() {
    asio::io_context io;

    asio::ip::tcp::resolver resolver(io);
    auto endpoints = resolver.resolve("example.com", "80");

    auto session = std::make_shared<Session>(io);
    session->start(endpoints);

    io.run();  // 运行事件循环，直到所有回调执行完
    return 0;
}
```

这段代码比同步版本长很多，但它的关键优势是：`io.run()` 运行期间，线程不会阻塞在任何一个 I/O 操作上——它在等网络的时候，可以同时处理其他的异步操作。如果有 1000 个并发请求，同步版本需要 1000 个线程，异步版本只需要几个线程（甚至一个）。

注意 `shared_from_this`：异步回调会在将来某个时刻执行，届时 `Session` 对象必须还活着。用 `shared_ptr` + `weak_from_this`/`shared_from_this` 来保证对象在回调执行完之前不被销毁，这是异步代码里管理对象生命周期的常用模式。

### C++20 协程：让异步代码看起来像同步代码

Asio 从 1.18 版本开始支持 C++20 协程（coroutine）。协程让异步代码的写法和同步代码一样直观：

```cpp
// 需要 C++20 和支持协程的 Asio 版本
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>

asio::awaitable<void> fetch(asio::io_context& io) {
    asio::ip::tcp::resolver resolver(io);
    auto endpoints = co_await resolver.async_resolve(
        "example.com", "80", asio::use_awaitable
    );

    asio::ip::tcp::socket sock(io);
    co_await asio::async_connect(sock, endpoints, asio::use_awaitable);

    std::string request = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    co_await asio::async_write(sock, asio::buffer(request), asio::use_awaitable);

    asio::streambuf buf;
    asio::error_code ec;
    co_await asio::async_read(sock, buf, asio::transfer_all(), asio::use_awaitable);

    std::cout << &buf;
}

int main() {
    asio::io_context io;
    asio::co_spawn(io, fetch(io), asio::detached);
    io.run();
}
```

`co_await` 暂停协程，把控制权还给事件循环；I/O 完成后，协程从暂停处恢复继续执行。代码结构和同步代码一模一样，但底层是完全异步的。这是现代 C++ 异步编程的方向。

---

## 35.2 用 Asio 写一个 TCP 客户端

把上面的知识整合成一个完整的、带错误处理的同步 TCP 客户端：

```cpp
// tcp_client.cpp
#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <string>
#include <stdexcept>

// 同步 TCP 客户端：连接到服务器，发送数据，接收响应
std::string tcp_request(const std::string& host,
                         const std::string& port,
                         const std::string& data) {
    asio::io_context io;

    // 解析主机名（可能触发 DNS 查询）
    asio::ip::tcp::resolver resolver(io);
    asio::error_code ec;
    auto endpoints = resolver.resolve(host, port, ec);
    if (ec) {
        throw std::runtime_error("DNS resolve failed: " + ec.message());
    }

    // 创建 socket 并连接
    asio::ip::tcp::socket sock(io);
    asio::connect(sock, endpoints, ec);
    if (ec) {
        throw std::runtime_error("Connect failed: " + ec.message());
    }

    // 发送数据
    asio::write(sock, asio::buffer(data), ec);
    if (ec) {
        throw std::runtime_error("Write failed: " + ec.message());
    }

    // 关闭发送方向（告诉服务器"我发完了"）
    sock.shutdown(asio::ip::tcp::socket::shutdown_send, ec);

    // 读取响应（直到 EOF）
    asio::streambuf response;
    asio::read(sock, response, asio::transfer_all(), ec);
    // 服务器关闭连接会触发 EOF，这是正常的
    if (ec && ec != asio::error::eof) {
        throw std::runtime_error("Read failed: " + ec.message());
    }

    // 把 streambuf 转成 string
    return std::string(
        std::istreambuf_iterator<char>(&response),
        std::istreambuf_iterator<char>()
    );
}

int main() {
    try {
        // 发一个原始的 HTTP/1.0 请求（HTTP/1.0 默认 Connection: close，简单）
        std::string request =
            "GET / HTTP/1.0\r\n"
            "Host: example.com\r\n"
            "\r\n";

        std::string response = tcp_request("example.com", "80", request);
        std::cout << response.substr(0, 500) << "...\n";  // 只打印前 500 字节
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
```

这个封装已经相当实用。但如果要处理真实的 HTTP：状态码、响应头、chunked 编码、HTTPS……还是很繁琐。这就是 cpp-httplib 的用武之地。

---

## 35.3 cpp-httplib：几行代码发一个 HTTP 请求

`cpp-httplib` 是一个单头文件的 C++ HTTP 库，支持 HTTP/1.1 和 HTTPS（需要 OpenSSL）。它的设计哲学是**极简**：一个头文件，几行代码，搞定 HTTP。

GitHub：https://github.com/yhirose/cpp-httplib

### 引入 cpp-httplib

方式一：直接下载 `httplib.h` 放进项目。

方式二：CMake FetchContent：

```cmake
include(FetchContent)

FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.0
)
FetchContent_MakeAvailable(cpp_httplib)

target_link_libraries(taco PRIVATE httplib::httplib)
```

如果需要 HTTPS 支持，还要链接 OpenSSL：

```cmake
find_package(OpenSSL REQUIRED)
target_link_libraries(taco PRIVATE httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(taco PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
```

### 发 GET 请求

```cpp
#define CPPHTTPLIB_OPENSSL_SUPPORT  // 启用 HTTPS
#include "httplib.h"
#include <iostream>

int main() {
    // HTTPS 客户端
    httplib::SSLClient cli("api.github.com");
    cli.set_connection_timeout(10);  // 10 秒超时

    // 发 GET 请求
    auto res = cli.Get("/users/torvalds");

    if (!res) {
        // 网络错误
        std::cerr << "Error: " << httplib::to_string(res.error()) << "\n";
        return 1;
    }

    if (res->status == 200) {
        std::cout << "Status: " << res->status << "\n";
        std::cout << "Body: " << res->body << "\n";
    } else {
        std::cerr << "HTTP error: " << res->status << "\n";
    }
    return 0;
}
```

`res` 是一个 `std::optional<httplib::Response>`，用 `!res` 检查网络错误，用 `res->status` 拿状态码，用 `res->body` 拿响应体。就这么简单。

### 发 POST 请求

```cpp
httplib::SSLClient cli("httpbin.org");

// POST JSON 数据
std::string body = R"({"name": "Miguel", "age": 12})";

auto res = cli.Post(
    "/post",                        // 路径
    body,                           // 请求体
    "application/json"              // Content-Type
);

if (res && res->status == 200) {
    std::cout << res->body << "\n";
}
```

### 设置请求头

```cpp
httplib::Headers headers = {
    {"Authorization", "Bearer your_token_here"},
    {"User-Agent", "Taco/0.1"}
};

auto res = cli.Get("/api/data", headers);
```

### 完整的错误处理

```cpp
#include "httplib.h"
#include <iostream>
#include <stdexcept>

std::string fetch_url(const std::string& url) {
    // 解析 URL：分离主机名和路径
    // 简化：假设 URL 格式为 https://host/path
    std::string host, path;
    if (url.substr(0, 8) == "https://") {
        auto rest = url.substr(8);
        auto slash = rest.find('/');
        if (slash == std::string::npos) {
            host = rest;
            path = "/";
        } else {
            host = rest.substr(0, slash);
            path = rest.substr(slash);
        }
    } else if (url.substr(0, 7) == "http://") {
        auto rest = url.substr(7);
        auto slash = rest.find('/');
        host = (slash == std::string::npos) ? rest : rest.substr(0, slash);
        path = (slash == std::string::npos) ? "/" : rest.substr(slash);
    } else {
        throw std::runtime_error("Unsupported URL scheme: " + url);
    }

    // 根据协议选择客户端
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    bool is_https = (url.substr(0, 8) == "https://");
#else
    bool is_https = false;
#endif

    httplib::Result res;
    if (is_https) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        httplib::SSLClient cli(host);
        cli.set_connection_timeout(30);
        cli.set_follow_location(true);  // 跟随重定向
        res = cli.Get(path);
#endif
    } else {
        httplib::Client cli(host);
        cli.set_connection_timeout(30);
        cli.set_follow_location(true);
        res = cli.Get(path);
    }

    if (!res) {
        throw std::runtime_error(
            "Network error: " + httplib::to_string(res.error())
        );
    }

    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error(
            "HTTP error: " + std::to_string(res->status)
        );
    }

    return res->body;
}
```

这个函数就是 v7 里 `fetchUrl()` 的 C++ 核心实现。

### 写一个简单的 HTTP 服务器

cpp-httplib 也支持服务器端，语法同样简洁：

```cpp
#include "httplib.h"
#include <iostream>

int main() {
    httplib::Server svr;

    // GET /hello
    svr.Get("/hello", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("Hello, World!", "text/plain");
    });

    // GET /greet?name=Miguel
    svr.Get("/greet", [](const httplib::Request& req, httplib::Response& res) {
        auto name = req.get_param_value("name");
        if (name.empty()) name = "World";
        res.set_content("Hello, " + name + "!", "text/plain");
    });

    // POST /echo
    svr.Post("/echo", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(req.body, "text/plain");
    });

    std::cout << "Server running on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
```

用 curl 测试：

```bash
curl http://localhost:8080/hello
# Hello, World!

curl "http://localhost:8080/greet?name=Miguel"
# Hello, Miguel!

curl -X POST -d "some data" http://localhost:8080/echo
# some data
```

Taco 的 v7 只需要客户端功能，但服务器功能这里也展示一下，以备将来使用。

---

## 35.4 网络生态概览：什么场景用什么库

C++ 的网络生态比较分散，不同场景有不同的选择：

### 低层次、高性能、异步：Asio / Boost.Asio

适合：需要精细控制 I/O 模型、高并发服务器、自定义协议、长连接。

学习曲线陡，但功能最强，性能最好。工业界广泛使用（Asio 本身是很多高级框架的基础层）。

```cpp
// Asio 核心：io_context + 异步操作 + 回调/协程
asio::io_context io;
asio::ip::tcp::acceptor acceptor(io, {asio::ip::tcp::v4(), 8080});
// async_accept, async_read, async_write...
```

### HTTP 客户端/服务器，快速开发：cpp-httplib

适合：需要做 HTTP 请求、写简单的 REST API 服务器、脚本工具、原型开发。

单头文件，零依赖（HTTPS 需要 OpenSSL），API 极简。性能够用，不适合高并发场景（内部每个连接一个线程）。

```cpp
httplib::Client cli("api.example.com");
auto res = cli.Get("/data");
```

### gRPC：微服务 RPC

适合：服务间通信、需要强类型接口定义、跨语言 RPC。

基于 Protobuf，性能强，但配置和学习成本高。

### libcurl：成熟的 HTTP 客户端（C 库，有 C++ 封装）

适合：需要支持更多协议（FTP、SFTP、SMTP…）、需要经过时间检验的实现。

C 接口有点繁琐，但有 curlpp 等 C++ 封装。

### cpr（C++ Requests）：C++ 版的 Python requests

适合：喜欢 Python requests 风格、快速写 HTTP 请求。

底层用 libcurl，API 更现代：

```cpp
#include <cpr/cpr.h>

auto r = cpr::Get(cpr::Url{"https://api.github.com/users/torvalds"},
                  cpr::Header{{"User-Agent", "Taco/0.1"}});
std::cout << r.text;  // 响应 body
```

### 选择建议

| 场景 | 推荐 |
|------|------|
| Taco 的 `fetchUrl()`（HTTP 客户端） | cpp-httplib |
| 高并发 TCP 服务器 | Asio |
| 微服务 RPC | gRPC |
| 需要更多协议支持 | libcurl / cpr |
| 快速原型，Python requests 风格 | cpr |

对于 Taco 的 v7，选择 **cpp-httplib**：零配置（单头文件），API 简单，直接对应 `fetchUrl()` 的同步语义，HTTPS 支持好。

---

### 关于 HTTPS 和 SSL/TLS

现代 API 几乎都强制 HTTPS。HTTPS = HTTP + TLS（Transport Layer Security）。TLS 在 TCP 之上加了一层加密和身份验证。

要用 cpp-httplib 发 HTTPS 请求，需要：

1. 安装 OpenSSL（大多数系统自带，或用包管理器安装）
2. 编译时定义 `CPPHTTPLIB_OPENSSL_SUPPORT`
3. 链接 `OpenSSL::SSL` 和 `OpenSSL::Crypto`

在 CMakeLists.txt 里：

```cmake
find_package(OpenSSL REQUIRED)
target_link_libraries(taco PRIVATE httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(taco PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
```

如果没有 OpenSSL，cpp-httplib 仍然可以用，只是只支持 HTTP，不支持 HTTPS。

---

## 小结

**Asio** 是 C++ 网络编程的工业级库。核心是 `io_context`（事件循环）+ 异步操作（`async_read`、`async_write`、`async_connect`）+ 完成处理程序（回调或 C++20 协程）。同步接口也有，适合简单场景。

**cpp-httplib** 是单头文件的 HTTP 库。`httplib::Client` 做 HTTP，`httplib::SSLClient` 做 HTTPS，`httplib::Server` 实现服务器。API 极简，几行代码完成一次 HTTP 请求。适合 Taco 的 `fetchUrl()` 场景。

**选择依据**：需要高并发、低层控制、自定义协议 → Asio；需要快速做 HTTP 请求或简单 REST 服务器 → cpp-httplib；生产环境微服务 → gRPC；需要多协议支持 → libcurl/cpr。

下一章是第八部分的项目章节：用 cpp-httplib 在 Taco 里实现 `fetchUrl()` 和 `postData()`，完成 v7。
