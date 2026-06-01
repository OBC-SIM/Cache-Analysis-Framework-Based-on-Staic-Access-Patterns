#include <gtest/gtest.h>
#include "memory/MemoryLayout.hpp"

TEST(MemoryLayout, first_object_base_is_zero) {
    MemoryLayout layout;
    layout.add_object("A", 400);  // 100 * 4
    EXPECT_EQ(layout.base_of("A"), 0u);
}

TEST(MemoryLayout, second_object_starts_aligned_after_first) {
    MemoryLayout layout;
    layout.add_object("A", 400);  // 100 * 4 = 400 bytes
    layout.add_object("B", 400);
    // align_up(400, 64) = 448
    EXPECT_EQ(layout.base_of("B"), 448u);
}

TEST(MemoryLayout, scalar_occupies_element_size_bytes) {
    MemoryLayout layout;
    layout.add_object("x", 4);  // scalar: element_size = 4
    layout.add_object("y", 4);
    // align_up(4, 64) = 64
    EXPECT_EQ(layout.base_of("y"), 64u);
}

TEST(MemoryLayout, all_objects_64_byte_aligned) {
    MemoryLayout layout;
    layout.add_object("A", 100);
    layout.add_object("B", 300);
    layout.add_object("C", 500);
    EXPECT_EQ(layout.base_of("A") % 64, 0u);
    EXPECT_EQ(layout.base_of("B") % 64, 0u);
    EXPECT_EQ(layout.base_of("C") % 64, 0u);
}
