# 第三十八章：VSCode 插件与 Language Server

---

Taco 可以运行了，但写 Taco 脚本的体验还不够好——所有关键字都是同一个颜色，没有自动补全，写错了没有提示。

这一章给 Taco 加上编辑器支持：先做语法高亮（不需要 C++，只是配置文件），然后实现一个基础的 Language Server（需要 C++，让编辑器能和解释器通信），最后打包发布到 VSCode Marketplace。

---

## 38.1 LSP 是什么

在 LSP 出现之前，每个编辑器（VSCode、Vim、Emacs、Sublime Text……）和每种语言都需要单独集成。N 个编辑器 × M 种语言 = N×M 个集成点，每个都要独立实现。

**Language Server Protocol（LSP）** 是 Microsoft 在 2016 年提出的协议，把语言智能和编辑器解耦：

```
┌─────────┐   LSP（JSON-RPC）   ┌────────────────┐
│  编辑器  │ <─────────────────> │ Language Server │
│(Client) │                     │   (Server)     │
└─────────┘                     └────────────────┘
```

- **编辑器（LSP Client）**：VSCode、Vim、Emacs 等，发送用户操作（光标移动、打字、保存）作为请求
- **Language Server**：独立进程，理解某种语言，回答编辑器的问题（这个词是什么、这里有没有错误、补全建议是什么）

有了 LSP，一个 Language Server 可以为所有支持 LSP 的编辑器服务。现在已经有几百种语言的 Language Server（clangd 为 C++，rust-analyzer 为 Rust，pyright 为 Python……）。

### LSP 通信格式

LSP 用 JSON-RPC 2.0 通过 stdin/stdout 通信（或 TCP socket）。每条消息有一个 header 和 body：

```
Content-Length: 89\r\n
\r\n
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"processId":12345,...}}
```

编辑器发送请求，Language Server 发送响应：

```json
// 编辑器 → Server：初始化
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
        "processId": 12345,
        "rootUri": "file:///home/user/project",
        "capabilities": { ... }
    }
}

// Server → 编辑器：初始化响应
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "capabilities": {
            "textDocumentSync": 1,
            "completionProvider": {},
            "hoverProvider": true
        }
    }
}
```

常用的 LSP 方法：

| 方法 | 方向 | 含义 |
|------|------|------|
| `initialize` | Client → Server | 初始化，交换能力列表 |
| `textDocument/didOpen` | Client → Server | 用户打开了文件 |
| `textDocument/didChange` | Client → Server | 用户修改了文件 |
| `textDocument/completion` | Client → Server | 请求自动补全 |
| `textDocument/hover` | Client → Server | 鼠标悬停，请求信息 |
| `textDocument/publishDiagnostics` | Server → Client | 推送错误/警告 |

---

## 38.2 语法高亮：TextMate grammar

语法高亮不需要 Language Server，只需要一个 **TextMate grammar** 文件（`.tmLanguage.json`）。这是一个正则表达式规则集，定义哪些词应该用什么颜色显示。

### VSCode 插件结构

```
taco-language/
  package.json          # 插件配置（名字、版本、贡献点）
  language-configuration.json  # 括号匹配、注释、缩进规则
  syntaxes/
    taco.tmLanguage.json  # TextMate 语法文件
  server/
    src/
      server.cpp        # Language Server 实现
    CMakeLists.txt
  client/
    src/
      extension.ts      # VSCode 插件入口（TypeScript）
  package-lock.json
```

### package.json

```json
{
    "name": "taco-language",
    "displayName": "Taco Language",
    "description": "Language support for the Taco scripting language",
    "version": "0.1.0",
    "publisher": "your-name",
    "engines": {
        "vscode": "^1.85.0"
    },
    "categories": ["Programming Languages"],
    "contributes": {
        "languages": [
            {
                "id": "taco",
                "aliases": ["Taco", "taco"],
                "extensions": [".taco"],
                "configuration": "./language-configuration.json"
            }
        ],
        "grammars": [
            {
                "language": "taco",
                "scopeName": "source.taco",
                "path": "./syntaxes/taco.tmLanguage.json"
            }
        ]
    },
    "main": "./client/out/extension.js",
    "scripts": {
        "compile": "tsc -p ./client/tsconfig.json",
        "vscode:prepublish": "npm run compile"
    },
    "dependencies": {
        "vscode-languageclient": "^9.0.1"
    },
    "devDependencies": {
        "@types/vscode": "^1.85.0",
        "typescript": "^5.3.3"
    }
}
```

### language-configuration.json

```json
{
    "comments": {
        "lineComment": "//"
    },
    "brackets": [
        ["{", "}"],
        ["[", "]"],
        ["(", ")"]
    ],
    "autoClosingPairs": [
        { "open": "{", "close": "}" },
        { "open": "[", "close": "]" },
        { "open": "(", "close": ")" },
        { "open": "\"", "close": "\"" }
    ],
    "surroundingPairs": [
        ["{", "}"],
        ["[", "]"],
        ["(", ")"],
        ["\"", "\""]
    ],
    "indentationRules": {
        "increaseIndentPattern": "^.*\\{\\s*$",
        "decreaseIndentPattern": "^\\s*\\}"
    }
}
```

### taco.tmLanguage.json

TextMate grammar 用正则表达式匹配代码里的不同部分，给每部分打上 scope（范围标签），编辑器主题根据 scope 决定颜色。

```json
{
    "$schema": "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json",
    "name": "Taco",
    "scopeName": "source.taco",
    "patterns": [
        { "include": "#comments" },
        { "include": "#strings" },
        { "include": "#keywords" },
        { "include": "#numbers" },
        { "include": "#operators" },
        { "include": "#functions" },
        { "include": "#variables" },
        { "include": "#builtins" }
    ],
    "repository": {
        "comments": {
            "name": "comment.line.double-slash.taco",
            "match": "//.*$"
        },
        "strings": {
            "name": "string.quoted.double.taco",
            "begin": "\"",
            "end": "\"",
            "patterns": [
                {
                    "name": "constant.character.escape.taco",
                    "match": "\\\\[nrt\\\\\"']"
                },
                {
                    "name": "meta.embedded.expression.taco",
                    "begin": "\\{",
                    "end": "\\}",
                    "patterns": [{ "include": "$self" }]
                }
            ]
        },
        "keywords": {
            "name": "keyword.control.taco",
            "match": "\\b(var|func|if|elseif|else|while|for|in|return|class|struct|enum|extends|self|super|switch|case|default|import|from|thread|channel|mutex|true|false|nil)\\b"
        },
        "numbers": {
            "name": "constant.numeric.taco",
            "match": "\\b([0-9][0-9_]*\\.?[0-9_]*)\\b"
        },
        "operators": {
            "patterns": [
                {
                    "name": "keyword.operator.comparison.taco",
                    "match": "(==|!=|>=|<=|>|<)"
                },
                {
                    "name": "keyword.operator.logical.taco",
                    "match": "(&&|\\|\\||!)"
                },
                {
                    "name": "keyword.operator.arithmetic.taco",
                    "match": "(\\+|-|\\*|/|%|\\^)"
                },
                {
                    "name": "keyword.operator.assignment.taco",
                    "match": "="
                },
                {
                    "name": "keyword.operator.other.taco",
                    "match": "(\\?\\.|\\?\\?|\\.\\.\\.)"
                }
            ]
        },
        "functions": {
            "name": "entity.name.function.taco",
            "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*\\()"
        },
        "builtins": {
            "name": "support.function.builtin.taco",
            "match": "\\b(print|input|type|number|string|bool|cat|echo|ls|mkdir|rm|mv|cp|pwd|cd|exists|exec|env|fetchUrl|postData|parseJson|toJson|range)\\b"
        },
        "variables": {
            "name": "variable.other.taco",
            "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b"
        }
    }
}
```

这个 grammar 实现了：
- 注释（`//`）：灰色
- 字符串（双引号）：绿色，支持字符串插值（`{expr}` 内部也有高亮）
- 关键字（`var`、`func`、`if`……）：蓝色
- 数字：橙色
- 运算符：高亮
- 函数调用：函数名高亮
- 内置函数（`print`、`fetchUrl`……）：特殊颜色

---

## 38.3 实现基础 Language Server

语法高亮不需要 Language Server，只是静态规则。但自动补全、错误提示、悬停信息需要真正理解代码——需要 Language Server。

### Language Server 的架构

Taco 的 Language Server 是一个独立的 C++ 可执行文件，通过 stdin/stdout 和 VSCode 通信。VSCode 插件（TypeScript）启动这个进程，然后通过 LSP 协议和它交互。

```
VSCode
  └── extension.ts（LSP Client）
        │  stdin/stdout
        ▼
  taco-lsp（C++ Language Server）
        └── 复用 Taco 的 Lexer、Parser
```

Language Server 内部复用 Taco 的 Lexer 和 Parser——这就是把解释器拆成独立模块的好处。

### LSP 消息解析

LSP 消息通过 stdin 逐条读取，格式是 header + JSON body：

```cpp
// lsp_server.cpp

#include <iostream>
#include <string>
#include <sstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 读取一条 LSP 消息（阻塞直到读到完整消息）
json read_message() {
    // 读取 header 行
    int content_length = -1;
    std::string line;

    while (std::getline(std::cin, line)) {
        // 移除 \r（Windows 换行符）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            // 空行：header 结束，读取 body
            break;
        }

        // 解析 Content-Length
        const std::string prefix = "Content-Length: ";
        if (line.substr(0, prefix.size()) == prefix) {
            content_length = std::stoi(line.substr(prefix.size()));
        }
    }

    if (content_length < 0) {
        throw std::runtime_error("Missing Content-Length header");
    }

    // 读取 body
    std::string body(content_length, '\0');
    std::cin.read(body.data(), content_length);

    return json::parse(body);
}

// 发送一条 LSP 消息
void send_message(const json& msg) {
    std::string body = msg.dump();
    std::cout << "Content-Length: " << body.size() << "\r\n";
    std::cout << "\r\n";
    std::cout << body;
    std::cout.flush();
}

// 发送响应
void send_response(const json& id, const json& result) {
    send_message({
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    });
}

// 发送错误响应
void send_error(const json& id, int code, const std::string& message) {
    send_message({
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", {{"code", code}, {"message", message}}}
    });
}

// 发送通知（没有 id，Server 主动发给 Client）
void send_notification(const std::string& method, const json& params) {
    send_message({
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    });
}
```

### 文档管理

Language Server 需要维护当前打开的所有文档的内容：

```cpp
// 存储所有打开的文档内容
std::unordered_map<std::string, std::string> open_documents;

void handle_did_open(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    auto text = params["textDocument"]["text"].get<std::string>();
    open_documents[uri] = text;

    // 打开时立刻做一次诊断
    publish_diagnostics(uri, text);
}

void handle_did_change(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    // contentChanges 是增量更新列表，这里取全量（simpler）
    auto changes = params["contentChanges"];
    if (!changes.empty()) {
        open_documents[uri] = changes.back()["text"].get<std::string>();
    }

    // 每次改动后重新做诊断
    publish_diagnostics(uri, open_documents[uri]);
}
```

### 错误诊断（Diagnostics）

Diagnostics 是 LSP 里最有价值的功能：实时显示代码错误。

Taco 的词法分析器和语法分析器在遇到错误时会抛出异常。Language Server 捕获这些异常，把错误位置和信息转成 LSP Diagnostic，推送给编辑器：

```cpp
// Taco 错误结构（在 error.h 里定义）
struct TacoError {
    std::string message;
    int line;    // 1-indexed
    int column;  // 0-indexed
};

// 把 Taco 错误转成 LSP Diagnostic
json make_diagnostic(const TacoError& err, int severity = 1) {
    // severity: 1=Error, 2=Warning, 3=Information, 4=Hint
    return {
        {"range", {
            {"start", {{"line", err.line - 1}, {"character", err.column}}},
            {"end",   {{"line", err.line - 1}, {"character", err.column + 1}}}
        }},
        {"severity", severity},
        {"source", "taco"},
        {"message", err.message}
    };
}

void publish_diagnostics(const std::string& uri, const std::string& source) {
    json diagnostics = json::array();

    try {
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.tokenize();  // 可能抛出 LexError

        // 语法分析
        Parser parser(tokens);
        auto ast = parser.parse();      // 可能抛出 ParseError

        // 也可以做更深的语义分析，比如检查未定义变量
        // 但那需要一个不执行、只分析的 pass，暂时不实现

    } catch (const LexError& e) {
        diagnostics.push_back(make_diagnostic({e.message(), e.line(), e.column()}));
    } catch (const ParseError& e) {
        diagnostics.push_back(make_diagnostic({e.message(), e.line(), e.column()}));
    } catch (...) {
        // 其他错误忽略
    }

    send_notification("textDocument/publishDiagnostics", {
        {"uri", uri},
        {"diagnostics", diagnostics}
    });
}
```

这样，用户一写错了（比如漏了括号、拼错了关键字），编辑器里立刻出现红色波浪线。

### 自动补全（Completion）

自动补全返回一个候选项列表。对于 Taco，最简单的补全策略是：返回所有关键字和内置函数。

```cpp
// 所有 Taco 关键字
static const std::vector<std::string> TACO_KEYWORDS = {
    "var", "func", "if", "elseif", "else", "while", "for", "in",
    "return", "class", "struct", "enum", "extends", "self", "super",
    "switch", "case", "default", "import", "from", "thread", "channel",
    "mutex", "true", "false", "nil"
};

// 所有内置函数
static const std::vector<std::string> TACO_BUILTINS = {
    "print", "input", "type", "number", "string", "bool",
    "cat", "echo", "ls", "mkdir", "rm", "mv", "cp", "pwd", "cd", "exists",
    "exec", "env", "fetchUrl", "postData", "parseJson", "toJson",
    "range", "random"
};

json handle_completion(const json& params) {
    json items = json::array();

    // 关键字补全
    for (const auto& kw : TACO_KEYWORDS) {
        items.push_back({
            {"label", kw},
            {"kind", 14},          // 14 = Keyword
            {"detail", "keyword"}
        });
    }

    // 内置函数补全
    for (const auto& fn : TACO_BUILTINS) {
        items.push_back({
            {"label", fn},
            {"kind", 3},           // 3 = Function
            {"detail", "built-in function"}
        });
    }

    // 更高级：扫描当前文档，找用户定义的变量和函数名
    // ... （从 open_documents 里取当前文件内容，跑 lexer，提取标识符）

    return items;
}
```

### 悬停信息（Hover）

鼠标悬停在某个词上时，显示文档或类型信息：

```cpp
// 内置函数的文档字符串
static const std::unordered_map<std::string, std::string> BUILTIN_DOCS = {
    {"print",     "print(value)\n\nPrint value to stdout."},
    {"fetchUrl",  "fetchUrl(url: string) -> string\n\nSend HTTP GET request. Returns response body."},
    {"postData",  "postData(url: string, body: string, contentType?: string) -> string\n\nSend HTTP POST request."},
    {"parseJson", "parseJson(str: string) -> any\n\nParse JSON string into Taco value (map, array, etc.)."},
    {"range",     "range(start: number, end: number) -> array\n\nGenerate integer sequence [start, end)."},
    // ... 其他内置函数 ...
};

json handle_hover(const json& params) {
    auto uri = params["textDocument"]["uri"].get<std::string>();
    auto line = params["position"]["line"].get<int>();
    auto character = params["position"]["character"].get<int>();

    // 从文档里取当前行
    auto& source = open_documents[uri];
    std::istringstream ss(source);
    std::string current_line;
    for (int i = 0; i <= line; i++) {
        std::getline(ss, current_line);
    }

    // 找光标位置的词
    // 向左找词的开始，向右找词的结束
    int start = character;
    while (start > 0 && (std::isalnum(current_line[start-1]) || current_line[start-1] == '_')) {
        start--;
    }
    int end = character;
    while (end < (int)current_line.size()
           && (std::isalnum(current_line[end]) || current_line[end] == '_')) {
        end++;
    }

    std::string word = current_line.substr(start, end - start);

    auto it = BUILTIN_DOCS.find(word);
    if (it != BUILTIN_DOCS.end()) {
        return {
            {"contents", {
                {"kind", "markdown"},
                {"value", "```taco\n" + it->second + "\n```"}
            }}
        };
    }

    return nullptr;  // 没有信息，不显示悬停框
}
```

### 主循环

把所有处理程序组装起来：

```cpp
int main() {
    // 禁用 stdout 缓冲（LSP 要求立即发送）
    std::cout.setf(std::ios::unitbuf);

    while (true) {
        json msg;
        try {
            msg = read_message();
        } catch (const std::exception& e) {
            // stdin 关闭（编辑器退出），退出 server
            break;
        }

        std::string method = msg.value("method", "");
        json id = msg.contains("id") ? msg["id"] : json(nullptr);
        json params = msg.value("params", json(nullptr));

        if (method == "initialize") {
            // 声明 server 的能力
            send_response(id, {
                {"capabilities", {
                    {"textDocumentSync", 1},          // 全量同步
                    {"completionProvider", json::object()},
                    {"hoverProvider", true}
                }},
                {"serverInfo", {
                    {"name", "taco-lsp"},
                    {"version", "0.1.0"}
                }}
            });

        } else if (method == "initialized") {
            // 通知：不需要回复

        } else if (method == "textDocument/didOpen") {
            handle_did_open(params);

        } else if (method == "textDocument/didChange") {
            handle_did_change(params);

        } else if (method == "textDocument/didClose") {
            auto uri = params["textDocument"]["uri"].get<std::string>();
            open_documents.erase(uri);

        } else if (method == "textDocument/completion") {
            send_response(id, handle_completion(params));

        } else if (method == "textDocument/hover") {
            auto result = handle_hover(params);
            send_response(id, result.is_null() ? json(nullptr) : result);

        } else if (method == "shutdown") {
            send_response(id, nullptr);

        } else if (method == "exit") {
            break;

        } else if (!id.is_null()) {
            // 未知请求：返回错误
            send_error(id, -32601, "Method not found: " + method);
        }
        // 未知通知（没有 id）：忽略
    }

    return 0;
}
```

---

## 38.4 自动补全与错误提示

把 Language Server 和 VSCode 插件连起来，需要写一个 TypeScript 的 client 端：

```typescript
// client/src/extension.ts
import * as path from 'path';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
    // Language Server 可执行文件的路径
    const serverPath = context.asAbsolutePath(
        path.join('server', 'build', 'taco-lsp')
    );

    const serverOptions: ServerOptions = {
        command: serverPath,
        transport: TransportKind.stdio   // 用 stdin/stdout 通信
    };

    const clientOptions: LanguageClientOptions = {
        // 只对 .taco 文件激活
        documentSelector: [{ scheme: 'file', language: 'taco' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.taco')
        }
    };

    client = new LanguageClient(
        'taco-language-server',
        'Taco Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
    console.log('Taco Language Server started.');
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) return undefined;
    return client.stop();
}
```

当用户打开 `.taco` 文件时，VSCode 自动启动 `taco-lsp` 进程，通过 stdin/stdout 建立 LSP 连接。之后：

- 每次文件内容变化 → 触发诊断 → 错误实时显示在编辑器里
- 按 `Ctrl+Space` → 请求补全 → 出现关键字和内置函数列表
- 鼠标悬停在内置函数上 → 显示文档字符串

效果大致如下：

```
// 写错了 elseif 的拼写：
if (x > 10) {
    print("big");
} elsief (x > 5) {       // ← 红色波浪线："Unexpected token 'elsief'"
    print("medium");
}

// 输入 fetch 后按 Ctrl+Space：
fetch                    // → 补全建议：fetchUrl
```

---

## 38.5 发布到 VSCode Marketplace

### 安装发布工具

```bash
npm install -g @vscode/vsce
```

### 打包插件

```bash
cd taco-language
npm install
npm run compile
# 编译 Language Server
cd server && cmake -B build && cmake --build build && cd ..
# 打包成 .vsix 文件
vsce package
# 生成 taco-language-0.1.0.vsix
```

`.vsix` 是 VSCode 插件的包格式，可以直接在 VSCode 里安装（Extensions → Install from VSIX）。

### 发布到 Marketplace

1. 在 https://marketplace.visualstudio.com 创建账号
2. 在 Azure DevOps 创建 Personal Access Token（PAT），权限选 Marketplace → Manage
3. 登录：`vsce login your-publisher-name`
4. 发布：`vsce publish`

发布后，任何人都可以在 VSCode 的扩展市场搜索 "Taco" 并安装。

### 在 package.json 里补充元信息

发布前完善插件的元信息：

```json
{
    "name": "taco-language",
    "displayName": "Taco Language",
    "description": "Syntax highlighting and language support for Taco (.taco) scripting language",
    "version": "0.1.0",
    "publisher": "your-publisher-name",
    "license": "MIT",
    "repository": {
        "type": "git",
        "url": "https://github.com/your-name/taco"
    },
    "bugs": {
        "url": "https://github.com/your-name/taco/issues"
    },
    "icon": "images/taco-icon.png",
    "keywords": ["taco", "scripting", "language"],
    "categories": ["Programming Languages"]
}
```

---

## 小结

**TextMate grammar**（`taco.tmLanguage.json`）是语法高亮的核心，用正则表达式规则把代码里的不同元素打上 scope 标签，主题根据 scope 决定颜色。不需要 C++，纯配置文件。

**Language Server Protocol（LSP）** 把语言智能和编辑器解耦。Language Server 是一个独立进程，通过 stdin/stdout 用 JSON-RPC 和编辑器通信。主要功能：

- `textDocument/publishDiagnostics`：推送错误信息（实时红线）
- `textDocument/completion`：提供自动补全列表
- `textDocument/hover`：悬停显示文档

Taco 的 Language Server 用 C++ 实现，内部复用 Lexer 和 Parser 来做语法错误检测，这体现了把解释器模块化的价值。

**VSCode 插件**：TypeScript 写的 client 负责启动 Language Server 进程，把 LSP 消息转发给 VSCode 的 Extension API。用户无感知，打开 `.taco` 文件就自动生效。

整个工具链：写一次 Language Server（C++），所有支持 LSP 的编辑器（VSCode、Neovim、Emacs、Helix……）都能用。这是现代语言工具链的标准做法。
