#include "report/MarkdownWriter.hpp"
#include <ostream>

namespace apex {

void MarkdownWriter::write_diagnostics(std::ostream& os,
                                       const std::vector<DiagnosticHint>& hints) {
    os << "# Diagnostics\n\n";
    os << "| kind | message | object |\n";
    os << "|------|---------|--------|\n";
    for (auto& h : hints) {
        os << "| " << h.kind
           << " | " << h.message
           << " | " << h.object
           << " |\n";
    }
}

}  // namespace apex
