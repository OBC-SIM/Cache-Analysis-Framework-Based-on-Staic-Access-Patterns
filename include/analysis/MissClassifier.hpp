#pragma once
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace apex
{

enum class MissType
{
  Cold,
  Capacity,
  Conflict,
  Policy
};

/**
 * @brief L1 miss를 cold / capacity / conflict / policy 로 분류한다.
 *
 * 동일 총 용량의 FA 쉐도우 캐시(LRU)를 실제 캐시와 병렬로 유지한다.
 *   cold     : 처음 보는 cache_line
 *   conflict : 실제 캐시 miss + FA hit  (set 배치 충돌)
 *   capacity : 실제 캐시 miss + FA miss (총 용량 부족)
 *   policy   : 정책상 L1에 fill되지 않아 반복되는 miss
 *
 * @note fill_l1이 true인 접근만 FA 상태를 갱신한다. no-write-allocate store
 *       miss처럼 실제 L1을 bypass하는 접근은 FA에도 fill하지 않는다.
 *
 * @param fa_capacity_lines  FA 쉐도우 캐시의 총 라인 수 (= 실제 캐시 size_bytes
 * / line_size)
 */
class MissClassifier
{
public:
  explicit MissClassifier(int fa_capacity_lines);

  /**
   * @brief 접근 1회를 처리하고 miss 유형을 반환한다.
   *
   * @param cache_line      접근한 캐시 라인 번호
   * @param is_actual_miss  실제 set-assoc 캐시에서 miss 발생 여부
   * @param fill_l1         실제 L1에 line을 채우거나 이미 존재하는 접근 여부
   * @return L1 miss 시 MissType, hit 시 std::nullopt
   */
  std::optional<MissType> classify(uint64_t cache_line, bool is_actual_miss,
                                   bool fill_l1 = true);

private:
  struct FaNode
  {
    uint64_t cache_line = 0;
    int prev = -1;
    int next = -1;
  };

  int fa_capacity_;
  int fa_lru_head_ = -1;
  int fa_mru_tail_ = -1;

  std::unordered_set<uint64_t> ever_seen_;
  std::vector<FaNode> fa_nodes_;
  std::unordered_map<uint64_t, int> fa_map_;

  // FA 쉐도우 캐시 접근: 히트 여부 반환 후 LRU 갱신 및 용량 초과 시 퇴출.
  bool fa_access(uint64_t cache_line);
  void detach_fa_node(int index);
  void append_fa_mru(int index);
};

}  // namespace apex
