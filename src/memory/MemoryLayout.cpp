#include "memory/MemoryLayout.hpp"
#include <stdexcept>

uint64_t MemoryLayout::align_up(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

void MemoryLayout::add_object(const std::string& name, uint64_t total_bytes) {
    bases_[name] = next_base_;
    next_base_ = align_up(next_base_ + total_bytes, 64);
}

uint64_t MemoryLayout::base_of(const std::string& name) const {
    auto it = bases_.find(name);
    if (it == bases_.end())
        throw std::runtime_error("unknown object: " + name);
    return it->second;
}
