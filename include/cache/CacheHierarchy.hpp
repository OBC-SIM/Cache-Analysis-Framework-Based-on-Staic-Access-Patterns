#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "cache/CacheConfig.hpp"
#include "cache/CacheLevel.hpp"

struct HierarchyAccessResult
{
  uint64_t delay_cycles;
  int miss_level;  // 0 = L1 hit, 1 = L2 hit, 2 = memory
};

/**
 * @brief private L1(s) → shared L2 → Memory 체인 시뮬레이터.
 *
 * cache.yaml의 write_policy와 write_allocate 설정에 따라 store miss fill과
 * write-through 전파 여부를 결정한다.
 *
 * @pre config.caches의 role="L1" 항목은 private_to >= 0이어야 한다.
 */
class CacheHierarchy
{
public:
  explicit CacheHierarchy(const HierarchyConfig & config);
  HierarchyAccessResult access(int core_id, uint64_t cache_line, bool is_store);

private:
  struct LevelEntry
  {
    CacheLevel level;
    int delay_cycles = 0;
    WritePolicy write_policy = WritePolicy::WriteBack;
    bool write_allocate = true;
  };

  std::vector<LevelEntry> l1s_;  // indexed by core_id
  LevelEntry l2_;
  int mem_delay_ = 120;
};
