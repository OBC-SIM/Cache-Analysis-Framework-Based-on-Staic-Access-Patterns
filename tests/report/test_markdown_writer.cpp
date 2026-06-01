#include <gtest/gtest.h>
#include <sstream>
#include "report/MarkdownWriter.hpp"

// MarkdownWriter::write_diagnostics(os, hints) → Markdown 표 형식 출력

TEST(MarkdownWriter, empty_hints_outputs_header_only) {
    std::ostringstream oss;
    apex::MarkdownWriter::write_diagnostics(oss, {});
    std::string out = oss.str();
    EXPECT_NE(out.find("#"), std::string::npos);  // 제목 행 존재
}

TEST(MarkdownWriter, hint_appears_in_output) {
    std::vector<apex::DiagnosticHint> hints = {
        {"conflict_padding", "padding/layout 검토", ""}
    };
    std::ostringstream oss;
    apex::MarkdownWriter::write_diagnostics(oss, hints);
    std::string out = oss.str();
    EXPECT_NE(out.find("conflict_padding"), std::string::npos);
    EXPECT_NE(out.find("padding/layout 검토"), std::string::npos);
}

TEST(MarkdownWriter, object_targeted_hint_includes_object_name) {
    std::vector<apex::DiagnosticHint> hints = {
        {"object_targeted", "A 우선 최적화 검토", "A"}
    };
    std::ostringstream oss;
    apex::MarkdownWriter::write_diagnostics(oss, hints);
    std::string out = oss.str();
    EXPECT_NE(out.find("A"), std::string::npos);
}