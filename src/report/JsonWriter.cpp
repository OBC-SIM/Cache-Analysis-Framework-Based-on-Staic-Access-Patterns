#include "report/JsonWriter.hpp"
#include <nlohmann/json.hpp>
#include <ostream>

namespace apex {

void JsonWriter::write_summary(std::ostream& os, const MissStats& stats) {
    nlohmann::json j;
    j["cold"]     = stats.cold;
    j["capacity"] = stats.capacity;
    j["conflict"] = stats.conflict;
    j["load"]     = stats.load;
    j["store"]    = stats.store;
    os << j.dump(2) << '\n';
}

}  // namespace apex
