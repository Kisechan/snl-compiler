#pragma once

#include "ast.h"
#include "diagnostic.h"
#include "symbol_table.h"

#include <optional>
#include <string>
#include <unordered_map>
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
    struct ProcInfo {
        const AstNode* node = nullptr;
        int scope_id = -1;
        int parent_scope_id = -1;
        int symbol_id = -1;
        int level = 0;
        int parameter_size = 0;
        int local_size = 0;
        std::string name;
        std::string label;
        std::string epilogue_label;
        std::unordered_map<int, int> local_offsets;
    };

    const SymbolTable* symbols_ = nullptr;
    std::vector<Diagnostic> diagnostics_;
    std::string output_;
    int label_counter_ = 0;
    std::vector<std::string> symbol_labels_;
    std::vector<ProcInfo> procedures_;
    std::unordered_map<const AstNode*, int> procedure_by_node_;
    std::unordered_map<int, int> procedure_by_scope_;
    std::unordered_map<int, int> procedure_by_symbol_;
    const ProcInfo* current_proc_ = nullptr;

    void prepare_global_labels();
    void prepare_procedures(const AstNode& root);
    void collect_procedures(const AstNode& owner, int owner_scope_id);
    ProcInfo make_proc_info(const AstNode* node,
                            int scope_id,
                            int parent_scope_id,
                            int symbol_id,
                            std::string name,
                            std::string label);
    int find_procedure_symbol_id(const std::string& name, int parent_scope_id) const;
    void emit_data_section();
    void emit_text_section(const AstNode& root);
    void emit_main(const AstNode& root);
    void emit_procedure(const ProcInfo& procedure);
    void emit_prologue(const ProcInfo& procedure);
    void emit_epilogue(const ProcInfo& procedure);
    void emit_statement_list(const AstNode& list);
    void emit_statement(const AstNode& statement);
    void emit_expression(const AstNode& expression);
    void emit_address(const AstNode& expression);
    void emit_symbol_address(const Symbol& symbol);
    void emit_frame_pointer_for_scope(int target_scope_id, const std::string& target_register);
    void emit_call(const AstNode& statement);
    void emit_argument(const AstNode& argument,
                       const ParameterType& parameter,
                       int argument_offset);
    void emit_copy_bytes(const std::string& destination_register,
                         const std::string& source_register,
                         int byte_count);

    const AstNode* find_main_statement_list(const AstNode& root) const;
    const Symbol* find_visible_symbol(const std::string& name,
                                      int scope_id,
                                      SymbolKind required_kind) const;
    const Symbol* find_visible_variable_symbol(const std::string& name,
                                               int scope_id) const;
    const Symbol* symbol_for_expression(const AstNode& expression);
    const ProcInfo* procedure_for_scope(int scope_id) const;
    const ProcInfo* procedure_for_symbol(int symbol_id) const;
    TypePtr type_for_expression(const AstNode& expression);
    TypePtr type_for_symbol(const Symbol& symbol) const;
    int size_for_type(TypePtr type) const;
    std::string label_for_symbol(const Symbol& symbol) const;
    std::string new_label(const std::string& prefix);
    static bool starts_with(const std::string& text, const std::string& prefix);
    static std::vector<std::string> split_words(const std::string& text);
    static std::string first_word(const std::string& text);

    void emit_line(const std::string& line);
    void emit_error(SourceLocation location, const std::string& message);
};

}  // namespace snl
