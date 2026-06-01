#include <gtest/gtest.h>
#include "ap/EventBuilder.hpp"
#include "ap/ApNode.hpp"
#include "ap/AccessEvent.hpp"

using namespace apex;

// ── 헬퍼 ─────────────────────────────────────────────────────

static std::vector<AccessEvent> build(std::vector<std::unique_ptr<ApNode>> nodes) {
    return EventBuilder{}.build(std::move(nodes));
}

// ── scalar ────────────────────────────────────────────────────

TEST(EventBuilder, scalar_produces_one_event) {
    auto n    = std::make_unique<ScalarNode>();
    n->name   = "x";
    n->op     = "load";

    std::vector<std::unique_ptr<ApNode>> nodes;
    nodes.push_back(std::move(n));
    auto events = build(std::move(nodes));

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].object_name, "x");
    EXPECT_EQ(events[0].op,          "load");
    EXPECT_EQ(events[0].sequence_id, 0u);
    EXPECT_TRUE(events[0].loop_stack.empty());
}

// ── array outside loop ────────────────────────────────────────

TEST(EventBuilder, array_access_outside_loop_produces_one_event) {
    auto n       = std::make_unique<ArrayNode>();
    n->name      = "A";
    n->op        = "store";
    n->indices   = {"i"};
    n->shape     = {10};
    n->elem_size = 4;

    std::vector<std::unique_ptr<ApNode>> nodes;
    nodes.push_back(std::move(n));
    auto events = build(std::move(nodes));

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].object_name, "A");
    EXPECT_EQ(events[0].sequence_id, 0u);
    EXPECT_TRUE(events[0].loop_stack.empty());
}

// ── loop unroll ───────────────────────────────────────────────

TEST(EventBuilder, loop_N_iterations_produces_N_events) {
    auto inner   = std::make_unique<ArrayNode>();
    inner->name  = "A";
    inner->op    = "load";
    inner->indices   = {"i"};
    inner->shape     = {8};
    inner->elem_size = 4;

    auto loop    = std::make_unique<LoopNode>();
    loop->var    = "i";
    loop->start  = 0;
    loop->bound  = 8;
    loop->depth  = 1;
    loop->body.push_back(std::move(inner));

    std::vector<std::unique_ptr<ApNode>> nodes;
    nodes.push_back(std::move(loop));
    auto events = build(std::move(nodes));

    ASSERT_EQ(events.size(), 8u);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(events[i].sequence_id, static_cast<uint64_t>(i));
        ASSERT_EQ(events[i].indices.size(), 1u);
        EXPECT_EQ(events[i].indices[0], i);
    }
}

TEST(EventBuilder, nested_loop_MxN_produces_MxN_events) {
    auto acc       = std::make_unique<ArrayNode>();
    acc->name      = "A";
    acc->op        = "load";
    acc->indices   = {"i", "j"};
    acc->shape     = {3, 4};
    acc->elem_size = 4;

    auto inner    = std::make_unique<LoopNode>();
    inner->var    = "j";
    inner->start  = 0;
    inner->bound  = 4;
    inner->depth  = 2;
    inner->body.push_back(std::move(acc));

    auto outer    = std::make_unique<LoopNode>();
    outer->var    = "i";
    outer->start  = 0;
    outer->bound  = 3;
    outer->depth  = 1;
    outer->body.push_back(std::move(inner));

    std::vector<std::unique_ptr<ApNode>> nodes;
    nodes.push_back(std::move(outer));
    auto events = build(std::move(nodes));

    ASSERT_EQ(events.size(), 12u);  // 3 * 4
}

// ── call expansion ────────────────────────────────────────────

TEST(EventBuilder, call_expansion_inlines_callee_events) {
    // callee "foo": ScalarNode x
    auto callee_node  = std::make_unique<ScalarNode>();
    callee_node->name = "x";
    callee_node->op   = "load";

    std::vector<std::unique_ptr<ApNode>> callee_body;
    callee_body.push_back(std::move(callee_node));

    // caller: CallStmt → "foo"
    auto call_node    = std::make_unique<CallNode>();
    call_node->callee = "foo";

    std::vector<std::unique_ptr<ApNode>> caller_nodes;
    caller_nodes.push_back(std::move(call_node));

    EventBuilder builder;
    builder.register_function("foo", std::move(callee_body));
    auto events = builder.build(std::move(caller_nodes));

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].object_name, "x");
    EXPECT_EQ(events[0].op,          "load");
}

TEST(EventBuilder, call_expansion_region_path_includes_call_site) {
    auto callee_node  = std::make_unique<ScalarNode>();
    callee_node->name = "y";
    callee_node->op   = "store";

    std::vector<std::unique_ptr<ApNode>> callee_body;
    callee_body.push_back(std::move(callee_node));

    auto call_node    = std::make_unique<CallNode>();
    call_node->callee = "bar";

    std::vector<std::unique_ptr<ApNode>> caller_nodes;
    caller_nodes.push_back(std::move(call_node));

    EventBuilder builder;
    builder.register_function("bar", std::move(callee_body));
    auto events = builder.build(std::move(caller_nodes));

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].region_path, "bar");
}

TEST(EventBuilder, loop_stack_records_active_iterations) {
    auto acc       = std::make_unique<ArrayNode>();
    acc->name      = "A";
    acc->op        = "load";
    acc->indices   = {"i", "j"};
    acc->shape     = {2, 3};
    acc->elem_size = 4;

    auto inner    = std::make_unique<LoopNode>();
    inner->var    = "j";
    inner->start  = 0;
    inner->bound  = 3;
    inner->depth  = 2;
    inner->body.push_back(std::move(acc));

    auto outer    = std::make_unique<LoopNode>();
    outer->var    = "i";
    outer->start  = 0;
    outer->bound  = 2;
    outer->depth  = 1;
    outer->body.push_back(std::move(inner));

    std::vector<std::unique_ptr<ApNode>> nodes;
    nodes.push_back(std::move(outer));
    auto events = build(std::move(nodes));

    // 첫 번째 이벤트: i=0, j=0
    ASSERT_EQ(events[0].loop_stack.size(), 2u);
    EXPECT_EQ(events[0].loop_stack[0].var,  "i");
    EXPECT_EQ(events[0].loop_stack[0].iter, 0);
    EXPECT_EQ(events[0].loop_stack[1].var,  "j");
    EXPECT_EQ(events[0].loop_stack[1].iter, 0);

    // 네 번째 이벤트: i=1, j=0
    ASSERT_EQ(events[3].loop_stack.size(), 2u);
    EXPECT_EQ(events[3].loop_stack[0].var,  "i");
    EXPECT_EQ(events[3].loop_stack[0].iter, 1);
    EXPECT_EQ(events[3].loop_stack[1].var,  "j");
    EXPECT_EQ(events[3].loop_stack[1].iter, 0);
}
