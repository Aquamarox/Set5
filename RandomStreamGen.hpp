#pragma once
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>

class RandomStreamGen {
public:
    struct Params {
        size_t streamLength = 200000;
        size_t universeSize = 50000;
        size_t minLength = 1;
        size_t maxLength = 30;
        uint64_t seed = 1;
    };

    explicit RandomStreamGen(const Params& p) : params_(p), rng_(p.seed) {
        universe_.reserve(params_.universeSize);
        for (size_t i = 0; i < params_.universeSize; ++i) {
            universe_.push_back(generateRandomString());
        }
    }

    const std::vector<std::string>& universe() const { return universe_; }

    std::vector<uint32_t> generateStreamIndices() {
        std::vector<uint32_t> indices;
        indices.reserve(params_.streamLength);
        std::uniform_int_distribution<uint32_t> dist(0, static_cast<uint32_t>(params_.universeSize) - 1);
        for (size_t i = 0; i < params_.streamLength; ++i) {
            indices.push_back(dist(rng_));
        }
        return indices;
    }

    std::vector<std::string> materializeStream(const std::vector<uint32_t>& indices) const {
        std::vector<std::string> stream;
        stream.reserve(indices.size());
        for (uint32_t idx : indices) {
            stream.push_back(universe_[idx]);
        }
        return stream;
    }

    static std::vector<size_t> prefixLengths(size_t streamLength, size_t steps) {
        std::vector<size_t> lengths;
        if (steps == 0) return lengths;
        lengths.reserve(steps);
        for (size_t i = 1; i <= steps; ++i) {
            double frac = static_cast<double>(i) / static_cast<double>(steps);
            size_t len = static_cast<size_t>(std::lround(frac * static_cast<double>(streamLength) + 0.5));
            len = std::min(len, streamLength);
            if (!lengths.empty() && len <= lengths.back()) continue;
            lengths.push_back(len);
        }
        if (lengths.empty() || lengths.back() != streamLength) {
            lengths.push_back(streamLength);
        }
        return lengths;
    }

private:
    Params params_;
    std::mt19937_64 rng_;
    std::vector<std::string> universe_;

    std::string generateRandomString() {
        static constexpr char alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789-";
        const size_t alphabetSize = sizeof(alphabet) - 1;

        std::uniform_int_distribution<size_t> lenDist(params_.minLength, params_.maxLength);
        std::uniform_int_distribution<size_t> charDist(0, alphabetSize - 1);

        size_t len = lenDist(rng_);
        std::string s(len, ' ');
        for (char& c : s) {
            c = alphabet[charDist(rng_)];
        }
        return s;
    }
};