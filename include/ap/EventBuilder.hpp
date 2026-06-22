#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ap/AccessEvent.hpp"
#include "ap/ApNode.hpp"
#include "ap/ApProgram.hpp"

namespace apex
{

using EventSink = std::function<void(AccessEvent &&)>;

/**
 * @brief LAT v2 ApProgram을 선형 AccessEvent 스트림으로 변환한다.
 *
 * roots(yard.analyze) 함수 본문을 순회·루프 언롤하고, Array 노드의 access_path를
 * IndexExpr로 평가해 resolve_offset으로 byte_offset(객체 base 기준)을 채운다.
 * Call은 callee 본문을 inline 전개하며, callee param을 호출자 인자로 바인딩한다
 * (object id → 실인자 객체, index 변수 → 인자 값).
 *
 * @note SMP core_id 기본값은 0이다.
 */
class EventBuilder
{
public:
  /**
   * @brief ApProgram을 AccessEvent 스트림으로 변환한다.
   * @param program 파싱된 ApProgram (functions/params/roots/objects/structs)
   * @return AccessEvent 목록 (object_name=object id, byte_offset 채움)
   */
  std::vector<AccessEvent> build_program(const ApProgram & program);

  /**
   * @brief ApProgram에서 생성한 event를 순서대로 sink에 전달한다.
   * @param program 파싱된 ApProgram
   * @param sink 각 event의 소유권을 즉시 전달받는 consumer
   */
  void visit_program(const ApProgram & program, const EventSink & sink);

private:
  /**
   * @param bindings  callee param 변수 이름 → 바인딩된 정수 값 (index 평가에 합류)
   * @param obj_subst callee param object id → 호출자 인자 object id (접근 객체 치환)
   */
  void visit_v2(const ApNode & node, const ApProgram & program,
                const EventSink & sink,
                std::vector<LoopFrame> & loop_stack,
                const std::string & region_path, uint64_t & seq,
                const std::unordered_map<std::string, int64_t> & bindings,
                const std::unordered_map<std::string, std::string> & obj_subst);

  /**
   * @brief Array 노드의 ObjectLayout을 돌려준다.
   *
   * 치환 없는 최상위 문맥(obj_subst 비어 있음)에서는 노드 포인터로 1회만
   * 해석·캐시해, 반복 접근에서 object id 문자열을 해시하지 않는다. 치환 문맥에서는
   * 노드가 호출처마다 다른 객체로 해석될 수 있어 캐시하지 않는다.
   *
   * 미등록 id는 빈 ObjectLayout을 가리킨다. 반환 참조는 program 수명 동안 유효하다.
   */
  const ObjectLayout & layout_of(
    const ArrayNode & a, const ApProgram & program,
    const std::unordered_map<std::string, std::string> & obj_subst);

  /**
   * @brief access_path의 index 식을 현재 loop/binding 문맥에서 정수로 평가한다.
   *
   * 단일 식별자(루프 변수)는 직전 loop_stack 슬롯 위치를 step별로 기억했다가
   * 이름이 일치하면 비교 1회로 재사용한다(활성 nest 내 induction 변수명이
   * 유일하다는 전제 — 생성 AP에서 성립). 복합 식은 기존대로 평가한다.
   */
  int64_t eval_bound_index(
    const RawIndexStep & step, const std::vector<LoopFrame> & loop_stack,
    const std::unordered_map<std::string, int64_t> & bindings);

  std::unordered_map<const ArrayNode *, const ObjectLayout *> node_cache_;
  std::unordered_map<const RawIndexStep *, int> slot_hint_;
};

}  // namespace apex
