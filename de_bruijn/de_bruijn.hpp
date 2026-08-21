#pragma once

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace debruijn::detail {
constexpr uint64_t ipow(uint64_t base, uint64_t exp) {
	if (base == 0) {
		return (exp == 0) ? 1ULL : 0ULL;
	}

	constexpr uint64_t UINT_MAX = std::numeric_limits<uint64_t>::max();
	uint64_t res = 1;

	for (uint64_t i = 1; i <= exp; i <<= 1) {
		if (exp & i) {
			if (res > UINT_MAX / base) {
				throw std::overflow_error("ipow: integer overflow in result");
			}
			res *= base;
		}
		if ((i << 1) <= exp) {
			if (base > UINT_MAX / base) {
				throw std::overflow_error("ipow: integer overflow in base");
			}
			base *= base;
		}
	}
	return res;
}
} // namespace debruijn::detail

template <uint64_t K = 2, uint64_t N = 6> class DeBruijn {
	static_assert(N >= 2, "N must be >= 2");
	static_assert(K >= 2, "K must be >= 2");

  private:
	static constexpr uint64_t k_n =
		(K == 2) ? (1ULL << N) : debruijn::detail::ipow(K, N);

	static_assert(k_n <= 1024,
				  "Sequence length is too large, use algorithms to "
				  "generate a single sequence instead");

	static constexpr size_t k_n_1 = k_n / K;
	static constexpr size_t k_n_2 = k_n_1 / K;

	// Threshold for stack array allocation (4 KB)
	static constexpr size_t STACK_THRESHOLD = 4096;

	// Smallest mask type fitting K bits
	using mask_t =
		std::conditional_t<(K <= 8), uint8_t,
						   std::conditional_t<(K <= 32), uint32_t, uint64_t>>;

	// Smallest vertex index type fitting k_n_1 nodes
	using node_t = std::conditional_t<
		(k_n_1 <= 256), uint8_t,
		std::conditional_t<(k_n_1 <= 65536), uint16_t, uint32_t>>;

	// Search state for K = 2, N <= 6
	struct SearchStateBinSmall {
		uint64_t unused_edges;
		uint64_t seq;
	};

	// Search state for small stack-friendly parallel distribution
	struct SearchState {
		int path_len;
		mask_t unused[k_n_1];
		node_t path[k_n + 2];
	};

	// Alphabet string
	std::string alph;

	// Initialize alphabet
	void initAlphabet(std::string_view custom_alph) {
		if (!custom_alph.empty()) {
			if (custom_alph.size() != K) {
				throw std::invalid_argument(
					"Custom alphabet length (" +
					std::to_string(custom_alph.size()) +
					") must match K=" + std::to_string(K));
			}
			alph = custom_alph;
		} else {
			static constexpr std::string_view DEFAULT_ALPH =
				"0123456789abcdefghijklmnopqrstuvwxyz";
			if (K > static_cast<int>(DEFAULT_ALPH.size())) {
				throw std::invalid_argument(
					"Default alphabet supports K <= 36. Please provide a "
					"custom alphabet.");
			}
			alph = DEFAULT_ALPH.substr(0, K);
		}
	}

	// Reconstruct 64-bit integer sequence from path (for binary K=2, N<=6)
	inline uint64_t buildBinarySeq(const node_t *path) const {
		uint64_t seq = path[0] >> 1;
		for (size_t i = 0; i < k_n; ++i) {
			seq = (seq << 1) | (path[i] & 1);
		}
		return seq >> (N - 2);
	}

	// Reconstruct string sequence from path (for N > 6 or K > 2)
	inline std::string buildStringSeq(const node_t *path) const {
		std::string s;
		s.reserve(k_n);
		for (size_t i = 0; i < k_n; ++i) {
			s += alph[path[i] % K];
		}
		return s;
	}

	// Recursive DFS for Eulerian circuits
	template <typename YieldFunc>
	bool dfs(int path_len, mask_t *unused, node_t *path,
			 YieldFunc &yield) const {
		size_t v = path[path_len - 1];

		if (!unused[v]) {
			if (static_cast<size_t>(path_len - 1) == k_n) {
				if constexpr (K == 2 && k_n <= 64) {
					uint64_t seq = buildBinarySeq(path);
					if (!yield(seq))
						return false;
				} else {
					std::string seq = buildStringSeq(path);
					if (!yield(seq))
						return false;
				}
			}
			return true;
		}

		if constexpr (K == 2) {
			if (unused[v] & 0b01) {
				unused[v] ^= 0b01;
				path[path_len] = g0[v];
				if (!dfs(path_len + 1, unused, path, yield))
					return false;
				unused[v] ^= 0b01;
			}
			if (unused[v] & 0b10) {
				unused[v] ^= 0b10;
				path[path_len] = g1[v];
				if (!dfs(path_len + 1, unused, path, yield))
					return false;
				unused[v] ^= 0b10;
			}
		} else {
			for (int i = 0; i < K; ++i) {
				if (unused[v] & (mask_t(1) << i)) {
					unused[v] ^= (mask_t(1) << i);
					path[path_len] = static_cast<node_t>((v % k_n_2) * K + i);
					if (!dfs(path_len + 1, unused, path, yield))
						return false;
					unused[v] ^= (mask_t(1) << i);
				}
			}
		}

		return true;
	}

	// Collect initial subtree roots up to target_depth
	void collectRoots(int path_len, mask_t *unused, node_t *path,
					  int target_depth, std::vector<SearchState> &roots) const {
		if (path_len >= target_depth) {
			SearchState state;
			state.path_len = path_len;
			std::memcpy(state.unused, unused, sizeof(state.unused));
			std::memcpy(state.path, path, sizeof(state.path));
			roots.push_back(state);
			return;
		}

		size_t v = path[path_len - 1];
		if (!unused[v]) {
			SearchState state;
			state.path_len = path_len;
			std::memcpy(state.unused, unused, sizeof(state.unused));
			std::memcpy(state.path, path, sizeof(state.path));
			roots.push_back(state);
			return;
		}

		if constexpr (K == 2) {
			if (unused[v] & 0b01) {
				unused[v] ^= 0b01;
				path[path_len] = g0[v];
				collectRoots(path_len + 1, unused, path, target_depth, roots);
				unused[v] ^= 0b01;
			}
			if (unused[v] & 0b10) {
				unused[v] ^= 0b10;
				path[path_len] = g1[v];
				collectRoots(path_len + 1, unused, path, target_depth, roots);
				unused[v] ^= 0b10;
			}
		} else {
			for (int i = 0; i < K; ++i) {
				if (unused[v] & (mask_t(1) << i)) {
					unused[v] ^= (mask_t(1) << i);
					path[path_len] = static_cast<node_t>((v % k_n_2) * K + i);
					collectRoots(path_len + 1, unused, path, target_depth,
								 roots);
					unused[v] ^= (mask_t(1) << i);
				}
			}
		}
	}

  public:
	explicit DeBruijn(std::string_view custom_alph = "") {
		initGraph();
		initAlphabet(custom_alph);
	}

	static constexpr int getK() {
		return K;
	}
	static constexpr int getN() {
		return N;
	}
	static constexpr size_t getSequenceLength() {
		return k_n;
	}

	const std::string &getAlphabet() const {
		return alph;
	}

	DeBruijn &setAlphabet(std::string_view new_alph) {
		initAlphabet(new_alph);
		return *this;
	}

	// Binary sequence validation
	bool isValid(uint64_t seq) const {
		if constexpr (K == 2 && k_n <= 64) {
			uint64_t x = (seq << (N - 1)) | (seq >> (k_n - N + 1));
			uint64_t subs = 0;
			uint64_t mask = (1ULL << N) - 1ULL;

			for (size_t i = 0; i < k_n; ++i) {
				uint64_t sub = x & mask;
				if (subs & (1ULL << sub)) {
					return false;
				}
				subs |= (1ULL << sub);
				x >>= 1;
			}

			uint64_t full_mask = (k_n == 64) ? ~0ULL : ((1ULL << k_n) - 1ULL);
			return subs == full_mask;
		}
		return false;
	}

	// Smeared-mask multiplier check
	static inline bool isSmearedMultiplier(uint64_t seq) {
		uint64_t subs = 0;
		uint64_t m = 1ULL;

		for (int i = 0; i < 64; ++i) {
			uint64_t idx = (m * seq) >> 58;
			uint64_t sub = 1ULL << idx;
			if (subs & sub) {
				return false;
			}
			subs |= sub;
			m = (m << 1) | 1ULL;
		}
		return subs == ~0ULL;
	}

	// String formatting
	std::string toString(uint64_t seq,
						 std::string_view override_alph = "") const {
		std::string_view a =
			override_alph.empty() ? std::string_view(alph) : override_alph;
		if (static_cast<int>(a.size()) != K) {
			throw std::invalid_argument("Alphabet size must match K=" +
										std::to_string(K));
		}

		std::string s;
		s.reserve(k_n);
		for (int i = static_cast<int>(k_n) - 1; i >= 0; --i) {
			s += a[(seq >> i) & 1];
		}
		return s;
	}

	std::string toHexString(uint64_t seq) const {
		std::stringstream ss;
		int hex_digits = (static_cast<int>(k_n) + 3) / 4;
		ss << "0x" << std::hex << std::setw(hex_digits) << std::setfill('0')
		   << seq;
		return ss.str();
	}

	// Single-threaded generator using lambda callback
	template <typename YieldFunc> void forEach(YieldFunc &&yield) const {
		auto wrapper = [&](auto &&seq) -> bool {
			if constexpr (std::is_same_v<std::decay_t<decltype(yield(seq))>,
										 void>) {
				yield(seq);
				return true;
			} else {
				return yield(seq);
			}
		};

		mask_t full_mask =
			(K == 64) ? ~mask_t(0) : static_cast<mask_t>((mask_t(1) << K) - 1);

		if constexpr (k_n <= STACK_THRESHOLD) {
			mask_t unused[k_n_1];
			for (size_t v = 0; v < k_n_1; ++v)
				unused[v] = full_mask;
			unused[0] ^= 1; // edge 0->0 taken

			node_t path[k_n + 2];
			path[0] = 0;
			path[1] = 0;

			dfs(2, unused, path, wrapper);
		} else {
			std::vector<mask_t> unused(k_n_1, full_mask);
			unused[0] ^= 1;

			std::vector<node_t> path(k_n + 2, 0);

			dfs(2, unused.data(), path.data(), wrapper);
		}
	}

	// Multi-threaded parallel generator using OpenMP
	template <typename YieldFunc>
	void forEachParallel(YieldFunc &&yield, int split_depth = 8) const {
		auto wrapper = [&](auto &&seq) -> bool {
			if constexpr (std::is_same_v<std::decay_t<decltype(yield(seq))>,
										 void>) {
				yield(seq);
				return true;
			} else {
				return yield(seq);
			}
		};

		mask_t full_mask =
			(K == 64) ? ~mask_t(0) : static_cast<mask_t>((mask_t(1) << K) - 1);

		if constexpr (k_n <= STACK_THRESHOLD) {
			mask_t unused[k_n_1];
			for (size_t v = 0; v < k_n_1; ++v)
				unused[v] = full_mask;
			unused[0] ^= 1; // edge 0->0 taken

			node_t path[k_n + 2];
			path[0] = 0;
			path[1] = 0;

			std::vector<SearchState> roots;
			collectRoots(2, unused, path, split_depth, roots);

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
			for (size_t i = 0; i < roots.size(); ++i) {
				mask_t local_unused[k_n_1];
				node_t local_path[k_n + 2];
				std::memcpy(local_unused, roots[i].unused,
							sizeof(local_unused));
				std::memcpy(local_path, roots[i].path, sizeof(local_path));

				dfs(roots[i].path_len, local_unused, local_path, wrapper);
			}
#else
			for (size_t i = 0; i < roots.size(); ++i) {
				mask_t local_unused[k_n_1];
				node_t local_path[k_n + 2];
				std::memcpy(local_unused, roots[i].unused,
							sizeof(local_unused));
				std::memcpy(local_path, roots[i].path, sizeof(local_path));

				dfs(roots[i].path_len, local_unused, local_path, wrapper);
			}
#endif
		} else {
			// Safe fallback for large N
			forEach(yield);
		}
	}
};
