#pragma once

#include "ast.h"
#include "diagnostic.h"
#include "symbol_table.h"
#include "type.h"

#include <vector>

namespace snl {

struct SemanticResult {
    std::vector<Diagnostic> diagnostics;
    SymbolTable symbols;
};

class SemanticAnalyzer {
public:
    SemanticResult analyze(AstNode& root);

private:
    struct ExprResult {
        TypePtr type;
        bool is_lvalue = false;
        int symbol_id = -1;
        std::string var_kind;
    };

    void analyze_program(AstNode& root);
    void process_declarations(AstNode& owner, std::size_t first_child);
    void process_type_section(AstNode& type_section);
    void process_var_section(AstNode& var_section);
    void process_proc_declaration(AstNode& proc);
    void process_statement_list(AstNode& list);
    void process_statement(AstNode& statement);

    ExprResult analyze_expression(AstNode& expression);
    ExprResult analyze_variable(AstNode& expression);
    ExprResult analyze_field_access(AstNode& base, AstNode& field_node, TypePtr base_type);

    TypePtr build_type_from_dec(AstNode& dec);
    TypePtr build_type_from_detail(const std::string& detail, const AstNode& node);
    std::vector<RecordField> build_record_fields(const AstNode& record_node);
    std::vector<ParameterType> collect_parameters(AstNode& proc);

    bool is_param_dec(const AstNode& node) const;
    std::size_t first_non_param_child(AstNode& proc) const;
    AstNode* find_first_child(AstNode& node, AstNodeKind kind, std::size_t start = 0);
    std::string first_word(const std::string& text) const;
    std::string strip_param_prefix(const std::string& detail, bool& is_var) const;
    bool starts_with(const std::string& text, const std::string& prefix) const;
    int parse_int(const std::string& text) const;

    void add_error(SourceLocation location, std::string message);
    Symbol* require_symbol(const std::string& name, SourceLocation location);
    const RecordField* find_field(TypePtr record_type, const std::string& name) const;

    SymbolTable table_;
    std::vector<Diagnostic> diagnostics_;
    TypePtr integer_type_;
    TypePtr char_type_;
    TypePtr boolean_type_;
    TypePtr error_type_;
};

}  // namespace snl
