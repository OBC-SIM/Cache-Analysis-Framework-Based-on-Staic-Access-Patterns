#include "memory/AddressMapper.hpp"

uint64_t AddressMapper::byte_address(uint64_t base,
                                     const std::vector<int64_t>& indices,
                                     const std::vector<int64_t>& shape,
                                     uint64_t elem_size) {
    // Row-major: flat = sum of indices[i] * stride[i]
    // stride[last] = 1, stride[i] = shape[i+1] * ... * shape[last]
    int64_t flat = 0;
    int64_t stride = 1;
    for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
        flat += indices[i] * stride;
        if (i > 0) stride *= shape[i];
    }
    return base + static_cast<uint64_t>(flat) * elem_size;
}

uint64_t AddressMapper::cache_line(uint64_t byte_addr, uint64_t line_size) {
    return byte_addr / line_size;
}
