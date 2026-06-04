#include "cache/CacheSet.hpp"

CacheSet::CacheSet(int num_ways) : num_ways_(num_ways) {}

SetAccessResult CacheSet::access(uint64_t tag, bool is_store)
{
  CacheAccessOptions options;
  options.is_store = is_store;
  return access(tag, options);
}

SetAccessResult CacheSet::access(uint64_t tag,
                                 const CacheAccessOptions & options)
{
  SetAccessResult result;

  for (auto it = entries_.begin(); it != entries_.end(); ++it)
  {
    if (it->tag == tag)
    {
      result.hit = true;
      if (options.is_store && options.mark_dirty_on_store) it->dirty = true;
      entries_.splice(entries_.begin(), entries_, it);  // promote to MRU
      return result;
    }
  }

  if (!options.allocate_on_miss) return result;

  // Miss: evict LRU if at capacity
  if (static_cast<int>(entries_.size()) >= num_ways_)
  {
    result.has_eviction = true;
    result.evicted_tag = entries_.back().tag;
    result.evicted_dirty = entries_.back().dirty;
    entries_.pop_back();
  }

  const bool dirty = options.is_store && options.mark_dirty_on_store;
  entries_.push_front({tag, dirty});
  return result;
}
