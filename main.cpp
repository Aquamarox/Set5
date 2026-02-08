#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>
#include <iomanip>
#include <cmath>
#include "RandomStreamGen.hpp"
#include "HashFuncGen.hpp"
#include "HyperLogLog.hpp"
#include "HyperLogLogPacked.hpp"

int main() {
    size_t numStreams, streamLength, universeSize, steps;
    uint8_t p;
    uint64_t seed, hashSeed;
    bool usePacked;
    std::string runFilename, summaryFilename;

    std::cout << "Number of streams (e.g., 100): ";
    std::cin >> numStreams;
    std::cout << "Stream length (e.g., 1000000): ";
    std::cin >> streamLength;
    std::cout << "Universe size (e.g., 500000): ";
    std::cin >> universeSize;
    std::cout << "Checkpoints (e.g., 20 for 5% steps): ";
    std::cin >> steps;
    std::cout << "p (register bits, e.g., 12): ";
    std::cin >> p;
    std::cout << "Seed (e.g., 42): ";
    std::cin >> seed;
    std::cout << "Hash seed (e.g., 123): ";
    std::cin >> hashSeed;
    std::cout << "Use packed version? (0/1): ";
    std::cin >> usePacked;
    std::cout << "Run CSV filename (e.g., run_p12.csv): ";
    std::cin >> runFilename;
    std::cout << "Summary CSV filename (e.g., summary_p12.csv): ";
    std::cin >> summaryFilename;

    std::ofstream runFile(runFilename);
    runFile << "stream_id,checkpoint,processed,exact,estimate\n";

    std::vector<std::vector<double>> estimatesPerStep;
    std::vector<size_t> exactPerStep;
    auto prefixLens = RandomStreamGen::prefixLengths(streamLength, steps);
    estimatesPerStep.resize(prefixLens.size());
    exactPerStep.resize(prefixLens.size(), 0);

    for (size_t streamId = 0; streamId < numStreams; ++streamId) {
        RandomStreamGen::Params params;
        params.streamLength = streamLength;
        params.universeSize = universeSize;
        params.minLength = 1;
        params.maxLength = 30;
        params.seed = seed + streamId;
        RandomStreamGen gen(params);

        auto indices = gen.generateStreamIndices();
        HashFuncGen hashFunc(hashSeed);

        std::unique_ptr<HyperLogLog> hll;
        std::unique_ptr<HyperLogLogPacked> hllPacked;
        if (usePacked) {
            hllPacked = std::make_unique<HyperLogLogPacked>(p);
        } else {
            hll = std::make_unique<HyperLogLog>(p);
        }

        std::unordered_set<uint32_t> exactSet;
        size_t processed = 0;

        for (size_t step = 0; step < prefixLens.size(); ++step) {
            size_t target = prefixLens[step];
            while (processed < target) {
                uint32_t idx = indices[processed];
                exactSet.insert(idx);
                if (usePacked) {
                    hllPacked->add(gen.universe()[idx], hashFunc);
                } else {
                    hll->add(gen.universe()[idx], hashFunc);
                }
                ++processed;
            }

            double estimate = usePacked ? hllPacked->estimate() : hll->estimate();
            size_t exact = exactSet.size();

            runFile << streamId << "," << step << "," << processed << "," << exact << "," << estimate << "\n";
            if (streamId == 0) exactPerStep[step] = exact;
            estimatesPerStep[step].push_back(estimate);
        }
    }

    runFile.close();

    std::ofstream summaryFile(summaryFilename);
    summaryFile << "checkpoint,processed,exact_mean,estimate_mean,estimate_std\n";

    for (size_t step = 0; step < prefixLens.size(); ++step) {
        double sum = 0.0;
        for (double est : estimatesPerStep[step]) sum += est;
        double mean = sum / numStreams;

        double sqSum = 0.0;
        for (double est : estimatesPerStep[step]) {
            double diff = est - mean;
            sqSum += diff * diff;
        }
        double stdDev = numStreams > 1 ? std::sqrt(sqSum / (numStreams - 1)) : 0.0;

        summaryFile << step << "," << prefixLens[step] << "," << exactPerStep[step] << "," << mean << "," << stdDev << "\n";
    }

    summaryFile.close();

    std::cout << "\n✓ Experiment completed!\n";
    std::cout << "  Run data saved to: " << runFilename << "\n";
    std::cout << "  Summary saved to: " << summaryFilename << "\n";
    std::cout << "  Memory usage: "
              << (usePacked ? HyperLogLogPacked(p).memoryBytes() : HyperLogLog(p).memoryBytes())
              << " bytes\n";

    return 0;
}