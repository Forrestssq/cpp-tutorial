# 第三十四章：网络编程基础

---

第七部分结束时，Taco 有了 REPL 和并发支持。第八部分只有三章，目标明确：给 Taco 加上网络能力，让它能发 HTTP 请求、调用 API。

这一章打基础。网络编程横跨操作系统、协议、I/O 模型三个层面，哪个都不简单。把这三个层面弄清楚，后面用库的时候才能知道库在帮你做什么，出了问题才知道往哪里查。

---

## 34.1 TCP/UDP、客户端/服务器模型

### 网络通信的层次

网络通信是分层的。从应用程序的视角看，最常打交道的是两层：

**传输层**：TCP 和 UDP。负责把数据从一台机器送到另一台机器的某个进程。

**应用层**：HTTP、WebSocket、gRPC……这些协议建立在 TCP（偶尔 UDP）之上，定义了数据的格式和交互方式。

写 C++ 网络代码，大多数时候是在和传输层打交道——直接操作 socket——然后在 socket 之上实现或使用应用层协议。

### TCP：可靠的字节流

TCP（Transmission Control Protocol，传输控制协议）是互联网最常用的传输协议。它的核心承诺是：

- **可靠**：数据一定到达，不丢包（丢了会重传）
- **有序**：数据按发送顺序到达
- **字节流**：没有消息边界，是一个连续的字节流

"字节流"这一点经常让新手困惑。TCP 不保证每次 `send` 的数据在对端一次 `recv` 就能收到——你 `send` 了 1000 字节，对端可能一次收到 500 字节，再收到 500 字节。所以应用层协议通常需要自己定义消息边界（比如 HTTP 用空行和 Content-Length）。

### UDP：不可靠的数据报

UDP（User Datagram Protocol）是另一种传输协议。它的特点：

- **不可靠**：包可能丢失，丢了不重传
- **有边界**：每次发送是一个独立的数据报，收到也是独立的
- **速度快**：没有连接建立、重传、拥塞控制的开销

UDP 适合对延迟敏感、能容忍少量丢包的场景：在线游戏、实时视频、DNS 查询。HTTP 用 TCP，所以 Taco 的网络功能只需要关心 TCP。

（HTTP/3 基于 QUIC，而 QUIC 运行在 UDP 上——但这是另一个话题，不在这里展开。）

### 客户端/服务器模型

几乎所有网络应用都是客户端/服务器（client/server）模型：

```
客户端                    服务器
  |                          |
  |----  建立连接  ---------->|
  |                          |
  |----  发送请求  ---------->|
  |                          |
  |<---  返回响应  -----------|
  |                          |
  |----  关闭连接  ---------->|
```

服务器的角色：
- **监听**（listen）：在某个端口等待连接
- **接受**（accept）：接受客户端的连接请求，创建一个新 socket 专门和这个客户端通信
- **处理**：读取请求，计算结果，发送响应

客户端的角色：
- **连接**（connect）：主动发起连接，指定服务器地址和端口
- **发送**（send）：发送请求
- **接收**（recv）：读取响应

Taco 的 `fetchUrl()` 是一个 HTTP 客户端——它主动连接到远程服务器，发送 HTTP 请求，读取响应。不需要实现服务器端。

### 端口号

端口号（port）是一个 16 位整数（0–65535），用来区分同一台机器上的不同服务。常见的端口号：

| 端口 | 服务 |
|------|------|
| 80 | HTTP |
| 443 | HTTPS |
| 22 | SSH |
| 3306 | MySQL |
| 5432 | PostgreSQL |

端口 0–1023 是"知名端口"，需要管理员权限才能使用。写服务器时通常选 1024 以上的端口。

---

## 34.2 用原生 socket API 写一个最简单的例子

在使用任何网络库之前，先看一下原生 socket API 长什么样。这层 API 来自 POSIX（Unix 系统的标准接口），在 Linux 和 macOS 上直接可用，Windows 上有 Winsock（基本相同，但有一些差异）。

理解原生 API 的价值在于：所有网络库（Asio、cpp-httplib 等）最终都是对这套 API 的封装。看到封装层时，你知道底下发生了什么。

### TCP 服务器（最简版）

```cpp
// server.cpp
#include <sys/socket.h>   // socket, bind, listen, accept, send, recv
#include <netinet/in.h>   // sockaddr_in
#include <arpa/inet.h>    // inet_addr, htons
#include <unistd.h>       // close
#include <cstring>        // memset
#include <cstdio>         // printf

int main() {
    // 1. 创建 socket
    // AF_INET：IPv4
    // SOCK_STREAM：TCP（流式）
    // 0：协议自动选择
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // 2. 设置 socket 选项
    // SO_REUSEADDR：允许重用地址，避免"Address already in use"
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 绑定地址和端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
    addr.sin_port = htons(8080);        // htons：主机字节序转网络字节序

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    // 4. 开始监听，最多 5 个排队连接
    listen(server_fd, 5);
    printf("Listening on port 8080...\n");

    // 5. 接受连接（阻塞，直到有客户端连进来）
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        return 1;
    }
    printf("Client connected.\n");

    // 6. 读取数据
    char buf[1024];
    int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        printf("Received: %s\n", buf);
    }

    // 7. 发送响应
    const char* response = "Hello from server!\n";
    send(client_fd, response, strlen(response), 0);

    // 8. 关闭连接
    close(client_fd);
    close(server_fd);
    return 0;
}
```

### TCP 客户端（最简版）

```cpp
// client.cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

int main() {
    // 1. 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // 2. 设置要连接的服务器地址
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    // 连接本机（127.0.0.1）
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // 3. 连接服务器（阻塞，直到连接成功或失败）
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }
    printf("Connected to server.\n");

    // 4. 发送数据
    const char* msg = "Hello from client!";
    send(sock, msg, strlen(msg), 0);

    // 5. 接收响应
    char buf[1024];
    int n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        printf("Server says: %s", buf);
    }

    // 6. 关闭
    close(sock);
    return 0;
}
```

### 运行这个例子

先开一个终端运行服务器，再开另一个运行客户端：

```bash
# 终端 1：编译并运行服务器
g++ -o server server.cpp && ./server
# Listening on port 8080...

# 终端 2：编译并运行客户端
g++ -o client client.cpp && ./client
# Connected to server.
# Server says: Hello from server!

# 终端 1 服务器输出：
# Client connected.
# Received: Hello from client!
```

也可以用 `nc`（netcat）代替客户端快速测试：

```bash
echo "hello" | nc 127.0.0.1 8080
```

### socket API 的核心函数梳理

| 函数 | 作用 |
|------|------|
| `socket(domain, type, protocol)` | 创建 socket，返回文件描述符 |
| `bind(fd, addr, addrlen)` | 绑定地址（服务器用） |
| `listen(fd, backlog)` | 开始监听（服务器用） |
| `accept(fd, addr, addrlen)` | 接受连接（服务器用），阻塞直到有连接 |
| `connect(fd, addr, addrlen)` | 发起连接（客户端用） |
| `send(fd, buf, len, flags)` | 发送数据 |
| `recv(fd, buf, len, flags)` | 接收数据，阻塞直到有数据到达 |
| `close(fd)` | 关闭 socket |

### 字节序

网络通信约定用**大端字节序**（big-endian，也叫网络字节序）。x86/ARM 机器通常是小端字节序（little-endian）。所以端口号和 IP 地址在写入 `sockaddr_in` 之前，需要转换字节序：

```cpp
htons(8080)   // host to network short（16位）
htonl(addr)   // host to network long（32位）
ntohs(port)   // network to host short
ntohl(addr)   // network to host long
```

`inet_pton` 把点分十进制的 IP 字符串（`"192.168.1.1"`）转成网络字节序的整数，顺便处理了字节序问题。

---

### 原生 API 的问题

上面的代码能跑，但有明显的问题：

**每次只能处理一个客户端**。`accept` 阻塞，`recv` 阻塞，处理完一个才能处理下一个。真实的服务器需要同时处理成百上千个连接。

**错误处理繁琐**。每个函数都返回 -1 表示失败，需要查 `errno` 才知道具体错误。

**不跨平台**。Windows 用 Winsock，头文件和部分函数名都不同。

**资源管理手动**。`close(fd)` 必须手动调用，忘了就泄漏文件描述符。

这就是为什么实际开发里几乎不直接用这套 API，而是用封装好的库。下一章介绍两个：Asio 和 cpp-httplib。

---

## 34.3 同步 I/O vs 异步 I/O 的概念

理解 I/O 模型是网络编程的关键。`recv` 调用会**阻塞**线程，直到有数据到达——在这段等待时间里，线程什么都做不了。这是**同步阻塞 I/O**。

### 几种 I/O 模型

**同步阻塞 I/O（Synchronous Blocking I/O）**

```
线程:  [  发起 recv  ] [........等待........] [  处理数据  ]
时间:  ─────────────────────────────────────────────────────>
数据:                                         [数据到达]
```

最简单，代码直接。问题是线程在等待时被占用，不能做其他事。要同时处理多个连接，就需要多个线程——每个连接一个线程。

**同步非阻塞 I/O（Synchronous Non-Blocking I/O）**

把 socket 设为非阻塞模式，`recv` 立刻返回：如果没有数据，返回 `EAGAIN`；如果有数据，返回数据。程序需要不断地轮询（poll）每个 socket：

```cpp
// 把 socket 设为非阻塞
fcntl(sock, F_SETFL, O_NONBLOCK);

// 轮询
while (true) {
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n < 0 && errno == EAGAIN) {
        // 暂时没数据，做别的事或者继续循环
        continue;
    }
    if (n > 0) {
        // 处理数据
    }
}
```

避免了线程阻塞，但忙等（busy-wait）浪费 CPU。实际中配合 `select`/`poll`/`epoll` 使用，让内核帮你监视多个 socket，只在有事件时唤醒。

**I/O 多路复用（I/O Multiplexing）**

用 `select`、`poll`（跨平台）或者 `epoll`（Linux）、`kqueue`（macOS）同时监视多个 socket，哪个有事件就处理哪个：

```cpp
// epoll 示例（Linux）
int epfd = epoll_create1(0);

// 把 server_fd 加入监视
epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = server_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

// 等待事件
epoll_event events[64];
int n = epoll_wait(epfd, events, 64, -1);  // 阻塞直到有事件
for (int i = 0; i < n; i++) {
    // 处理 events[i].data.fd 上的事件
}
```

一个线程就能高效管理数千个连接，这是 Nginx、Redis 等高性能服务的核心技术。

**异步 I/O（Asynchronous I/O）**

发起 I/O 操作，不等结果，注册一个回调（callback）或者通过 `std::future` 等机制。操作完成时，系统通知程序处理结果：

```cpp
// 概念示意（不是真实 API）
async_recv(sock, buf, [](int bytes_read) {
    // 这里是数据到达时的回调
    process(buf, bytes_read);
});

// 主线程可以做别的事情
do_other_work();
```

Asio 库（下一章）就是基于这个模型的——用 `async_read`、`async_write` 发起异步 I/O，在回调里处理结果。

### 如何选择

对于 Taco 的 `fetchUrl()`，情况很简单：

- `fetchUrl` 是一个**同步**的调用——Taco 脚本调用它，等它返回结果，然后继续执行
- 发起一次 HTTP 请求，等响应，返回结果
- 不需要同时处理多个连接

所以最简单的**同步阻塞**模型就够了。这也是 `cpp-httplib` 的工作方式——底层用同步 socket，API 非常简单：

```cpp
httplib::Client cli("api.github.com");
auto res = cli.Get("/users/torvalds");
// 这里 res 里已经有响应了
```

如果将来 Taco 要实现并发网络请求（比如同时发多个 `fetchUrl`，用 `thread` 并行），那时候才需要考虑异步或多线程方案。

---

### 一个更真实的问题：字节流的拆包

用 TCP 发 HTTP 请求，会碰到字节流的问题。HTTP 响应的格式是：

```
HTTP/1.1 200 OK\r\n
Content-Type: application/json\r\n
Content-Length: 42\r\n
\r\n
{"login":"torvalds","id":1024025,...}
```

头部以 `\r\n\r\n` 结束，body 长度由 `Content-Length` 给出。程序需要：

1. 不断 `recv`，直到看到 `\r\n\r\n`，这时候头部读完了
2. 解析头部，找到 `Content-Length`
3. 再 `recv` 指定字节数，读完 body

如果手动实现，大概是这样：

```cpp
std::string response;
char buf[4096];

// 读头部
while (true) {
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n <= 0) break;
    response.append(buf, n);
    // 找到头部结束标记
    if (response.find("\r\n\r\n") != std::string::npos) break;
}

// 解析 Content-Length
auto pos = response.find("Content-Length: ");
int content_length = 0;
if (pos != std::string::npos) {
    content_length = std::stoi(response.substr(pos + 16));
}

// 计算还需要读多少 body
auto header_end = response.find("\r\n\r\n") + 4;
int body_received = response.size() - header_end;
int remaining = content_length - body_received;

// 继续读 body
while (remaining > 0) {
    int n = recv(sock, buf, std::min(remaining, (int)sizeof(buf)), 0);
    if (n <= 0) break;
    response.append(buf, n);
    remaining -= n;
}
```

这只是 HTTP/1.1 的一个简化版本，还没处理 chunked transfer encoding、Keep-Alive、重定向……这就是为什么没人手写 HTTP 客户端，都用库。

---

## 小结

**TCP 和 UDP** 是两种传输层协议。TCP 可靠有序，是 HTTP 的基础；UDP 速度快但不可靠，适合实时应用。

**客户端/服务器模型**：服务器监听端口，接受连接；客户端主动发起连接，发送请求，接收响应。`fetchUrl()` 是一个纯客户端实现。

**原生 socket API**：`socket` → `connect` → `send`/`recv` → `close` 是客户端的基本流程。服务器多一个 `bind` → `listen` → `accept`。端口号和 IP 地址需要处理字节序。

**I/O 模型**：
- 同步阻塞：最简单，线程在等待时阻塞
- 同步非阻塞 + I/O 多路复用（epoll）：一个线程管理多个连接，高性能服务器的基础
- 异步 I/O：发起操作后不等结果，通过回调或 future 处理

对于 Taco 的 `fetchUrl()`，同步阻塞就够了。下一章介绍 Asio（异步 I/O 库）和 cpp-httplib（同步 HTTP 客户端），并选择适合 Taco 的方案。
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
