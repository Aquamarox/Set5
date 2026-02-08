# SET5: HyperLogLog Implementation

Probabilistic algorithm for estimating the number of distinct elements in data streams.

## Implementation

- **HashFuncGen.hpp** — Polynomial hash + universal linear hashing (modulo prime P = 4294967311)
- **RandomStreamGen.hpp** — Stream generator with controlled universe size
- **HyperLogLog.hpp** — Standard HyperLogLog (8-bit registers)
- **HyperLogLogPacked.hpp** — Packed version (6-bit registers, 25% memory saving)
- **main.cpp** — Experiment runner with CSV output
- **plot.py** — Visualization script (Python 3 + pandas + matplotlib)

## Build & Run

```bash
# Сборка
cmake -S . -B build
cmake --build build

# Запуск эксперимента
./build/SET5
# Ввести параметры:
#   Number of streams: 100
#   Stream length: 1000000
#   Universe size: 500000
#   Checkpoints: 20
#   p: 12
#   Seed: 42
#   Hash seed: 123
#   Use packed: 0
#   Run CSV: run_p12.csv
#   Summary CSV: summary_p12.csv
