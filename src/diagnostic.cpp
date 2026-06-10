#include "diagnostic.h"

#include <sstream>

namespace snl {

std::string diagnostic_stage_name(DiagnosticStage stage) {
    switch (stage) {
        case DiagnosticStage::Lexical:
            return "lexical";
        case DiagnosticStage::Syntax:
            return "syntax";
        case DiagnosticStage::Semantic:
            return "semantic";
        case DiagnosticStage::Codegen:
            return "codegen";
        case DiagnosticStage::CommandLine:
            return "command-line";
        case DiagnosticStage::File:
            return "file";
    }
    return "unknown";
}

std::string format_diagnostic(const Diagnostic& diagnostic) {
    std::ostringstream out;
    out << diagnostic_stage_name(diagnostic.stage) << " error";
    if (diagnostic.location.line > 0 && diagnostic.location.column > 0) {
        out << " at " << diagnostic.location.line << ':'
            << diagnostic.location.column;
    }
    out << ": " << diagnostic.message;
    return out.str();
}

}  // namespace snl
