#include "cli/RunLogger.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace apex
{

namespace
{

double percent(uint64_t part, uint64_t total)
{
  if (total == 0) return 0.0;
  return 100.0 * static_cast<double>(part) / static_cast<double>(total);
}

std::string join_roots(const std::vector<std::string> & roots)
{
  if (roots.empty()) return "(none)";
  std::ostringstream os;
  for (std::size_t i = 0; i < roots.size(); ++i)
  {
    if (i > 0) os << ", ";
    os << roots[i];
  }
  return os.str();
}

std::string output_dir_with_slash(const std::string & output_dir)
{
  if (output_dir.empty()) return "./";
  if (output_dir.back() == '/') return output_dir;
  return output_dir + "/";
}

std::string heading(const std::string & text, bool color_enabled)
{
  if (!color_enabled) return text;
  return "\033[1;36m" + text + "\033[0m";
}

std::string color(const std::string & text, const std::string & code,
                  bool color_enabled)
{
  if (!color_enabled) return text;
  return "\033[" + code + "m" + text + "\033[0m";
}

std::string label(const std::string & text, bool color_enabled)
{
  return color(text, "2;37", color_enabled);
}

std::string number(uint64_t value, const std::string & code,
                   bool color_enabled)
{
  return color(std::to_string(value), code, color_enabled);
}

std::string rate(double value, const std::string & code, bool color_enabled)
{
  std::ostringstream os;
  os << std::fixed << std::setprecision(2) << value;
  return color(os.str(), code, color_enabled);
}

void write_cache_config(std::ostream & os, const HierarchyConfig & config,
                        bool color_enabled)
{
  os << '\n' << heading("Cache config", color_enabled) << '\n';
  for (const auto & cache : config.caches)
  {
    os << "  " << color(cache.name, "1;35", color_enabled) << " ("
       << color(cache.role, "36", color_enabled) << "): "
       << number(cache.size_bytes, "32", color_enabled) << " bytes, line "
       << number(cache.line_size, "32", color_enabled) << ", assoc "
       << number(cache.associativity, "32", color_enabled) << '\n';
  }
  os << "  " << color(config.memory.name, "1;35", color_enabled) << ": "
     << number(config.memory.delay_cycles, "32", color_enabled) << " cycles\n";
}

}  // namespace

std::string RunLogger::format(const RunLogContext & context, RunLogMode mode)
{
  return format(context, mode, false);
}

std::string RunLogger::format(const RunLogContext & context, RunLogMode mode,
                              bool color_enabled)
{
  if (mode == RunLogMode::Quiet)
    return "wrote results to " + output_dir_with_slash(context.output_dir) +
           "\n";

  std::ostringstream os;
  os << std::fixed << std::setprecision(2);

  os << heading("APEX-Cache run", color_enabled) << '\n';
  os << "  " << label("input:", color_enabled) << "  "
     << color(context.input_path, "34", color_enabled) << '\n';
  os << "  " << label("cache:", color_enabled) << "  "
     << color(context.cache_path, "34", color_enabled) << '\n';
  os << "  " << label("output:", color_enabled) << ' '
     << color(output_dir_with_slash(context.output_dir), "34", color_enabled)
     << "\n\n";

  os << heading("Loaded AP", color_enabled) << '\n';
  os << "  " << label("roots:", color_enabled) << "     "
     << color(join_roots(context.roots), "35", color_enabled) << '\n';
  os << "  " << label("objects:", color_enabled) << "   "
     << number(context.object_count, "32", color_enabled) << '\n';
  os << "  " << label("accesses:", color_enabled) << "  "
     << number(context.cache_stats.total_accesses, "32", color_enabled)
     << "\n\n";

  os << heading("Cache summary", color_enabled) << '\n';
  os << "  " << label("L1 misses:", color_enabled) << ' '
     << number(context.cache_stats.l1_misses, "33", color_enabled) << " / "
     << number(context.cache_stats.total_accesses, "32", color_enabled) << " ("
     << rate(percent(context.cache_stats.l1_misses,
                    context.cache_stats.total_accesses),
             "33", color_enabled)
     << "%)\n";
  os << "  " << label("L1 hits:", color_enabled) << "   "
     << number(context.cache_stats.l1_hits, "32", color_enabled) << '\n';
  os << "  " << label("L2 hits:", color_enabled) << "   "
     << number(context.cache_stats.l2_hits, "32", color_enabled) << '\n';
  os << "  " << label("L2 misses:", color_enabled) << ' '
     << number(context.cache_stats.l2_misses, "33", color_enabled) << '\n';
  os << "  " << label("memory:", color_enabled) << "    "
     << number(context.cache_stats.memory_accesses, "31", color_enabled)
     << '\n';
  os << "  " << label("cycles:", color_enabled) << "    "
     << number(context.cache_stats.total_cycles, "36", color_enabled)
     << " total, "
     << rate(context.cache_stats.average_cycles_per_access(), "36",
             color_enabled)
     << " avg/access\n\n";

  os << heading("Miss breakdown", color_enabled) << '\n';
  os << "  " << label("cold:", color_enabled) << "     "
     << number(context.miss_stats.cold, "34", color_enabled) << '\n';
  os << "  " << label("capacity:", color_enabled) << ' '
     << number(context.miss_stats.capacity, "33", color_enabled) << '\n';
  os << "  " << label("conflict:", color_enabled) << ' '
     << number(context.miss_stats.conflict, "31", color_enabled) << '\n';
  os << "  " << label("load:", color_enabled) << "     "
     << number(context.miss_stats.load, "36", color_enabled) << '\n';
  os << "  " << label("store:", color_enabled) << "    "
     << number(context.miss_stats.store, "35", color_enabled) << "\n\n";

  std::vector<std::pair<std::string, ObjectAccessStats>> objects(
    context.cache_stats.objects.begin(), context.cache_stats.objects.end());
  std::sort(objects.begin(), objects.end(), [](const auto & lhs,
                                               const auto & rhs) {
    if (lhs.second.misses != rhs.second.misses)
      return lhs.second.misses > rhs.second.misses;
    if (lhs.second.accesses != rhs.second.accesses)
      return lhs.second.accesses > rhs.second.accesses;
    return lhs.first < rhs.first;
  });

  os << heading("Top objects by misses", color_enabled) << '\n';
  const std::size_t limit = std::min<std::size_t>(objects.size(), 3);
  if (limit == 0)
  {
    os << "  (none)\n";
  }
  for (std::size_t i = 0; i < limit; ++i)
  {
    const auto & [name, stats] = objects[i];
    os << "  " << color(name, "35", color_enabled) << "  "
       << number(stats.misses, "33", color_enabled) << " / "
       << number(stats.accesses, "32", color_enabled) << " ("
       << rate(percent(stats.misses, stats.accesses), "33", color_enabled)
       << "%)\n";
  }

  if (mode == RunLogMode::Verbose)
    write_cache_config(os, context.cache_config, color_enabled);

  os << '\n' << heading("Wrote reports", color_enabled) << '\n';
  for (const auto & path : context.report_paths)
    os << "  " << color(path, "34", color_enabled) << '\n';

  return os.str();
}

}  // namespace apex
