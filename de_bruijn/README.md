# De Bruijn Sequence Generator & Smeared-Mask Multiplier Search

A high-performance implementation and search engine for **De Bruijn sequences** and **De Bruijn multiplier constants** in Python and C++.

This project includes:
1. **Generic De Bruijn Generator (`debruijn.py`)**: An Eulerian circuit generator supporting arbitrary alphabet sizes $k \in [2, 36]$ and sequence lengths $n \ge 2$, with an optimized binary traversal engine and bitmask validator.
2. **High-Performance C++ Search Engine (`search.cpp`)**: A multi-threaded, native 64-bit C++ implementation searching all $2^{26} = 67,108,864$ binary De Bruijn sequences of order $n=6$ for **smeared-mask multipliers**.
3. **Python Search Script (`search.py`)**: A Python-based script for prototyping and testing sequence properties.

---

## 📌 Background & Motivation

### What is a De Bruijn Sequence?
A binary De Bruijn sequence $B(2, n)$ is a cyclic binary sequence of length $2^n$ in which every possible binary substring of length $n$ appears exactly once. 

The total number of distinct binary De Bruijn sequences of order $n$ (up to cyclic shift) is given by **de Bruijn's Theorem**:

$$\text{Count}(n) = 2^{2^{n-1} - n}$$

* **$n = 3$** (length 8): $2^{4-3} = 2^1 = \mathbf{2}$ sequences
* **$n = 4$** (length 16): $2^{8-4} = 2^4 = \mathbf{16}$ sequences
* **$n = 5$** (length 32): $2^{16-5} = 2^{11} = \mathbf{2,048}$ sequences
* **$n = 6$** (length 64): $2^{32-6} = 2^{26} = \mathbf{67,108,864}$ sequences

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
int msb = Table[(uint64_t)(v * 0x03f119d26af7285bULL) >> 58];
```

This project searches the entire 67.1-million sequence space of order $n=6$ to discover these exact multiplier constants.

---

## 🚀 Quick Start

### 1. Python Implementation (`debruijn.py`)

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
# DNA 4-letter alphabet (A, C, G, T), order n=3 (length 64)
dna_db = DeBruijn(n=3, k=4, alph="ACGT")
for seq in dna_db:
    print(dna_db.seq_str(seq, dna_db.k_n))
```

---

### 2. High-Performance C++ Search Engine (`search.cpp`)

#### Compilation
Compile with maximum optimizations and OpenMP multi-threading:

```bash
g++ -O3 -march=native -fopenmp search.cpp -o search
```

| Flag | Description |
| :--- | :--- |
| **`-O3`** | Enables function inlining, register allocation, and loop unrolling |
| **`-march=native`** | Enables modern 64-bit hardware bit-manipulation instructions (BMI2, AVX2) |
| **`-fopenmp`** | Enables multi-core parallelism across all CPU threads |

#### Running the Search
```bash
./search
```

**Example Output:**
```text
========================================================
 Smeared De Bruijn Multiplier Search (n=6, k=2)
 Total search space: 2^26 = 67,108,864 sequences
========================================================

[MATCH] 0000001111110001000110011101001001101010111101110010100001011011  0x03f119d26af7285b
[MATCH] 0000001111110001000110011101001001101010111101110010110110000101  0x03f119d26af72d85
[MATCH] 0000001111110001000110011101001001101011000010111101101110010101  0x03f119d26b0bdb95
...
========================================================
 Search Completed
 Total Sequences Searched: 67108864
 Total Matches Found:      388704
 Elapsed Time:             2.4510 seconds
 Throughput:               27380197 seq/sec
========================================================
```

---

## ⚡ Performance & Optimization Details

1. **Symmetric DFS Backtracking:** 
   Traverses the De Bruijn graph $B(2, n-1)$ by finding Eulerian circuits without dead-ends.
2. **Bytearray / 1D Lookup Acceleration:**
   Replaces 2D nested tuples with flat 1D tuples (`g0`, `g1`) and `bytearray` bitmasks for in-place mutation.
3. **Register-level Bitwise Verification:**
   The C++ engine keeps the entire DFS stack and sequence integers in CPU registers (`rax`, `rbx`), evaluating sequences in under 5 nanoseconds each.

---

## 📄 License
MIT License. Free for research, academic, and commercial use.
