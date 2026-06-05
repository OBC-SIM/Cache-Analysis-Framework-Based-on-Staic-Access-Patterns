#include "pipeline/Pipeline.hpp"

#include <map>

#include "analysis/MissClassifier.hpp"
#include "ap/AccessEvent.hpp"
#include "ap/EventBuilder.hpp"
#include "cache/CacheHierarchy.hpp"
#include "memory/AddressMapper.hpp"
#include "memory/MemoryLayout.hpp"

namespace apex
{

Pipeline::Pipeline(HierarchyConfig config) : config_(std::move(config)) {}

PipelineResult Pipeline::run(const ApProgram & program)
{
  int line_size = 32;
  for (const auto & c : config_.caches)
    if (c.role == "L1")
    {
      line_size = c.line_size;
      break;
    }

  // 객체 배치: metadata.objects 크기로 line_size 정렬 (id 순, 결정적).
  MemoryLayout layout(static_cast<uint64_t>(line_size));
  for (const auto & [id, obj] : program.objects)
    layout.add_object(id, obj.total_bytes());

  std::vector<AccessEvent> events = EventBuilder{}.build_program(program);

  for (auto & e : events)
  {
    uint64_t base = layout.base_of(e.object_name);
    uint64_t byte_addr = base + static_cast<uint64_t>(e.byte_offset);
    e.cache_line =
      AddressMapper::cache_line(byte_addr, static_cast<uint64_t>(line_size));
  }

  return simulate(events);
}

PipelineResult Pipeline::simulate(const std::vector<AccessEvent> & events)
{
  // core별 FA 쉐도우 캐시 (cold/capacity/conflict 분류용)
  std::map<int, MissClassifier> classifiers;
  auto classifier_for = [&](int core) -> MissClassifier & {
    auto it = classifiers.find(core);
    if (it != classifiers.end()) return it->second;
    int lines = 0;
    for (const auto & c : config_.caches)
    {
      if (c.role == "L1" && c.private_to == core)
      {
        lines =
          static_cast<int>(c.size_bytes / static_cast<uint64_t>(c.line_size));
        break;
      }
    }
    return classifiers.emplace(core, MissClassifier{lines}).first->second;
  };

  CacheHierarchy hierarchy(config_);
  PipelineResult result;

  for (const auto & e : events)
  {
    bool is_store = (e.op == "store");
    HierarchyAccessResult res =
      hierarchy.access(e.core_id, e.cache_line, is_store);
    bool l1_miss = (res.miss_level >= 1);

    result.cache_stats.record_access(
      e.object_name, e.op, res.miss_level, res.delay_cycles,
      res.write_through_writes, res.writebacks, res.dirty_evictions,
      res.writeback_cycles);

    auto miss_type = classifier_for(e.core_id).classify(e.cache_line, l1_miss);

    if (!l1_miss) continue;

    result.attribution.record(e.region_path, e.object_name, e.op,
                              res.miss_level);

    if (miss_type)
    {
      switch (*miss_type)
      {
        case MissType::Cold:
          result.stats.cold += 1;
          break;
        case MissType::Capacity:
          result.stats.capacity += 1;
          break;
        case MissType::Conflict:
          result.stats.conflict += 1;
          break;
      }
    }
    if (is_store)
      result.stats.store += 1;
    else
      result.stats.load += 1;
    result.stats.by_object[e.object_name] += 1;
  }

  return result;
}

}  // namespace apex
