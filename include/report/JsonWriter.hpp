#pragma once
#include <iosfwd>
#include "analysis/Diagnostics.hpp"

namespace apex {

/**
 * @brief 분석 결과를 JSON 형식으로 출력한다.
 *
 * nlohmann/json을 사용하며 std::ostream을 받아 파일·스트림 모두 지원한다.
 */
struct JsonWriter {
    /**
     * @brief miss 유형·op별 요약을 JSON 객체로 출력한다.
     * @param os    출력 스트림
     * @param stats 집계된 miss 통계
     */
    static void write_summary(std::ostream& os, const MissStats& stats);
};

}  // namespace apex