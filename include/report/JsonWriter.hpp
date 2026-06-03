#pragma once
#include <iosfwd>

#include "analysis/Diagnostics.hpp"
#include "analysis/SimulationStats.hpp"

namespace apex
{

/**
 * @brief 분석 결과를 JSON 형식으로 출력한다.
 *
 * nlohmann/json을 사용하며 std::ostream을 받아 파일·스트림 모두 지원한다.
 */
struct JsonWriter
{
  /**
   * @brief miss 유형·op별 요약을 JSON 객체로 출력한다.
   * @param os    출력 스트림
   * @param stats 집계된 miss 통계
   */
  static void write_summary(std::ostream & os, const MissStats & stats);

  /**
   * @brief miss 요약과 캐시 동작 통계를 함께 JSON 객체로 출력한다.
   * @param os          출력 스트림
   * @param miss_stats  집계된 miss 통계
   * @param cache_stats 전체 접근·계층·cycle 통계
   */
  static void write_summary(std::ostream & os, const MissStats & miss_stats,
                            const SimulationStats & cache_stats);
};

}  // namespace apex
