#pragma once

#include "ast.h"
#include "diagnostic.h"
#include "symbol_table.h"

#include <optional>
#include <string>
#include <vector>

namespace snl {

struct MipsResult {
    std::string assembly;
    std::vector<Diagnostic> diagnostics;
};

class MipsGenerator {
public:
    MipsResult generate(const AstNode& root, const SymbolTable& symbols);

private:
    const SymbolTable* symbols_ = nullptr;
    std::vector<Diagnostic> diagnostics_;
    std::string output_;
    int label_counter_ = 0;
    std::vector<std::string> symbol_labels_;

    void prepare_global_labels();
    void emit_data_section();
    void emit_text_section(const AstNode& root);
    void emit_statement_list(const AstNode& list);
    void emit_statement(const AstNode& statement);
    void emit_expression(const AstNode& expression);
    bool emit_simplified_expression(const AstNode& expression);

    const AstNode* find_main_statement_list(const AstNode& root) const;
    const Symbol* find_global_symbol(const std::string& name) const;
    const Symbol* symbol_for_expression(const AstNode& expression);
    std::optional<int> constant_value(const AstNode& expression) const;
    bool same_storage(const AstNode& left, const AstNode& right) const;
    bool expression_preserves_storage(const AstNode& storage,
                                      const AstNode& expression) const;
    std::string label_for_symbol(const Symbol& symbol) const;
    std::string new_label(const std::string& prefix);

    void emit_line(const std::string& line);
    void emit_error(SourceLocation location, const std::string& message);
};

}  // namespace snl
