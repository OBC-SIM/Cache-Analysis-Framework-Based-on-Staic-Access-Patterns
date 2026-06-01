#include <gtest/gtest.h>
#include <sstream>
#include <nlohmann/json.hpp>
#include "report/JsonWriter.hpp"

// JsonWriter::write_summary(os, stats) → 유효한 JSON, 필드 존재 확인

TEST(JsonWriter, output_is_valid_json) {
    apex::MissStats stats;
    stats.cold = 1; stats.capacity = 2; stats.conflict = 3;
    stats.load = 4; stats.store = 2;

    std::ostringstream oss;
    apex::JsonWriter::write_summary(oss, stats);

    EXPECT_NO_THROW(nlohmann::json::parse(oss.str()));
}

TEST(JsonWriter, summary_contains_required_fields) {
    apex::MissStats stats;
    stats.cold = 1; stats.capacity = 2; stats.conflict = 3;
    stats.load = 4; stats.store = 2;

    std::ostringstream oss;
    apex::JsonWriter::write_summary(oss, stats);
    auto j = nlohmann::json::parse(oss.str());

    EXPECT_TRUE(j.contains("cold"));
    EXPECT_TRUE(j.contains("capacity"));
    EXPECT_TRUE(j.contains("conflict"));
    EXPECT_TRUE(j.contains("load"));
    EXPECT_TRUE(j.contains("store"));
}

TEST(JsonWriter, summary_values_match_stats) {
    apex::MissStats stats;
    stats.cold = 5; stats.capacity = 3; stats.conflict = 2;
    stats.load = 8; stats.store = 2;

    std::ostringstream oss;
    apex::JsonWriter::write_summary(oss, stats);
    auto j = nlohmann::json::parse(oss.str());

    EXPECT_EQ(j["cold"].get<uint64_t>(),     5u);
    EXPECT_EQ(j["capacity"].get<uint64_t>(), 3u);
    EXPECT_EQ(j["conflict"].get<uint64_t>(), 2u);
}
