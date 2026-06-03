#pragma once
#include <iosfwd>

#include "analysis/Attribution.hpp"
#include "analysis/Diagnostics.hpp"
#include "analysis/SimulationStats.hpp"

namespace apex
{

/**
 * @brief 분석 결과를 CSV 형식으로 출력한다.
 *
 * 파일 경로 대신 std::ostream을 받아 테스트 용이성을 확보한다.
 * 호출자가 파일 스트림을 열어 전달하는 방식으로 사용한다.
 */
struct CsvWriter
{
  /**
   * @brief miss 유형·op별 요약 1행을 출력한다.
   * @param os    출력 스트림
   * @param stats 집계된 miss 통계
   */
  static void write_summary(std::ostream & os, const MissStats & stats);

  /**
   * @brief miss 요약과 캐시 동작 통계를 CSV 1행으로 출력한다.
   * @param os          출력 스트림
   * @param miss_stats  집계된 miss 통계
   * @param cache_stats 전체 접근·계층·cycle 통계
   */
  static void write_summary(std::ostream & os, const MissStats & miss_stats,
                            const SimulationStats & cache_stats);

  /**
   * @brief object별 miss 카운트 테이블을 출력한다.
   * @param os   출력 스트림
   * @param attr Attribution 집계 결과
   */
  static void write_object_breakdown(std::ostream & os,
                                     const Attribution & attr);

  /**
   * @brief object별 access/hit/miss/miss_rate 테이블을 출력한다.
   * @param os          출력 스트림
   * @param cache_stats 전체 접근·계층·cycle 통계
   */
  static void write_object_breakdown(std::ostream & os,
                                     const SimulationStats & cache_stats);
};

}  // namespace apex
