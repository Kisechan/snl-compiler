#include "lexer.h"

#include <cctype>
#include <unordered_map>

namespace snl {

namespace {

const std::unordered_map<std::string, TokenType>& reserved_words() {
    static const std::unordered_map<std::string, TokenType> words = {
        {"program", TokenType::Program},
        {"procedure", TokenType::Procedure},
        {"type", TokenType::Type},
        {"var", TokenType::Var},
        {"if", TokenType::If},
        {"then", TokenType::Then},
        {"else", TokenType::Else},
        {"fi", TokenType::Fi},
        {"while", TokenType::While},
        {"do", TokenType::Do},
        {"endwh", TokenType::EndWh},
        {"begin", TokenType::Begin},
        {"end", TokenType::End},
        {"read", TokenType::Read},
        {"write", TokenType::Write},
        {"array", TokenType::Array},
        {"of", TokenType::Of},
        {"record", TokenType::Record},
        {"return", TokenType::Return},
        {"integer", TokenType::Integer},
        {"char", TokenType::Char},
    };
    return words;
}

std::string one_char_string(char ch) {
    return std::string(1, ch);
}

}  // namespace

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

LexerResult Lexer::scan_tokens() {
    while (!is_at_end()) {
        scan_token();
    }

    add_token(TokenType::EndFile, SourceLocation{line_, column_}, "");
    return std::move(result_);
}

bool Lexer::is_at_end() const {
    return index_ >= source_.size();
}

char Lexer::current() const {
    if (is_at_end()) {
        return '\0';
    }
    return source_[index_];
}

char Lexer::peek() const {
    if (index_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[index_ + 1];
}

char Lexer::advance() {
    char ch = current();
    ++index_;
    if (ch == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return ch;
}

bool Lexer::match(char expected) {
    if (is_at_end() || current() != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::scan_token() {
    const SourceLocation start{line_, column_};
    const char ch = advance();

    switch (ch) {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            return;
        case '{':
            skip_comment();
            return;
        case '\'':
            scan_char_const();
            return;
        case ':':
            if (match('=')) {
                add_token(TokenType::Assign, start, ":=");
            } else {
                add_error(start, "expected '=' after ':'");
            }
            return;
        case '=':
            add_token(TokenType::Eq, start, "=");
            return;
        case '<':
            add_token(TokenType::Lt, start, "<");
            return;
        case '+':
            add_token(TokenType::Plus, start, "+");
            return;
        case '-':
            add_token(TokenType::Minus, start, "-");
            return;
        case '*':
            add_token(TokenType::Times, start, "*");
            return;
        case '/':
            add_token(TokenType::Over, start, "/");
            return;
        case '(':
            add_token(TokenType::LParen, start, "(");
            return;
        case ')':
            add_token(TokenType::RParen, start, ")");
            return;
        case '[':
            add_token(TokenType::LMidParen, start, "[");
            return;
        case ']':
            add_token(TokenType::RMidParen, start, "]");
            return;
        case '.':
            if (match('.')) {
                add_token(TokenType::Underange, start, "..");
            } else {
                add_token(TokenType::Dot, start, ".");
            }
            return;
        case ';':
            add_token(TokenType::Semi, start, ";");
            return;
        case ',':
            add_token(TokenType::Comma, start, ",");
            return;
        default:
            break;
    }

    if (is_alpha(ch)) {
        --index_;
        --column_;
        scan_identifier();
        return;
    }
    if (is_digit(ch)) {
        --index_;
        --column_;
        scan_number();
        return;
    }

    add_error(start, "invalid character '" + one_char_string(ch) + "'");
}

void Lexer::scan_identifier() {
    const SourceLocation start{line_, column_};
    const std::size_t start_index = index_;
    while (!is_at_end() && is_alnum(current())) {
        advance();
    }

    const std::string lexeme = source_.substr(start_index, index_ - start_index);
    const auto found = reserved_words().find(lexeme);
    if (found != reserved_words().end()) {
        add_token(found->second, start, lexeme);
    } else {
        add_token(TokenType::Id, start, lexeme, lexeme);
    }
}

void Lexer::scan_number() {
    const SourceLocation start{line_, column_};
    const std::size_t start_index = index_;
    while (!is_at_end() && is_digit(current())) {
        advance();
    }

    const std::string lexeme = source_.substr(start_index, index_ - start_index);
    add_token(TokenType::IntConst, start, lexeme, lexeme);
}

void Lexer::scan_char_const() {
    const SourceLocation start{line_, column_ - 1};

    if (is_at_end() || current() == '\n' || !is_alnum(current())) {
        add_error(start, "invalid character constant");
        while (!is_at_end() && current() != '\'' && current() != '\n') {
            advance();
        }
        if (!is_at_end() && current() == '\'') {
            advance();
        }
        return;
    }

    const char value = advance();
    if (!match('\'')) {
        add_error(start, "unterminated or too-long character constant");
        while (!is_at_end() && current() != '\'' && current() != '\n') {
            advance();
        }
        if (!is_at_end() && current() == '\'') {
            advance();
        }
        return;
    }

    add_token(TokenType::CharConst,
              start,
              "'" + one_char_string(value) + "'",
              one_char_string(value));
}

void Lexer::skip_comment() {
    const SourceLocation start{line_, column_ - 1};
    while (!is_at_end() && current() != '}') {
        advance();
    }

    if (is_at_end()) {
        add_error(start, "unterminated comment");
        return;
    }

    advance();
}

void Lexer::add_token(TokenType type,
                      SourceLocation location,
                      std::string lexeme) {
    add_token(type, location, std::move(lexeme), "");
}

void Lexer::add_token(TokenType type,
                      SourceLocation location,
                      std::string lexeme,
                      std::string value) {
    result_.tokens.push_back(Token{
        type,
        location,
        std::move(lexeme),
        std::move(value),
    });
}

void Lexer::add_error(SourceLocation location, std::string message) {
    result_.diagnostics.push_back(Diagnostic{
        DiagnosticStage::Lexical,
        location,
        std::move(message),
    });
}

bool Lexer::is_alpha(char ch) {
    const auto value = static_cast<unsigned char>(ch);
    return std::isalpha(value) != 0;
}

bool Lexer::is_digit(char ch) {
    const auto value = static_cast<unsigned char>(ch);
    return std::isdigit(value) != 0;
}

bool Lexer::is_alnum(char ch) {
    const auto value = static_cast<unsigned char>(ch);
    return std::isalnum(value) != 0;
}

}  // namespace snl
