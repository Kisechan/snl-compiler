#pragma once

#include <string>

namespace snl {

struct SourceLocation {
    int line = 1;
    int column = 1;
};

enum class DiagnosticStage {
    Lexical,
    CommandLine,
    File
};

struct Diagnostic {
    DiagnosticStage stage = DiagnosticStage::Lexical;
    SourceLocation location;
    std::string message;
};

std::string diagnostic_stage_name(DiagnosticStage stage);
std::string format_diagnostic(const Diagnostic& diagnostic);

}  // namespace snl
