#include "diagnostic.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "symbol_table.h"
#include "token.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void print_usage(std::ostream& out) {
    out << "usage: snlc --tokens <input.snl>\n"
        << "       snlc --ast <input.snl>\n"
        << "       snlc --semantic <input.snl>\n";
}

bool read_file(const std::string& path, std::string& contents) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    contents = buffer.str();
    return true;
}

void print_token(const snl::Token& token) {
    std::cout << token.location.line << ' ' << token.location.column << ' '
              << snl::token_type_name(token.type);
    if (!token.value.empty()) {
        std::cout << ' ' << token.value;
    }
    std::cout << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(std::cerr);
        return 2;
    }

    const std::string command = argv[1];
    if (command != "--tokens" && command != "--ast" && command != "--semantic") {
        if (command == "--mips" || command == "--all") {
            std::cerr << command << " is not implemented in this stage\n";
            return 2;
        }

        print_usage(std::cerr);
        return 2;
    }

    if (argc != 3) {
        print_usage(std::cerr);
        return 2;
    }

    std::string source;
    if (!read_file(argv[2], source)) {
        std::cerr << "file error: cannot read input file '" << argv[2] << "'\n";
        return 2;
    }

    snl::Lexer lexer(std::move(source));
    snl::LexerResult result = lexer.scan_tokens();

    if (command == "--tokens") {
        for (const snl::Token& token : result.tokens) {
            print_token(token);
        }
        for (const snl::Diagnostic& diagnostic : result.diagnostics) {
            std::cerr << snl::format_diagnostic(diagnostic) << '\n';
        }
        return result.diagnostics.empty() ? 0 : 1;
    }

    if (!result.diagnostics.empty()) {
        for (const snl::Diagnostic& diagnostic : result.diagnostics) {
            std::cerr << snl::format_diagnostic(diagnostic) << '\n';
        }
        return 1;
    }

    snl::Parser parser(result.tokens);
    snl::ParserResult parser_result = parser.parse();
    if (parser_result.root) {
        if (command == "--ast") {
            snl::print_ast(*parser_result.root, std::cout);
        }
    }
    for (const snl::Diagnostic& diagnostic : parser_result.diagnostics) {
        std::cerr << snl::format_diagnostic(diagnostic) << '\n';
    }

    if (!parser_result.diagnostics.empty()) {
        return 1;
    }

    if (command == "--semantic") {
        snl::SemanticAnalyzer analyzer;
        snl::SemanticResult semantic_result = analyzer.analyze(*parser_result.root);
        for (const snl::Diagnostic& diagnostic : semantic_result.diagnostics) {
            std::cerr << snl::format_diagnostic(diagnostic) << '\n';
        }
        if (!semantic_result.diagnostics.empty()) {
            return 1;
        }
        snl::print_symbol_table_summary(semantic_result.symbols, std::cout);
    }

    return 0;
}
