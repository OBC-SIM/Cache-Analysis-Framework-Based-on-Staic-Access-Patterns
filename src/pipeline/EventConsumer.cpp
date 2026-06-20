#include "pipeline/EventConsumer.hpp"

#include <utility>

#include "memory/AddressMapper.hpp"

namespace apex
{

EventConsumer::EventConsumer(const HierarchyConfig & config,
                             const MemoryLayout & layout, int line_size)
  : config_(config), layout_(layout), line_size_(line_size), hierarchy_(config)
{
}

MissClassifier & EventConsumer::classifier_for(int core)
{
  auto found = classifiers_.find(core);
  if (found != classifiers_.end()) return found->second;

  int lines = 0;
  for (const auto & cache : config_.caches)
    if (cache.role == "L1" && cache.private_to == core)
    {
      lines = static_cast<int>(cache.size_bytes /
                               static_cast<uint64_t>(cache.line_size));
      break;
    }
  return classifiers_.emplace(core, MissClassifier{lines}).first->second;
}

void EventConsumer::consume(AccessEvent && event)
{
  const uint64_t base = layout_.base_of(event.object_name);
  const uint64_t byte_address =
    base + static_cast<uint64_t>(event.byte_offset);
  const uint64_t cache_line = AddressMapper::cache_line(
    byte_address, static_cast<uint64_t>(line_size_));
  const bool is_store = event.op == "store";
  const HierarchyAccessResult access =
    hierarchy_.access(event.core_id, cache_line, is_store);
  const bool l1_miss = access.miss_level >= 1;

  result_.cache_stats.record_access(
    event.object_name, event.op, access.miss_level, access.delay_cycles,
    access.write_through_writes, access.writebacks, access.dirty_evictions,
    access.writeback_cycles);

  bool write_allocate = true;
  for (const auto & cache : config_.caches)
    if (cache.role == "L1" && cache.private_to == event.core_id)
    {
      write_allocate = cache.write_allocate;
      break;
    }
  const bool fill_l1 = !(l1_miss && is_store && !write_allocate);
  const auto miss_type =
    classifier_for(event.core_id).classify(cache_line, l1_miss, fill_l1);
  if (!l1_miss) return;

  result_.attribution.record(event.region_path, event.object_name, event.op,
                             access.miss_level);
  if (miss_type)
  {
    switch (*miss_type)
    {
      case MissType::Cold: result_.stats.cold += 1; break;
      case MissType::Capacity: result_.stats.capacity += 1; break;
      case MissType::Conflict: result_.stats.conflict += 1; break;
      case MissType::Policy: result_.stats.policy += 1; break;
    }
  }
  if (is_store)
    result_.stats.store += 1;
  else
    result_.stats.load += 1;
  result_.stats.by_object[event.object_name] += 1;
}

PipelineResult EventConsumer::take_result()
{
  return std::move(result_);
}

}  // namespace apex
