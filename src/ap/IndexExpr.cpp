#include "ap/IndexExpr.hpp"

#include <cctype>
#include <stdexcept>

namespace apex
{
namespace
{

bool is_ident_char(char c)
{
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// 한 항(term)을 값으로 환산한다: 전부 숫자면 정수 리터럴, 아니면 변수 조회.
int64_t resolve_term(const std::string& tok,
                     const std::unordered_map<std::string, int64_t>& vars)
{
  bool numeric = !tok.empty();
  for (char c : tok)
    if (!std::isdigit(static_cast<unsigned char>(c))) numeric = false;

  if (numeric) return std::stoll(tok);

  auto it = vars.find(tok);
  if (it == vars.end())
    throw std::runtime_error("IndexExpr: unknown identifier '" + tok + "'");
  return it->second;
}

}  // namespace

int64_t eval_index(const std::string& expr,
                   const std::unordered_map<std::string, int64_t>& vars)
{
  std::string s;
  for (char c : expr)
    if (!std::isspace(static_cast<unsigned char>(c))) s += c;
  if (s.empty()) throw std::runtime_error("IndexExpr: empty expression");

  int64_t result = 0;
  std::size_t i = 0;
  while (i < s.size())
  {
    int sign = 1;
    if (s[i] == '+' || s[i] == '-')
    {
      if (s[i] == '-') sign = -1;
      ++i;
    }
    std::size_t start = i;
    while (i < s.size() && is_ident_char(s[i])) ++i;
    if (i == start)
      throw std::runtime_error("IndexExpr: malformed expression '" + expr + "'");
    result += sign * resolve_term(s.substr(start, i - start), vars);
  }
  return result;
}

}  // namespace apex
