#include "cache/CacheLevel.hpp"

CacheLevel::CacheLevel(int num_sets, int num_ways) : num_sets_(num_sets)
{
  sets_.reserve(num_sets);
  for (int i = 0; i < num_sets; ++i) sets_.emplace_back(num_ways);
}

LevelAccessResult CacheLevel::access(uint64_t cache_line, bool is_store)
{
  CacheAccessOptions options;
  options.is_store = is_store;
  return access(cache_line, options);
}

LevelAccessResult CacheLevel::access(uint64_t cache_line,
                                     const CacheAccessOptions & options)
{
  uint64_t si = set_index_of(cache_line, num_sets_);
  uint64_t tag = tag_of(cache_line, num_sets_);
  auto sr = sets_[si].access(tag, options);
  uint64_t evicted_cache_line = 0;
  if (sr.has_eviction)
    evicted_cache_line = sr.evicted_tag * static_cast<uint64_t>(num_sets_) + si;
  return {sr.hit, sr, evicted_cache_line};
}

uint64_t CacheLevel::set_index_of(uint64_t cache_line, int num_sets)
{
  return cache_line % static_cast<uint64_t>(num_sets);
}

uint64_t CacheLevel::tag_of(uint64_t cache_line, int num_sets)
{
  return cache_line / static_cast<uint64_t>(num_sets);
}
