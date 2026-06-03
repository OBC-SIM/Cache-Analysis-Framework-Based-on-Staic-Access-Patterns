#pragma once
#include <memory>
#include <vector>

#include "analysis/Attribution.hpp"
#include "analysis/Diagnostics.hpp"  // MissStats
#include "ap/AccessEvent.hpp"
#include "ap/ApNode.hpp"
#include "ap/ApProgram.hpp"
#include "cache/CacheConfig.hpp"

namespace apex
{

/**
 * @brief 전체 분석 파이프라인의 결과.
 *
 * stats는 Report Writer가 소비하는 miss 유형·op·object 집계,
 * attribution은 region inclusive/exclusive 등 세부 귀속.
 */
struct PipelineResult
{
  MissStats stats;
  Attribution attribution;
};

/**
 * @brief AP 노드 트리를 받아 메모리 배치 → 캐시 시뮬레이션 → miss 귀속까지
 *        한 번에 수행한다.
 *
 * 객체 base는 cache line_size로 정렬되어 miss 상한(upper-bound) 모델을 따른다
 * (README "메모리 모델" 참조). core_id는 AccessEvent 값을 그대로 사용하며,
 * 함수 간 call 전개는 MVP 범위 밖이다(top-level 노드만 처리).
 */
class Pipeline
{
public:
  explicit Pipeline(HierarchyConfig config);

  /**
   * @brief 노드 트리를 소비하여 분석 결과를 생성한다.
   * @param nodes 최상위 ApNode 목록 (소유권 이전)
   * @return miss 집계 및 귀속 결과
   */
  PipelineResult run(std::vector<std::unique_ptr<ApNode>> nodes);

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

  /// cache_line이 채워진 이벤트 스트림을 시뮬레이션해 miss를 분류·귀속한다.
  PipelineResult simulate(const std::vector<AccessEvent> & events);
};

}  // namespace apex
