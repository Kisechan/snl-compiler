#pragma once

#include "diagnostic.h"
#include "token.h"

#include <string>
#include <vector>

namespace snl {

struct LexerResult {
    std::vector<Token> tokens;
    std::vector<Diagnostic> diagnostics;
};

class Lexer {
public:
    explicit Lexer(std::string source);

    LexerResult scan_tokens();

private:
    bool is_at_end() const;
    char current() const;
    char peek() const;
    char advance();
    bool match(char expected);

    void scan_token();
    void scan_identifier();
    void scan_number();
    void scan_char_const();
    void skip_comment();

    void add_token(TokenType type, SourceLocation location, std::string lexeme);
    void add_token(TokenType type,
                   SourceLocation location,
                   std::string lexeme,
                   std::string value);
    void add_error(SourceLocation location, std::string message);

    static bool is_alpha(char ch);
    static bool is_digit(char ch);
    static bool is_alnum(char ch);

    std::string source_;
    std::size_t index_ = 0;
    int line_ = 1;
    int column_ = 1;
    LexerResult result_;
};

}  // namespace snl
