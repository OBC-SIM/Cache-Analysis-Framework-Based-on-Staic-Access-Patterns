#include "pipeline/Pipeline.hpp"

#include "ap/EventBuilder.hpp"
#include "memory/MemoryLayout.hpp"
#include "pipeline/EventConsumer.hpp"

namespace apex
{

Pipeline::Pipeline(HierarchyConfig config) : config_(std::move(config)) {}

PipelineResult Pipeline::run(const ApProgram & program)
{
  int line_size = 32;
  for (const auto & c : config_.caches)
    if (c.role == "L1")
    {
      line_size = c.line_size;
      break;
    }

  // 객체 배치: metadata.objects 크기로 line_size 정렬 (id 순, 결정적).
  MemoryLayout layout(static_cast<uint64_t>(line_size));
  for (const auto & [id, obj] : program.objects)
    layout.add_object(id, obj.total_bytes());

  EventConsumer consumer(config_, layout, line_size);
  EventBuilder{}.visit_program(program, [&](AccessEvent && event) {
    consumer.consume(std::move(event));
  });
  return consumer.take_result();
}

}  // namespace apex
