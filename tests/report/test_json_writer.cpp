#include <nlohmann/json.hpp>

#include <gtest/gtest.h>
#include <sstream>

#include "report/JsonWriter.hpp"
#include "analysis/SimulationStats.hpp"

// JsonWriter::write_summary(os, stats) → 유효한 JSON, 필드 존재 확인

TEST(JsonWriter, output_is_valid_json)
{
  apex::MissStats stats;
  stats.cold = 1;
  stats.capacity = 2;
  stats.conflict = 3;
  stats.load = 4;
  stats.store = 2;

  std::ostringstream oss;
  apex::JsonWriter::write_summary(oss, stats);

  EXPECT_NO_THROW(nlohmann::json::parse(oss.str()));
}

TEST(JsonWriter, summary_contains_required_fields)
{
  apex::MissStats stats;
  stats.cold = 1;
  stats.capacity = 2;
  stats.conflict = 3;
  stats.load = 4;
  stats.store = 2;

  std::ostringstream oss;
  apex::JsonWriter::write_summary(oss, stats);
  auto j = nlohmann::json::parse(oss.str());

  EXPECT_TRUE(j.contains("cold"));
  EXPECT_TRUE(j.contains("capacity"));
  EXPECT_TRUE(j.contains("conflict"));
  EXPECT_TRUE(j.contains("load"));
  EXPECT_TRUE(j.contains("store"));
}

TEST(JsonWriter, summary_values_match_stats)
{
  apex::MissStats stats;
  stats.cold = 5;
  stats.capacity = 3;
  stats.conflict = 2;
  stats.load = 8;
  stats.store = 2;

  std::ostringstream oss;
  apex::JsonWriter::write_summary(oss, stats);
  auto j = nlohmann::json::parse(oss.str());

  EXPECT_EQ(j["cold"].get<uint64_t>(), 5u);
  EXPECT_EQ(j["capacity"].get<uint64_t>(), 3u);
  EXPECT_EQ(j["conflict"].get<uint64_t>(), 2u);
}

TEST(JsonWriter, extended_summary_contains_access_level_and_cycle_stats)
{
  apex::MissStats miss;
  miss.cold = 1;
  miss.capacity = 2;
  miss.conflict = 0;
  miss.load = 2;
  miss.store = 1;

  apex::SimulationStats cache;
  cache.record_access("A", "load", 0, 4);
  cache.record_access("A", "load", 1, 16);
  cache.record_access("B", "store", 2, 136);

  std::ostringstream oss;
  apex::JsonWriter::write_summary(oss, miss, cache);
  auto j = nlohmann::json::parse(oss.str());

  EXPECT_EQ(j["accesses"]["total"].get<uint64_t>(), 3u);
  EXPECT_EQ(j["accesses"]["load"].get<uint64_t>(), 2u);
  EXPECT_EQ(j["accesses"]["store"].get<uint64_t>(), 1u);
  EXPECT_EQ(j["levels"]["L1"]["hits"].get<uint64_t>(), 1u);
  EXPECT_EQ(j["levels"]["L1"]["misses"].get<uint64_t>(), 2u);
  EXPECT_EQ(j["levels"]["L2"]["hits"].get<uint64_t>(), 1u);
  EXPECT_EQ(j["levels"]["L2"]["misses"].get<uint64_t>(), 1u);
  EXPECT_EQ(j["levels"]["Memory"]["accesses"].get<uint64_t>(), 1u);
  EXPECT_EQ(j["cycles"]["total"].get<uint64_t>(), 156u);
  EXPECT_DOUBLE_EQ(j["cycles"]["average_per_access"].get<double>(), 52.0);
}

TEST(JsonWriter, extended_summary_preserves_legacy_miss_fields)
{
  apex::MissStats miss;
  miss.cold = 1;
  miss.capacity = 2;
  miss.conflict = 3;
  miss.load = 4;
  miss.store = 2;

  apex::SimulationStats cache;

  std::ostringstream oss;
  apex::JsonWriter::write_summary(oss, miss, cache);
  auto j = nlohmann::json::parse(oss.str());

  EXPECT_EQ(j["cold"].get<uint64_t>(), 1u);
  EXPECT_EQ(j["capacity"].get<uint64_t>(), 2u);
  EXPECT_EQ(j["conflict"].get<uint64_t>(), 3u);
  EXPECT_EQ(j["load"].get<uint64_t>(), 4u);
  EXPECT_EQ(j["store"].get<uint64_t>(), 2u);
}

TEST(JsonWriter, extended_summary_contains_write_traffic_stats)
{
  apex::MissStats miss;
  apex::SimulationStats cache;
  cache.record_access("A", "store", 0, 16, 3, 2, 1, 24);

  std::ostringstream oss;
  apex::JsonWriter::write_summary(oss, miss, cache);
  auto j = nlohmann::json::parse(oss.str());

  EXPECT_EQ(j["write_traffic"]["write_through_writes"].get<uint64_t>(), 3u);
  EXPECT_EQ(j["write_traffic"]["writebacks"].get<uint64_t>(), 2u);
  EXPECT_EQ(j["write_traffic"]["dirty_evictions"].get<uint64_t>(), 1u);
  EXPECT_EQ(j["write_traffic"]["writeback_cycles"].get<uint64_t>(), 24u);
}
