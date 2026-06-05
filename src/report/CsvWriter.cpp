#include "report/CsvWriter.hpp"

#include <ostream>

namespace apex
{

void CsvWriter::write_summary(std::ostream & os, const MissStats & stats)
{
  os << "cold,capacity,conflict,load,store\n";
  os << stats.cold << ',' << stats.capacity << ',' << stats.conflict << ','
     << stats.load << ',' << stats.store << '\n';
}

void CsvWriter::write_summary(std::ostream & os, const MissStats & miss_stats,
                              const SimulationStats & cache_stats)
{
  os << "cold,capacity,conflict,load,store,total_accesses,load_accesses,"
        "store_accesses,l1_hits,l1_misses,l1_hit_rate,l2_hits,l2_misses,"
        "l2_hit_rate,memory_accesses,total_cycles,average_cycles_per_access,"
        "write_through_writes,writebacks,dirty_evictions,writeback_cycles\n";
  os << miss_stats.cold << ',' << miss_stats.capacity << ','
     << miss_stats.conflict << ',' << miss_stats.load << ',' << miss_stats.store
     << ',' << cache_stats.total_accesses << ',' << cache_stats.load_accesses
     << ',' << cache_stats.store_accesses << ',' << cache_stats.l1_hits << ','
     << cache_stats.l1_misses << ',' << cache_stats.l1_hit_rate() << ','
     << cache_stats.l2_hits << ',' << cache_stats.l2_misses << ','
     << cache_stats.l2_hit_rate() << ',' << cache_stats.memory_accesses << ','
     << cache_stats.total_cycles << ','
     << cache_stats.average_cycles_per_access() << ','
     << cache_stats.write_through_writes << ',' << cache_stats.writebacks << ','
     << cache_stats.dirty_evictions << ',' << cache_stats.writeback_cycles
     << '\n';
}

void CsvWriter::write_object_breakdown(std::ostream & os,
                                       const Attribution & attr)
{
  os << "object,miss_count\n";
  for (auto & [name, cnt] : attr.objects())
  {
    os << name << ',' << cnt << '\n';
  }
}

void CsvWriter::write_object_breakdown(std::ostream & os,
                                       const SimulationStats & cache_stats)
{
  os << "object,accesses,hits,misses,miss_rate,load_accesses,store_accesses,"
        "load_misses,store_misses\n";
  for (const auto & [name, stats] : cache_stats.objects)
  {
    os << name << ',' << stats.accesses << ',' << stats.hits << ','
       << stats.misses << ',' << stats.miss_rate() << ','
       << stats.load_accesses << ',' << stats.store_accesses << ','
       << stats.load_misses << ',' << stats.store_misses << '\n';
  }
}

}  // namespace apex
