#pragma once
#include <string_view>
#include <random>
#include <cstdint>

class HashFuncGen {
public:
  explicit HashFuncGen(uint64_t seed = 0x52ULL) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> dist(0, P - 1);
    a_ = dist(rng);
    b_ = dist(rng);
    if (a_ == 0) a_ = 1;
  }

  uint32_t operator()(std::string_view s) const {
    uint64_t x = polyHashModP(s);
    uint64_t y = (mulMod(a_, x) + b_) % P;
    return static_cast<uint32_t>(y);
  }

  static constexpr uint64_t primeModulus() { return P; }
  static constexpr uint64_t base() { return B; }

private:
  static constexpr uint64_t P = 4294967311ULL;
  static constexpr uint64_t B = 911382323ULL;
  uint64_t a_ = 1;
  uint64_t b_ = 0;

  static uint64_t polyHashModP(std::string_view s) {
    uint64_t x = 0;
    for (unsigned char ch : s) {
      x = (mulMod(x, B) + static_cast<uint64_t>(ch)) % P;
    }
    return x;
  }

  static uint64_t mulMod(uint64_t a, uint64_t b) {
#ifdef __SIZEOF_INT128__
    __uint128_t prod = static_cast<__uint128_t>(a) * static_cast<__uint128_t>(b);
    return static_cast<uint64_t>(prod % P);
#else
    uint64_t res = 0;
    a %= P;
    while (b) {
      if (b & 1) res = (res + a) % P;
      a = (a << 1) % P;
      b >>= 1;
    }
    return res;
#endif
  }
};