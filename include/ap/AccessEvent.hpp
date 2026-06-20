#pragma once

#include <cstdint>
#include <string>

namespace apex
{

/** @brief 루프 언롤 시점의 활성 루프 상태 (유도 변수 이름, 현재 iteration 값).
 */
struct LoopFrame
{
  std::string var;
  int64_t iter = 0;
};

/**
 * @brief AP 트리에서 복원된 단일 메모리 접근 이벤트 (LAT v2).
 *
 * EventBuilder가 access_path를 평가해 byte_offset(객체 base 기준)을 채운다.
 */
struct AccessEvent
{
  uint64_t sequence_id = 0;
  std::string region_path;
  int32_t core_id = 0;

  std::string op;           ///< "load" | "store"
  std::string object_name;  ///< metadata.objects의 object id

  int64_t byte_offset = 0;    ///< 객체 base 기준 오프셋 (EventBuilder가 채움)
};

}  // namespace apex
