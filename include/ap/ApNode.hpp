#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace apex {

/**
 * @brief AP JSON의 노드 종류.
 */
enum class ApNodeKind { Scalar, Array, Call, Loop };

struct ApNode {
    virtual ~ApNode() = default;
    virtual ApNodeKind kind() const = 0;
};

/**
 * @brief 스칼라 변수 접근 노드.
 *
 * @pre op은 "load" 또는 "store"
 */
struct ScalarNode : ApNode {
    std::string name;
    std::string op;

    ApNodeKind kind() const override { return ApNodeKind::Scalar; }
};

/**
 * @brief 배열 접근 노드.
 *
 * shape가 비어 있으면 ApLoader가 shapes.yaml에서 보완한다.
 *
 * @pre op은 "load" 또는 "store"
 * @pre indices의 각 원소는 루프 유도 변수 이름
 */
struct ArrayNode : ApNode {
    std::string              name;
    std::vector<std::string> indices;
    std::vector<int64_t>     shape;
    int64_t                  elem_size = 4;
    std::string              op;

    ApNodeKind kind() const override { return ApNodeKind::Array; }
};

/**
 * @brief 함수 호출 노드.
 */
struct CallNode : ApNode {
    std::string              callee;
    std::vector<std::string> args;

    ApNodeKind kind() const override { return ApNodeKind::Call; }
};

/**
 * @brief 루프 노드. body에 자식 노드를 소유한다.
 *
 * trip_count = bound - start
 */
struct LoopNode : ApNode {
    std::string                        var;
    int64_t                            start = 0;
    int64_t                            bound = 0;
    int64_t                            depth = 1;
    std::vector<std::unique_ptr<ApNode>> body;

    ApNodeKind kind() const override { return ApNodeKind::Loop; }

    int64_t trip_count() const { return bound - start; }
};

}  // namespace apex
