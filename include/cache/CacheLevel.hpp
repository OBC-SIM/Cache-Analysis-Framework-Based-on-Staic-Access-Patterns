#pragma once
#include <cstdint>
#include <vector>

#include "cache/CacheSet.hpp"

struct LevelAccessResult
{
  bool hit;
  SetAccessResult set_result;
  uint64_t evicted_cache_line = 0;
};

/**
 * @brief num_sets 개의 CacheSet으로 구성된 캐시 레벨.
 *
 * set_index = cache_line % num_sets
 * tag       = cache_line / num_sets
 * 옵션에 따라 미스 시 해당 셋에 채운다.
 */
class CacheLevel
{
public:
  CacheLevel() = default;
  CacheLevel(int num_sets, int num_ways);

  LevelAccessResult access(uint64_t cache_line, bool is_store);

  /**
   * @brief 옵션에 따라 cache line을 접근하고 set 결과를 반환한다.
   *
   * @param cache_line flat cache line 번호.
   * @param options 접근 종류, miss allocation, dirty 표시 정책.
   * @return level hit 여부와 set-level eviction metadata.
   */
  LevelAccessResult access(uint64_t cache_line,
                           const CacheAccessOptions & options);

  static uint64_t set_index_of(uint64_t cache_line, int num_sets);
  static uint64_t tag_of(uint64_t cache_line, int num_sets);

private:
  int num_sets_ = 0;
  std::vector<CacheSet> sets_;
};
