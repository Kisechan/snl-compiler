#include "semantic.h"

#include <sstream>
#include <utility>

namespace snl {

namespace {

std::vector<std::string> split_words(const std::string& text) {
    std::istringstream input(text);
    std::vector<std::string> words;
    std::string word;
    while (input >> word) {
        words.push_back(word);
    }
    return words;
}

std::string join_name(const std::vector<std::string>& names) {
    return names.empty() ? "<anonymous>" : names.front();
}

}  // namespace

SemanticResult SemanticAnalyzer::analyze(AstNode& root) {
    integer_type_ = make_primitive_type(TypeKind::Integer);
    char_type_ = make_primitive_type(TypeKind::Char);
    boolean_type_ = make_primitive_type(TypeKind::Boolean);
    error_type_ = make_error_type();

    analyze_program(root);

    SemanticResult result;
    result.diagnostics = std::move(diagnostics_);
    result.symbols = std::move(table_);
    return result;
}

void SemanticAnalyzer::analyze_program(AstNode& root) {
    std::string program_name = "program";
    if (!root.children.empty() && !root.children[0]->names.empty()) {
        program_name = root.children[0]->names.front();
    }
    table_.enter_scope(program_name);
    process_declarations(root, 1);
    if (AstNode* body = find_first_child(root, AstNodeKind::StmLK, 1)) {
        process_statement_list(*body);
    }
    table_.leave_scope();
}

void SemanticAnalyzer::process_declarations(AstNode& owner, std::size_t first_child) {
    if (AstNode* type_section = find_first_child(owner, AstNodeKind::TypeK, first_child)) {
        process_type_section(*type_section);
    }
    if (AstNode* var_section = find_first_child(owner, AstNodeKind::VarK, first_child)) {
        process_var_section(*var_section);
    }

    for (std::size_t i = first_child; i < owner.children.size(); ++i) {
        if (owner.children[i]->kind == AstNodeKind::ProcDecK) {
            process_proc_declaration(*owner.children[i]);
        }
    }
}

void SemanticAnalyzer::process_type_section(AstNode& type_section) {
    for (const auto& child : type_section.children) {
        AstNode& dec = *child;
        TypePtr target = build_type_from_dec(dec);
        for (const std::string& name : dec.names) {
            TypePtr alias = make_alias_type(name, target);
            if (!table_.insert(name, SymbolKind::Type, alias)) {
                add_error(dec.location, "duplicate definition of '" + name + "'");
            }
        }
    }
}

void SemanticAnalyzer::process_var_section(AstNode& var_section) {
    for (const auto& child : var_section.children) {
        AstNode& dec = *child;
        TypePtr type = build_type_from_dec(dec);
        for (const std::string& name : dec.names) {
            if (!table_.insert(name, SymbolKind::Variable, type)) {
                add_error(dec.location, "duplicate definition of '" + name + "'");
            }
        }
    }
}

void SemanticAnalyzer::process_proc_declaration(AstNode& proc) {
    const std::string proc_name = join_name(proc.names);
    std::vector<ParameterType> parameters = collect_parameters(proc);
    TypePtr proc_type = make_procedure_type(parameters, table_.current_scope() ? table_.current_scope()->level + 1 : 0);
    Symbol* inserted = table_.insert(proc_name, SymbolKind::Procedure, proc_type);
    if (!inserted) {
        add_error(proc.location, "duplicate definition of '" + proc_name + "'");
    }
    const int proc_symbol_id = inserted ? inserted->id : -1;

    table_.enter_scope("procedure " + proc_name);
    for (const ParameterType& parameter : parameters) {
        if (!table_.insert(parameter.name, SymbolKind::Parameter, parameter.type, parameter.is_var)) {
            add_error(proc.location, "duplicate definition of parameter '" + parameter.name + "'");
        }
    }

    const std::size_t body_start = first_non_param_child(proc);
    process_declarations(proc, body_start);
    if (AstNode* body = find_first_child(proc, AstNodeKind::StmLK, body_start)) {
        process_statement_list(*body);
    }
    const int local_size = table_.current_scope() ? table_.current_scope()->next_offset : 0;
    table_.leave_scope();

    if (Symbol* symbol = table_.get(proc_symbol_id)) {
        symbol->local_size = local_size;
    }
}

void SemanticAnalyzer::process_statement_list(AstNode& list) {
    for (const auto& child : list.children) {
        process_statement(*child);
    }
}

void SemanticAnalyzer::process_statement(AstNode& statement) {
    if (statement.detail == "Assign") {
        if (statement.children.size() < 2) {
            return;
        }
        ExprResult lhs = analyze_expression(*statement.children[0]);
        ExprResult rhs = analyze_expression(*statement.children[1]);
        if (!lhs.is_lvalue && !is_error_type(lhs.type)) {
            add_error(statement.location, "left side of assignment is not assignable");
        }
        if (!type_equivalent(lhs.type, rhs.type)) {
            add_error(statement.location,
                      "assignment type mismatch: " + format_type(lhs.type) +
                      " := " + format_type(rhs.type));
        }
        return;
    }

    if (statement.detail == "Read") {
        const std::string name = join_name(statement.names);
        Symbol* symbol = require_symbol(name, statement.location);
        if (symbol && symbol->kind != SymbolKind::Variable && symbol->kind != SymbolKind::Parameter) {
            add_error(statement.location, "'" + name + "' is not a variable");
        }
        return;
    }

    if (statement.detail == "Write" || statement.detail == "Return") {
        if (!statement.children.empty()) {
            ExprResult value = analyze_expression(*statement.children[0]);
            TypePtr resolved = resolve_alias(value.type);
            if (statement.detail == "Write" && !is_error_type(resolved) &&
                resolved->kind != TypeKind::Integer && resolved->kind != TypeKind::Char) {
                add_error(statement.location, "write expression must be integer or char");
            }
        }
        return;
    }

    if (statement.detail == "If" || statement.detail == "While") {
        if (!statement.children.empty()) {
            ExprResult condition = analyze_expression(*statement.children[0]);
            if (!type_equivalent(condition.type, boolean_type_)) {
                add_error(statement.location,
                          statement.detail + " condition must be boolean");
            }
        }
        for (std::size_t i = 1; i < statement.children.size(); ++i) {
            process_statement_list(*statement.children[i]);
        }
        return;
    }

    if (statement.detail == "Call") {
        if (statement.children.empty()) {
            return;
        }
        const std::string callee_name = first_word(statement.children[0]->detail);
        Symbol* callee = require_symbol(callee_name, statement.location);
        if (!callee || callee->kind != SymbolKind::Procedure) {
            add_error(statement.location, "'" + callee_name + "' is not a procedure");
            return;
        }
        TypePtr proc_type = resolve_alias(callee->type);
        const std::size_t expected = proc_type ? proc_type->parameters.size() : 0;
        const std::size_t actual = statement.children.size() - 1;
        if (expected != actual) {
            add_error(statement.location,
                      "procedure '" + callee_name + "' expects " +
                      std::to_string(expected) + " argument(s), got " +
                      std::to_string(actual));
        }
        const std::size_t count = expected < actual ? expected : actual;
        for (std::size_t i = 0; i < count; ++i) {
            ExprResult arg = analyze_expression(*statement.children[i + 1]);
            const ParameterType& param = proc_type->parameters[i];
            if (!type_equivalent(param.type, arg.type)) {
                add_error(statement.children[i + 1]->location,
                          "argument " + std::to_string(i + 1) +
                          " type mismatch: expected " + format_type(param.type) +
                          ", got " + format_type(arg.type));
            }
            if (param.is_var && !arg.is_lvalue) {
                add_error(statement.children[i + 1]->location,
                          "var argument " + std::to_string(i + 1) +
                          " must be an assignable variable");
            }
        }
        return;
    }
}

SemanticAnalyzer::ExprResult SemanticAnalyzer::analyze_expression(AstNode& expression) {
    ExprResult result{error_type_, false, -1, ""};

    if (starts_with(expression.detail, "Const '")) {
        result.type = char_type_;
    } else if (starts_with(expression.detail, "Const ")) {
        result.type = integer_type_;
    } else if (starts_with(expression.detail, "Op ")) {
        const std::string op = expression.detail.substr(3);
        if (expression.children.size() < 2) {
            result.type = error_type_;
        } else {
            ExprResult lhs = analyze_expression(*expression.children[0]);
            ExprResult rhs = analyze_expression(*expression.children[1]);
            if (op == "+" || op == "-" || op == "*" || op == "/") {
                if (!type_equivalent(lhs.type, integer_type_) ||
                    !type_equivalent(rhs.type, integer_type_)) {
                    add_error(expression.location, "arithmetic operands must be integer");
                }
                result.type = integer_type_;
            } else {
                if (!type_equivalent(lhs.type, rhs.type)) {
                    add_error(expression.location,
                              "comparison operands must have equivalent types");
                }
                result.type = boolean_type_;
            }
        }
    } else {
        result = analyze_variable(expression);
    }

    expression.semantic_type = format_type(result.type);
    expression.semantic_is_lvalue = result.is_lvalue;
    expression.semantic_symbol_id = result.symbol_id;
    expression.semantic_var_kind = result.var_kind;
    return result;
}

SemanticAnalyzer::ExprResult SemanticAnalyzer::analyze_variable(AstNode& expression) {
    const std::string name = first_word(expression.detail);
    const std::string kind = split_words(expression.detail).size() >= 2
                                 ? split_words(expression.detail)[1]
                                 : "IdV";
    Symbol* symbol = require_symbol(name, expression.location);
    if (!symbol) {
        return ExprResult{error_type_, true, -1, kind};
    }
    if (symbol->kind != SymbolKind::Variable && symbol->kind != SymbolKind::Parameter) {
        add_error(expression.location, "'" + name + "' is not a variable");
        return ExprResult{error_type_, false, symbol->id, kind};
    }

    TypePtr type = symbol->type;
    if (kind == "ArrayMembV") {
        TypePtr resolved = resolve_alias(type);
        if (!resolved || resolved->kind != TypeKind::Array) {
            add_error(expression.location, "'" + name + "' is not an array");
            return ExprResult{error_type_, true, symbol->id, kind};
        }
        if (!expression.children.empty()) {
            ExprResult index = analyze_expression(*expression.children[0]);
            if (!type_equivalent(index.type, integer_type_)) {
                add_error(expression.children[0]->location, "array index must be integer");
            }
        }
        return ExprResult{resolved->element_type, true, symbol->id, kind};
    }

    if (kind == "FieldMembV") {
        if (expression.children.empty()) {
            return ExprResult{error_type_, true, symbol->id, kind};
        }
        return analyze_field_access(expression, *expression.children[0], type);
    }

    return ExprResult{type, true, symbol->id, kind};
}

SemanticAnalyzer::ExprResult SemanticAnalyzer::analyze_field_access(AstNode& base,
                                                                    AstNode& field_node,
                                                                    TypePtr base_type) {
    TypePtr record_type = resolve_alias(std::move(base_type));
    const std::string field_name = first_word(field_node.detail);
    if (!record_type || record_type->kind != TypeKind::Record) {
        add_error(base.location, "'" + first_word(base.detail) + "' is not a record");
        return ExprResult{error_type_, true, -1, "FieldMembV"};
    }

    const RecordField* field = find_field(record_type, field_name);
    if (!field) {
        add_error(field_node.location, "record field '" + field_name + "' does not exist");
        return ExprResult{error_type_, true, -1, "FieldMembV"};
    }

    if (field_node.detail.find("ArrayMembV") != std::string::npos) {
        TypePtr array_type = resolve_alias(field->type);
        if (!array_type || array_type->kind != TypeKind::Array) {
            add_error(field_node.location, "field '" + field_name + "' is not an array");
            return ExprResult{error_type_, true, -1, "ArrayMembV"};
        }
        if (!field_node.children.empty()) {
            ExprResult index = analyze_expression(*field_node.children[0]);
            if (!type_equivalent(index.type, integer_type_)) {
                add_error(field_node.children[0]->location, "array index must be integer");
            }
        }
        return ExprResult{array_type->element_type, true, -1, "ArrayMembV"};
    }

    return ExprResult{field->type, true, -1, "FieldMembV"};
}

TypePtr SemanticAnalyzer::build_type_from_dec(AstNode& dec) {
    return build_type_from_detail(dec.detail, dec);
}

TypePtr SemanticAnalyzer::build_type_from_detail(const std::string& raw_detail,
                                                 const AstNode& node) {
    bool ignored_var = false;
    const std::string detail = strip_param_prefix(raw_detail, ignored_var);
    const std::vector<std::string> words = split_words(detail);
    if (words.empty()) {
        return error_type_;
    }
    if (words[0] == "IntegerK") {
        return integer_type_;
    }
    if (words[0] == "CharK") {
        return char_type_;
    }
    if (words[0] == "IdK" && words.size() >= 2) {
        Symbol* symbol = require_symbol(words[1], node.location);
        if (!symbol) {
            return error_type_;
        }
        if (symbol->kind != SymbolKind::Type) {
            add_error(node.location, "'" + words[1] + "' is not a type");
            return error_type_;
        }
        return symbol->type;
    }
    if (words[0] == "ArrayK" && words.size() >= 3) {
        const std::string range = words[1];
        const std::size_t dots = range.find("..");
        const int low = dots == std::string::npos ? 0 : parse_int(range.substr(0, dots));
        const int high = dots == std::string::npos ? -1 : parse_int(range.substr(dots + 2));
        TypePtr element = words[2] == "CharK" ? char_type_ : integer_type_;
        if (high < low) {
            add_error(node.location, "array upper bound is smaller than lower bound");
            return error_type_;
        }
        return make_array_type(low, high, element);
    }
    if (words[0] == "RecordK") {
        return make_record_type(build_record_fields(node));
    }
    return error_type_;
}

std::vector<RecordField> SemanticAnalyzer::build_record_fields(const AstNode& record_node) {
    std::vector<RecordField> fields;
    if (record_node.children.empty()) {
        return fields;
    }
    const AstNode& field_list = *record_node.children[0];
    for (const auto& field_dec_ptr : field_list.children) {
        const AstNode& field_dec = *field_dec_ptr;
        TypePtr field_type = build_type_from_detail(field_dec.detail, field_dec);
        for (const std::string& name : field_dec.names) {
            bool duplicate = false;
            for (const RecordField& existing : fields) {
                if (existing.name == name) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                add_error(field_dec.location, "duplicate record field '" + name + "'");
            } else {
                fields.push_back(RecordField{name, field_type, 0, 0});
            }
        }
    }
    return fields;
}

std::vector<ParameterType> SemanticAnalyzer::collect_parameters(AstNode& proc) {
    std::vector<ParameterType> params;
    for (const auto& child : proc.children) {
        if (!is_param_dec(*child)) {
            break;
        }
        bool is_var = false;
        TypePtr type = build_type_from_detail(child->detail, *child);
        strip_param_prefix(child->detail, is_var);
        for (const std::string& name : child->names) {
            params.push_back(ParameterType{name, type, is_var, 0, 0});
        }
    }
    return params;
}

bool SemanticAnalyzer::is_param_dec(const AstNode& node) const {
    return node.kind == AstNodeKind::DecK &&
           (starts_with(node.detail, "value param: ") ||
            starts_with(node.detail, "var param: "));
}

std::size_t SemanticAnalyzer::first_non_param_child(AstNode& proc) const {
    std::size_t index = 0;
    while (index < proc.children.size() && is_param_dec(*proc.children[index])) {
        ++index;
    }
    return index;
}

AstNode* SemanticAnalyzer::find_first_child(AstNode& node,
                                            AstNodeKind kind,
                                            std::size_t start) {
    for (std::size_t i = start; i < node.children.size(); ++i) {
        if (node.children[i]->kind == kind) {
            return node.children[i].get();
        }
    }
    return nullptr;
}

std::string SemanticAnalyzer::first_word(const std::string& text) const {
    const std::vector<std::string> words = split_words(text);
    return words.empty() ? "" : words.front();
}

std::string SemanticAnalyzer::strip_param_prefix(const std::string& detail,
                                                 bool& is_var) const {
    if (starts_with(detail, "value param: ")) {
        is_var = false;
        return detail.substr(std::string("value param: ").size());
    }
    if (starts_with(detail, "var param: ")) {
        is_var = true;
        return detail.substr(std::string("var param: ").size());
    }
    return detail;
}

bool SemanticAnalyzer::starts_with(const std::string& text,
                                   const std::string& prefix) const {
    return text.rfind(prefix, 0) == 0;
}

int SemanticAnalyzer::parse_int(const std::string& text) const {
    try {
        return std::stoi(text);
    } catch (...) {
        return 0;
    }
}

void SemanticAnalyzer::add_error(SourceLocation location, std::string message) {
    diagnostics_.push_back(Diagnostic{
        DiagnosticStage::Semantic,
        location,
        std::move(message),
    });
}

Symbol* SemanticAnalyzer::require_symbol(const std::string& name,
                                         SourceLocation location) {
    Symbol* symbol = table_.find(name);
    if (!symbol) {
        add_error(location, "undeclared identifier '" + name + "'");
    }
    return symbol;
}

const RecordField* SemanticAnalyzer::find_field(TypePtr record_type,
                                                const std::string& name) const {
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

}  // namespace snl
