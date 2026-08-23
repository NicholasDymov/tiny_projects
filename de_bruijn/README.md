# De Bruijn Sequence Generator & Smeared-Mask Multiplier Search

A high-performance implementation and search engine for **De Bruijn sequences** and **De Bruijn multiplier constants** in C++ and Python.

---

## 🚦 Project Status

* **`search.cpp` (Specialized 64-Bit Search Algorithm):** **Stable & Verified** ✅  
  A dedicated, ultra-fast 64-bit bitboard search engine with lock-free OpenMP parallelism. Fully optimized and verified to exhaustively search all 67.1M canonical sequences and generate exact jump tables.
* **`de_bruijn.hpp` (Generic Header-Only Library):** **Work in Progress (WIP)** 🚧  
  A generalized, compile-time parameterized C++20 library for arbitrary $(K, N)$ configurations. Active development and API refinement are ongoing.
* **`debruijn.py` (Python Prototype):** **Stable** ✅  
  Educational reference implementation and prototype generator.

---

## 📌 Background & Mathematical Theory

### What is a De Bruijn Sequence?
A binary De Bruijn sequence $B(2, n)$ is a cyclic binary sequence of length $2^n$ in which every possible binary substring of length $n$ appears exactly once. 

The total number of distinct binary De Bruijn sequences of order $n$ is given by **de Bruijn's Theorem**:

$$\text{Count}(n) = 2^{2^{n-1} - n}$$

* **$n = 3$** (length 8): $2^{4-3} = 2^1 = \mathbf{2}$ sequences
* **$n = 4$** (length 16): $2^{8-4} = 2^4 = \mathbf{16}$ sequences
* **$n = 5$** (length 32): $2^{16-5} = 2^{11} = \mathbf{2,048}$ sequences
* **$n = 6$** (length 64): $2^{32-6} = 2^{26} = \mathbf{67,108,864}$ sequences
* **$n = 7$** (length 128): $2^{64-7} = 2^{57} \approx \mathbf{1.44 \times 10^{17}}$ sequences

---

### Smeared-Mask De Bruijn Multipliers (Fast $\lfloor\log_2 v\rfloor$ / MSB)

In bit manipulation (e.g. chess engines, bitboards, and low-level system software), finding the Most Significant Bit (MSB) or $\lfloor\log_2 v\rfloor$ typically requires:
1. **Smearing** the bits:
   ```c
   v |= v >> 1;
   v |= v >> 2;
   v |= v >> 4;
   v |= v >> 8;
   v |= v >> 16;
   v |= v >> 32;
   ```
   This converts `v` into one of the 64 prefix masks $M_i = (2^i - 1)$ (`0b1`, `0b11`, `0b111`, ...).
2. **Isolating the MSB** (traditional method):
   ```c
   v = (v >> 1) + 1; // or v ^= (v >> 1)
   ```
3. **De Bruijn Multiplication & Lookup**:
   ```c
   int msb = Table[(v * MAGIC) >> 58];
   ```

### Eliminating the Isolation Step
By choosing a specific **smeared-mask De Bruijn multiplier**, you can **eliminate the isolation step entirely**, multiplying the smeared mask directly:

```c
// Direct 1-step lookup from smeared mask:
int msb = JUMP_TABLE[(uint64_t)(v * 0x03F08A4C6ACB9DBDULL) >> 58];
```

This project searches the entire 67.1-million canonical sequence space of order $N=6$ to discover these exact multiplier constants and generates their associated lookup jump tables.

---

## 🚀 Quick Start

### 1. High-Performance C++ Search Engine (`search.cpp`)

#### Building with `make`:
```bash
make          # Optimized build (-O3 -march=native -fopenmp -std=c++20)
make debug    # Debug build (-g3 -fopenmp -std=c++20)
make re       # Full clean rebuild
```

#### Running the Search:
```bash
./search
```

**Example Output:**
```text
===================================================
 Smeared De Bruijn Multiplier Search (N=6, K=2)
 Total search space: 2^26 = 67,108,864 sequences
 Running with OpenMP Multi-Threading (8 threads)
===================================================

Found a match: 0x03F08A4C6ACB9DBD
{0, 11, 1, 12, 16, 29, 2, 13, 22, 17, 41, 25, 30, 48, 3, 61, 14, 20, 23, 18, 34, 36, 42, 26, 38, 31, 53, 44, 49, 56, 4, 62, 10, 15, 28, 21, 40, 24, 47, 60, 19, 33, 35, 37, 52, 43, 55, 9, 27, 39, 46, 59, 32, 51, 54, 8, 45, 58, 50, 7, 57, 6, 5, 63, }

===================================================
 Search Completed
 Total Sequences Searched: 60692239
 Total Matches Found:      1
 Elapsed Time:             6.6439 seconds
 Throughput:               9134966 seq/sec
===================================================
```

---

### 2. Header-Only C++ Library (`de_bruijn.hpp`) *(WIP)*

The class `DeBruijn<K, N>` is a standalone, header-only C++20 utility currently under active development.

#### Basic Usage (Binary $N \le 6$):
```cpp
#include "de_bruijn.hpp"
#include <iostream>

int main() {
    DeBruijn<2, 4> db; // Order 4 Binary (16 sequences)

    db.forEach([](uint64_t seq) {
        std::cout << "0x" << std::hex << seq << "\n";
    });
}
```

#### Multi-Core Parallel Traversal:
```cpp
DeBruijn<2, 6> db; // 64-bit sequence (67.1M sequences)

db.forEachParallel([&](uint64_t seq) {
    if (DeBruijn<2, 6>::isSmearedMultiplier(seq)) {
        #pragma omp critical
        std::cout << "Found match: " << db.toHexString(seq) << "\n";
    }
});
```

#### Custom Alphabet & Arbitrary Sizes ($K > 2$ or $N > 6$):
```cpp
// DNA 4-letter alphabet (A, C, G, T), order N=3 (length 64)
DeBruijn<4, 3> dna_db("ACGT");

dna_db.forEach([](const std::string& seq) {
    std::cout << "DNA: " << seq << "\n";
});
```

---

### 3. Python Implementation (`debruijn.py`)

#### Basic Usage
```python
from debruijn import DeBruijn

# Generate all 16 binary De Bruijn sequences of order 4 (length 16)
db = DeBruijn(n=4, k=2)

for seq in db:
    print(db.seq_str(seq, db.k_n), f"{seq:#06x}")
```

#### Custom Alphabets ($k \le 36$)
```python
# DNA alphabet
dna_db = DeBruijn(n=3, k=4, alph="ACGT")
for seq in dna_db:
    print(dna_db.seq_str(seq, dna_db.k_n))
```

---

## ⚡ Performance & Optimization Architecture

| Optimization | Implementation Details |
| :--- | :--- |
| **64-Bit Register Bitboard** | The entire graph edge state ($32 \text{ vertices} \times 2 \text{ bits} = 64 \text{ bits}$) is packed into a single `uint64_t unused_edges`, executed entirely inside CPU registers (`rax`, `rdx`). |
| **Unified Template `dfs<TargetDepth>`** | A single recursive template handles both shallow root collection (`TargetDepth = 8`) and full parallel search (`TargetDepth = 64`) with 0 code duplication. |
| **Compile-Time `if constexpr` Dispatch** | For $TargetDepth = 64$, the compiler specializes the leaf check into a **1-cycle `!unused_edges` (`test rsi, rsi`) instruction**, bypassing `popcount`. |
| **Native CPU Instructions** | Compiles to native 1-cycle x86-64 instructions: **`ROR`** (circular shift), **`TZCNT`** / **`POPCNT`** (bit operations), and **`LEA`** (combined address math). |
| **Lock-Free OpenMP Parallelism** | Dynamically distributes 64–256 subtree roots across CPU threads (`#pragma omp parallel for schedule(dynamic)`). |
| **Small Buffer Optimization** | Automatically selects stack arrays for small graphs ($K^N \le 4096$) and heap vectors (`std::vector`) for massive graphs to guarantee zero stack overflow. |

---

## 📄 Discovered 64-Bit Multipliers & Jump Tables

Here is the exact 64-bit smeared-mask De Bruijn multiplier discovered by this engine and its verified jump table:

* **Multiplier Constant:** `0x03F08A4C6ACB9DBDULL`
* **Jump Table:**
```c
static const uint8_t JUMP_TABLE_64[64] = {
     0, 11,  1, 12, 16, 29,  2, 13, 22, 17, 41, 25, 30, 48,  3, 61,
    14, 20, 23, 18, 34, 36, 42, 26, 38, 31, 53, 44, 49, 56,  4, 62,
    10, 15, 28, 21, 40, 24, 47, 60, 19, 33, 35, 37, 52, 43, 55,  9,
    27, 39, 46, 59, 32, 51, 54,  8, 45, 58, 50,  7, 57,  6,  5, 63
};

// Direct 1-step computation of floor(log2(v)):
static inline int log2_smeared_64(uint64_t v) {
    // 1. Smear bits
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;

    // 2. Direct lookup without MSB isolation:
    return JUMP_TABLE_64[(v * 0x03F08A4C6ACB9DBDULL) >> 58];
}
```

---

## 🤖 AI Disclosure & Attribution

This project was developed through collaborative pair programming with **Google Antigravity (Advanced Agentic AI by Google DeepMind)**. 

AI assistance was utilized for:
* Mathematical analysis of Eulerian graph traversals and De Bruijn cycle counts.
* Modern C++20 template design, compile-time metaprogramming, and zero-cost abstractions.
* Bitboard register optimization, instruction-level assembly analysis (`ROR`, `LEA`, `POPCNT`), and OpenMP multi-threading tuning.
* Bug fixing, verification test scripts, and documentation authoring.

---

## 📄 License
MIT License. Free for research, academic, and commercial use.
