#include "ast.h"

#include <utility>

namespace snl {

std::unique_ptr<AstNode> make_ast_node(AstNodeKind kind,
                                       SourceLocation location,
                                       std::string detail) {
    return std::make_unique<AstNode>(AstNode{
        kind,
        location,
        std::move(detail),
        {},
        {},
        {},
        false,
        -1,
        {},
    });
}

void add_child(AstNode& parent, std::unique_ptr<AstNode> child) {
    if (child) {
        parent.children.push_back(std::move(child));
    }
}

std::string ast_node_kind_name(AstNodeKind kind) {
    switch (kind) {
        case AstNodeKind::ProK:
            return "ProK";
        case AstNodeKind::PheadK:
            return "PheadK";
        case AstNodeKind::TypeK:
            return "TypeK";
        case AstNodeKind::VarK:
            return "VarK";
        case AstNodeKind::ProcDecK:
            return "ProcDecK";
        case AstNodeKind::StmLK:
            return "StmLK";
        case AstNodeKind::DecK:
            return "DecK";
        case AstNodeKind::StmtK:
            return "StmtK";
        case AstNodeKind::ExpK:
            return "ExpK";
    }
    return "UnknownK";
}

void print_ast(const AstNode& node, std::ostream& out, int indent) {
    out << std::string(static_cast<std::size_t>(indent), ' ')
        << ast_node_kind_name(node.kind);
    if (!node.detail.empty()) {
        out << ' ' << node.detail;
    }
    for (const std::string& name : node.names) {
        out << ' ' << name;
    }
    out << '\n';

    for (const std::unique_ptr<AstNode>& child : node.children) {
        print_ast(*child, out, indent + 2);
    }
}

}  // namespace snl
