#include "token.h"

namespace snl {

std::string token_type_name(TokenType type) {
    switch (type) {
        case TokenType::Program:
            return "PROGRAM";
        case TokenType::Procedure:
            return "PROCEDURE";
        case TokenType::Type:
            return "TYPE";
        case TokenType::Var:
            return "VAR";
        case TokenType::If:
            return "IF";
        case TokenType::Then:
            return "THEN";
        case TokenType::Else:
            return "ELSE";
        case TokenType::Fi:
            return "FI";
        case TokenType::While:
            return "WHILE";
        case TokenType::Do:
            return "DO";
        case TokenType::EndWh:
            return "ENDWH";
        case TokenType::Begin:
            return "BEGIN";
        case TokenType::End:
            return "END";
        case TokenType::Read:
            return "READ";
        case TokenType::Write:
            return "WRITE";
        case TokenType::Array:
            return "ARRAY";
        case TokenType::Of:
            return "OF";
        case TokenType::Record:
            return "RECORD";
        case TokenType::Return:
            return "RETURN";
        case TokenType::Integer:
            return "INTEGER";
        case TokenType::Char:
            return "CHAR";
        case TokenType::Id:
            return "ID";
        case TokenType::IntConst:
            return "INTC";
        case TokenType::CharConst:
            return "CHARC";
        case TokenType::Assign:
            return "ASSIGN";
        case TokenType::Eq:
            return "EQ";
        case TokenType::Lt:
            return "LT";
        case TokenType::Plus:
            return "PLUS";
        case TokenType::Minus:
            return "MINUS";
        case TokenType::Times:
            return "TIMES";
        case TokenType::Over:
            return "OVER";
        case TokenType::LParen:
            return "LPAREN";
        case TokenType::RParen:
            return "RPAREN";
        case TokenType::LMidParen:
            return "LMIDPAREN";
        case TokenType::RMidParen:
            return "RMIDPAREN";
        case TokenType::Dot:
            return "DOT";
        case TokenType::Semi:
            return "SEMI";
        case TokenType::Comma:
            return "COMMA";
        case TokenType::Underange:
            return "UNDERANGE";
        case TokenType::EndFile:
            return "EOF";
    }
    return "UNKNOWN";
}

}  // namespace snl
