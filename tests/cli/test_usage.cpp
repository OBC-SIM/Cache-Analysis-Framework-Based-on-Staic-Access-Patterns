#include <gtest/gtest.h>

#include <string>

#include "cli/Usage.hpp"

namespace
{

TEST(UsageTest, ShowsRunCommandSyntax)
{
  const std::string help = apex::Usage::text();

  EXPECT_NE(help.find(
              "apex-cache run <input.ap.json> --cache <cache.yaml> [options]"),
            std::string::npos);
  EXPECT_NE(help.find("apex-cache help"), std::string::npos);
  EXPECT_NE(help.find("apex-cache --help"), std::string::npos);
}

TEST(UsageTest, ListsRunOptions)
{
  const std::string help = apex::Usage::text();

  EXPECT_NE(help.find("--cache <cache.yaml>"), std::string::npos);
  EXPECT_NE(help.find("--output <dir>"), std::string::npos);
  EXPECT_NE(help.find("(default: results)"), std::string::npos);
  EXPECT_NE(help.find("--quiet"), std::string::npos);
  EXPECT_NE(help.find("--verbose"), std::string::npos);
  EXPECT_NE(help.find("--no-color"), std::string::npos);
  EXPECT_NE(help.find("-h, --help"), std::string::npos);
}

TEST(UsageTest, ListsGeneratedReports)
{
  const std::string help = apex::Usage::text();

  EXPECT_NE(help.find("<input-base>.json"), std::string::npos);
  EXPECT_NE(help.find("<input-base>.csv"), std::string::npos);
  EXPECT_NE(help.find("<input-base>_objects.csv"), std::string::npos);
  EXPECT_NE(help.find("<input-base>_diagnostics.md"), std::string::npos);
}

TEST(UsageTest, ShowsExamples)
{
  const std::string help = apex::Usage::text();

  EXPECT_NE(help.find("examples/test_stencil_g_ape.json"), std::string::npos);
  EXPECT_NE(help.find("--output results --verbose"), std::string::npos);
}

}  // namespace
