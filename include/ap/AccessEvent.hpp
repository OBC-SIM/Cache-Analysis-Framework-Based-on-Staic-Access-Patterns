#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace apex {

/** @brief 루프 언롤 시점의 활성 루프 상태 (유도 변수 이름, 현재 iteration 값). */
struct LoopFrame {
    std::string var;
    int64_t     iter = 0;
};

/**
 * @brief AP 트리에서 복원된 단일 메모리 접근 이벤트.
 *
 * byte_address와 cache_line은 Phase 3(Memory Layer)에서 채워진다.
 * EventBuilder는 byte_address = 0, cache_line = 0으로 초기화한다.
 */
struct AccessEvent {
    uint64_t    sequence_id  = 0;
    uint64_t    region_id    = 0;
    std::string region_path;

    int32_t     core_id      = 0;

    std::string op;           ///< "load" | "store"
    std::string object_name;
    std::vector<int64_t> indices;   ///< 평가된 정수 인덱스

    uint64_t    byte_address = 0;
    uint64_t    cache_line   = 0;
    int32_t     size         = 4;   ///< bytes

    std::vector<LoopFrame> loop_stack;

    std::string source;       ///< 선택적. 없으면 빈 문자열
};

}  // namespace apex
