#include <gtest/gtest.h>

#include "cache/CacheLevel.hpp"

TEST(CacheLevel, cold_miss_on_first_access)
{
  CacheLevel level(8, 4);
  auto r = level.access(17, false);
  EXPECT_FALSE(r.hit);
}

TEST(CacheLevel, hit_after_fill)
{
  CacheLevel level(8, 4);
  level.access(17, false);  // miss → fill
  auto r = level.access(17, false);
  EXPECT_TRUE(r.hit);
}

TEST(CacheLevel, store_miss_without_write_allocate_does_not_fill)
{
  CacheLevel level(8, 4);
  CacheAccessOptions opts;
  opts.is_store = true;
  opts.allocate_on_miss = false;

  auto first = level.access(17, opts);
  auto second = level.access(17, false);

  EXPECT_FALSE(first.hit);
  EXPECT_FALSE(second.hit);
}

TEST(CacheLevel, store_hit_write_through_does_not_mark_dirty)
{
  CacheLevel level(1, 1);
  level.access(17, false);

  CacheAccessOptions opts;
  opts.is_store = true;
  opts.mark_dirty_on_store = false;
  level.access(17, opts);
  auto evict = level.access(18, false);

  EXPECT_FALSE(evict.set_result.evicted_dirty);
}

TEST(CacheLevel, set_index_is_cache_line_mod_num_sets)
{
  EXPECT_EQ(CacheLevel::set_index_of(17, 8), 1u);  // 17 % 8 = 1
}

TEST(CacheLevel, tag_is_cache_line_div_num_sets)
{
  EXPECT_EQ(CacheLevel::tag_of(17, 8), 2u);  // 17 / 8 = 2
}
