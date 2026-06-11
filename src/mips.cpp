#include "mips.h"

#include "type.h"

#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace snl {
namespace {

std::string sanitize_label_part(const std::string& text) {
    std::string result;
    for (char ch : text) {
        const unsigned char value = static_cast<unsigned char>(ch);
        if (std::isalnum(value) || ch == '_') {
            result.push_back(ch);
        } else {
            result.push_back('_');
        }
    }
    return result.empty() ? "anon" : result;
}

std::string program_name(const AstNode& root) {
    if (!root.children.empty() && !root.children[0]->names.empty()) {
        return root.children[0]->names.front();
    }
    return "program";
}

const AstNode* find_first_child(const AstNode& node, AstNodeKind kind) {
    for (const auto& child : node.children) {
        if (child && child->kind == kind) {
            return child.get();
        }
    }
    return nullptr;
}

const RecordField* find_record_field(TypePtr record_type, const std::string& name) {
    record_type = resolve_alias(std::move(record_type));
    if (!record_type || record_type->kind != TypeKind::Record) {
        return nullptr;
    }
    for (const RecordField& field : record_type->fields) {
        if (field.name == name) {
            return &field;
        }
    }
    return nullptr;
}

bool is_char_expression(const AstNode& expression) {
    return expression.semantic_type == "char";
}

}  // namespace

MipsResult MipsGenerator::generate(const AstNode& root, const SymbolTable& symbols) {
    symbols_ = &symbols;
    diagnostics_.clear();
    output_.clear();
    label_counter_ = 0;
    symbol_labels_.assign(symbols.symbols().size(), {});
    procedures_.clear();
    procedure_by_node_.clear();
    procedure_by_scope_.clear();
    procedure_by_symbol_.clear();
    current_proc_ = nullptr;

    prepare_global_labels();
    prepare_procedures(root);
    emit_data_section();
    emit_text_section(root);

    MipsResult result;
    result.diagnostics = diagnostics_;
    if (result.diagnostics.empty()) {
        result.assembly = output_;
    }
    return result;
}

void MipsGenerator::prepare_global_labels() {
    if (!symbols_ || symbols_->scopes().empty()) {
        return;
    }

    const Scope& global = symbols_->scopes().front();
    for (int symbol_id : global.symbols) {
        const Symbol* symbol = symbols_->get(symbol_id);
        if (!symbol || symbol->kind != SymbolKind::Variable) {
            continue;
        }
        if (symbol->id >= 0 && static_cast<std::size_t>(symbol->id) < symbol_labels_.size()) {
            symbol_labels_[static_cast<std::size_t>(symbol->id)] =
                "_snl_var_" + std::to_string(symbol->id) + "_" +
                sanitize_label_part(symbol->name);
        }
    }
}

void MipsGenerator::prepare_procedures(const AstNode& root) {
    ProcInfo main_info = make_proc_info(&root, 0, -1, -1, program_name(root), "main");
    procedures_.push_back(std::move(main_info));
    procedure_by_node_[&root] = 0;
    procedure_by_scope_[0] = 0;

    collect_procedures(root, 0);
}

void MipsGenerator::collect_procedures(const AstNode& owner, int owner_scope_id) {
    for (const auto& child : owner.children) {
        if (!child || child->kind != AstNodeKind::ProcDecK) {
            continue;
        }

        const std::string name = child->names.empty() ? "procedure" : child->names.front();
        const int scope_id = static_cast<int>(procedures_.size());
        const int symbol_id = find_procedure_symbol_id(name, owner_scope_id);
        ProcInfo info = make_proc_info(child.get(),
                                       scope_id,
                                       owner_scope_id,
                                       symbol_id,
                                       name,
                                       "_snl_proc_" + std::to_string(scope_id) +
                                           "_" + sanitize_label_part(name));
        const int index = static_cast<int>(procedures_.size());
        procedures_.push_back(std::move(info));
        procedure_by_node_[child.get()] = index;
        procedure_by_scope_[scope_id] = index;
        if (symbol_id >= 0) {
            procedure_by_symbol_[symbol_id] = index;
        }

        collect_procedures(*child, scope_id);
    }
}

MipsGenerator::ProcInfo MipsGenerator::make_proc_info(const AstNode* node,
                                                      int scope_id,
                                                      int parent_scope_id,
                                                      int symbol_id,
                                                      std::string name,
                                                      std::string label) {
    ProcInfo info;
    info.node = node;
    info.scope_id = scope_id;
    info.parent_scope_id = parent_scope_id;
    info.symbol_id = symbol_id;
    info.name = std::move(name);
    info.label = std::move(label);
    info.epilogue_label = new_label("epilogue_" + sanitize_label_part(info.name));

    if (!symbols_ || scope_id < 0 ||
        static_cast<std::size_t>(scope_id) >= symbols_->scopes().size()) {
        return info;
    }

    const Scope& scope = symbols_->scopes()[static_cast<std::size_t>(scope_id)];
    info.level = scope.level;
    int local_size = 0;
    for (int id : scope.symbols) {
        const Symbol* symbol = symbols_->get(id);
        if (!symbol) {
            continue;
        }
        if (symbol->kind == SymbolKind::Parameter) {
            info.parameter_size += symbol->size;
        } else if (symbol->kind == SymbolKind::Variable) {
            local_size += symbol->size;
            info.local_offsets[symbol->id] = -local_size;
        }
    }
    info.local_size = local_size;
    return info;
}

int MipsGenerator::find_procedure_symbol_id(const std::string& name,
                                            int parent_scope_id) const {
    if (!symbols_ || parent_scope_id < 0 ||
        static_cast<std::size_t>(parent_scope_id) >= symbols_->scopes().size()) {
        return -1;
    }

    const Scope& scope = symbols_->scopes()[static_cast<std::size_t>(parent_scope_id)];
    const auto found = scope.by_name.find(name);
    if (found == scope.by_name.end()) {
        return -1;
    }
    const Symbol* symbol = symbols_->get(found->second);
    return symbol && symbol->kind == SymbolKind::Procedure ? symbol->id : -1;
}

void MipsGenerator::emit_data_section() {
    emit_line(".data");
    if (!symbols_ || symbols_->scopes().empty()) {
        emit_line("");
        return;
    }

    const Scope& global = symbols_->scopes().front();
    for (int symbol_id : global.symbols) {
        const Symbol* symbol = symbols_->get(symbol_id);
        if (!symbol || symbol->kind != SymbolKind::Variable) {
            continue;
        }

        const std::string label = label_for_symbol(*symbol);
        if (label.empty()) {
            continue;
        }
        if (symbol->size == 4) {
            emit_line(label + ": .word 0");
        } else if (symbol->size > 0) {
            emit_line(label + ": .space " + std::to_string(symbol->size));
        } else {
            emit_error(SourceLocation{}, "cannot allocate variable '" + symbol->name + "'");
        }
    }
    emit_line("");
}

void MipsGenerator::emit_text_section(const AstNode& root) {
    emit_line(".text");
    emit_main(root);
    for (std::size_t i = 1; i < procedures_.size(); ++i) {
        emit_line("");
        emit_procedure(procedures_[i]);
    }
}

void MipsGenerator::emit_main(const AstNode& root) {
    const AstNode* body = find_main_statement_list(root);
    if (!body) {
        emit_error(root.location, "missing main statement list");
        return;
    }

    emit_line(".globl main");
    emit_line("main:");
    current_proc_ = procedure_for_scope(0);
    emit_statement_list(*body);
    emit_line("    li $v0, 10");
    emit_line("    syscall");
    current_proc_ = nullptr;
}

void MipsGenerator::emit_procedure(const ProcInfo& procedure) {
    const AstNode* body = procedure.node ? find_first_child(*procedure.node, AstNodeKind::StmLK) : nullptr;
    if (!body) {
        emit_error(procedure.node ? procedure.node->location : SourceLocation{},
                   "missing procedure body for '" + procedure.name + "'");
        return;
    }

    emit_line(procedure.label + ":");
    current_proc_ = &procedure;
    emit_prologue(procedure);
    emit_statement_list(*body);
    emit_line(procedure.epilogue_label + ":");
    emit_epilogue(procedure);
    current_proc_ = nullptr;
}

void MipsGenerator::emit_prologue(const ProcInfo& procedure) {
    emit_line("    addiu $sp, $sp, -8");
    emit_line("    sw $fp, 4($sp)");
    emit_line("    sw $ra, 0($sp)");
    emit_line("    move $fp, $sp");
    if (procedure.local_size > 0) {
        emit_line("    addiu $sp, $sp, -" + std::to_string(procedure.local_size));
    }
}

void MipsGenerator::emit_epilogue(const ProcInfo&) {
    emit_line("    move $sp, $fp");
    emit_line("    lw $ra, 0($fp)");
    emit_line("    lw $fp, 4($fp)");
    emit_line("    jr $ra");
}

void MipsGenerator::emit_statement_list(const AstNode& list) {
    for (const auto& child : list.children) {
        if (child) {
            emit_statement(*child);
        }
    }
}

void MipsGenerator::emit_statement(const AstNode& statement) {
    if (statement.detail == "Assign") {
        if (statement.children.size() < 2) {
            emit_error(statement.location, "malformed assignment statement");
            return;
        }

        const int lhs_size = size_for_type(type_for_expression(*statement.children[0]));
        if (lhs_size > 4) {
            emit_address(*statement.children[0]);
            emit_line("    addiu $sp, $sp, -4");
            emit_line("    sw $t0, 0($sp)");
            emit_address(*statement.children[1]);
            emit_line("    move $t1, $t0");
            emit_line("    lw $t0, 0($sp)");
            emit_line("    addiu $sp, $sp, 4");
            emit_copy_bytes("$t0", "$t1", lhs_size);
            return;
        }

        emit_expression(*statement.children[1]);
        emit_line("    addiu $sp, $sp, -4");
        emit_line("    sw $t0, 0($sp)");
        emit_address(*statement.children[0]);
        emit_line("    lw $t1, 0($sp)");
        emit_line("    addiu $sp, $sp, 4");
        emit_line("    sw $t1, 0($t0)");
        return;
    }

    if (statement.detail == "Read") {
        if (statement.names.empty()) {
            emit_error(statement.location, "malformed read statement");
            return;
        }

        const Symbol* symbol = find_visible_variable_symbol(
            statement.names.front(),
            current_proc_ ? current_proc_->scope_id : 0);
        if (!symbol) {
            emit_error(statement.location,
                       "cannot resolve read target '" + statement.names.front() + "'");
            return;
        }
        TypePtr type = resolve_alias(symbol->type);
        if (!type || (type->kind != TypeKind::Integer && type->kind != TypeKind::Char)) {
            emit_error(statement.location,
                       "read currently supports integer or char variables only");
            return;
        }

        emit_line(std::string("    li $v0, ") + (type->kind == TypeKind::Char ? "12" : "5"));
        emit_line("    syscall");
        emit_symbol_address(*symbol);
        emit_line("    sw $v0, 0($t0)");
        return;
    }

    if (statement.detail == "Write") {
        if (statement.children.empty()) {
            emit_error(statement.location, "malformed write statement");
            return;
        }

        const AstNode& value = *statement.children[0];
        emit_expression(value);
        emit_line("    move $a0, $t0");
        emit_line(std::string("    li $v0, ") + (is_char_expression(value) ? "11" : "1"));
        emit_line("    syscall");
        return;
    }

    if (statement.detail == "If") {
        if (statement.children.size() < 3) {
            emit_error(statement.location, "malformed if statement");
            return;
        }

        const std::optional<int> condition = constant_value(*statement.children[0]);
        if (condition) {
            emit_statement_list(*statement.children[*condition ? 1 : 2]);
            return;
        }

        const std::string else_label = new_label("else");
        const std::string end_label = new_label("endif");
        emit_expression(*statement.children[0]);
        emit_line("    beq $t0, $zero, " + else_label);
        emit_statement_list(*statement.children[1]);
        emit_line("    j " + end_label);
        emit_line(else_label + ":");
        emit_statement_list(*statement.children[2]);
        emit_line(end_label + ":");
        return;
    }

    if (statement.detail == "While") {
        if (statement.children.size() < 2) {
            emit_error(statement.location, "malformed while statement");
            return;
        }

        const std::optional<int> condition = constant_value(*statement.children[0]);
        if (condition && *condition == 0) {
            return;
        }

        const std::string begin_label = new_label("while");
        const std::string end_label = new_label("endwhile");
        emit_line(begin_label + ":");
        emit_expression(*statement.children[0]);
        emit_line("    beq $t0, $zero, " + end_label);
        emit_statement_list(*statement.children[1]);
        emit_line("    j " + begin_label);
        emit_line(end_label + ":");
        return;
    }

    if (statement.detail == "Call") {
        emit_call(statement);
        return;
    }

    if (statement.detail == "Return") {
        if (!statement.children.empty()) {
            emit_expression(*statement.children[0]);
        }
        if (current_proc_ && current_proc_->scope_id != 0) {
            emit_line("    j " + current_proc_->epilogue_label);
        } else {
            emit_line("    li $v0, 10");
            emit_line("    syscall");
        }
        return;
    }

    emit_error(statement.location,
               "statement '" + statement.detail + "' is not supported in this MIPS stage");
}

void MipsGenerator::emit_expression(const AstNode& expression) {
    const std::optional<int> folded = constant_value(expression);
    if (folded) {
        emit_line("    li $t0, " + std::to_string(*folded));
        return;
    }

    if (starts_with(expression.detail, "Const '")) {
        const int value = expression.detail.size() >= 9
                              ? static_cast<unsigned char>(expression.detail[7])
                              : 0;
        emit_line("    li $t0, " + std::to_string(value));
        return;
    }

    if (starts_with(expression.detail, "Const ")) {
        emit_line("    li $t0, " + expression.detail.substr(6));
        return;
    }

    if (starts_with(expression.detail, "Op ")) {
        if (emit_simplified_expression(expression)) {
            return;
        }

        if (expression.children.size() < 2) {
            emit_error(expression.location,
                       "operator expression '" + expression.detail + "' is missing operands");
            emit_line("    li $t0, 0");
            return;
        }

        const std::string op = expression.detail.substr(3);
        emit_expression(*expression.children[0]);
        emit_line("    addiu $sp, $sp, -4");
        emit_line("    sw $t0, 0($sp)");
        emit_expression(*expression.children[1]);
        emit_line("    lw $t1, 0($sp)");
        emit_line("    addiu $sp, $sp, 4");

        if (op == "+") {
            emit_line("    addu $t0, $t1, $t0");
        } else if (op == "-") {
            emit_line("    subu $t0, $t1, $t0");
        } else if (op == "*") {
            emit_line("    mul $t0, $t1, $t0");
        } else if (op == "/") {
            emit_line("    div $t1, $t0");
            emit_line("    mflo $t0");
        } else if (op == "<") {
            emit_line("    slt $t0, $t1, $t0");
        } else if (op == "=") {
            emit_line("    xor $t0, $t1, $t0");
            emit_line("    sltiu $t0, $t0, 1");
        } else {
            emit_error(expression.location,
                       "operator '" + op + "' is not supported in this MIPS stage");
            emit_line("    li $t0, 0");
        }
        return;
    }

    if (size_for_type(type_for_expression(expression)) > 4) {
        emit_error(expression.location,
                   "aggregate expression '" + expression.detail +
                   "' cannot be used as a scalar value");
        emit_line("    li $t0, 0");
        return;
    }

    emit_address(expression);
    emit_line("    lw $t0, 0($t0)");
}

void MipsGenerator::emit_address(const AstNode& expression) {
    const std::vector<std::string> words = split_words(expression.detail);
    if (words.size() < 2) {
        emit_error(expression.location,
                   "variable form '" + expression.detail +
                   "' is not supported in this MIPS stage");
        emit_line("    li $t0, 0");
        return;
    }

    const Symbol* symbol = symbol_for_expression(expression);
    if (!symbol) {
        emit_line("    li $t0, 0");
        return;
    }

    const std::string kind = words[1];
    if (kind == "IdV") {
        emit_symbol_address(*symbol);
        return;
    }

    if (kind == "ArrayMembV") {
        TypePtr array_type = resolve_alias(symbol->type);
        if (!array_type || array_type->kind != TypeKind::Array) {
            emit_error(expression.location,
                       "'" + symbol->name + "' is not an array");
            emit_line("    li $t0, 0");
            return;
        }
        emit_symbol_address(*symbol);
        emit_line("    addiu $sp, $sp, -4");
        emit_line("    sw $t0, 0($sp)");
        if (!expression.children.empty()) {
            emit_expression(*expression.children[0]);
        } else {
            emit_line("    li $t0, 0");
        }
        emit_line("    lw $t1, 0($sp)");
        emit_line("    addiu $sp, $sp, 4");
        if (array_type->low != 0) {
            emit_line("    addiu $t0, $t0, -" + std::to_string(array_type->low));
        }
        emit_line("    li $t2, " + std::to_string(size_for_type(array_type->element_type)));
        emit_line("    mul $t0, $t0, $t2");
        emit_line("    addu $t0, $t1, $t0");
        return;
    }

    if (kind == "FieldMembV") {
        TypePtr record_type = resolve_alias(symbol->type);
        if (!record_type || record_type->kind != TypeKind::Record ||
            expression.children.empty()) {
            emit_error(expression.location,
                       "'" + symbol->name + "' is not a record");
            emit_line("    li $t0, 0");
            return;
        }

        const AstNode& field_node = *expression.children[0];
        const std::string field_name = first_word(field_node.detail);
        const RecordField* field = find_record_field(record_type, field_name);
        if (!field) {
            emit_error(field_node.location,
                       "record field '" + field_name + "' does not exist");
            emit_line("    li $t0, 0");
            return;
        }

        emit_symbol_address(*symbol);
        if (field->offset != 0) {
            emit_line("    addiu $t0, $t0, " + std::to_string(field->offset));
        }

        const std::vector<std::string> field_words = split_words(field_node.detail);
        if (field_words.size() >= 2 && field_words[1] == "ArrayMembV") {
            TypePtr array_type = resolve_alias(field->type);
            if (!array_type || array_type->kind != TypeKind::Array) {
                emit_error(field_node.location,
                           "field '" + field_name + "' is not an array");
                return;
            }
            emit_line("    addiu $sp, $sp, -4");
            emit_line("    sw $t0, 0($sp)");
            if (!field_node.children.empty()) {
                emit_expression(*field_node.children[0]);
            } else {
                emit_line("    li $t0, 0");
            }
            emit_line("    lw $t1, 0($sp)");
            emit_line("    addiu $sp, $sp, 4");
            if (array_type->low != 0) {
                emit_line("    addiu $t0, $t0, -" + std::to_string(array_type->low));
            }
            emit_line("    li $t2, " + std::to_string(size_for_type(array_type->element_type)));
            emit_line("    mul $t0, $t0, $t2");
            emit_line("    addu $t0, $t1, $t0");
        }
        return;
    }

    emit_error(expression.location,
               "variable form '" + expression.detail +
               "' is not supported in this MIPS stage");
    emit_line("    li $t0, 0");
}

void MipsGenerator::emit_symbol_address(const Symbol& symbol) {
    if (symbol.scope_id == 0) {
        const std::string label = label_for_symbol(symbol);
        if (label.empty()) {
            emit_error(SourceLocation{},
                       "variable '" + symbol.name + "' has no MIPS storage label");
            emit_line("    li $t0, 0");
            return;
        }
        emit_line("    la $t0, " + label);
        return;
    }

    emit_frame_pointer_for_scope(symbol.scope_id, "$t0");
    if (symbol.kind == SymbolKind::Parameter) {
        const int offset = 8 + symbol.offset;
        if (symbol.is_var_param) {
            emit_line("    lw $t0, " + std::to_string(offset) + "($t0)");
        } else {
            emit_line("    addiu $t0, $t0, " + std::to_string(offset));
        }
        return;
    }

    const ProcInfo* owner = procedure_for_scope(symbol.scope_id);
    if (!owner) {
        emit_error(SourceLocation{},
                   "cannot find frame owner for variable '" + symbol.name + "'");
        emit_line("    li $t0, 0");
        return;
    }
    const auto found = owner->local_offsets.find(symbol.id);
    if (found == owner->local_offsets.end()) {
        emit_error(SourceLocation{},
                   "cannot find local offset for variable '" + symbol.name + "'");
        emit_line("    li $t0, 0");
        return;
    }
    emit_line("    addiu $t0, $t0, " + std::to_string(found->second));
}

void MipsGenerator::emit_frame_pointer_for_scope(int target_scope_id,
                                                 const std::string& target_register) {
    if (!current_proc_) {
        emit_error(SourceLocation{}, "missing current procedure context");
        emit_line("    li " + target_register + ", 0");
        return;
    }
    if (target_scope_id == 0) {
        emit_line("    li " + target_register + ", 0");
        return;
    }
    if (current_proc_->scope_id == 0) {
        emit_error(SourceLocation{}, "main has no frame for local scope access");
        emit_line("    li " + target_register + ", 0");
        return;
    }

    emit_line("    move " + target_register + ", $fp");
    int scope_id = current_proc_->scope_id;
    while (scope_id != target_scope_id) {
        const ProcInfo* procedure = procedure_for_scope(scope_id);
        if (!procedure || procedure->parent_scope_id < 0) {
            emit_error(SourceLocation{}, "cannot follow static link to target scope");
            return;
        }
        emit_line("    lw " + target_register + ", " +
                  std::to_string(8 + procedure->parameter_size) +
                  "(" + target_register + ")");
        scope_id = procedure->parent_scope_id;
        if (scope_id == 0 && target_scope_id != 0) {
            emit_error(SourceLocation{}, "target scope is not an enclosing scope");
            return;
        }
    }
}

void MipsGenerator::emit_call(const AstNode& statement) {
    if (statement.children.empty()) {
        emit_error(statement.location, "malformed call statement");
        return;
    }

    const std::string callee_name = first_word(statement.children[0]->detail);
    const int current_scope = current_proc_ ? current_proc_->scope_id : 0;
    const Symbol* callee = find_visible_symbol(callee_name, current_scope, SymbolKind::Procedure);
    if (!callee) {
        emit_error(statement.location, "cannot resolve procedure '" + callee_name + "'");
        return;
    }

    TypePtr proc_type = resolve_alias(callee->type);
    const ProcInfo* callee_info = procedure_for_symbol(callee->id);
    if (!proc_type || proc_type->kind != TypeKind::Procedure || !callee_info) {
        emit_error(statement.location, "'" + callee_name + "' is not a code-generatable procedure");
        return;
    }

    const int argument_size = proc_type->size;
    const int call_area_size = argument_size + 4;
    if (call_area_size > 0) {
        emit_line("    addiu $sp, $sp, -" + std::to_string(call_area_size));
    }

    const std::size_t expected = proc_type->parameters.size();
    const std::size_t actual = statement.children.size() - 1;
    const std::size_t count = expected < actual ? expected : actual;
    for (std::size_t i = 0; i < count; ++i) {
        emit_argument(*statement.children[i + 1],
                      proc_type->parameters[i],
                      proc_type->parameters[i].offset);
    }

    if (callee->scope_id == 0) {
        emit_line("    sw $zero, " + std::to_string(argument_size) + "($sp)");
    } else {
        emit_frame_pointer_for_scope(callee->scope_id, "$t0");
        emit_line("    sw $t0, " + std::to_string(argument_size) + "($sp)");
    }
    emit_line("    jal " + callee_info->label);
    if (call_area_size > 0) {
        emit_line("    addiu $sp, $sp, " + std::to_string(call_area_size));
    }
}

void MipsGenerator::emit_argument(const AstNode& argument,
                                  const ParameterType& parameter,
                                  int argument_offset) {
    const int size = parameter.size;
    if (parameter.is_var) {
        emit_address(argument);
        emit_line("    sw $t0, " + std::to_string(argument_offset) + "($sp)");
        return;
    }

    if (size <= 4) {
        emit_expression(argument);
        emit_line("    sw $t0, " + std::to_string(argument_offset) + "($sp)");
        return;
    }

    emit_address(argument);
    emit_line("    move $t1, $t0");
    emit_line("    addiu $t0, $sp, " + std::to_string(argument_offset));
    emit_copy_bytes("$t0", "$t1", size);
}

void MipsGenerator::emit_copy_bytes(const std::string& destination_register,
                                    const std::string& source_register,
                                    int byte_count) {
    for (int offset = 0; offset < byte_count; offset += 4) {
        emit_line("    lw $t2, " + std::to_string(offset) + "(" + source_register + ")");
        emit_line("    sw $t2, " + std::to_string(offset) + "(" + destination_register + ")");
    }
}

const AstNode* MipsGenerator::find_main_statement_list(const AstNode& root) const {
    for (const auto& child : root.children) {
        if (child && child->kind == AstNodeKind::StmLK) {
            return child.get();
        }
    }
    return nullptr;
}

const Symbol* MipsGenerator::find_visible_symbol(const std::string& name,
                                                 int scope_id,
                                                 SymbolKind required_kind) const {
    if (!symbols_) {
        return nullptr;
    }

    while (scope_id >= 0 &&
           static_cast<std::size_t>(scope_id) < symbols_->scopes().size()) {
        const Scope& scope = symbols_->scopes()[static_cast<std::size_t>(scope_id)];
        const auto found = scope.by_name.find(name);
        if (found != scope.by_name.end()) {
            const Symbol* symbol = symbols_->get(found->second);
            if (symbol && symbol->kind == required_kind) {
                return symbol;
            }
        }
        scope_id = scope.parent;
    }
    return nullptr;
}

const Symbol* MipsGenerator::find_visible_variable_symbol(const std::string& name,
                                                          int scope_id) const {
    if (!symbols_) {
        return nullptr;
    }

    while (scope_id >= 0 &&
           static_cast<std::size_t>(scope_id) < symbols_->scopes().size()) {
        const Scope& scope = symbols_->scopes()[static_cast<std::size_t>(scope_id)];
        const auto found = scope.by_name.find(name);
        if (found != scope.by_name.end()) {
            const Symbol* symbol = symbols_->get(found->second);
            if (symbol && (symbol->kind == SymbolKind::Variable ||
                           symbol->kind == SymbolKind::Parameter)) {
                return symbol;
            }
            return nullptr;
        }
        scope_id = scope.parent;
    }
    return nullptr;
}

const Symbol* MipsGenerator::symbol_for_expression(const AstNode& expression) {
    const std::string name = first_word(expression.detail);
    const Symbol* symbol = symbols_ ? symbols_->get(expression.semantic_symbol_id) : nullptr;
    if (!symbol) {
        const int scope = current_proc_ ? current_proc_->scope_id : 0;
        symbol = find_visible_variable_symbol(name, scope);
    }
    if (!symbol || (symbol->kind != SymbolKind::Variable &&
                    symbol->kind != SymbolKind::Parameter)) {
        emit_error(expression.location,
                   "cannot resolve variable '" + name + "'");
        return nullptr;
    }
    return symbol;
}

const MipsGenerator::ProcInfo* MipsGenerator::procedure_for_scope(int scope_id) const {
    const auto found = procedure_by_scope_.find(scope_id);
    if (found == procedure_by_scope_.end()) {
        return nullptr;
    }
    return &procedures_[static_cast<std::size_t>(found->second)];
}

const MipsGenerator::ProcInfo* MipsGenerator::procedure_for_symbol(int symbol_id) const {
    const auto found = procedure_by_symbol_.find(symbol_id);
    if (found == procedure_by_symbol_.end()) {
        return nullptr;
    }
    return &procedures_[static_cast<std::size_t>(found->second)];
}

TypePtr MipsGenerator::type_for_expression(const AstNode& expression) {
    if (starts_with(expression.detail, "Const '")) {
        return make_primitive_type(TypeKind::Char);
    }
    if (starts_with(expression.detail, "Const ")) {
        return make_primitive_type(TypeKind::Integer);
    }
    if (starts_with(expression.detail, "Op ")) {
        const std::string op = expression.detail.substr(3);
        return make_primitive_type(op == "+" || op == "-" || op == "*" || op == "/"
                                       ? TypeKind::Integer
                                       : TypeKind::Boolean);
    }

    const Symbol* symbol = symbol_for_expression(expression);
    if (!symbol) {
        return make_error_type();
    }

    TypePtr type = symbol->type;
    const std::vector<std::string> words = split_words(expression.detail);
    if (words.size() < 2) {
        return type;
    }

    if (words[1] == "ArrayMembV") {
        TypePtr array_type = resolve_alias(type);
        return array_type && array_type->kind == TypeKind::Array
                   ? array_type->element_type
                   : make_error_type();
    }

    if (words[1] == "FieldMembV" && !expression.children.empty()) {
        TypePtr record_type = resolve_alias(type);
        const std::string field_name = first_word(expression.children[0]->detail);
        const RecordField* field = find_record_field(record_type, field_name);
        if (!field) {
            return make_error_type();
        }
        TypePtr field_type = field->type;
        const std::vector<std::string> field_words = split_words(expression.children[0]->detail);
        if (field_words.size() >= 2 && field_words[1] == "ArrayMembV") {
            TypePtr array_type = resolve_alias(field_type);
            return array_type && array_type->kind == TypeKind::Array
                       ? array_type->element_type
                       : make_error_type();
        }
        return field_type;
    }

    return type;
}

TypePtr MipsGenerator::type_for_symbol(const Symbol& symbol) const {
    return symbol.type;
}

int MipsGenerator::size_for_type(TypePtr type) const {
    type = resolve_alias(std::move(type));
    return type ? type->size : 0;
}

std::string MipsGenerator::label_for_symbol(const Symbol& symbol) const {
    if (symbol.id < 0 || static_cast<std::size_t>(symbol.id) >= symbol_labels_.size()) {
        return {};
    }
    return symbol_labels_[static_cast<std::size_t>(symbol.id)];
}

std::string MipsGenerator::new_label(const std::string& prefix) {
    return "_snl_" + prefix + "_" + std::to_string(label_counter_++);
}

bool MipsGenerator::starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::vector<std::string> MipsGenerator::split_words(const std::string& text) {
    std::istringstream input(text);
    std::vector<std::string> words;
    std::string word;
    while (input >> word) {
        words.push_back(word);
    }
    return words;
}

std::string MipsGenerator::first_word(const std::string& text) {
    std::vector<std::string> words = split_words(text);
    return words.empty() ? "" : words.front();
}

void MipsGenerator::emit_line(const std::string& line) {
    output_ += line;
    output_ += '\n';
}

void MipsGenerator::emit_error(SourceLocation location, const std::string& message) {
    diagnostics_.push_back(Diagnostic{DiagnosticStage::Codegen, location, message});
}

}  // namespace snl
