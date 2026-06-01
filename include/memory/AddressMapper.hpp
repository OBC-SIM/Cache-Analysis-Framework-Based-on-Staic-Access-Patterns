#pragma once
#include <cstdint>
#include <vector>

/**
 * @brief 배열 인덱스를 row-major byte address 및 cache line 번호로 변환한다.
 */
class AddressMapper {
public:
    /**
     * @brief N차원 row-major 인덱스를 byte address로 변환한다.
     *
     * @param[in] base      객체의 base address
     * @param[in] indices   각 차원의 인덱스 (length == shape.size())
     * @param[in] shape     각 차원의 크기
     * @param[in] elem_size 원소 하나의 크기 (bytes)
     * @return byte address
     * @pre indices.size() == shape.size()
     */
    static uint64_t byte_address(uint64_t base,
                                 const std::vector<int64_t>& indices,
                                 const std::vector<int64_t>& shape,
                                 uint64_t elem_size);

    /**
     * @brief byte address를 cache line 번호로 변환한다.
     *
     * @param[in] byte_addr byte address
     * @param[in] line_size cache line 크기 (bytes)
     * @return cache line 번호 (= byte_addr / line_size)
     */
    static uint64_t cache_line(uint64_t byte_addr, uint64_t line_size);
};
