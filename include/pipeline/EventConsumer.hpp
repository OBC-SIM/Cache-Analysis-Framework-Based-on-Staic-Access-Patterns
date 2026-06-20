#pragma once

#include <map>

#include "analysis/MissClassifier.hpp"
#include "ap/AccessEvent.hpp"
#include "cache/CacheConfig.hpp"
#include "cache/CacheHierarchy.hpp"
#include "memory/MemoryLayout.hpp"
#include "pipeline/PipelineResult.hpp"

namespace apex
{

/** @brief Streaming access event에 주소를 부여하고 즉시 cache simulation한다. */
class EventConsumer
{
public:
  /**
   * @brief 고정된 cache 설정과 object memory layout으로 consumer를 초기화한다.
   * @param config 분석에 사용할 cache hierarchy 설정
   * @param layout consumer보다 오래 유지되는 object memory layout
   * @param line_size L1 cache line 크기(byte)
   */
  EventConsumer(const HierarchyConfig & config, const MemoryLayout & layout,
                int line_size);

  /** @brief 단일 event의 주소 계산, cache 접근, 통계 집계를 완료한다. */
  void consume(AccessEvent && event);

  /** @brief 지금까지 누적한 분석 결과를 이동해 반환한다. */
  PipelineResult take_result();

private:
  MissClassifier & classifier_for(int core);

  const HierarchyConfig & config_;
  const MemoryLayout & layout_;
  int line_size_;
  CacheHierarchy hierarchy_;
  std::map<int, MissClassifier> classifiers_;
  PipelineResult result_;
};

}  // namespace apex
