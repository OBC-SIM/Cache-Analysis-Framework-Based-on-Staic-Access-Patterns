#pragma once

#include <string>
#include <vector>

#include "analysis/Diagnostics.hpp"
#include "analysis/SimulationStats.hpp"
#include "cache/CacheConfig.hpp"

namespace apex
{

enum class RunLogMode
{
  Default,
  Quiet,
  Verbose
};

struct RunLogContext
{
  std::string input_path;
  std::string cache_path;
  std::string output_dir;
  std::vector<std::string> report_paths;
  std::vector<std::string> roots;
  std::size_t object_count = 0;
  HierarchyConfig cache_config;
  MissStats miss_stats;
  SimulationStats cache_stats;
};

/**
 * @brief apex-cache run 결과를 터미널 요약 문자열로 포맷한다.
 *
 * formatter는 파일 I/O를 하지 않으며, 호출자가 수집한 run context와 pipeline
 * 결과만 사용한다. 성공 summary는 stdout에 출력하기 위한 문자열이다.
 */
class RunLogger
{
public:
  /**
   * @brief 지정된 verbosity 모드에 맞는 run summary를 생성한다.
   * @param context 입력·설정·리포트 경로와 pipeline 결과
   * @param mode 출력 상세 수준
   * @return 터미널에 출력할 summary 문자열
   */
  static std::string format(const RunLogContext & context, RunLogMode mode);
  static std::string format(const RunLogContext & context, RunLogMode mode,
                            bool color_enabled);
};

}  // namespace apex
