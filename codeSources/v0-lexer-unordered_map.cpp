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
