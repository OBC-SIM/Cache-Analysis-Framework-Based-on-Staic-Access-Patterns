#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

/**
 * @brief 객체들을 flat 주소 공간에 64-byte 정렬로 순서대로 배치한다.
 *
 * @note add_object 호출 순서가 배치 순서를 결정한다.
 */
class MemoryLayout {
public:
    /**
     * @brief 객체를 레이아웃에 추가하고 base address를 할당한다.
     *
     * @param[in] name        객체 이름
     * @param[in] total_bytes 객체 전체 크기 (bytes)
     */
    void add_object(const std::string& name, uint64_t total_bytes);

    /**
     * @brief 객체의 base address를 반환한다.
     *
     * @param[in] name 객체 이름
     * @return 할당된 base address
     * @throws std::runtime_error name이 등록되지 않은 경우
     */
    uint64_t base_of(const std::string& name) const;

private:
    static uint64_t align_up(uint64_t value, uint64_t alignment);

    std::unordered_map<std::string, uint64_t> bases_;
    uint64_t next_base_ = 0;
};
