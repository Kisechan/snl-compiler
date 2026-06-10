#include "symbol_table.h"

#include <utility>

namespace snl {

int SymbolTable::enter_scope(std::string name) {
    const int parent = scope_stack_.empty() ? -1 : scope_stack_.back();
    const int id = static_cast<int>(scopes_.size());
    const int level = parent < 0 ? 0 : scopes_[static_cast<std::size_t>(parent)].level + 1;
    scopes_.push_back(Scope{id, parent, level, std::move(name), 0, {}, {}});
    scope_stack_.push_back(id);
    return id;
}

void SymbolTable::leave_scope() {
    if (!scope_stack_.empty()) {
        scope_stack_.pop_back();
    }
}

Symbol* SymbolTable::insert(std::string name,
                            SymbolKind kind,
                            TypePtr type,
                            bool is_var_param) {
    Scope* scope = current_scope();
    if (!scope || scope->by_name.find(name) != scope->by_name.end()) {
        return nullptr;
    }

    const int id = static_cast<int>(symbols_.size());
    const int size = kind == SymbolKind::Procedure ? 0 :
                     (is_var_param ? 4 : (type ? type->size : 0));
    Symbol symbol;
    symbol.id = id;
    symbol.name = name;
    symbol.kind = kind;
    symbol.type = std::move(type);
    symbol.scope_id = scope->id;
    symbol.level = scope->level;
    symbol.offset = scope->next_offset;
    symbol.size = size;
    symbol.is_var_param = is_var_param;

    if (kind == SymbolKind::Variable || kind == SymbolKind::Parameter) {
        scope->next_offset += size;
    }

    symbols_.push_back(std::move(symbol));
    scope->symbols.push_back(id);
    scope->by_name.emplace(symbols_.back().name, id);
    return &symbols_.back();
}

Symbol* SymbolTable::find_current(const std::string& name) {
    Scope* scope = current_scope();
    if (!scope) {
        return nullptr;
    }
    const auto found = scope->by_name.find(name);
    return found == scope->by_name.end() ? nullptr : get(found->second);
}

Symbol* SymbolTable::find(const std::string& name) {
    for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it) {
        Scope& scope = scopes_[static_cast<std::size_t>(*it)];
        const auto found = scope.by_name.find(name);
        if (found != scope.by_name.end()) {
            return get(found->second);
        }
    }
    return nullptr;
}

const Symbol* SymbolTable::find(const std::string& name) const {
    for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it) {
        const Scope& scope = scopes_[static_cast<std::size_t>(*it)];
        const auto found = scope.by_name.find(name);
        if (found != scope.by_name.end()) {
            return get(found->second);
        }
    }
    return nullptr;
}

Symbol* SymbolTable::get(int id) {
    if (id < 0 || static_cast<std::size_t>(id) >= symbols_.size()) {
        return nullptr;
    }
    return &symbols_[static_cast<std::size_t>(id)];
}

const Symbol* SymbolTable::get(int id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= symbols_.size()) {
        return nullptr;
    }
    return &symbols_[static_cast<std::size_t>(id)];
}

Scope* SymbolTable::current_scope() {
    if (scope_stack_.empty()) {
        return nullptr;
    }
    return &scopes_[static_cast<std::size_t>(scope_stack_.back())];
}

const Scope* SymbolTable::current_scope() const {
    if (scope_stack_.empty()) {
        return nullptr;
    }
    return &scopes_[static_cast<std::size_t>(scope_stack_.back())];
}

const std::vector<Scope>& SymbolTable::scopes() const {
    return scopes_;
}

const std::vector<Symbol>& SymbolTable::symbols() const {
    return symbols_;
}

std::string symbol_kind_name(SymbolKind kind) {
    switch (kind) {
        case SymbolKind::Type:
            return "type";
        case SymbolKind::Variable:
            return "variable";
        case SymbolKind::Procedure:
            return "procedure";
        case SymbolKind::Parameter:
            return "parameter";
    }
    return "unknown";
}

void print_symbol_table_summary(const SymbolTable& table, std::ostream& out) {
    out << "semantic ok\n";
    for (const Scope& scope : table.scopes()) {
        out << "Scope #" << scope.id << " " << scope.name
            << " level=" << scope.level
            << " parent=" << scope.parent
            << " size=" << scope.next_offset << '\n';
        for (int symbol_id : scope.symbols) {
            const Symbol* symbol = table.get(symbol_id);
            if (!symbol) {
                continue;
            }
            out << "  #" << symbol->id
                << " " << symbol_kind_name(symbol->kind)
                << " " << symbol->name
                << " type=" << format_type(symbol->type)
                << " level=" << symbol->level
                << " offset=" << symbol->offset
                << " size=" << symbol->size;
            if (symbol->kind == SymbolKind::Parameter) {
                out << " mode=" << (symbol->is_var_param ? "var" : "value");
            }
            if (symbol->kind == SymbolKind::Procedure) {
                out << " local_size=" << symbol->local_size;
            }
            out << '\n';
        }
    }
}

}  // namespace snl
