#include "ap/EventBuilder.hpp"

#include <unordered_map>
#include <variant>

#include "ap/AddressResolver.hpp"
#include "ap/IndexExpr.hpp"

namespace apex
{

// ── public ────────────────────────────────────────────────────

EventBuilder & EventBuilder::register_function(
  const std::string & name, std::vector<std::unique_ptr<ApNode>> nodes)
{
  func_map_[name] = std::move(nodes);
  return *this;
}

std::vector<AccessEvent>
EventBuilder::build(std::vector<std::unique_ptr<ApNode>> nodes)
{
  std::vector<AccessEvent> out;
  std::vector<LoopFrame> loop_stack;
  uint64_t seq = 0;
  for (const auto & n : nodes) visit(*n, out, loop_stack, "", seq);
  return out;
}

// ── private ───────────────────────────────────────────────────

void EventBuilder::visit(const ApNode & node, std::vector<AccessEvent> & out,
                         std::vector<LoopFrame> & loop_stack,
                         const std::string & region_path, uint64_t & seq)
{
  switch (node.kind())
  {
    case ApNodeKind::Scalar: {
      const auto & s = static_cast<const ScalarNode &>(node);
      AccessEvent e;
      e.sequence_id = seq++;
      e.region_path = region_path;
      e.op = s.op;
      e.object_name = s.name;
      e.loop_stack = loop_stack;
      out.push_back(std::move(e));
      break;
    }

    case ApNodeKind::Array: {
      const auto & a = static_cast<const ArrayNode &>(node);
      AccessEvent e;
      e.sequence_id = seq++;
      e.region_path = region_path;
      e.op = a.op;
      e.object_name = a.name;
      e.size = static_cast<int32_t>(a.elem_size);
      e.loop_stack = loop_stack;

      // 유도 변수 이름을 loop_stack에서 찾아 정수 인덱스로 평가
      for (const auto & idx_var : a.indices)
      {
        bool found = false;
        for (auto it = loop_stack.rbegin(); it != loop_stack.rend(); ++it)
        {
          if (it->var == idx_var)
          {
            e.indices.push_back(it->iter);
            found = true;
            break;
          }
        }
        if (!found) e.indices.push_back(0);
      }
      out.push_back(std::move(e));
      break;
    }

    case ApNodeKind::Loop: {
      const auto & l = static_cast<const LoopNode &>(node);
      for (int64_t iter = l.start; iter < l.bound; ++iter)
      {
        loop_stack.push_back({l.var, iter});
        for (const auto & child : l.body)
          visit(*child, out, loop_stack, region_path, seq);
        loop_stack.pop_back();
      }
      break;
    }

    case ApNodeKind::Call: {
      const auto & c = static_cast<const CallNode &>(node);
      auto it = func_map_.find(c.callee);
      if (it == func_map_.end()) break;
      const std::string path =
        region_path.empty() ? c.callee : region_path + "/" + c.callee;
      for (const auto & child : it->second)
        visit(*child, out, loop_stack, path, seq);
      break;
    }
  }
}

// ── LAT v2 ────────────────────────────────────────────────────

std::vector<AccessEvent> EventBuilder::build_program(const ApProgram & program)
{
  std::vector<AccessEvent> out;
  std::vector<LoopFrame> loop_stack;
  uint64_t seq = 0;
  for (const std::string & root : program.roots)
  {
    auto it = program.functions.find(root);
    if (it == program.functions.end()) continue;
    for (const auto & n : it->second)
      visit_v2(*n, program, out, loop_stack, root, seq);
  }
  return out;
}

void EventBuilder::visit_v2(const ApNode & node, const ApProgram & program,
                            std::vector<AccessEvent> & out,
                            std::vector<LoopFrame> & loop_stack,
                            const std::string & region_path, uint64_t & seq)
{
  switch (node.kind())
  {
    case ApNodeKind::Scalar: {
      const auto & s = static_cast<const ScalarNode &>(node);
      AccessEvent e;
      e.sequence_id = seq++;
      e.region_path = region_path;
      e.op = s.op;
      e.object_name = s.object;
      e.loop_stack = loop_stack;
      out.push_back(std::move(e));
      break;
    }

    case ApNodeKind::Array: {
      const auto & a = static_cast<const ArrayNode &>(node);

      std::unordered_map<std::string, int64_t> vars;
      for (const auto & f : loop_stack) vars[f.var] = f.iter;

      std::vector<AccessStep> path;
      path.reserve(a.access_path.size());
      for (const auto & rs : a.access_path)
      {
        if (std::holds_alternative<RawIndexStep>(rs))
          path.push_back(
            IndexStep{eval_index(std::get<RawIndexStep>(rs).expr, vars)});
        else
          path.push_back(std::get<FieldStep>(rs));
      }

      ObjectLayout obj;
      auto it = program.objects.find(a.object);
      if (it != program.objects.end()) obj = it->second;

      AccessEvent e;
      e.sequence_id = seq++;
      e.region_path = region_path;
      e.op = a.op;
      e.object_name = a.object;
      e.byte_offset = resolve_offset(obj, path, program.structs);
      e.loop_stack = loop_stack;
      out.push_back(std::move(e));
      break;
    }

    case ApNodeKind::Loop: {
      const auto & l = static_cast<const LoopNode &>(node);
      for (int64_t iter = l.start; iter < l.bound; ++iter)
      {
        loop_stack.push_back({l.var, iter});
        for (const auto & child : l.body)
          visit_v2(*child, program, out, loop_stack, region_path, seq);
        loop_stack.pop_back();
      }
      break;
    }

    case ApNodeKind::Call: {
      const auto & c = static_cast<const CallNode &>(node);
      auto it = program.functions.find(c.callee);
      if (it == program.functions.end()) break;
      const std::string path =
        region_path.empty() ? c.callee : region_path + "/" + c.callee;
      for (const auto & child : it->second)
        visit_v2(*child, program, out, loop_stack, path, seq);
      break;
    }
  }
}

}  // namespace apex
