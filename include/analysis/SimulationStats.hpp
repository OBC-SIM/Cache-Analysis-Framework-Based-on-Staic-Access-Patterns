#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace apex
{

struct ObjectAccessStats
{
  uint64_t accesses = 0;
  uint64_t hits = 0;
  uint64_t misses = 0;
  uint64_t load_accesses = 0;
  uint64_t store_accesses = 0;
  uint64_t load_misses = 0;
  uint64_t store_misses = 0;

  double miss_rate() const
  {
    if (accesses == 0) return 0.0;
    return static_cast<double>(misses) / static_cast<double>(accesses);
  }
};

/**
 * @brief 캐시 계층 전체의 접근·hit/miss·cycle 통계를 누적한다.
 *
 * miss_level은 CacheHierarchy의 계약을 따른다: 0은 L1 hit, 1은 L2 hit,
 * 2 이상은 memory access이다.
 */
struct SimulationStats
{
  uint64_t total_accesses = 0;
  uint64_t load_accesses = 0;
  uint64_t store_accesses = 0;
  uint64_t l1_hits = 0;
  uint64_t l1_misses = 0;
  uint64_t l2_hits = 0;
  uint64_t l2_misses = 0;
  uint64_t memory_accesses = 0;
  uint64_t total_cycles = 0;
  std::unordered_map<std::string, ObjectAccessStats> objects;

  void record_access(const std::string & object_name, const std::string & op,
                     int miss_level, uint64_t delay_cycles)
  {
    total_accesses += 1;
    total_cycles += delay_cycles;

    const bool is_store = (op == "store");
    if (is_store)
      store_accesses += 1;
    else
      load_accesses += 1;

    ObjectAccessStats & object = objects[object_name];
    object.accesses += 1;
    if (is_store)
      object.store_accesses += 1;
    else
      object.load_accesses += 1;

    if (miss_level == 0)
    {
      l1_hits += 1;
      object.hits += 1;
      return;
    }

    l1_misses += 1;
    object.misses += 1;
    if (is_store)
      object.store_misses += 1;
    else
      object.load_misses += 1;

    if (miss_level == 1)
    {
      l2_hits += 1;
      return;
    }

    l2_misses += 1;
    memory_accesses += 1;
  }

  double average_cycles_per_access() const
  {
    if (total_accesses == 0) return 0.0;
    return static_cast<double>(total_cycles) /
           static_cast<double>(total_accesses);
  }

  double l1_hit_rate() const
  {
    if (total_accesses == 0) return 0.0;
    return static_cast<double>(l1_hits) / static_cast<double>(total_accesses);
  }

  double l2_hit_rate() const
  {
    if (l1_misses == 0) return 0.0;
    return static_cast<double>(l2_hits) / static_cast<double>(l1_misses);
  }
};

}  // namespace apex
