#pragma once

#include "diagnostic.h"

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace snl {

enum class AstNodeKind {
    ProK,
    PheadK,
    TypeK,
    VarK,
    ProcDecK,
    StmLK,
    DecK,
    StmtK,
    ExpK
};

struct AstNode {
    AstNodeKind kind;
    SourceLocation location;
    std::string detail;
    std::vector<std::string> names;
    std::vector<std::unique_ptr<AstNode>> children;
    std::string semantic_type;
    bool semantic_is_lvalue = false;
    int semantic_symbol_id = -1;
    std::string semantic_var_kind;
};

std::unique_ptr<AstNode> make_ast_node(AstNodeKind kind,
                                       SourceLocation location,
                                       std::string detail = {});

void add_child(AstNode& parent, std::unique_ptr<AstNode> child);
std::string ast_node_kind_name(AstNodeKind kind);
void print_ast(const AstNode& node, std::ostream& out, int indent = 0);

}  // namespace snl
