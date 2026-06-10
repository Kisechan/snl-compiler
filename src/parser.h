#pragma once

#include "ast.h"
#include "diagnostic.h"
#include "token.h"

#include <memory>
#include <initializer_list>
#include <string>
#include <vector>

namespace snl {

struct ParserResult {
    std::unique_ptr<AstNode> root;
    std::vector<Diagnostic> diagnostics;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    ParserResult parse();

private:
    struct TypeSyntax {
        std::string detail;
        std::vector<std::unique_ptr<AstNode>> children;
        SourceLocation location;
    };

    bool is_at_end() const;
    const Token& current() const;
    const Token& previous() const;
    bool check(TokenType type) const;
    bool check_any(std::initializer_list<TokenType> types) const;
    bool match(TokenType type);
    const Token& advance();
    const Token& expect(TokenType type, const std::string& expected);

    std::unique_ptr<AstNode> parse_program();
    std::unique_ptr<AstNode> parse_program_head();
    void parse_declare_part(AstNode& program);
    std::unique_ptr<AstNode> parse_type_dec();
    std::unique_ptr<AstNode> parse_type_dec_list();
    TypeSyntax parse_type_name();
    TypeSyntax parse_base_type();
    TypeSyntax parse_structure_type();
    TypeSyntax parse_array_type();
    TypeSyntax parse_record_type();
    std::unique_ptr<AstNode> parse_field_dec_list();
    std::vector<std::string> parse_id_list();

    std::unique_ptr<AstNode> parse_var_dec();
    std::unique_ptr<AstNode> parse_var_dec_list();
    std::vector<std::string> parse_var_id_list();

    void parse_proc_dec(AstNode& program);
    std::unique_ptr<AstNode> parse_proc_declaration();
    void parse_param_list(AstNode& proc);
    std::unique_ptr<AstNode> parse_param();
    std::vector<std::string> parse_form_list();

    std::unique_ptr<AstNode> parse_program_body();
    std::unique_ptr<AstNode> parse_stm_list();
    std::unique_ptr<AstNode> parse_statement();
    std::unique_ptr<AstNode> parse_if_statement();
    std::unique_ptr<AstNode> parse_while_statement();
    std::unique_ptr<AstNode> parse_read_statement();
    std::unique_ptr<AstNode> parse_write_statement();
    std::unique_ptr<AstNode> parse_return_statement();
    std::unique_ptr<AstNode> parse_id_statement();
    std::vector<std::unique_ptr<AstNode>> parse_act_param_list();

    std::unique_ptr<AstNode> parse_rel_exp();
    std::unique_ptr<AstNode> parse_exp();
    std::unique_ptr<AstNode> parse_term();
    std::unique_ptr<AstNode> parse_factor();
    std::unique_ptr<AstNode> parse_variable_after_id(const Token& id);
    std::unique_ptr<AstNode> parse_field_var(const Token& id);

    std::unique_ptr<AstNode> make_dec(TypeSyntax type, std::vector<std::string> names);
    bool is_statement_start(TokenType type) const;
    bool is_sync_token(TokenType type) const;
    void synchronize();
    void add_syntax_error(SourceLocation location, const std::string& message);

    const std::vector<Token>& tokens_;
    std::size_t index_ = 0;
    std::vector<Diagnostic> diagnostics_;
    Token synthetic_token_{TokenType::EndFile, SourceLocation{}, "", ""};
};

}  // namespace snl
