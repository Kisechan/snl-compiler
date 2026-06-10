#include "mips.h"

#include "type.h"

#include <cctype>
#include <optional>
#include <sstream>
#include <string>

namespace snl {
namespace {

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::vector<std::string> split_words(const std::string& text) {
    std::istringstream input(text);
    std::vector<std::string> words;
    std::string word;
    while (input >> word) {
        words.push_back(word);
    }
    return words;
}

std::string first_word(const std::string& text) {
    std::vector<std::string> words = split_words(text);
    return words.empty() ? "" : words.front();
}

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

    prepare_global_labels();
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
    const AstNode* body = find_main_statement_list(root);
    if (!body) {
        emit_error(root.location, "missing main statement list");
        return;
    }

    emit_line(".text");
    emit_line(".globl main");
    emit_line("main:");
    emit_statement_list(*body);
    emit_line("    li $v0, 10");
    emit_line("    syscall");
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

        const AstNode& lhs = *statement.children[0];
        const AstNode& rhs = *statement.children[1];
        if (expression_preserves_storage(lhs, rhs)) {
            return;
        }

        const Symbol* symbol = symbol_for_expression(lhs);
        if (!symbol) {
            return;
        }

        emit_expression(rhs);
        emit_line("    sw $t0, " + label_for_symbol(*symbol));
        return;
    }

    if (statement.detail == "Read") {
        if (statement.names.empty()) {
            emit_error(statement.location, "malformed read statement");
            return;
        }

        const Symbol* symbol = find_global_symbol(statement.names.front());
        if (!symbol) {
            emit_error(statement.location,
                       "read only supports main-scope variables in this MIPS stage");
            return;
        }
        TypePtr type = resolve_alias(symbol->type);
        if (!type || type->kind != TypeKind::Integer) {
            emit_error(statement.location,
                       "read currently supports integer variables only");
            return;
        }

        emit_line("    li $v0, 5");
        emit_line("    syscall");
        emit_line("    sw $v0, " + label_for_symbol(*symbol));
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

    emit_error(statement.location,
               "statement '" + statement.detail +
               "' is not supported in this MIPS stage");
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

    const Symbol* symbol = symbol_for_expression(expression);
    if (!symbol) {
        emit_line("    li $t0, 0");
        return;
    }
    emit_line("    lw $t0, " + label_for_symbol(*symbol));
}

bool MipsGenerator::emit_simplified_expression(const AstNode& expression) {
    if (!starts_with(expression.detail, "Op ") || expression.children.size() < 2) {
        return false;
    }

    const std::string op = expression.detail.substr(3);
    const AstNode& left = *expression.children[0];
    const AstNode& right = *expression.children[1];
    const std::optional<int> left_constant = constant_value(left);
    const std::optional<int> right_constant = constant_value(right);

    if (op == "+" && right_constant && *right_constant == 0) {
        emit_expression(left);
        return true;
    }
    if (op == "+" && left_constant && *left_constant == 0) {
        emit_expression(right);
        return true;
    }
    if (op == "-" && right_constant && *right_constant == 0) {
        emit_expression(left);
        return true;
    }
    if (op == "*" && right_constant && *right_constant == 1) {
        emit_expression(left);
        return true;
    }
    if (op == "*" && left_constant && *left_constant == 1) {
        emit_expression(right);
        return true;
    }
    if (op == "*" &&
        ((right_constant && *right_constant == 0) ||
         (left_constant && *left_constant == 0))) {
        emit_line("    li $t0, 0");
        return true;
    }
    if (op == "/" && right_constant && *right_constant == 1) {
        emit_expression(left);
        return true;
    }
    if (same_storage(left, right)) {
        if (op == "=") {
            emit_line("    li $t0, 1");
            return true;
        }
        if (op == "<") {
            emit_line("    li $t0, 0");
            return true;
        }
    }

    return false;
}

const AstNode* MipsGenerator::find_main_statement_list(const AstNode& root) const {
    for (const auto& child : root.children) {
        if (child && child->kind == AstNodeKind::StmLK) {
            return child.get();
        }
    }
    return nullptr;
}

const Symbol* MipsGenerator::find_global_symbol(const std::string& name) const {
    if (!symbols_ || symbols_->scopes().empty()) {
        return nullptr;
    }

    const Scope& global = symbols_->scopes().front();
    const auto found = global.by_name.find(name);
    if (found == global.by_name.end()) {
        return nullptr;
    }

    const Symbol* symbol = symbols_->get(found->second);
    if (!symbol || symbol->kind != SymbolKind::Variable) {
        return nullptr;
    }
    return symbol;
}

const Symbol* MipsGenerator::symbol_for_expression(const AstNode& expression) {
    const std::vector<std::string> words = split_words(expression.detail);
    if (words.size() < 2 || words[1] != "IdV") {
        emit_error(expression.location,
                   "variable form '" + expression.detail +
                   "' is not supported in this MIPS stage");
        return nullptr;
    }

    const Symbol* symbol = symbols_ ? symbols_->get(expression.semantic_symbol_id) : nullptr;
    if (!symbol) {
        symbol = find_global_symbol(first_word(expression.detail));
    }
    if (!symbol || symbol->kind != SymbolKind::Variable) {
        emit_error(expression.location,
                   "only main-scope variables are supported in this MIPS stage");
        return nullptr;
    }
    if (symbol->scope_id != 0) {
        emit_error(expression.location,
                   "local variables and parameters are not supported in this MIPS stage");
        return nullptr;
    }
    if (label_for_symbol(*symbol).empty()) {
        emit_error(expression.location,
                   "variable '" + symbol->name + "' has no MIPS storage label");
        return nullptr;
    }
    return symbol;
}

std::optional<int> MipsGenerator::constant_value(const AstNode& expression) const {
    if (starts_with(expression.detail, "Const '")) {
        if (expression.detail.size() >= 9) {
            return static_cast<unsigned char>(expression.detail[7]);
        }
        return 0;
    }

    if (starts_with(expression.detail, "Const ")) {
        try {
            return std::stoi(expression.detail.substr(6));
        } catch (...) {
            return std::nullopt;
        }
    }

    if (!starts_with(expression.detail, "Op ") || expression.children.size() < 2) {
        return std::nullopt;
    }

    const std::optional<int> left = constant_value(*expression.children[0]);
    const std::optional<int> right = constant_value(*expression.children[1]);
    if (!left || !right) {
        return std::nullopt;
    }

    const std::string op = expression.detail.substr(3);
    if (op == "+") {
        return *left + *right;
    }
    if (op == "-") {
        return *left - *right;
    }
    if (op == "*") {
        return *left * *right;
    }
    if (op == "/") {
        if (*right == 0) {
            return std::nullopt;
        }
        return *left / *right;
    }
    if (op == "<") {
        return *left < *right ? 1 : 0;
    }
    if (op == "=") {
        return *left == *right ? 1 : 0;
    }
    return std::nullopt;
}

bool MipsGenerator::same_storage(const AstNode& left, const AstNode& right) const {
    const std::vector<std::string> left_words = split_words(left.detail);
    const std::vector<std::string> right_words = split_words(right.detail);
    if (left_words.size() < 2 || right_words.size() < 2 ||
        left_words[1] != "IdV" || right_words[1] != "IdV") {
        return false;
    }

    if (left.semantic_symbol_id >= 0 && right.semantic_symbol_id >= 0) {
        return left.semantic_symbol_id == right.semantic_symbol_id;
    }

    return left_words.front() == right_words.front();
}

bool MipsGenerator::expression_preserves_storage(const AstNode& storage,
                                                 const AstNode& expression) const {
    if (same_storage(storage, expression)) {
        return true;
    }

    if (!starts_with(expression.detail, "Op ") || expression.children.size() < 2) {
        return false;
    }

    const std::string op = expression.detail.substr(3);
    const AstNode& left = *expression.children[0];
    const AstNode& right = *expression.children[1];
    const std::optional<int> left_constant = constant_value(left);
    const std::optional<int> right_constant = constant_value(right);

    if ((op == "+" || op == "-") && right_constant && *right_constant == 0) {
        return expression_preserves_storage(storage, left);
    }
    if (op == "+" && left_constant && *left_constant == 0) {
        return expression_preserves_storage(storage, right);
    }
    if ((op == "*" || op == "/") && right_constant && *right_constant == 1) {
        return expression_preserves_storage(storage, left);
    }
    if (op == "*" && left_constant && *left_constant == 1) {
        return expression_preserves_storage(storage, right);
    }

    return false;
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

void MipsGenerator::emit_line(const std::string& line) {
    output_ += line;
    output_ += '\n';
}

void MipsGenerator::emit_error(SourceLocation location, const std::string& message) {
    diagnostics_.push_back(Diagnostic{DiagnosticStage::Codegen, location, message});
}

}  // namespace snl
