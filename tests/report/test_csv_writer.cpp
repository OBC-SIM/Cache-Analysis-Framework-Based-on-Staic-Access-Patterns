#include <gtest/gtest.h>
#include <sstream>

#include "analysis/Attribution.hpp"
#include "analysis/SimulationStats.hpp"
#include "report/CsvWriter.hpp"

// CsvWriter::write_summary(os, stats)  → 헤더 + 데이터 1행
// CsvWriter::write_object_breakdown(os, attribution) → object별 miss 행
// 출력은 std::ostream에 쓴다 (파일 I/O 없이 stringstream으로 검증).

TEST(CsvWriter, summary_has_header_row)
{
  apex::MissStats stats;
  stats.cold = 5;
  stats.capacity = 3;
  stats.conflict = 2;
  stats.load = 8;
  stats.store = 2;

  std::ostringstream oss;
  apex::CsvWriter::write_summary(oss, stats);
  std::string out = oss.str();

  EXPECT_NE(out.find("cold"), std::string::npos);
  EXPECT_NE(out.find("capacity"), std::string::npos);
  EXPECT_NE(out.find("conflict"), std::string::npos);
}

TEST(CsvWriter, summary_data_row_matches_stats)
{
  apex::MissStats stats;
  stats.cold = 5;
  stats.capacity = 3;
  stats.conflict = 2;
  stats.load = 8;
  stats.store = 2;

  std::ostringstream oss;
  apex::CsvWriter::write_summary(oss, stats);
  std::string out = oss.str();

  EXPECT_NE(out.find("5"), std::string::npos);
  EXPECT_NE(out.find("3"), std::string::npos);
  EXPECT_NE(out.find("2"), std::string::npos);
}

TEST(CsvWriter, object_breakdown_lists_all_objects)
{
  apex::Attribution attr;
  attr.record("loop_i", "A", "load", 1);
  attr.record("loop_i", "A", "load", 1);
  attr.record("loop_j", "B", "store", 1);

  std::ostringstream oss;
  apex::CsvWriter::write_object_breakdown(oss, attr);
  std::string out = oss.str();

  EXPECT_NE(out.find("A"), std::string::npos);
  EXPECT_NE(out.find("B"), std::string::npos);
  EXPECT_NE(out.find("2"), std::string::npos);  // A miss count
  EXPECT_NE(out.find("1"), std::string::npos);  // B miss count
}

TEST(CsvWriter, extended_summary_contains_access_level_and_cycle_columns)
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
  apex::CsvWriter::write_summary(oss, miss, cache);
  std::string out = oss.str();

  EXPECT_NE(out.find("total_accesses"), std::string::npos);
  EXPECT_NE(out.find("l1_hits"), std::string::npos);
  EXPECT_NE(out.find("l1_misses"), std::string::npos);
  EXPECT_NE(out.find("l2_hits"), std::string::npos);
  EXPECT_NE(out.find("memory_accesses"), std::string::npos);
  EXPECT_NE(out.find("total_cycles"), std::string::npos);
  EXPECT_NE(out.find("average_cycles_per_access"), std::string::npos);
}

TEST(CsvWriter, object_breakdown_contains_accesses_hits_misses_and_rate)
{
  apex::SimulationStats cache;
  cache.record_access("A", "load", 0, 4);
  cache.record_access("A", "load", 0, 4);
  cache.record_access("A", "store", 2, 136);

  std::ostringstream oss;
  apex::CsvWriter::write_object_breakdown(oss, cache);
  std::string out = oss.str();

  EXPECT_NE(out.find("object,accesses,hits,misses,miss_rate"), std::string::npos);
  EXPECT_NE(out.find("A,3,2,1"), std::string::npos);
}
