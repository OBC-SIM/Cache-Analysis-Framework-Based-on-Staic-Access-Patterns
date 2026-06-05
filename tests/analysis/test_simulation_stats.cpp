#include <gtest/gtest.h>

#include "analysis/SimulationStats.hpp"

using namespace apex;

TEST(SimulationStats, default_stats_are_zero)
{
  SimulationStats stats;

  EXPECT_EQ(stats.total_accesses, 0u);
  EXPECT_EQ(stats.load_accesses, 0u);
  EXPECT_EQ(stats.store_accesses, 0u);
  EXPECT_EQ(stats.l1_hits, 0u);
  EXPECT_EQ(stats.l1_misses, 0u);
  EXPECT_EQ(stats.l2_hits, 0u);
  EXPECT_EQ(stats.l2_misses, 0u);
  EXPECT_EQ(stats.memory_accesses, 0u);
  EXPECT_EQ(stats.total_cycles, 0u);
  EXPECT_EQ(stats.write_through_writes, 0u);
  EXPECT_EQ(stats.writebacks, 0u);
  EXPECT_EQ(stats.dirty_evictions, 0u);
  EXPECT_EQ(stats.writeback_cycles, 0u);
  EXPECT_DOUBLE_EQ(stats.average_cycles_per_access(), 0.0);
}

TEST(SimulationStats, records_l1_hit_access)
{
  SimulationStats stats;
  stats.record_access("A", "load", 0, 4);

  EXPECT_EQ(stats.total_accesses, 1u);
  EXPECT_EQ(stats.load_accesses, 1u);
  EXPECT_EQ(stats.store_accesses, 0u);
  EXPECT_EQ(stats.l1_hits, 1u);
  EXPECT_EQ(stats.l1_misses, 0u);
  EXPECT_EQ(stats.total_cycles, 4u);
}

TEST(SimulationStats, records_l2_hit_as_l1_miss)
{
  SimulationStats stats;
  stats.record_access("A", "store", 1, 16);

  EXPECT_EQ(stats.total_accesses, 1u);
  EXPECT_EQ(stats.store_accesses, 1u);
  EXPECT_EQ(stats.l1_misses, 1u);
  EXPECT_EQ(stats.l2_hits, 1u);
  EXPECT_EQ(stats.l2_misses, 0u);
  EXPECT_EQ(stats.memory_accesses, 0u);
}

TEST(SimulationStats, records_memory_access_as_l2_miss)
{
  SimulationStats stats;
  stats.record_access("A", "load", 2, 136);

  EXPECT_EQ(stats.l1_misses, 1u);
  EXPECT_EQ(stats.l2_hits, 0u);
  EXPECT_EQ(stats.l2_misses, 1u);
  EXPECT_EQ(stats.memory_accesses, 1u);
}

TEST(SimulationStats, accumulates_total_cycles)
{
  SimulationStats stats;
  stats.record_access("A", "load", 0, 4);
  stats.record_access("A", "load", 1, 16);
  stats.record_access("A", "load", 2, 136);

  EXPECT_EQ(stats.total_cycles, 156u);
  EXPECT_DOUBLE_EQ(stats.average_cycles_per_access(), 52.0);
}

TEST(SimulationStats, records_object_access_and_miss_counts)
{
  SimulationStats stats;
  stats.record_access("A", "load", 0, 4);
  stats.record_access("A", "load", 0, 4);
  stats.record_access("A", "load", 2, 136);

  const auto & object = stats.objects.at("A");
  EXPECT_EQ(object.accesses, 3u);
  EXPECT_EQ(object.hits, 2u);
  EXPECT_EQ(object.misses, 1u);
  EXPECT_DOUBLE_EQ(object.miss_rate(), 1.0 / 3.0);
}

TEST(SimulationStats, separates_object_load_and_store_counts)
{
  SimulationStats stats;
  stats.record_access("A", "load", 0, 4);
  stats.record_access("A", "store", 1, 16);

  const auto & object = stats.objects.at("A");
  EXPECT_EQ(object.load_accesses, 1u);
  EXPECT_EQ(object.store_accesses, 1u);
  EXPECT_EQ(object.load_misses, 0u);
  EXPECT_EQ(object.store_misses, 1u);
}

TEST(SimulationStats, records_write_traffic)
{
  SimulationStats stats;

  stats.record_access("A", "store", 0, 16, 1, 2, 3, 24);

  EXPECT_EQ(stats.write_through_writes, 1u);
  EXPECT_EQ(stats.writebacks, 2u);
  EXPECT_EQ(stats.dirty_evictions, 3u);
  EXPECT_EQ(stats.writeback_cycles, 24u);
}
