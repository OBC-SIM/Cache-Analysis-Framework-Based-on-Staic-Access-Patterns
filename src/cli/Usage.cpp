#include "cli/Usage.hpp"

namespace apex
{

std::string Usage::text()
{
  return
    "Usage:\n"
    "  apex-cache run <input.ap.json> --cache <cache.yaml> [options]\n"
    "  apex-cache help\n"
    "  apex-cache --help\n"
    "\n"
    "Options:\n"
    "  --cache <cache.yaml>  Cache hierarchy YAML file (required)\n"
    "  --output <dir>       Directory for report files (default: results)\n"
    "  --quiet              Print only the output directory message\n"
    "  --verbose            Print cache config details\n"
    "  --no-color           Disable ANSI color output\n"
    "  -h, --help           Show this help message\n"
    "\n"
    "Reports:\n"
    "  <input-base>.json\n"
    "  <input-base>.csv\n"
    "  <input-base>_objects.csv\n"
    "  <input-base>_diagnostics.md\n"
    "\n"
    "Example:\n"
    "  apex-cache run examples/test_stencil_g_ape.json --cache settings/cache.yaml\n"
    "  apex-cache run examples/test_stencil_g_ape.json --cache settings/cache.yaml --output results --verbose\n";
}

}  // namespace apex
