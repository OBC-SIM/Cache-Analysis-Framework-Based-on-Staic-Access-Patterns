#include "ap/EventBuilder.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>
#include <variant>

#include "ap/AddressResolver.hpp"
#include "ap/IndexExpr.hpp"

namespace apex
{
int64_t EventBuilder::eval_bound_index(
  const RawIndexStep & step, const std::vector<LoopFrame> & loop_stack,
  const std::unordered_map<std::string, int64_t> & bindings)
{
  const std::string & expr = step.expr;

  bool identifier = !expr.empty();
  for (char c : expr)
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
    {
      identifier = false;
      break;
    }
  if (identifier)
  {
    // 슬롯 힌트: 직전에 expr이 있던 loop_stack 위치를 이름 1회 비교로 재검증해
    // 재사용한다. 빗나가면(문맥 변화) 아래 역스캔이 다시 채운다.
    auto hint = slot_hint_.find(&step);
    if (hint != slot_hint_.end())
    {
      const std::size_t s = static_cast<std::size_t>(hint->second);
      if (s < loop_stack.size() && loop_stack[s].var == expr)
        return loop_stack[s].iter;
    }
    for (std::size_t s = loop_stack.size(); s-- > 0;)
      if (loop_stack[s].var == expr)
      {
        slot_hint_[&step] = static_cast<int>(s);
        return loop_stack[s].iter;
      }
    auto bound = bindings.find(expr);
    if (bound != bindings.end()) return bound->second;
  }

  std::unordered_map<std::string, int64_t> vars;
  for (const auto & frame : loop_stack) vars[frame.var] = frame.iter;
  for (const auto & binding : bindings) vars[binding.first] = binding.second;
  return eval_index(expr, vars);
}

const ObjectLayout & EventBuilder::layout_of(
  const ArrayNode & a, const ApProgram & program,
  const std::unordered_map<std::string, std::string> & obj_subst)
{
  static const ObjectLayout kEmpty;

  if (obj_subst.empty())  // 치환 없음 → 노드당 1회 해석 후 캐시(string 해시 제거).
  {
    auto cached = node_cache_.find(&a);
    if (cached != node_cache_.end()) return *cached->second;
    auto it = program.objects.find(a.object);
    const ObjectLayout * ptr =
      it != program.objects.end() ? &it->second : &kEmpty;
    node_cache_.emplace(&a, ptr);
    return *ptr;
  }

  auto sit = obj_subst.find(a.object);  // 치환 문맥 → 호출처마다 달라 캐시 안 함.
  const std::string & object = sit != obj_subst.end() ? sit->second : a.object;
  auto it = program.objects.find(object);
  return it != program.objects.end() ? it->second : kEmpty;
}

std::vector<AccessEvent> EventBuilder::build_program(const ApProgram & program)
{
  std::vector<AccessEvent> out;
  visit_program(program, [&](AccessEvent && event) {
    out.push_back(std::move(event));
  });
  return out;
}

void EventBuilder::visit_program(const ApProgram & program,
                                 const EventSink & sink)
{
  node_cache_.clear();  // 이전 program의 노드 포인터가 남지 않게 한다.
  slot_hint_.clear();
  std::vector<LoopFrame> loop_stack;
  uint64_t seq = 0;
  const std::unordered_map<std::string, int64_t> no_bindings;
  const std::unordered_map<std::string, std::string> no_subst;
  for (const std::string & root : program.roots)
  {
    auto it = program.functions.find(root);
    if (it == program.functions.end()) continue;
    for (const auto & n : it->second)
      visit_v2(*n, program, sink, loop_stack, root, seq, no_bindings, no_subst);
  }
}

void EventBuilder::visit_v2(
  const ApNode & node, const ApProgram & program, const EventSink & sink,
  std::vector<LoopFrame> & loop_stack, const std::string & region_path,
  uint64_t & seq, const std::unordered_map<std::string, int64_t> & bindings,
  const std::unordered_map<std::string, std::string> & obj_subst)
{
  // callee param 치환을 반영해 접근 객체 id를 해석한다.
  auto resolve_object = [&](const std::string & id) {
    auto it = obj_subst.find(id);
    return it != obj_subst.end() ? it->second : id;
  };

  switch (node.kind())
  {
    case ApNodeKind::Scalar: {
      const auto & s = static_cast<const ScalarNode &>(node);
      AccessEvent e;
      e.sequence_id = seq++;
      e.region_path = region_path;
      e.op = s.op;
      e.object_name = resolve_object(s.object);
      sink(std::move(e));
      break;
    }

    case ApNodeKind::Array: {
      const auto & a = static_cast<const ArrayNode &>(node);
      const std::string object = resolve_object(a.object);
      const ObjectLayout & obj = layout_of(a, program, obj_subst);

      int64_t byte_offset = 0;
      const bool index_only = std::all_of(
        a.access_path.begin(), a.access_path.end(),
        [](const RawAccessStep & step) {
          return std::holds_alternative<RawIndexStep>(step);
        });
      if (index_only)
      {
        int64_t stride = obj.elem_size;
        int shape_index = static_cast<int>(obj.shape.size());
        for (std::size_t p = a.access_path.size(); p-- > 0;)
        {
          const auto & raw = std::get<RawIndexStep>(a.access_path[p]);
          byte_offset += eval_bound_index(raw, loop_stack, bindings) * stride;
          --shape_index;
          if (shape_index >= 0)
            stride *= obj.shape[static_cast<std::size_t>(shape_index)];
        }
      }
      else
      {
        std::vector<AccessStep> path;
        path.reserve(a.access_path.size());
        for (const auto & raw : a.access_path)
          if (std::holds_alternative<RawIndexStep>(raw))
            path.push_back(IndexStep{eval_bound_index(
              std::get<RawIndexStep>(raw), loop_stack, bindings)});
          else
            path.push_back(std::get<FieldStep>(raw));
        byte_offset = resolve_offset(obj, path, program.structs);
      }

      AccessEvent e;
      e.sequence_id = seq++;
      e.region_path = region_path;
      e.op = a.op;
      e.object_name = object;
      e.byte_offset = byte_offset;
      sink(std::move(e));
      break;
    }

    case ApNodeKind::Loop: {
      const auto & l = static_cast<const LoopNode &>(node);
      for (int64_t iter = l.start; iter < l.bound; ++iter)
      {
        loop_stack.push_back({l.var, iter});
        for (const auto & child : l.body)
          visit_v2(*child, program, sink, loop_stack, region_path, seq, bindings,
                   obj_subst);
        loop_stack.pop_back();
      }
      break;
    }

    case ApNodeKind::Call: {
      const auto & c = static_cast<const CallNode &>(node);
      auto fit = program.functions.find(c.callee);
      if (fit == program.functions.end()) break;

      // 호출자 스코프 변수(루프 + 현재 바인딩)로 인자 값을 평가한다.
      std::unordered_map<std::string, int64_t> caller_vars;
      for (const auto & f : loop_stack) caller_vars[f.var] = f.iter;
      for (const auto & b : bindings) caller_vars[b.first] = b.second;

      // callee param ↔ 호출자 인자로 object 치환·index 바인딩을 구성한다.
      // arg_objects[k]가 등록된 객체면 치환, 아니면(루프변수·리터럴) 정수로 평가.
      std::unordered_map<std::string, int64_t> callee_bindings;
      std::unordered_map<std::string, std::string> callee_subst;
      auto pit = program.params.find(c.callee);
      if (pit != program.params.end())
      {
        const auto & params = pit->second;
        for (size_t k = 0; k < params.size() && k < c.arg_objects.size(); ++k)
        {
          const std::string resolved = resolve_object(c.arg_objects[k]);
          if (program.objects.count(resolved))
            callee_subst["function:" + c.callee + "::param:" + params[k]] =
              resolved;
          else
            callee_bindings[params[k]] = eval_index(resolved, caller_vars);
        }
      }

      const std::string path =
        region_path.empty() ? c.callee : region_path + "/" + c.callee;
      for (const auto & child : fit->second)
        visit_v2(*child, program, sink, loop_stack, path, seq, callee_bindings,
                 callee_subst);
      break;
    }
  }
}

}  // namespace apex
