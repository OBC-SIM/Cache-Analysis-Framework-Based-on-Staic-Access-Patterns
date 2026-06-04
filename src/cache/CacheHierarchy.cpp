#include "cache/CacheHierarchy.hpp"

#include <stdexcept>

namespace
{

CacheAccessOptions options_for(bool is_store, WritePolicy write_policy,
                               bool write_allocate)
{
  CacheAccessOptions options;
  options.is_store = is_store;
  options.allocate_on_miss = !is_store || write_allocate;
  options.mark_dirty_on_store =
    is_store && write_policy == WritePolicy::WriteBack;
  return options;
}

}  // namespace

CacheHierarchy::CacheHierarchy(const HierarchyConfig & config)
{
  mem_delay_ = config.memory.delay_cycles;

  for (const auto & c : config.caches)
  {
    int num_sets = static_cast<int>(
      c.size_bytes / (static_cast<uint64_t>(c.line_size) * c.associativity));

    LevelEntry entry;
    entry.level = CacheLevel(num_sets, c.associativity);
    entry.delay_cycles = c.delay_cycles;
    entry.write_policy = c.write_policy;
    entry.write_allocate = c.write_allocate;

    if (c.role == "L1")
    {
      int core = c.private_to;
      if (core < 0)
        throw std::runtime_error("L1 cache must have private_to >= 0");
      while (static_cast<int>(l1s_.size()) <= core) l1s_.emplace_back();
      l1s_[core] = std::move(entry);
    }
    else
    {
      l2_ = std::move(entry);
    }
  }
}

HierarchyAccessResult CacheHierarchy::access(int core_id, uint64_t cache_line,
                                             bool is_store)
{
  auto & l1e = l1s_[core_id];
  const auto l1_options =
    options_for(is_store, l1e.write_policy, l1e.write_allocate);

  auto r1 = l1e.level.access(cache_line, l1_options);
  if (r1.hit)
  {
    uint64_t delay = static_cast<uint64_t>(l1e.delay_cycles);
    if (is_store && l1e.write_policy == WritePolicy::WriteThrough)
    {
      auto r2 = l2_.level.access(
        cache_line, options_for(true, l2_.write_policy, l2_.write_allocate));
      delay += static_cast<uint64_t>(l2_.delay_cycles);
      if (!r2.hit) delay += static_cast<uint64_t>(mem_delay_);
    }
    return {delay, 0};
  }

  const bool l2_store =
    is_store && (l1e.write_policy == WritePolicy::WriteThrough ||
                 !l1e.write_allocate);
  auto r2 = l2_.level.access(
    cache_line, options_for(l2_store, l2_.write_policy, l2_.write_allocate));
  if (r2.hit)
  {
    return {static_cast<uint64_t>(l1e.delay_cycles + l2_.delay_cycles), 1};
  }

  return {
    static_cast<uint64_t>(l1e.delay_cycles + l2_.delay_cycles + mem_delay_), 2};
}
