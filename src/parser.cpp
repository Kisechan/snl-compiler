#include "parser.h"

#include <sstream>
#include <utility>

namespace snl {

namespace {

std::string got_text(const Token& token) {
    std::ostringstream out;
    out << token_type_name(token.type);
    if (!token.lexeme.empty()) {
        out << " '" << token.lexeme << "'";
    }
    return out.str();
}

std::unique_ptr<AstNode> make_exp(SourceLocation location, std::string detail) {
    return make_ast_node(AstNodeKind::ExpK, location, std::move(detail));
}

}  // namespace

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

ParserResult Parser::parse() {
    ParserResult result;
    result.root = parse_program();
    result.diagnostics = std::move(diagnostics_);
    return result;
}

bool Parser::is_at_end() const {
    return current().type == TokenType::EndFile;
}

const Token& Parser::current() const {
    if (index_ >= tokens_.size()) {
        return synthetic_token_;
    }
    return tokens_[index_];
}

const Token& Parser::previous() const {
    if (index_ == 0 || tokens_.empty()) {
        return synthetic_token_;
    }
    return tokens_[index_ - 1];
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::check_any(std::initializer_list<TokenType> types) const {
    for (TokenType type : types) {
        if (check(type)) {
            return true;
        }
    }
    return false;
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

const Token& Parser::advance() {
    if (!is_at_end()) {
        ++index_;
    }
    return previous();
}

const Token& Parser::expect(TokenType type, const std::string& expected) {
    if (check(type)) {
        return advance();
    }

    add_syntax_error(current().location,
                     "expected " + expected + ", got " + got_text(current()));
    if (!is_at_end() && !is_sync_token(current().type)) {
        advance();
    }
    return previous();
}

std::unique_ptr<AstNode> Parser::parse_program() {
    auto program = make_ast_node(AstNodeKind::ProK, current().location);
    add_child(*program, parse_program_head());
    parse_declare_part(*program);
    add_child(*program, parse_program_body());
    expect(TokenType::Dot, "'.'");
    expect(TokenType::EndFile, "EOF");
    return program;
}

std::unique_ptr<AstNode> Parser::parse_program_head() {
    const Token& start = expect(TokenType::Program, "'program'");
    const Token& name = expect(TokenType::Id, "program name");
    auto node = make_ast_node(AstNodeKind::PheadK, start.location);
    if (name.type == TokenType::Id) {
        node->names.push_back(name.value);
    }
    return node;
}

void Parser::parse_declare_part(AstNode& program) {
    add_child(program, parse_type_dec());
    add_child(program, parse_var_dec());
    parse_proc_dec(program);
}

std::unique_ptr<AstNode> Parser::parse_type_dec() {
    auto node = make_ast_node(AstNodeKind::TypeK, current().location);
    if (!match(TokenType::Type)) {
        return node;
    }

    while (check(TokenType::Id)) {
        add_child(*node, parse_type_dec_list());
    }
    return node;
}

std::unique_ptr<AstNode> Parser::parse_type_dec_list() {
    const Token& type_id = expect(TokenType::Id, "type identifier");
    expect(TokenType::Eq, "'='");
    TypeSyntax type = parse_type_name();
    std::vector<std::string> names;
    if (type_id.type == TokenType::Id) {
        names.push_back(type_id.value);
    }
    auto node = make_dec(std::move(type), std::move(names));
    expect(TokenType::Semi, "';'");
    return node;
}

Parser::TypeSyntax Parser::parse_type_name() {
    if (check(TokenType::Integer) || check(TokenType::Char)) {
        return parse_base_type();
    }
    if (check(TokenType::Array) || check(TokenType::Record)) {
        return parse_structure_type();
    }
    const Token& id = expect(TokenType::Id, "type name");
    return TypeSyntax{"IdK " + id.value, {}, id.location};
}

Parser::TypeSyntax Parser::parse_base_type() {
    if (match(TokenType::Integer)) {
        return TypeSyntax{"IntegerK", {}, previous().location};
    }
    const Token& token = expect(TokenType::Char, "'char'");
    return TypeSyntax{"CharK", {}, token.location};
}

Parser::TypeSyntax Parser::parse_structure_type() {
    if (check(TokenType::Array)) {
        return parse_array_type();
    }
    return parse_record_type();
}

Parser::TypeSyntax Parser::parse_array_type() {
    const Token& start = expect(TokenType::Array, "'array'");
    expect(TokenType::LMidParen, "'['");
    const Token& low = expect(TokenType::IntConst, "array lower bound");
    expect(TokenType::Underange, "'..'");
    const Token& top = expect(TokenType::IntConst, "array upper bound");
    expect(TokenType::RMidParen, "']'");
    expect(TokenType::Of, "'of'");
    TypeSyntax elem = parse_base_type();

    std::string detail = "ArrayK " + low.value + ".." + top.value + " " + elem.detail;
    return TypeSyntax{detail, {}, start.location};
}

Parser::TypeSyntax Parser::parse_record_type() {
    const Token& start = expect(TokenType::Record, "'record'");
    auto fields = parse_field_dec_list();
    expect(TokenType::End, "'end'");

    TypeSyntax result{"RecordK", {}, start.location};
    result.children.push_back(std::move(fields));
    return result;
}

std::unique_ptr<AstNode> Parser::parse_field_dec_list() {
    auto fields = make_ast_node(AstNodeKind::DecK, current().location, "FieldList");
    while (check_any({TokenType::Integer, TokenType::Char, TokenType::Array})) {
        TypeSyntax type = check(TokenType::Array) ? parse_array_type() : parse_base_type();
        std::vector<std::string> names = parse_id_list();
        add_child(*fields, make_dec(std::move(type), std::move(names)));
        expect(TokenType::Semi, "';'");
    }
    return fields;
}

std::vector<std::string> Parser::parse_id_list() {
    std::vector<std::string> names;
    const Token& id = expect(TokenType::Id, "identifier");
    if (id.type == TokenType::Id) {
        names.push_back(id.value);
    }
    while (match(TokenType::Comma)) {
        const Token& next = expect(TokenType::Id, "identifier");
        if (next.type == TokenType::Id) {
            names.push_back(next.value);
        }
    }
    return names;
}

std::unique_ptr<AstNode> Parser::parse_var_dec() {
    auto node = make_ast_node(AstNodeKind::VarK, current().location);
    if (!match(TokenType::Var)) {
        return node;
    }

    while (check_any({TokenType::Integer,
                      TokenType::Char,
                      TokenType::Array,
                      TokenType::Record,
                      TokenType::Id})) {
        add_child(*node, parse_var_dec_list());
    }
    return node;
}

std::unique_ptr<AstNode> Parser::parse_var_dec_list() {
    TypeSyntax type = parse_type_name();
    std::vector<std::string> names = parse_var_id_list();
    auto node = make_dec(std::move(type), std::move(names));
    expect(TokenType::Semi, "';'");
    return node;
}

std::vector<std::string> Parser::parse_var_id_list() {
    return parse_id_list();
}

void Parser::parse_proc_dec(AstNode& program) {
    while (check(TokenType::Procedure)) {
        add_child(program, parse_proc_declaration());
    }
}

std::unique_ptr<AstNode> Parser::parse_proc_declaration() {
    const Token& start = expect(TokenType::Procedure, "'procedure'");
    const Token& name = expect(TokenType::Id, "procedure name");
    auto proc = make_ast_node(AstNodeKind::ProcDecK, start.location);
    if (name.type == TokenType::Id) {
        proc->names.push_back(name.value);
    }

    expect(TokenType::LParen, "'('");
    parse_param_list(*proc);
    expect(TokenType::RParen, "')'");
    expect(TokenType::Semi, "';'");
    parse_declare_part(*proc);
    add_child(*proc, parse_program_body());
    return proc;
}

void Parser::parse_param_list(AstNode& proc) {
    if (check(TokenType::RParen)) {
        return;
    }
    add_child(proc, parse_param());
    while (match(TokenType::Semi)) {
        add_child(proc, parse_param());
    }
}

std::unique_ptr<AstNode> Parser::parse_param() {
    const bool is_var_param = match(TokenType::Var);
    TypeSyntax type = parse_type_name();
    if (is_var_param) {
        type.detail = "var param: " + type.detail;
    } else {
        type.detail = "value param: " + type.detail;
    }
    std::vector<std::string> names = parse_form_list();
    return make_dec(std::move(type), std::move(names));
}

std::vector<std::string> Parser::parse_form_list() {
    return parse_id_list();
}

std::unique_ptr<AstNode> Parser::parse_program_body() {
    const Token& start = expect(TokenType::Begin, "'begin'");
    auto body = parse_stm_list();
    body->location = start.location;
    expect(TokenType::End, "'end'");
    return body;
}

std::unique_ptr<AstNode> Parser::parse_stm_list() {
    auto list = make_ast_node(AstNodeKind::StmLK, current().location);
    if (!is_statement_start(current().type)) {
        add_syntax_error(current().location,
                         "expected statement, got " + got_text(current()));
        synchronize();
        if (!match(TokenType::Semi) || !is_statement_start(current().type)) {
            return list;
        }
    }

    add_child(*list, parse_statement());
    while (match(TokenType::Semi)) {
        if (!is_statement_start(current().type)) {
            break;
        }
        add_child(*list, parse_statement());
    }
    return list;
}

std::unique_ptr<AstNode> Parser::parse_statement() {
    switch (current().type) {
        case TokenType::If:
            return parse_if_statement();
        case TokenType::While:
            return parse_while_statement();
        case TokenType::Read:
            return parse_read_statement();
        case TokenType::Write:
            return parse_write_statement();
        case TokenType::Return:
            return parse_return_statement();
        case TokenType::Id:
            return parse_id_statement();
        default:
            add_syntax_error(current().location,
                             "expected statement, got " + got_text(current()));
            synchronize();
            return nullptr;
    }
}

std::unique_ptr<AstNode> Parser::parse_if_statement() {
    const Token& start = expect(TokenType::If, "'if'");
    auto node = make_ast_node(AstNodeKind::StmtK, start.location, "If");
    add_child(*node, parse_rel_exp());
    expect(TokenType::Then, "'then'");
    add_child(*node, parse_stm_list());
    expect(TokenType::Else, "'else'");
    add_child(*node, parse_stm_list());
    expect(TokenType::Fi, "'fi'");
    return node;
}

std::unique_ptr<AstNode> Parser::parse_while_statement() {
    const Token& start = expect(TokenType::While, "'while'");
    auto node = make_ast_node(AstNodeKind::StmtK, start.location, "While");
    add_child(*node, parse_rel_exp());
    expect(TokenType::Do, "'do'");
    add_child(*node, parse_stm_list());
    expect(TokenType::EndWh, "'endwh'");
    return node;
}

std::unique_ptr<AstNode> Parser::parse_read_statement() {
    const Token& start = expect(TokenType::Read, "'read'");
    auto node = make_ast_node(AstNodeKind::StmtK, start.location, "Read");
    expect(TokenType::LParen, "'('");
    const Token& id = expect(TokenType::Id, "input variable");
    if (id.type == TokenType::Id) {
        node->names.push_back(id.value);
    }
    expect(TokenType::RParen, "')'");
    return node;
}

std::unique_ptr<AstNode> Parser::parse_write_statement() {
    const Token& start = expect(TokenType::Write, "'write'");
    auto node = make_ast_node(AstNodeKind::StmtK, start.location, "Write");
    expect(TokenType::LParen, "'('");
    add_child(*node, parse_exp());
    expect(TokenType::RParen, "')'");
    return node;
}

std::unique_ptr<AstNode> Parser::parse_return_statement() {
    const Token& start = expect(TokenType::Return, "'return'");
    auto node = make_ast_node(AstNodeKind::StmtK, start.location, "Return");
    expect(TokenType::LParen, "'('");
    add_child(*node, parse_exp());
    expect(TokenType::RParen, "')'");
    return node;
}

std::unique_ptr<AstNode> Parser::parse_id_statement() {
    const Token& id = expect(TokenType::Id, "identifier");
    if (check(TokenType::LParen)) {
        auto call = make_ast_node(AstNodeKind::StmtK, id.location, "Call");
        auto callee = make_exp(id.location, id.value + " IdV");
        add_child(*call, std::move(callee));
        expect(TokenType::LParen, "'('");
        for (auto& arg : parse_act_param_list()) {
            add_child(*call, std::move(arg));
        }
        expect(TokenType::RParen, "')'");
        return call;
    }

    auto assign = make_ast_node(AstNodeKind::StmtK, id.location, "Assign");
    add_child(*assign, parse_variable_after_id(id));
    expect(TokenType::Assign, "':='");
    add_child(*assign, parse_exp());
    return assign;
}

std::vector<std::unique_ptr<AstNode>> Parser::parse_act_param_list() {
    std::vector<std::unique_ptr<AstNode>> args;
    if (check(TokenType::RParen)) {
        return args;
    }

    args.push_back(parse_exp());
    while (match(TokenType::Comma)) {
        args.push_back(parse_exp());
    }
    return args;
}

std::unique_ptr<AstNode> Parser::parse_rel_exp() {
    auto left = parse_exp();
    if (!check_any({TokenType::Lt, TokenType::Eq})) {
        add_syntax_error(current().location,
                         "expected comparison operator, got " + got_text(current()));
        return left;
    }

    const Token& op = advance();
    auto node = make_exp(op.location, "Op " + op.lexeme);
    add_child(*node, std::move(left));
    add_child(*node, parse_exp());
    return node;
}

std::unique_ptr<AstNode> Parser::parse_exp() {
    auto left = parse_term();
    if (check_any({TokenType::Plus, TokenType::Minus})) {
        const Token& op = advance();
        auto node = make_exp(op.location, "Op " + op.lexeme);
        add_child(*node, std::move(left));
        add_child(*node, parse_exp());
        return node;
    }
    return left;
}

std::unique_ptr<AstNode> Parser::parse_term() {
    auto left = parse_factor();
    if (check_any({TokenType::Times, TokenType::Over})) {
        const Token& op = advance();
        auto node = make_exp(op.location, "Op " + op.lexeme);
        add_child(*node, std::move(left));
        add_child(*node, parse_term());
        return node;
    }
    return left;
}

std::unique_ptr<AstNode> Parser::parse_factor() {
    if (match(TokenType::LParen)) {
        auto node = parse_exp();
        expect(TokenType::RParen, "')'");
        return node;
    }
    if (match(TokenType::IntConst)) {
        return make_exp(previous().location, "Const " + previous().value);
    }
    if (match(TokenType::CharConst)) {
        return make_exp(previous().location, "Const '" + previous().value + "'");
    }
    if (match(TokenType::Id)) {
        return parse_variable_after_id(previous());
    }

    add_syntax_error(current().location,
                     "expected expression factor, got " + got_text(current()));
    if (!is_at_end() && !is_sync_token(current().type)) {
        advance();
    }
    return make_exp(previous().location, "Error");
}

std::unique_ptr<AstNode> Parser::parse_variable_after_id(const Token& id) {
    if (match(TokenType::LMidParen)) {
        auto node = make_exp(id.location, id.value + " ArrayMembV");
        add_child(*node, parse_exp());
        expect(TokenType::RMidParen, "']'");
        return node;
    }
    if (match(TokenType::Dot)) {
        auto node = make_exp(id.location, id.value + " FieldMembV");
        const Token& field = expect(TokenType::Id, "field name");
        add_child(*node, parse_field_var(field));
        return node;
    }
    return make_exp(id.location, id.value + " IdV");
}

std::unique_ptr<AstNode> Parser::parse_field_var(const Token& id) {
    auto node = make_exp(id.location, id.value + " IdV");
    if (match(TokenType::LMidParen)) {
        node->detail = id.value + " ArrayMembV";
        add_child(*node, parse_exp());
        expect(TokenType::RMidParen, "']'");
    }
    return node;
}

std::unique_ptr<AstNode> Parser::make_dec(TypeSyntax type, std::vector<std::string> names) {
    auto node = make_ast_node(AstNodeKind::DecK, type.location, std::move(type.detail));
    node->names = std::move(names);
    for (auto& child : type.children) {
        add_child(*node, std::move(child));
    }
    return node;
}

bool Parser::is_statement_start(TokenType type) const {
    return type == TokenType::If || type == TokenType::While ||
           type == TokenType::Read || type == TokenType::Write ||
           type == TokenType::Return || type == TokenType::Id;
}

bool Parser::is_sync_token(TokenType type) const {
    return type == TokenType::Semi || type == TokenType::End ||
           type == TokenType::Else || type == TokenType::Fi ||
           type == TokenType::EndWh || type == TokenType::Dot ||
           type == TokenType::EndFile;
}

void Parser::synchronize() {
    while (!is_at_end() && !is_sync_token(current().type)) {
        advance();
    }
}

void Parser::add_syntax_error(SourceLocation location, const std::string& message) {
    diagnostics_.push_back(Diagnostic{
        DiagnosticStage::Syntax,
        location,
        message,
    });
}

}  // namespace snl
