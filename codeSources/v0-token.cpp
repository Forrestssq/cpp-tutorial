#include "token.h"

std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::Number:       return "NUMBER";
        case TokenType::String:       return "STRING";
        case TokenType::True:         return "TRUE";
        case TokenType::False:        return "FALSE";
        case TokenType::Nil:          return "NIL";
        case TokenType::Identifier:   return "IDENTIFIER";
        case TokenType::Var:          return "VAR";
        case TokenType::Func:         return "FUNC";
        case TokenType::If:           return "IF";
        case TokenType::Elseif:       return "ELSEIF";
        case TokenType::Else:         return "ELSE";
        case TokenType::While:        return "WHILE";
        case TokenType::For:          return "FOR";
        case TokenType::In:           return "IN";
        case TokenType::Return:       return "RETURN";
        case TokenType::Class:        return "CLASS";
        case TokenType::Struct:       return "STRUCT";
        case TokenType::Enum:         return "ENUM";
        case TokenType::Extends:      return "EXTENDS";
        case TokenType::Self:         return "SELF";
        case TokenType::Super:        return "SUPER";
        case TokenType::Switch:       return "SWITCH";
        case TokenType::Case:         return "CASE";
        case TokenType::Default:      return "DEFAULT";
        case TokenType::Import:       return "IMPORT";
        case TokenType::From:         return "FROM";
        case TokenType::Thread:       return "THREAD";
        case TokenType::Channel:      return "CHANNEL";
        case TokenType::Plus:         return "PLUS";
        case TokenType::Minus:        return "MINUS";
        case TokenType::Star:         return "STAR";
        case TokenType::Slash:        return "SLASH";
        case TokenType::Percent:      return "PERCENT";
        case TokenType::Caret:        return "CARET";
        case TokenType::Equal:        return "EQUAL";
        case TokenType::NotEqual:     return "NOT_EQUAL";
        case TokenType::Greater:      return "GREATER";
        case TokenType::Less:         return "LESS";
        case TokenType::GreaterEqual: return "GREATER_EQUAL";
        case TokenType::LessEqual:    return "LESS_EQUAL";
        case TokenType::And:          return "AND";
        case TokenType::Or:           return "OR";
        case TokenType::Not:          return "NOT";
        case TokenType::Assign:       return "ASSIGN";
        case TokenType::Question:     return "QUESTION";
        case TokenType::Colon:        return "COLON";
        case TokenType::Dot:          return "DOT";
        case TokenType::Ellipsis:     return "ELLIPSIS";
        case TokenType::PipeArrow:    return "PIPE_ARROW";
        case TokenType::LeftParen:    return "LEFT_PAREN";
        case TokenType::RightParen:   return "RIGHT_PAREN";
        case TokenType::LeftBrace:    return "LEFT_BRACE";
        case TokenType::RightBrace:   return "RIGHT_BRACE";
        case TokenType::LeftBracket:  return "LEFT_BRACKET";
        case TokenType::RightBracket: return "RIGHT_BRACKET";
        case TokenType::Semicolon:    return "SEMICOLON";
        case TokenType::Comma:        return "COMMA";
        case TokenType::Taco:         return "TACO";
        case TokenType::EndOfFile:    return "EOF";
    }
    return "UNKNOWN";
}
