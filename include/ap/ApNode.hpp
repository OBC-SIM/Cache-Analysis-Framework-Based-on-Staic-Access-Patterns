#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "ap/AccessLayout.hpp"

namespace apex
{

/**
 * @brief AP JSON의 노드 종류.
 */
enum class ApNodeKind
{
  Scalar,
  Array,
  Call,
  Loop
};

struct ApNode
{
  virtual ~ApNode() = default;
  virtual ApNodeKind kind() const = 0;
};

/**
 * @brief 미평가 인덱스 스텝 — affine 식 문자열("i","i-1","5").
 *
 * EventBuilder가 루프 문맥으로 평가해 IndexStep으로 바꾼다.
 */
struct RawIndexStep
{
  std::string expr;
};

/**
 * @brief access_path의 한 스텝(미평가, sum 타입) — LAT v2.
 *
 * 미평가 인덱스(RawIndexStep)와 구조체 필드(FieldStep) 중 하나만 보유한다.
 */
using RawAccessStep = std::variant<RawIndexStep, FieldStep>;

/**
 * @brief 스칼라 변수 접근 노드.
 *
 * @pre op은 "load" 또는 "store"
 */
struct ScalarNode : ApNode
{
  std::string object;  ///< metadata.objects의 object id
  std::string op;

  ApNodeKind kind() const override { return ApNodeKind::Scalar; }
};

/**
 * @brief 배열/구조체 접근 노드.
 *
 * shape/elem_size는 object id로 metadata.objects/structs에서 해석한다.
 * access_path가 인덱스·필드 스텝을 순서대로 담는다.
 *
 * @pre op은 "load" 또는 "store"
 */
struct ArrayNode : ApNode
{
  std::string object;  ///< metadata.objects의 object id
  std::vector<RawAccessStep> access_path;
  std::string op;

  ApNodeKind kind() const override { return ApNodeKind::Array; }
};

/**
 * @brief 함수 호출 노드.
 */
struct CallNode : ApNode
{
  std::string callee;
  std::vector<std::string> args;
  std::vector<std::string> arg_objects;  ///< LAT v2: 인자별 object id(또는 리터럴)

  ApNodeKind kind() const override { return ApNodeKind::Call; }
};

/**
 * @brief 루프 노드. body에 자식 노드를 소유한다.
 *
 * trip_count = bound - start
 */
struct LoopNode : ApNode
{
  std::string var;
  int64_t start = 0;
  int64_t bound = 0;
  int64_t depth = 1;
  std::vector<std::unique_ptr<ApNode>> body;

  ApNodeKind kind() const override { return ApNodeKind::Loop; }

  int64_t trip_count() const { return bound - start; }
};

}  // namespace apex
