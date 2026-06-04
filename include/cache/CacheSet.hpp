#pragma once
#include <cstdint>
#include <list>

struct SetAccessResult
{
  bool hit = false;
  bool has_eviction = false;
  uint64_t evicted_tag = 0;
  bool evicted_dirty = false;
};

/**
 * @brief CacheSet 접근 시 fill과 dirty 표시 정책을 지정한다.
 *
 * @param is_store store 접근이면 true.
 * @param allocate_on_miss miss 시 새 line을 채울지 여부.
 * @param mark_dirty_on_store store hit/fill 시 dirty bit를 표시할지 여부.
 */
struct CacheAccessOptions
{
  bool is_store = false;
  bool allocate_on_miss = true;
  bool mark_dirty_on_store = true;
};

/**
 * @brief N-way LRU 캐시 셋.
 *
 * front = MRU, back = LRU.
 * 옵션에 따라 미스 시 새 태그를 채우고, 용량 초과 시 LRU를 퇴출한다.
 */
class CacheSet
{
public:
  explicit CacheSet(int num_ways);
  SetAccessResult access(uint64_t tag, bool is_store);

  /**
   * @brief 옵션에 따라 태그를 조회하고 필요 시 fill한다.
   *
   * @param tag set-local cache tag.
   * @param options 접근 종류, miss allocation, dirty 표시 정책.
   * @return hit 여부와 eviction metadata.
   */
  SetAccessResult access(uint64_t tag, const CacheAccessOptions & options);

private:
  struct Entry
  {
    uint64_t tag;
    bool dirty;
  };
  int num_ways_;
  std::list<Entry> entries_;  // front = MRU, back = LRU
};
