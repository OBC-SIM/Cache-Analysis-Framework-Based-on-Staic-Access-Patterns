#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

namespace apex
{

/**
 * @brief access_path의 affine 인덱스 식을 정수 값으로 평가한다.
 *
 * LAT v2의 `{kind:"index", value:"<expr>"}`에서 value는 루프 유도 변수와
 * 정수 상수를 `+`/`-`로 결합한 affine 식이다 (예: "i", "5", "i-1", "i+j").
 * 곱셈 등 비affine 연산은 현재 입력에 나타나지 않아 지원하지 않는다.
 *
 * @param expr 평가할 식 문자열. 공백은 무시된다.
 * @param vars 루프 유도 변수 이름 → 현재 iteration 값.
 * @return 평가된 정수 인덱스 값.
 * @throws std::runtime_error vars에 없는 식별자를 만나거나 식이 형식에 맞지 않을 때.
 */
int64_t eval_index(const std::string& expr,
                   const std::unordered_map<std::string, int64_t>& vars);

}  // namespace apex
