#include <gtest/gtest.h>
#include "memory/AddressMapper.hpp"

TEST(AddressMapper, 1d_index_0_maps_to_base) {
    uint64_t addr = AddressMapper::byte_address(128, {0}, {100}, 4);
    EXPECT_EQ(addr, 128u);
}

TEST(AddressMapper, 1d_index_k_maps_to_base_plus_k_times_elem_size) {
    uint64_t addr = AddressMapper::byte_address(0, {5}, {100}, 4);
    EXPECT_EQ(addr, 20u);  // 5 * 4
}

TEST(AddressMapper, 2d_row_major_i_j_correct) {
    // A[3][5], shape=[10,20] → base + (3*20 + 5) * 4 = 65 * 4 = 260
    uint64_t addr = AddressMapper::byte_address(0, {3, 5}, {10, 20}, 4);
    EXPECT_EQ(addr, 260u);
}

TEST(AddressMapper, cache_line_equals_address_div_line_size) {
    uint64_t line = AddressMapper::cache_line(128, 64);
    EXPECT_EQ(line, 2u);
}
