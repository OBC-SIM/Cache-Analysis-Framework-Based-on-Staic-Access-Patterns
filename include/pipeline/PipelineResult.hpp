#pragma once

#include "analysis/Attribution.hpp"
#include "analysis/Diagnostics.hpp"
#include "analysis/SimulationStats.hpp"

namespace apex
{

/** @brief Cache simulation 결과와 miss 귀속 정보를 묶는다. */
struct PipelineResult
{
  MissStats stats;
  SimulationStats cache_stats;
  Attribution attribution;
};

}  // namespace apex
