#pragma once

#include "parser.h"
#include "token.h"

#include <vector>

namespace snl {

class LL1Parser {
public:
    explicit LL1Parser(const std::vector<Token>& tokens);

    ParserResult parse();

private:
    enum class NonTerminal {
        Program,
        ProgramHead,
        DeclarePart,
        TypeDec,
        TypeDecList,
        TypeName,
        BaseType,
        StructureType,
        ArrayType,
        RecordType,
        FieldDecList,
        FieldDec,
        IdList,
        IdListTail,
        VarDec,
        VarDecList,
        ProcDec,
        ProcDeclaration,
        ParamList,
        ParamMore,
        Param,
        ProgramBody,
        StmList,
        StmMore,
        StmMoreAfterSemi,
        Statement,
        IdStatementTail,
        VariableTail,
        FieldVar,
        FieldVarTail,
        ActParamList,
        ActParamMore,
        RelExp,
        CmpOp,
        Exp,
        ExpTail,
        Term,
        TermTail,
        Factor,
    };

    struct Symbol {
        bool terminal = false;
        TokenType token = TokenType::EndFile;
        NonTerminal non_terminal = NonTerminal::Program;
    };

    using Production = std::vector<Symbol>;

    bool parse_table();
    bool is_at_end() const;
    const Token& current() const;
    const Production* find_production(NonTerminal non_terminal, TokenType lookahead) const;
    void add_syntax_error(SourceLocation location, const std::string& message);

    static Symbol terminal(TokenType type);
    static Symbol non_terminal(NonTerminal symbol);
    static const char* non_terminal_name(NonTerminal symbol);
    static const char* token_display_name(TokenType type);

    const std::vector<Token>& tokens_;
    std::size_t index_ = 0;
    std::vector<Diagnostic> diagnostics_;
    Token synthetic_token_{TokenType::EndFile, SourceLocation{}, "", ""};
};

}  // namespace snl
