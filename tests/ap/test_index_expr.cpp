#include <gtest/gtest.h>

#include <stdexcept>

#include "ap/IndexExpr.hpp"

// access_path의 index value는 affine 식 문자열이다:
//   루프 변수("i"), 정수 리터럴("5"), var±const("i-1","i+1"), 변수 합("i+j").
// eval_index(expr, vars) → 정수 값. 미지의 변수는 예외.

using apex::eval_index;

TEST(IndexExpr, evaluates_integer_literal)
{
  EXPECT_EQ(eval_index("5", {}), 5);
}

TEST(IndexExpr, evaluates_negative_literal)
{
  EXPECT_EQ(eval_index("-3", {}), -3);
}

TEST(IndexExpr, evaluates_single_variable)
{
  EXPECT_EQ(eval_index("i", {{"i", 3}}), 3);
}

TEST(IndexExpr, evaluates_var_minus_constant)
{
  EXPECT_EQ(eval_index("i-1", {{"i", 3}}), 2);
}

TEST(IndexExpr, evaluates_var_plus_constant)
{
  EXPECT_EQ(eval_index("i+1", {{"i", 3}}), 4);
}

TEST(IndexExpr, evaluates_sum_of_two_variables)
{
  EXPECT_EQ(eval_index("i+j", {{"i", 3}, {"j", 4}}), 7);
}

TEST(IndexExpr, tolerates_whitespace)
{
  EXPECT_EQ(eval_index(" i - 1 ", {{"i", 10}}), 9);
}

TEST(IndexExpr, throws_on_unknown_variable)
{
  EXPECT_THROW(eval_index("k", {{"i", 3}}), std::runtime_error);
}
