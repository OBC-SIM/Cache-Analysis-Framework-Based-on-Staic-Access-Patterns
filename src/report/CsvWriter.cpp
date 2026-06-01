#include "report/CsvWriter.hpp"
#include <ostream>

namespace apex {

void CsvWriter::write_summary(std::ostream& os, const MissStats& stats) {
    os << "cold,capacity,conflict,load,store\n";
    os << stats.cold    << ','
       << stats.capacity << ','
       << stats.conflict << ','
       << stats.load     << ','
       << stats.store    << '\n';
}

void CsvWriter::write_object_breakdown(std::ostream& os, const Attribution& attr) {
    os << "object,miss_count\n";
    for (auto& [name, cnt] : attr.objects()) {
        os << name << ',' << cnt << '\n';
    }
}

}  // namespace apex
