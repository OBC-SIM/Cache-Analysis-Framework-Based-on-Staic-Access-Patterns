#pragma once

#include <string>
#include <vector>

#include "ap/AccessEvent.hpp"
#include "ap/ApNode.hpp"
#include "ap/ApProgram.hpp"

namespace apex
{

/**
 * @brief LAT v2 ApProgram을 선형 AccessEvent 스트림으로 변환한다.
 *
 * roots(yard.analyze) 함수 본문을 순회·루프 언롤하고, Array 노드의 access_path를
 * IndexExpr로 평가해 resolve_offset으로 byte_offset(객체 base 기준)을 채운다.
 * Call은 region_path inline 전개한다(인자 객체 치환은 미지원).
 *
 * @note SMP core_id 기본값은 0이다.
 */
class EventBuilder
{
public:
  /**
   * @brief ApProgram을 AccessEvent 스트림으로 변환한다.
   * @param program 파싱된 ApProgram (functions/roots/objects/structs)
   * @return AccessEvent 목록 (object_name=object id, byte_offset 채움)
   */
  std::vector<AccessEvent> build_program(const ApProgram & program);

private:
  void visit_v2(const ApNode & node, const ApProgram & program,
                std::vector<AccessEvent> & out,
                std::vector<LoopFrame> & loop_stack,
                const std::string & region_path, uint64_t & seq);
};

}  // namespace apex
