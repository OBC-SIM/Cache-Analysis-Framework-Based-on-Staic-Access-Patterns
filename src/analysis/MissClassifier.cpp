#include "analysis/MissClassifier.hpp"

namespace apex
{

MissClassifier::MissClassifier(int fa_capacity_lines)
  : fa_capacity_(fa_capacity_lines)
{
  if (fa_capacity_ > 0)
  {
    fa_nodes_.reserve(static_cast<size_t>(fa_capacity_));
    fa_map_.reserve(static_cast<size_t>(fa_capacity_));
  }
}

bool MissClassifier::fa_access(uint64_t cache_line)
{
  if (fa_capacity_ <= 0) return false;

  auto it = fa_map_.find(cache_line);
  bool hit = (it != fa_map_.end());

  if (hit)
  {
    int index = it->second;
    detach_fa_node(index);
    append_fa_mru(index);
    return true;
  }

  int index = -1;
  if (static_cast<int>(fa_nodes_.size()) < fa_capacity_)
  {
    index = static_cast<int>(fa_nodes_.size());
    fa_nodes_.push_back({cache_line, -1, -1});
  }
  else
  {
    index = fa_lru_head_;
    fa_map_.erase(fa_nodes_[index].cache_line);
    detach_fa_node(index);
    fa_nodes_[index].cache_line = cache_line;
  }

  append_fa_mru(index);
  fa_map_[cache_line] = index;
  return false;
}

void MissClassifier::detach_fa_node(int index)
{
  FaNode & node = fa_nodes_[index];
  if (node.prev >= 0)
    fa_nodes_[node.prev].next = node.next;
  else
    fa_lru_head_ = node.next;

  if (node.next >= 0)
    fa_nodes_[node.next].prev = node.prev;
  else
    fa_mru_tail_ = node.prev;

  node.prev = -1;
  node.next = -1;
}

void MissClassifier::append_fa_mru(int index)
{
  FaNode & node = fa_nodes_[index];
  node.prev = fa_mru_tail_;
  node.next = -1;

  if (fa_mru_tail_ >= 0)
    fa_nodes_[fa_mru_tail_].next = index;
  else
    fa_lru_head_ = index;

  fa_mru_tail_ = index;
}

std::optional<MissType> MissClassifier::classify(uint64_t cache_line,
                                                 bool is_actual_miss,
                                                 bool fill_l1)
{
  bool is_cold = (ever_seen_.count(cache_line) == 0);
  bool fa_hit = fill_l1 ? fa_access(cache_line) : false;
  ever_seen_.insert(cache_line);

  if (!is_actual_miss) return std::nullopt;

  if (is_cold) return MissType::Cold;
  if (!fill_l1) return MissType::Policy;
  if (fa_hit) return MissType::Conflict;
  return MissType::Capacity;
}

}  // namespace apex
