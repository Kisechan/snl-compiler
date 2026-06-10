#include "diagnostic.h"
#include "ast.h"
#include "lexer.h"
#include "ll1_parser.h"
#include "mips.h"
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
        << "       snlc --ast [--recursive|--ll1] <input.snl>\n"
        << "       snlc --semantic [--recursive|--ll1] <input.snl>\n"
        << "       snlc --mips [--recursive|--ll1] <input.snl> -o <output.asm>\n";
}

enum class ParserMode {
    Recursive,
    LL1
};

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
    if (command != "--tokens" && command != "--ast" &&
        command != "--semantic" && command != "--mips") {
        if (command == "--all") {
            std::cerr << command << " is not implemented in this stage\n";
            return 2;
        }

        print_usage(std::cerr);
        return 2;
    }

    std::string input_path;
    std::string output_path;
    ParserMode parser_mode = ParserMode::Recursive;
    bool saw_parser_mode = false;

    for (int index = 2; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--recursive" || arg == "--ll1") {
            if (saw_parser_mode || command == "--tokens") {
                print_usage(std::cerr);
                return 2;
            }
            parser_mode = arg == "--ll1" ? ParserMode::LL1 : ParserMode::Recursive;
            saw_parser_mode = true;
            continue;
        }
        if (arg == "-o") {
            if (command != "--mips" || index + 1 >= argc || !output_path.empty()) {
                print_usage(std::cerr);
                return 2;
            }
            output_path = argv[++index];
            continue;
        }
        if (input_path.empty()) {
            input_path = arg;
            continue;
        }
        print_usage(std::cerr);
        return 2;
    }

    if (input_path.empty()) {
        print_usage(std::cerr);
        return 2;
    }

    if (command == "--mips") {
        if (output_path.empty()) {
            print_usage(std::cerr);
            return 2;
        }
    } else if (!output_path.empty()) {
        print_usage(std::cerr);
        return 2;
    }

    std::string source;
    if (!read_file(input_path, source)) {
        std::cerr << "file error: cannot read input file '" << input_path << "'\n";
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

    snl::ParserResult parser_result;
    if (parser_mode == ParserMode::LL1) {
        snl::LL1Parser parser(result.tokens);
        parser_result = parser.parse();
    } else {
        snl::Parser parser(result.tokens);
        parser_result = parser.parse();
    }
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

    if (command == "--semantic" || command == "--mips") {
        snl::SemanticAnalyzer analyzer;
        snl::SemanticResult semantic_result = analyzer.analyze(*parser_result.root);
        for (const snl::Diagnostic& diagnostic : semantic_result.diagnostics) {
            std::cerr << snl::format_diagnostic(diagnostic) << '\n';
        }
        if (!semantic_result.diagnostics.empty()) {
            return 1;
        }
        if (command == "--semantic") {
            snl::print_symbol_table_summary(semantic_result.symbols, std::cout);
            return 0;
        }

        snl::MipsGenerator generator;
        snl::MipsResult mips_result = generator.generate(*parser_result.root,
                                                         semantic_result.symbols);
        for (const snl::Diagnostic& diagnostic : mips_result.diagnostics) {
            std::cerr << snl::format_diagnostic(diagnostic) << '\n';
        }
        if (!mips_result.diagnostics.empty()) {
            return 1;
        }

        std::ofstream output(output_path, std::ios::binary);
        if (!output) {
            std::cerr << "file error: cannot write output file '" << output_path << "'\n";
            return 2;
        }
        output << mips_result.assembly;
    }

    return 0;
}
