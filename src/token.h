#pragma once

#include "diagnostic.h"

#include <string>

namespace snl {

enum class TokenType {
    Program,
    Procedure,
    Type,
    Var,
    If,
    Then,
    Else,
    Fi,
    While,
    Do,
    EndWh,
    Begin,
    End,
    Read,
    Write,
    Array,
    Of,
    Record,
    Return,
    Integer,
    Char,

    Id,
    IntConst,
    CharConst,

    Assign,
    Eq,
    Lt,
    Plus,
    Minus,
    Times,
    Over,
    LParen,
    RParen,
    LMidParen,
    RMidParen,
    Dot,
    Semi,
    Comma,
    Underange,

    EndFile
};

struct Token {
    TokenType type;
    SourceLocation location;
    std::string lexeme;
    std::string value;
};

std::string token_type_name(TokenType type);

}  // namespace snl
