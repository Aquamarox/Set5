#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "HashFuncGen.hpp"

class HyperLogLogPacked {
public:
    explicit HyperLogLogPacked(uint8_t p) : p_(p), m_(1u << p) {
        size_t bits = 6ull * static_cast<size_t>(m_);
        size_t bytes = (bits + 7ull) / 8ull;
        data_.assign(bytes, 0);
    }

    void reset() {
        std::fill(data_.begin(), data_.end(), 0);
    }

    void addHash(uint32_t hashValue) {
        uint32_t index = hashValue >> (32 - p_);
        uint32_t w = (hashValue << p_) | (1u << (p_ - 1));
        uint32_t rho = countLeadingZeros(w) + 1;
        if (rho > 63) rho = 63;
        setRegisterMax(index, static_cast<uint8_t>(rho));
    }

    void add(const std::string& element, const HashFuncGen& hashFunc) {
        addHash(hashFunc(element));
    }

    double estimate() const {
        double inverseSum = 0.0;
        uint32_t zeroRegisters = 0;

        for (uint32_t i = 0; i < m_; ++i) {
            uint8_t reg = getRegister(i);
            inverseSum += std::ldexp(1.0, -static_cast<int>(reg));
            if (reg == 0) ++zeroRegisters;
        }

        double alpha = alphaM(m_);
        double estimateValue = alpha * static_cast<double>(m_) * static_cast<double>(m_) / inverseSum;

        if (estimateValue <= 2.5 * static_cast<double>(m_) && zeroRegisters > 0) {
            return static_cast<double>(m_) * std::log(static_cast<double>(m_) / static_cast<double>(zeroRegisters));
        }

        constexpr double two32 = 4294967296.0;
        if (estimateValue > (two32 / 30.0)) {
            double ratio = estimateValue / two32;
            if (ratio >= 1.0) ratio = 0.999999999;
            return -two32 * std::log(1.0 - ratio);
        }

        return estimateValue;
    }

    uint8_t p() const { return p_; }
    uint32_t m() const { return m_; }
    size_t memoryBytes() const { return data_.size(); }
    size_t standardMemoryBytes() const { return m_; }

private:
    uint8_t p_;
    uint32_t m_;
    std::vector<uint8_t> data_;

    static double alphaM(uint32_t m) {
        if (m == 16) return 0.673;
        if (m == 32) return 0.697;
        if (m == 64) return 0.709;
        return 0.7213 / (1.0 + 1.079 / static_cast<double>(m));
    }

    static uint32_t countLeadingZeros(uint32_t x) {
#ifdef __GNUC__
        return x == 0 ? 32 : __builtin_clz(x);
#else
        if (x == 0) return 32;
        uint32_t count = 0;
        while ((x & 0x80000000u) == 0) {
            count++;
            x <<= 1;
        }
        return count;
#endif
    }

    uint8_t getRegister(uint32_t index) const {
        uint64_t bitPos = 6ull * static_cast<uint64_t>(index);
        size_t bytePos = static_cast<size_t>(bitPos / 8ull);
        uint32_t shift = static_cast<uint32_t>(bitPos % 8ull);

        uint32_t window = data_[bytePos];
        if (bytePos + 1 < data_.size()) window |= static_cast<uint32_t>(data_[bytePos + 1]) << 8;
        if (bytePos + 2 < data_.size()) window |= static_cast<uint32_t>(data_[bytePos + 2]) << 16;

        return static_cast<uint8_t>((window >> shift) & 0x3Fu);
    }

    void setRegisterMax(uint32_t index, uint8_t value) {
        if (value > 63) value = 63;
        uint8_t current = getRegister(index);
        if (value <= current) return;

        uint64_t bitPos = 6ull * static_cast<uint64_t>(index);
        size_t bytePos = static_cast<size_t>(bitPos / 8ull);
        uint32_t shift = static_cast<uint32_t>(bitPos % 8ull);

        uint32_t window = data_[bytePos];
        if (bytePos + 1 < data_.size()) window |= static_cast<uint32_t>(data_[bytePos + 1]) << 8;
        if (bytePos + 2 < data_.size()) window |= static_cast<uint32_t>(data_[bytePos + 2]) << 16;

        window &= ~(0x3Fu << shift);
        window |= (static_cast<uint32_t>(value) & 0x3Fu) << shift;

        data_[bytePos] = static_cast<uint8_t>(window & 0xFFu);
        if (bytePos + 1 < data_.size()) data_[bytePos + 1] = static_cast<uint8_t>((window >> 8) & 0xFFu);
        if (bytePos + 2 < data_.size()) data_[bytePos + 2] = static_cast<uint8_t>((window >> 16) & 0xFFu);
    }
};