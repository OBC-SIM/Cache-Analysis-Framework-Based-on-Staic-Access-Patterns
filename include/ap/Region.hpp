#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace apex {

enum class RegionKind { Function, Loop, Block, Call, Custom };

/**
 * @brief AP의 분석 단위. 트리 구조를 이루며 inclusive/exclusive 집계에 사용된다.
 *
 * parent_id가 없으면 루트(function) Region이다.
 */
struct Region {
    uint64_t                 id   = 0;
    std::string              name;
    RegionKind               kind = RegionKind::Loop;
    std::optional<uint64_t>  parent_id;
    std::optional<int64_t>   trip_count;
    std::vector<uint64_t>    children;
};

}  // namespace apex
