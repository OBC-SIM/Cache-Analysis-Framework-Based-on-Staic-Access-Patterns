#include "report/JsonWriter.hpp"

#include <nlohmann/json.hpp>

#include <ostream>

namespace apex
{

void JsonWriter::write_summary(std::ostream & os, const MissStats & stats)
{
  nlohmann::json j;
  j["cold"] = stats.cold;
  j["capacity"] = stats.capacity;
  j["conflict"] = stats.conflict;
  j["load"] = stats.load;
  j["store"] = stats.store;
  os << j.dump(2) << '\n';
}

void JsonWriter::write_summary(std::ostream & os, const MissStats & miss_stats,
                               const SimulationStats & cache_stats)
{
  nlohmann::json j;
  j["cold"] = miss_stats.cold;
  j["capacity"] = miss_stats.capacity;
  j["conflict"] = miss_stats.conflict;
  j["load"] = miss_stats.load;
  j["store"] = miss_stats.store;

  j["accesses"] = {
    {"total", cache_stats.total_accesses},
    {"load", cache_stats.load_accesses},
    {"store", cache_stats.store_accesses},
  };
  j["levels"]["L1"] = {
    {"hits", cache_stats.l1_hits},
    {"misses", cache_stats.l1_misses},
    {"hit_rate", cache_stats.l1_hit_rate()},
  };
  j["levels"]["L2"] = {
    {"hits", cache_stats.l2_hits},
    {"misses", cache_stats.l2_misses},
    {"hit_rate", cache_stats.l2_hit_rate()},
  };
  j["levels"]["Memory"] = {{"accesses", cache_stats.memory_accesses}};
  j["cycles"] = {
    {"total", cache_stats.total_cycles},
    {"average_per_access", cache_stats.average_cycles_per_access()},
  };
  j["write_traffic"] = {
    {"write_through_writes", cache_stats.write_through_writes},
    {"writebacks", cache_stats.writebacks},
    {"dirty_evictions", cache_stats.dirty_evictions},
    {"writeback_cycles", cache_stats.writeback_cycles},
  };

  os << j.dump(2) << '\n';
}

}  // namespace apex
