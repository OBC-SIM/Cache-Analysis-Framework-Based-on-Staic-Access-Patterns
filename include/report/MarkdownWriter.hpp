#pragma once
#include <iosfwd>
#include <vector>
#include "analysis/Diagnostics.hpp"

namespace apex {

/**
 * @brief 진단 힌트를 Markdown 형식으로 출력한다.
 */
struct MarkdownWriter {
    /**
     * @brief DiagnosticHint 목록을 Markdown 표로 출력한다.
     * @param os    출력 스트림
     * @param hints Diagnostics::generate() 결과
     */
    static void write_diagnostics(std::ostream& os,
                                  const std::vector<DiagnosticHint>& hints);
};

}  // namespace apex
