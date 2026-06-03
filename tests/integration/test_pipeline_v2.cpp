#include <gtest/gtest.h>

#include <string>

#include "ap/ApLoader.hpp"
#include "cache/CacheConfig.hpp"
#include "pipeline/Pipeline.hpp"

// LAT v2 end-to-end: ApLoader.load_program_string → Pipeline.run(ApProgram).
// 손계산 가능한 커널로 동작을 고정한다 (TLD).

using namespace apex;

namespace
{
HierarchyConfig make_config(uint64_t l1_bytes, int line_size, int assoc)
{
  HierarchyConfig cfg;
  cfg.num_cores = 1;

  CacheConfig l1;
  l1.name = "L1D0";
  l1.role = "L1";
  l1.private_to = 0;
  l1.size_bytes = l1_bytes;
  l1.line_size = line_size;
  l1.associativity = assoc;
  l1.write_policy = WritePolicy::WriteBack;
  l1.write_allocate = true;
  l1.next = "L2";

  CacheConfig l2;
  l2.name = "L2";
  l2.role = "LLC";
  l2.private_to = -1;
  l2.size_bytes = 1u << 20;
  l2.line_size = line_size;
  l2.associativity = 8;
  l2.write_policy = WritePolicy::WriteBack;
  l2.next = "Memory";

  cfg.caches = {l1, l2};
  cfg.memory.delay_cycles = 30;
  return cfg;
}

PipelineResult run(const char* json, HierarchyConfig cfg)
{
  ApProgram p = ApLoader{}.load_program_string(json);
  return Pipeline(std::move(cfg)).run(p);
}

uint64_t total_misses(const MissStats& s)
{
  return s.cold + s.capacity + s.conflict;
}

// 1D 순차: A[64], line 32(=8 ints/line) → 8 라인 → 8 cold miss.
const char* kSeq1d = R"({
  "schema_version":2,
  "metadata":{"objects":{"global::A":{"kind":"array","shape":[64],"elem_type":"i32","elem_size":4}},"structs":{}},
  "functions":[{"function":"f","params":[],"annotations":["yard.analyze"],"body":[
    {"type":"Loop","var":"i","start":0,"bound":64,"depth":1,"body":[
      {"type":"Array","object":"global::A","access_path":[{"kind":"index","value":"i"}],"op":"load"}
    ]}
  ]}]
})";

// 2D local 커널: A[i][j] store + B[i][j] load, 64x64, line 64(=16 ints/line).
// 행당 64원소=4라인 → 4 miss × 64행 = 256/배열.
const char* kLocal2d = R"({
  "schema_version":2,
  "metadata":{"objects":{
    "fn::A":{"kind":"array","shape":[64,64],"elem_type":"i32","elem_size":4},
    "fn::B":{"kind":"array","shape":[64,64],"elem_type":"i32","elem_size":4}},"structs":{}},
  "functions":[{"function":"f","params":[],"annotations":["yard.analyze"],"body":[
    {"type":"Loop","var":"i","start":0,"bound":64,"depth":1,"body":[
      {"type":"Loop","var":"j","start":0,"bound":64,"depth":2,"body":[
        {"type":"Array","object":"fn::B","access_path":[{"kind":"index","value":"i"},{"kind":"index","value":"j"}],"op":"load"},
        {"type":"Array","object":"fn::A","access_path":[{"kind":"index","value":"i"},{"kind":"index","value":"j"}],"op":"store"}
      ]}
    ]}
  ]}]
})";
}  // namespace

TEST(PipelineV2, sequential_1d_one_miss_per_line)
{
  auto r = run(kSeq1d, make_config(32768, 32, 8));
  EXPECT_EQ(r.stats.cold, 8u);
  EXPECT_EQ(total_misses(r.stats), 8u);
}

TEST(PipelineV2, local_2d_kernel_cold_miss_count)
{
  auto r = run(kLocal2d, make_config(1u << 20, 64, 8));
  EXPECT_EQ(total_misses(r.stats), 512u);  // 256 + 256
}

TEST(PipelineV2, local_2d_kernel_load_store_split)
{
  auto r = run(kLocal2d, make_config(1u << 20, 64, 8));
  EXPECT_EQ(r.stats.load, 256u);   // B
  EXPECT_EQ(r.stats.store, 256u);  // A
}

TEST(PipelineV2, per_object_miss_attribution)
{
  auto r = run(kLocal2d, make_config(1u << 20, 64, 8));
  EXPECT_EQ(r.stats.by_object["fn::A"], 256u);
  EXPECT_EQ(r.stats.by_object["fn::B"], 256u);
}
