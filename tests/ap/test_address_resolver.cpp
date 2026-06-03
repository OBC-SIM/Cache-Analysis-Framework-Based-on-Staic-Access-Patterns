#include <gtest/gtest.h>

#include "ap/AccessLayout.hpp"
#include "ap/AddressResolver.hpp"

// resolve_offset: 평가된 access_path + layout → 객체-상대 byte offset.
// 인덱스는 이미 정수로 평가됐다고 가정(EventBuilder가 IndexExpr로 처리).
// row-major(trailing 정렬) + 구조체 field offset/stride 누적.

using apex::AccessStep;
using apex::FieldLayout;
using apex::FieldStep;
using apex::IndexStep;
using apex::ObjectLayout;
using apex::resolve_offset;
using apex::StructLayout;

namespace
{
AccessStep idx(int64_t v)
{
  return IndexStep{v};
}
AccessStep fld(int64_t i)
{
  return FieldStep{i};
}
}  // namespace

// 전역 2D 배열 A[8][8], elem 4: A[2][3] → (2*8+3)*4 = 76
TEST(AddressResolver, global_2d_row_major)
{
  ObjectLayout obj;
  obj.shape = {8, 8};
  obj.elem_size = 4;
  EXPECT_EQ(resolve_offset(obj, {idx(2), idx(3)}, {}), 76);
}

// 포인터 param: shape는 trailing([64])뿐, indices는 2개.
// A[2][3] → (2*64+3)*4 = 524 (선두 index stride = product(shape)*elem)
TEST(AddressResolver, pointer_param_extra_leading_index)
{
  ObjectLayout obj;
  obj.shape = {64};
  obj.elem_size = 4;
  EXPECT_EQ(resolve_offset(obj, {idx(2), idx(3)}, {}), 524);
}

// 1D: array[5], elem 4 → 20
TEST(AddressResolver, one_dimensional)
{
  ObjectLayout obj;
  obj.shape = {100};
  obj.elem_size = 4;
  EXPECT_EQ(resolve_offset(obj, {idx(5)}, {}), 20);
}

// 구조체: o.items[i].x / .y  (test_struct fixture 수치)
//   Outer { tag@0(i32), items@8([4 x S], elem 16) }, S { x@0(i32), y@8(double) }
//   x: 8 + i*16 + 0 ;  y: 8 + i*16 + 8
TEST(AddressResolver, struct_array_field_path)
{
  std::map<std::string, StructLayout> structs;

  StructLayout s;
  s.size = 16;
  s.fields.push_back(FieldLayout{0, {}, 4, ""});   // x
  s.fields.push_back(FieldLayout{8, {}, 8, ""});   // y
  structs["S"] = s;

  StructLayout outer;
  outer.size = 72;
  outer.fields.push_back(FieldLayout{0, {}, 4, ""});       // tag
  outer.fields.push_back(FieldLayout{8, {4}, 16, "S"});    // items: [4 x S]
  structs["Outer"] = outer;

  ObjectLayout o;        // pointer to Outer
  o.struct_type = "Outer";
  o.elem_size = 72;

  // o.items[2].x
  EXPECT_EQ(resolve_offset(o, {fld(1), idx(2), fld(0)}, structs), 8 + 2 * 16 + 0);
  // o.items[2].y
  EXPECT_EQ(resolve_offset(o, {fld(1), idx(2), fld(1)}, structs), 8 + 2 * 16 + 8);
}
