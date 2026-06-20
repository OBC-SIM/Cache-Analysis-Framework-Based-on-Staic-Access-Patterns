#pragma once
#include "ap/ApProgram.hpp"
#include "cache/CacheConfig.hpp"
#include "pipeline/PipelineResult.hpp"

namespace apex
{

/**
 * @brief LAT v2 ApProgram을 받아 메모리 배치 → 캐시 시뮬레이션 → miss 귀속까지
 *        한 번에 수행한다.
 *
 * 객체 base는 cache line_size로 정렬되어 miss 상한(upper-bound) 모델을 따른다
 * (README "메모리 모델" 참조). core_id는 AccessEvent 값을 그대로 사용한다.
 */
class Pipeline
{
public:
  explicit Pipeline(HierarchyConfig config);

  /**
   * @brief LAT v2 ApProgram을 받아 분석 결과를 생성한다.
   *
   * 객체 base는 metadata.objects 크기로 line_size 정렬 배치되고, 각 접근의
   * byte_address = base + byte_offset(EventBuilder가 채움)으로 계산된다.
   *
   * @param program 파싱된 ApProgram
   * @return miss 집계 및 귀속 결과
   */
  PipelineResult run(const ApProgram & program);

private:
  HierarchyConfig config_;
};

}  // namespace apex
