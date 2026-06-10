#include "ll1_parser.h"

#include "diagnostic.h"

#include <sstream>
#include <utility>

namespace snl {

namespace {

std::string got_text(const Token& token) {
    std::ostringstream out;
    out << token_type_name(token.type);
    if (!token.lexeme.empty()) {
        out << " '" << token.lexeme << "'";
    }
    return out.str();
}

}  // namespace

LL1Parser::LL1Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

ParserResult LL1Parser::parse() {
    ParserResult result;
    if (!parse_table()) {
        result.diagnostics = std::move(diagnostics_);
        return result;
    }

    Parser recursive_parser(tokens_);
    result = recursive_parser.parse();
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.diagnostics = std::move(diagnostics_);
    return result;
}

bool LL1Parser::parse_table() {
    std::vector<Symbol> stack;
    stack.push_back(terminal(TokenType::EndFile));
    stack.push_back(non_terminal(NonTerminal::Program));

    while (!stack.empty()) {
        const Symbol symbol = stack.back();
        stack.pop_back();

        if (symbol.terminal) {
            if (current().type != symbol.token) {
                add_syntax_error(current().location,
                                 std::string("expected ") +
                                     token_display_name(symbol.token) +
                                     ", got " + got_text(current()));
                return false;
            }
            if (!is_at_end()) {
                ++index_;
            }
            continue;
        }

        const Production* production =
            find_production(symbol.non_terminal, current().type);
        if (!production) {
            add_syntax_error(current().location,
                             std::string("expected ") +
                                 non_terminal_name(symbol.non_terminal) +
                                 ", got " + got_text(current()));
            return false;
        }

        for (auto it = production->rbegin(); it != production->rend(); ++it) {
            stack.push_back(*it);
        }
    }

    return true;
}

bool LL1Parser::is_at_end() const {
    return current().type == TokenType::EndFile;
}

const Token& LL1Parser::current() const {
    if (index_ >= tokens_.size()) {
        return synthetic_token_;
    }
    return tokens_[index_];
}

const LL1Parser::Production* LL1Parser::find_production(
    NonTerminal symbol,
    TokenType lookahead) const {
    using NT = NonTerminal;
    using TT = TokenType;
    const auto make_t = terminal;
    const auto make_n = non_terminal;

    switch (symbol) {
        case NT::Program:
            if (lookahead == TT::Program) {
                static const Production production = {
                    make_n(NT::ProgramHead), make_n(NT::DeclarePart),
                    make_n(NT::ProgramBody), make_t(TT::Dot), make_t(TT::EndFile)};
                return &production;
            }
            return nullptr;

        case NT::ProgramHead:
            if (lookahead == TT::Program) {
                static const Production production = {make_t(TT::Program), make_t(TT::Id)};
                return &production;
            }
            return nullptr;

        case NT::DeclarePart:
            if (lookahead == TT::Type || lookahead == TT::Var ||
                lookahead == TT::Procedure || lookahead == TT::Begin) {
                static const Production production = {
                    make_n(NT::TypeDec), make_n(NT::VarDec), make_n(NT::ProcDec)};
                return &production;
            }
            return nullptr;

        case NT::TypeDec:
            if (lookahead == TT::Type) {
                static const Production production = {
                    make_t(TT::Type), make_n(NT::TypeDecList)};
                return &production;
            }
            if (lookahead == TT::Var || lookahead == TT::Procedure ||
                lookahead == TT::Begin) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::TypeDecList:
            if (lookahead == TT::Id) {
                static const Production production = {
                    make_t(TT::Id), make_t(TT::Eq), make_n(NT::TypeName), make_t(TT::Semi),
                    make_n(NT::TypeDecList)};
                return &production;
            }
            if (lookahead == TT::Var || lookahead == TT::Procedure ||
                lookahead == TT::Begin) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::TypeName:
            if (lookahead == TT::Integer || lookahead == TT::Char) {
                static const Production production = {make_n(NT::BaseType)};
                return &production;
            }
            if (lookahead == TT::Array || lookahead == TT::Record) {
                static const Production production = {make_n(NT::StructureType)};
                return &production;
            }
            if (lookahead == TT::Id) {
                static const Production production = {make_t(TT::Id)};
                return &production;
            }
            return nullptr;

        case NT::BaseType:
            if (lookahead == TT::Integer) {
                static const Production production = {make_t(TT::Integer)};
                return &production;
            }
            if (lookahead == TT::Char) {
                static const Production production = {make_t(TT::Char)};
                return &production;
            }
            return nullptr;

        case NT::StructureType:
            if (lookahead == TT::Array) {
                static const Production production = {make_n(NT::ArrayType)};
                return &production;
            }
            if (lookahead == TT::Record) {
                static const Production production = {make_n(NT::RecordType)};
                return &production;
            }
            return nullptr;

        case NT::ArrayType:
            if (lookahead == TT::Array) {
                static const Production production = {
                    make_t(TT::Array), make_t(TT::LMidParen), make_t(TT::IntConst),
                    make_t(TT::Underange), make_t(TT::IntConst), make_t(TT::RMidParen),
                    make_t(TT::Of), make_n(NT::BaseType)};
                return &production;
            }
            return nullptr;

        case NT::RecordType:
            if (lookahead == TT::Record) {
                static const Production production = {
                    make_t(TT::Record), make_n(NT::FieldDecList), make_t(TT::End)};
                return &production;
            }
            return nullptr;

        case NT::FieldDecList:
            if (lookahead == TT::Integer || lookahead == TT::Char ||
                lookahead == TT::Array) {
                static const Production production = {
                    make_n(NT::FieldDec), make_n(NT::FieldDecList)};
                return &production;
            }
            if (lookahead == TT::End) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::FieldDec:
            if (lookahead == TT::Integer || lookahead == TT::Char) {
                static const Production production = {
                    make_n(NT::BaseType), make_n(NT::IdList), make_t(TT::Semi)};
                return &production;
            }
            if (lookahead == TT::Array) {
                static const Production production = {
                    make_n(NT::ArrayType), make_n(NT::IdList), make_t(TT::Semi)};
                return &production;
            }
            return nullptr;

        case NT::IdList:
            if (lookahead == TT::Id) {
                static const Production production = {make_t(TT::Id), make_n(NT::IdListTail)};
                return &production;
            }
            return nullptr;

        case NT::IdListTail:
            if (lookahead == TT::Comma) {
                static const Production production = {
                    make_t(TT::Comma), make_t(TT::Id), make_n(NT::IdListTail)};
                return &production;
            }
            if (lookahead == TT::Semi || lookahead == TT::RParen) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::VarDec:
            if (lookahead == TT::Var) {
                static const Production production = {
                    make_t(TT::Var), make_n(NT::VarDecList)};
                return &production;
            }
            if (lookahead == TT::Procedure || lookahead == TT::Begin) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::VarDecList:
            if (lookahead == TT::Integer || lookahead == TT::Char ||
                lookahead == TT::Array || lookahead == TT::Record ||
                lookahead == TT::Id) {
                static const Production production = {
                    make_n(NT::TypeName), make_n(NT::IdList), make_t(TT::Semi),
                    make_n(NT::VarDecList)};
                return &production;
            }
            if (lookahead == TT::Procedure || lookahead == TT::Begin) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::ProcDec:
            if (lookahead == TT::Procedure) {
                static const Production production = {
                    make_n(NT::ProcDeclaration), make_n(NT::ProcDec)};
                return &production;
            }
            if (lookahead == TT::Begin) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::ProcDeclaration:
            if (lookahead == TT::Procedure) {
                static const Production production = {
                    make_t(TT::Procedure), make_t(TT::Id), make_t(TT::LParen),
                    make_n(NT::ParamList), make_t(TT::RParen), make_t(TT::Semi),
                    make_n(NT::DeclarePart), make_n(NT::ProgramBody)};
                return &production;
            }
            return nullptr;

        case NT::ParamList:
            if (lookahead == TT::Var || lookahead == TT::Integer ||
                lookahead == TT::Char || lookahead == TT::Array ||
                lookahead == TT::Record || lookahead == TT::Id) {
                static const Production production = {
                    make_n(NT::Param), make_n(NT::ParamMore)};
                return &production;
            }
            if (lookahead == TT::RParen) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::ParamMore:
            if (lookahead == TT::Semi) {
                static const Production production = {
                    make_t(TT::Semi), make_n(NT::Param), make_n(NT::ParamMore)};
                return &production;
            }
            if (lookahead == TT::RParen) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::Param:
            if (lookahead == TT::Var) {
                static const Production production = {
                    make_t(TT::Var), make_n(NT::TypeName), make_n(NT::IdList)};
                return &production;
            }
            if (lookahead == TT::Integer || lookahead == TT::Char ||
                lookahead == TT::Array || lookahead == TT::Record ||
                lookahead == TT::Id) {
                static const Production production = {
                    make_n(NT::TypeName), make_n(NT::IdList)};
                return &production;
            }
            return nullptr;

        case NT::ProgramBody:
            if (lookahead == TT::Begin) {
                static const Production production = {
                    make_t(TT::Begin), make_n(NT::StmList), make_t(TT::End)};
                return &production;
            }
            return nullptr;

        case NT::StmList:
            if (lookahead == TT::If || lookahead == TT::While ||
                lookahead == TT::Read || lookahead == TT::Write ||
                lookahead == TT::Return || lookahead == TT::Id) {
                static const Production production = {
                    make_n(NT::Statement), make_n(NT::StmMore)};
                return &production;
            }
            return nullptr;

        case NT::StmMore:
            if (lookahead == TT::Semi) {
                static const Production production = {
                    make_t(TT::Semi), make_n(NT::StmMoreAfterSemi)};
                return &production;
            }
            if (lookahead == TT::End || lookahead == TT::Else ||
                lookahead == TT::Fi || lookahead == TT::EndWh) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::StmMoreAfterSemi:
            if (lookahead == TT::If || lookahead == TT::While ||
                lookahead == TT::Read || lookahead == TT::Write ||
                lookahead == TT::Return || lookahead == TT::Id) {
                static const Production production = {
                    make_n(NT::Statement), make_n(NT::StmMore)};
                return &production;
            }
            if (lookahead == TT::End || lookahead == TT::Else ||
                lookahead == TT::Fi || lookahead == TT::EndWh) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::Statement:
            if (lookahead == TT::If) {
                static const Production production = {
                    make_t(TT::If), make_n(NT::RelExp), make_t(TT::Then), make_n(NT::StmList),
                    make_t(TT::Else), make_n(NT::StmList), make_t(TT::Fi)};
                return &production;
            }
            if (lookahead == TT::While) {
                static const Production production = {
                    make_t(TT::While), make_n(NT::RelExp), make_t(TT::Do),
                    make_n(NT::StmList), make_t(TT::EndWh)};
                return &production;
            }
            if (lookahead == TT::Read) {
                static const Production production = {
                    make_t(TT::Read), make_t(TT::LParen), make_t(TT::Id), make_t(TT::RParen)};
                return &production;
            }
            if (lookahead == TT::Write) {
                static const Production production = {
                    make_t(TT::Write), make_t(TT::LParen), make_n(NT::Exp), make_t(TT::RParen)};
                return &production;
            }
            if (lookahead == TT::Return) {
                static const Production production = {
                    make_t(TT::Return), make_t(TT::LParen), make_n(NT::Exp), make_t(TT::RParen)};
                return &production;
            }
            if (lookahead == TT::Id) {
                static const Production production = {
                    make_t(TT::Id), make_n(NT::IdStatementTail)};
                return &production;
            }
            return nullptr;

        case NT::IdStatementTail:
            if (lookahead == TT::LParen) {
                static const Production production = {
                    make_t(TT::LParen), make_n(NT::ActParamList), make_t(TT::RParen)};
                return &production;
            }
            if (lookahead == TT::LMidParen || lookahead == TT::Dot ||
                lookahead == TT::Assign) {
                static const Production production = {
                    make_n(NT::VariableTail), make_t(TT::Assign), make_n(NT::Exp)};
                return &production;
            }
            return nullptr;

        case NT::VariableTail:
            if (lookahead == TT::LMidParen) {
                static const Production production = {
                    make_t(TT::LMidParen), make_n(NT::Exp), make_t(TT::RMidParen)};
                return &production;
            }
            if (lookahead == TT::Dot) {
                static const Production production = {
                    make_t(TT::Dot), make_n(NT::FieldVar)};
                return &production;
            }
            if (lookahead == TT::Assign || lookahead == TT::Times ||
                lookahead == TT::Over || lookahead == TT::Plus ||
                lookahead == TT::Minus || lookahead == TT::Lt ||
                lookahead == TT::Eq || lookahead == TT::Then ||
                lookahead == TT::Do || lookahead == TT::RParen ||
                lookahead == TT::RMidParen || lookahead == TT::Semi ||
                lookahead == TT::Else || lookahead == TT::Fi ||
                lookahead == TT::EndWh || lookahead == TT::Comma ||
                lookahead == TT::End) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::FieldVar:
            if (lookahead == TT::Id) {
                static const Production production = {
                    make_t(TT::Id), make_n(NT::FieldVarTail)};
                return &production;
            }
            return nullptr;

        case NT::FieldVarTail:
            if (lookahead == TT::LMidParen) {
                static const Production production = {
                    make_t(TT::LMidParen), make_n(NT::Exp), make_t(TT::RMidParen)};
                return &production;
            }
            if (lookahead == TT::Assign || lookahead == TT::Times ||
                lookahead == TT::Over || lookahead == TT::Plus ||
                lookahead == TT::Minus || lookahead == TT::Lt ||
                lookahead == TT::Eq || lookahead == TT::Then ||
                lookahead == TT::Do || lookahead == TT::RParen ||
                lookahead == TT::RMidParen || lookahead == TT::Semi ||
                lookahead == TT::Else || lookahead == TT::Fi ||
                lookahead == TT::EndWh || lookahead == TT::Comma ||
                lookahead == TT::End) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::ActParamList:
            if (lookahead == TT::LParen || lookahead == TT::IntConst ||
                lookahead == TT::CharConst || lookahead == TT::Id) {
                static const Production production = {
                    make_n(NT::Exp), make_n(NT::ActParamMore)};
                return &production;
            }
            if (lookahead == TT::RParen) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::ActParamMore:
            if (lookahead == TT::Comma) {
                static const Production production = {
                    make_t(TT::Comma), make_n(NT::Exp), make_n(NT::ActParamMore)};
                return &production;
            }
            if (lookahead == TT::RParen) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::RelExp:
            if (lookahead == TT::LParen || lookahead == TT::IntConst ||
                lookahead == TT::CharConst || lookahead == TT::Id) {
                static const Production production = {
                    make_n(NT::Exp), make_n(NT::CmpOp), make_n(NT::Exp)};
                return &production;
            }
            return nullptr;

        case NT::CmpOp:
            if (lookahead == TT::Lt) {
                static const Production production = {make_t(TT::Lt)};
                return &production;
            }
            if (lookahead == TT::Eq) {
                static const Production production = {make_t(TT::Eq)};
                return &production;
            }
            return nullptr;

        case NT::Exp:
            if (lookahead == TT::LParen || lookahead == TT::IntConst ||
                lookahead == TT::CharConst || lookahead == TT::Id) {
                static const Production production = {make_n(NT::Term), make_n(NT::ExpTail)};
                return &production;
            }
            return nullptr;

        case NT::ExpTail:
            if (lookahead == TT::Plus) {
                static const Production production = {make_t(TT::Plus), make_n(NT::Exp)};
                return &production;
            }
            if (lookahead == TT::Minus) {
                static const Production production = {make_t(TT::Minus), make_n(NT::Exp)};
                return &production;
            }
            if (lookahead == TT::Lt || lookahead == TT::Eq ||
                lookahead == TT::Then || lookahead == TT::Do ||
                lookahead == TT::RParen || lookahead == TT::RMidParen ||
                lookahead == TT::Semi || lookahead == TT::Else ||
                lookahead == TT::Fi || lookahead == TT::EndWh ||
                lookahead == TT::Comma || lookahead == TT::End) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::Term:
            if (lookahead == TT::LParen || lookahead == TT::IntConst ||
                lookahead == TT::CharConst || lookahead == TT::Id) {
                static const Production production = {
                    make_n(NT::Factor), make_n(NT::TermTail)};
                return &production;
            }
            return nullptr;

        case NT::TermTail:
            if (lookahead == TT::Times) {
                static const Production production = {make_t(TT::Times), make_n(NT::Term)};
                return &production;
            }
            if (lookahead == TT::Over) {
                static const Production production = {make_t(TT::Over), make_n(NT::Term)};
                return &production;
            }
            if (lookahead == TT::Plus || lookahead == TT::Minus ||
                lookahead == TT::Lt || lookahead == TT::Eq ||
                lookahead == TT::Then || lookahead == TT::Do ||
                lookahead == TT::RParen || lookahead == TT::RMidParen ||
                lookahead == TT::Semi || lookahead == TT::Else ||
                lookahead == TT::Fi || lookahead == TT::EndWh ||
                lookahead == TT::Comma || lookahead == TT::End) {
                static const Production epsilon = {};
                return &epsilon;
            }
            return nullptr;

        case NT::Factor:
            if (lookahead == TT::LParen) {
                static const Production production = {
                    make_t(TT::LParen), make_n(NT::Exp), make_t(TT::RParen)};
                return &production;
            }
            if (lookahead == TT::IntConst) {
                static const Production production = {make_t(TT::IntConst)};
                return &production;
            }
            if (lookahead == TT::CharConst) {
                static const Production production = {make_t(TT::CharConst)};
                return &production;
            }
            if (lookahead == TT::Id) {
                static const Production production = {
                    make_t(TT::Id), make_n(NT::VariableTail)};
                return &production;
            }
            return nullptr;
    }

    return nullptr;
}

void LL1Parser::add_syntax_error(SourceLocation location,
                                 const std::string& message) {
    diagnostics_.push_back(Diagnostic{
        DiagnosticStage::Syntax,
        location,
        message,
    });
}

LL1Parser::Symbol LL1Parser::terminal(TokenType type) {
    return Symbol{true, type, NonTerminal::Program};
}

LL1Parser::Symbol LL1Parser::non_terminal(NonTerminal symbol) {
    return Symbol{false, TokenType::EndFile, symbol};
}

const char* LL1Parser::non_terminal_name(NonTerminal symbol) {
    using NT = NonTerminal;
    switch (symbol) {
        case NT::Program:
            return "program";
        case NT::ProgramHead:
            return "program head";
        case NT::DeclarePart:
            return "declaration part";
        case NT::TypeDec:
            return "type declaration section";
        case NT::TypeDecList:
            return "type declaration list";
        case NT::TypeName:
            return "type name";
        case NT::BaseType:
            return "base type";
        case NT::StructureType:
            return "structure type";
        case NT::ArrayType:
            return "array type";
        case NT::RecordType:
            return "record type";
        case NT::FieldDecList:
            return "record field declaration list";
        case NT::FieldDec:
            return "record field declaration";
        case NT::IdList:
            return "identifier list";
        case NT::IdListTail:
            return "identifier list tail";
        case NT::VarDec:
            return "variable declaration section";
        case NT::VarDecList:
            return "variable declaration list";
        case NT::ProcDec:
            return "procedure declaration section";
        case NT::ProcDeclaration:
            return "procedure declaration";
        case NT::ParamList:
            return "parameter list";
        case NT::ParamMore:
            return "more parameters";
        case NT::Param:
            return "parameter";
        case NT::ProgramBody:
            return "program body";
        case NT::StmList:
            return "statement list";
        case NT::StmMore:
            return "more statements";
        case NT::StmMoreAfterSemi:
            return "statement after semicolon";
        case NT::Statement:
            return "statement";
        case NT::IdStatementTail:
            return "identifier statement tail";
        case NT::VariableTail:
            return "variable suffix";
        case NT::FieldVar:
            return "record field";
        case NT::FieldVarTail:
            return "record field suffix";
        case NT::ActParamList:
            return "actual parameter list";
        case NT::ActParamMore:
            return "more actual parameters";
        case NT::RelExp:
            return "relational expression";
        case NT::CmpOp:
            return "comparison operator";
        case NT::Exp:
            return "expression";
        case NT::ExpTail:
            return "expression tail";
        case NT::Term:
            return "term";
        case NT::TermTail:
            return "term tail";
        case NT::Factor:
            return "factor";
    }
    return "grammar symbol";
}

const char* LL1Parser::token_display_name(TokenType type) {
    switch (type) {
        case TokenType::Program:
            return "'program'";
        case TokenType::Procedure:
            return "'procedure'";
        case TokenType::Type:
            return "'type'";
        case TokenType::Var:
            return "'var'";
        case TokenType::If:
            return "'if'";
        case TokenType::Then:
            return "'then'";
        case TokenType::Else:
            return "'else'";
        case TokenType::Fi:
            return "'fi'";
        case TokenType::While:
            return "'while'";
        case TokenType::Do:
            return "'do'";
        case TokenType::EndWh:
            return "'endwh'";
        case TokenType::Begin:
            return "'begin'";
        case TokenType::End:
            return "'end'";
        case TokenType::Read:
            return "'read'";
        case TokenType::Write:
            return "'write'";
        case TokenType::Array:
            return "'array'";
        case TokenType::Of:
            return "'of'";
        case TokenType::Record:
            return "'record'";
        case TokenType::Return:
            return "'return'";
        case TokenType::Integer:
            return "'integer'";
        case TokenType::Char:
            return "'char'";
        case TokenType::Id:
            return "identifier";
        case TokenType::IntConst:
            return "integer constant";
        case TokenType::CharConst:
            return "character constant";
        case TokenType::Assign:
            return "':='";
        case TokenType::Eq:
            return "'='";
        case TokenType::Lt:
            return "'<'";
        case TokenType::Plus:
            return "'+'";
        case TokenType::Minus:
            return "'-'";
        case TokenType::Times:
            return "'*'";
        case TokenType::Over:
            return "'/'";
        case TokenType::LParen:
            return "'('";
        case TokenType::RParen:
            return "')'";
        case TokenType::LMidParen:
            return "'['";
        case TokenType::RMidParen:
            return "']'";
        case TokenType::Dot:
            return "'.'";
        case TokenType::Semi:
            return "';'";
        case TokenType::Comma:
            return "','";
        case TokenType::Underange:
            return "'..'";
        case TokenType::EndFile:
            return "EOF";
    }
    return "token";
}

}  // namespace snl
