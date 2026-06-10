#pragma once

#include "type.h"

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace snl {

enum class SymbolKind {
    Type,
    Variable,
    Procedure,
    Parameter
};

struct Symbol {
    int id = -1;
    std::string name;
    SymbolKind kind = SymbolKind::Variable;
    TypePtr type;
    int scope_id = -1;
    int level = 0;
    int offset = 0;
    int size = 0;
    bool is_var_param = false;
    int local_size = 0;
};

struct Scope {
    int id = -1;
    int parent = -1;
    int level = 0;
    std::string name;
    int next_offset = 0;
    std::vector<int> symbols;
    std::unordered_map<std::string, int> by_name;
};

class SymbolTable {
public:
    int enter_scope(std::string name);
    void leave_scope();

    Symbol* insert(std::string name,
                   SymbolKind kind,
                   TypePtr type,
                   bool is_var_param = false);
    Symbol* find_current(const std::string& name);
    Symbol* find(const std::string& name);
    const Symbol* find(const std::string& name) const;
    Symbol* get(int id);
    const Symbol* get(int id) const;

    Scope* current_scope();
    const Scope* current_scope() const;
    const std::vector<Scope>& scopes() const;
    const std::vector<Symbol>& symbols() const;

private:
    std::vector<Scope> scopes_;
    std::vector<Symbol> symbols_;
    std::vector<int> scope_stack_;
};

std::string symbol_kind_name(SymbolKind kind);
void print_symbol_table_summary(const SymbolTable& table, std::ostream& out);

}  // namespace snl
