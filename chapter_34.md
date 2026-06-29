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
