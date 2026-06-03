#include <gtest/gtest.h>

#include <sstream>

#include "analysis/Diagnostics.hpp"
#include "analysis/SimulationStats.hpp"
#include "cli/RunLogger.hpp"

using namespace apex;

namespace
{
RunLogContext make_context()
{
  RunLogContext ctx;
  ctx.input_path = "examples/test_stencil_g_ape.json";
  ctx.cache_path = "settings/cache.yaml";
  ctx.output_dir = "results";
  ctx.report_paths = {
    "/workspace/APEX-Cache/results/test_stencil_ape.json",
    "/workspace/APEX-Cache/results/test_stencil_ape.csv",
    "/workspace/APEX-Cache/results/test_stencil_ape_objects.csv",
    "/workspace/APEX-Cache/results/test_stencil_ape_diagnostics.md",
  };
  ctx.roots = {"stencil_1d_kernel"};
  ctx.object_count = 2;

  ctx.miss_stats.cold = 14;
  ctx.miss_stats.capacity = 0;
  ctx.miss_stats.conflict = 0;
  ctx.miss_stats.load = 1;
  ctx.miss_stats.store = 13;

  ctx.cache_stats.record_access("function:stencil_1d::param:out", "store", 2,
                                136);
  for (int i = 0; i < 12; ++i)
    ctx.cache_stats.record_access("function:stencil_1d::param:out", "store", 0,
                                  4);
  ctx.cache_stats.record_access("function:stencil_1d::param:in", "load", 2,
                                136);

  CacheConfig l1;
  l1.name = "L1D0";
  l1.role = "L1";
  l1.size_bytes = 32768;
  l1.line_size = 32;
  l1.associativity = 8;
  ctx.cache_config.caches.push_back(l1);

  return ctx;
}
}  // namespace

TEST(RunLogger, default_summary_contains_run_context)
{
  const std::string out = RunLogger::format(make_context(), RunLogMode::Default);

  EXPECT_NE(out.find("APEX-Cache run"), std::string::npos);
  EXPECT_NE(out.find("examples/test_stencil_g_ape.json"), std::string::npos);
  EXPECT_NE(out.find("settings/cache.yaml"), std::string::npos);
  EXPECT_NE(out.find("results"), std::string::npos);
}

TEST(RunLogger, default_summary_contains_loaded_ap_counts)
{
  const std::string out = RunLogger::format(make_context(), RunLogMode::Default);

  EXPECT_NE(out.find("Loaded AP"), std::string::npos);
  EXPECT_NE(out.find("stencil_1d_kernel"), std::string::npos);
  EXPECT_NE(out.find("objects:   2"), std::string::npos);
  EXPECT_NE(out.find("accesses:  14"), std::string::npos);
}

TEST(RunLogger, default_summary_contains_cache_stats)
{
  const std::string out = RunLogger::format(make_context(), RunLogMode::Default);

  EXPECT_NE(out.find("Cache summary"), std::string::npos);
  EXPECT_NE(out.find("L1 misses: 2 / 14"), std::string::npos);
  EXPECT_NE(out.find("L1 hits:   12"), std::string::npos);
  EXPECT_NE(out.find("memory:    2"), std::string::npos);
  EXPECT_NE(out.find("cycles:"), std::string::npos);
}

TEST(RunLogger, default_summary_contains_miss_breakdown)
{
  const std::string out = RunLogger::format(make_context(), RunLogMode::Default);

  EXPECT_NE(out.find("Miss breakdown"), std::string::npos);
  EXPECT_NE(out.find("cold:     14"), std::string::npos);
  EXPECT_NE(out.find("capacity: 0"), std::string::npos);
  EXPECT_NE(out.find("conflict: 0"), std::string::npos);
  EXPECT_NE(out.find("load:     1"), std::string::npos);
  EXPECT_NE(out.find("store:    13"), std::string::npos);
}

TEST(RunLogger, default_summary_lists_top_objects_by_misses)
{
  const std::string out = RunLogger::format(make_context(), RunLogMode::Default);
  const auto out_pos = out.find("function:stencil_1d::param:out");
  const auto in_pos = out.find("function:stencil_1d::param:in");

  ASSERT_NE(out_pos, std::string::npos);
  ASSERT_NE(in_pos, std::string::npos);
  EXPECT_LT(out_pos, in_pos);
  EXPECT_NE(out.find("1 / 13"), std::string::npos);
}

TEST(RunLogger, default_summary_lists_report_paths)
{
  const std::string out = RunLogger::format(make_context(), RunLogMode::Default);

  EXPECT_NE(out.find("Wrote reports"), std::string::npos);
  EXPECT_NE(out.find("/workspace/APEX-Cache/results/test_stencil_ape.json"),
            std::string::npos);
  EXPECT_NE(out.find("/workspace/APEX-Cache/results/test_stencil_ape.csv"),
            std::string::npos);
  EXPECT_NE(
    out.find("/workspace/APEX-Cache/results/test_stencil_ape_objects.csv"),
    std::string::npos);
  EXPECT_NE(out.find(
              "/workspace/APEX-Cache/results/test_stencil_ape_diagnostics.md"),
            std::string::npos);
}

TEST(RunLogger, quiet_summary_only_reports_output_directory)
{
  const std::string out = RunLogger::format(make_context(), RunLogMode::Quiet);

  EXPECT_NE(out.find("wrote results to results/"), std::string::npos);
  EXPECT_EQ(out.find("Cache summary"), std::string::npos);
}

TEST(RunLogger, verbose_summary_includes_cache_config)
{
  const std::string out = RunLogger::format(make_context(), RunLogMode::Verbose);

  EXPECT_NE(out.find("Cache config"), std::string::npos);
  EXPECT_NE(out.find("L1D0"), std::string::npos);
  EXPECT_NE(out.find("32768"), std::string::npos);
  EXPECT_NE(out.find("line 32"), std::string::npos);
}

TEST(RunLogger, color_enabled_wraps_section_headings)
{
  const std::string out =
    RunLogger::format(make_context(), RunLogMode::Default, true);

  EXPECT_NE(out.find("\033[1;36mAPEX-Cache run\033[0m"),
            std::string::npos);
  EXPECT_NE(out.find("\033[1;36mCache summary\033[0m"), std::string::npos);
}

TEST(RunLogger, color_disabled_keeps_plain_text)
{
  const std::string out =
    RunLogger::format(make_context(), RunLogMode::Default, false);

  EXPECT_NE(out.find("APEX-Cache run"), std::string::npos);
  EXPECT_EQ(out.find("\033["), std::string::npos);
}
