#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace apex
{

/**
 * @brief 구조체 한 필드의 배치 정보 (LAT v2 metadata.structs.fields 대응).
 *
 * @param offset      구조체 시작 기준 바이트 오프셋
 * @param shape       필드가 배열이면 그 차원들, 아니면 비어 있음
 * @param elem_size   필드 원소(또는 스칼라) 바이트 크기
 * @param struct_type 필드 원소가 구조체면 그 이름, 아니면 ""
 */
struct FieldLayout
{
  int64_t offset = 0;
  std::vector<int64_t> shape;
  int64_t elem_size = 0;
  std::string struct_type;
};

/** @brief 구조체 한 타입의 배치 (LAT v2 metadata.structs[name] 대응). */
struct StructLayout
{
  int64_t size = 0;
  int64_t align = 0;
  std::vector<FieldLayout> fields;  ///< field index 순
};

/**
 * @brief 접근 base 객체의 타입 배치 (LAT v2 metadata.objects[id] 대응).
 *
 * 배열은 shape에 차원(전역/로컬=full, 포인터 param=trailing), struct_type="".
 * 구조체(포인터)는 struct_type에 이름, shape는 비어 있음.
 */
struct ObjectLayout
{
  std::vector<int64_t> shape;
  int64_t elem_size = 0;
  std::string struct_type;

  /** @brief 객체 전체 바이트 크기 (배치용). 구조체면 그 크기, 배열이면 product(shape)*elem. */
  uint64_t total_bytes() const
  {
    if (!struct_type.empty()) return static_cast<uint64_t>(elem_size);
    uint64_t n = 1;
    for (int64_t d : shape) n *= static_cast<uint64_t>(d);
    return n * static_cast<uint64_t>(elem_size);
  }
};

/** @brief 평가된 배열 인덱스 스텝. */
struct IndexStep
{
  int64_t value = 0;
};

/** @brief 구조체 필드 스텝 (raw·evaluated 공용). */
struct FieldStep
{
  int64_t index = 0;
};

/**
 * @brief 평가된 access_path 한 스텝 (sum 타입).
 *
 * 인덱스 스텝(평가된 정수)과 필드 스텝 중 하나만 보유한다.
 */
using AccessStep = std::variant<IndexStep, FieldStep>;

}  // namespace apex
