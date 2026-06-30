#pragma once
#include <string>

// TokenType 枚举：用 enum class 而不是普通 enum
// enum class 更安全：不会隐式转换成 int，不同枚举的值不会互相冲突
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
    While,        // while
    For,          // for
    In,           // in
    Return,       // return
    Class,        // class
    Struct,       // struct
    Enum,         // enum
    Extends,      // extends
    Self,         // self
    Super,        // super
    Switch,       // switch
    Case,         // case
    Default,      // default
    Import,       // import
    From,         // from
    Thread,       // thread
    Channel,      // channel

    // 运算符
    Plus,         // +
    Minus,        // -
    Star,         // *
    Slash,        // /
    Percent,      // %
    Caret,        // ^
    Equal,        // ==
    NotEqual,     // !=
    Greater,      // >
    Less,         // <
    GreaterEqual, // >=
    LessEqual,    // <=
    And,          // &&
    Or,           // ||
    Not,          // !
    Assign,       // =
    Question,     // ?
    Colon,        // :
    Dot,          // .
    Ellipsis,     // ...
    PipeArrow,    // |>

    // 分隔符
    LeftParen,    // (
    RightParen,   // )
    LeftBrace,    // {
    RightBrace,   // }
    LeftBracket,  // [
    RightBracket, // ]
    Semicolon,    // ;
    Comma,        // ,

    // 彩蛋
    Taco,         // 🌮

    // 特殊
    EndOfFile,
};

// Token 结构体
struct Token {
    TokenType   type;
    std::string value;   // 原始文本
    int         line;    // 行号（从 1 开始）
    int         column;  // 列号（从 1 开始）
};

// 把 TokenType 转成字符串，方便调试
std::string token_type_to_string(TokenType type);
