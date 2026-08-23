#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

constexpr uint64_t K = 2;
constexpr uint64_t N = 6;
constexpr uint64_t K_N = 1ULL << N;
constexpr uint64_t K_N_1 = 1ULL << (N - 1);
constexpr uint64_t ROOT_DEPTH = 8;

std::atomic<bool> stop_search(false);

inline bool check(uint64_t seq) {
	constexpr uint64_t mask = (K_N < 64) ? (1ULL << K_N) - 1ULL : ~0ULL;
	uint64_t subs = 0;
	uint64_t m = 1ULL;

	for (uint64_t i = 0; i < K_N; i++) {
		uint64_t sub = 1ULL << (((m * seq) & mask) >> (K_N - N));
		if (subs & sub) {
			return false;
		}
		subs |= sub;
		m = (m << 1) | 1ULL;
	}
	return true;
}

inline bool is_valid(uint64_t seq) {
	uint64_t subs = 0;

	for (uint64_t i = 0; i < K_N; i++) {
		uint64_t sub = 1ULL << (seq & (K_N - 1));
		if (subs & sub) {
			return false;
		}
		subs |= sub;
		seq = (seq >> 1) | ((seq & 1ULL) << (K_N - 1));
	}
	return true;
}

inline void print_table(uint64_t seq) {
	constexpr uint64_t mask = (K_N < 64) ? (1ULL << K_N) - 1ULL : ~0ULL;
	uint64_t m = 1ULL;

	uint64_t table[K_N];
	for (uint64_t i = 0; i < K_N; i++) {
		uint64_t pos = ((m * seq) & mask) >> (K_N - N);
		table[pos] = i;
		m = (m << 1) | 1ULL;
	}
	std::cout << '{';
	for (auto x : table) {
		std::cout << x << ", ";
	}
	std::cout << '}' << std::endl;
}

template <uint64_t TargetDepth = K_N, typename CallbackType>
bool dfs(uint64_t path, uint64_t unused_edges, CallbackType &&callback) {
	static_assert(TargetDepth <= K_N, "Target depth must not exceed K ^ N");

	if (stop_search.load(std::memory_order_relaxed)) {
		return false;
	}

	if constexpr (TargetDepth == K_N) {
		if (!unused_edges) {
			return callback(path);
		}
	} else {
		if (std::popcount(unused_edges) == K_N - TargetDepth) {
			return callback(path, unused_edges);
		}
	}
	uint8_t v = path & (K_N_1 - 1);
	uint64_t edge0 = 1ULL << (v << 1);
	uint64_t edge1 = edge0 << 1;
	if (unused_edges & edge0) {
		if (!dfs<TargetDepth>(path << 1, unused_edges ^ edge0, callback))
			return false;
	}
	if (unused_edges & edge1) {
		if (!dfs<TargetDepth>(path << 1 | 1ULL, unused_edges ^ edge1, callback))
			return false;
	}
	return true;
}

int main() {

	std::cout << "===================================================\n";
	std::cout << " Smeared De Bruijn Multiplier Search (N=" << N << ", K=" << K
			  << ")\n";
	std::cout << " Total search space: 2^26 = 67,108,864 sequences\n";
#ifdef _OPENMP
	std::cout << " Running with OpenMP Multi-Threading ("
			  << omp_get_max_threads() << " threads)\n";
#else
	std::cout << " Running in Single-Threaded Mode\n";
#endif
	std::cout << "===================================================\n\n";

	std::atomic<uint64_t> total_searched(0);
	std::atomic<uint64_t> matches(0);
	std::vector<std::pair<uint64_t, uint64_t>> roots;
	uint64_t start = 1ULL;
	uint64_t unused_edges = ~0b11ULL;

	auto start_time = std::chrono::high_resolution_clock::now();

	dfs<ROOT_DEPTH>(start, unused_edges,
					[&](uint64_t path, uint64_t unused_edges) {
						roots.push_back({path, unused_edges});
						return true;
					});

#pragma omp parallel for schedule(dynamic)
	for (size_t i = 0; i < roots.size(); i++) {
		dfs(roots[i].first, roots[i].second, [&](uint64_t path) {
			total_searched.fetch_add(1, std::memory_order_relaxed);
			assert(is_valid(path));
			uint64_t seq = path >> (N - 1);
			if (check(seq)) {
#pragma omp critical
				{
					if (!stop_search.load(std::memory_order_relaxed)) {
						std::cout << "Found a match: 0x" << std::uppercase
								  << std::hex << std::setw(16)
								  << std::setfill('0') << seq << std::dec
								  << '\n';
						print_table(seq);
						stop_search.store(true, std::memory_order_relaxed);
					}
				}
				matches.fetch_add(1, std::memory_order_relaxed);
				return false;
			}
			return true;
		});
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end_time - start_time;

	std::cout << "\n===================================================\n";
	std::cout << " Search Completed\n";
	std::cout << " Total Sequences Searched: " << total_searched.load() << "\n";
	std::cout << " Total Matches Found:      " << matches.load() << "\n";
	std::cout << " Elapsed Time:             " << std::fixed
			  << std::setprecision(4) << elapsed.count() << " seconds\n";
	if (elapsed.count() > 0) {
		std::cout << " Throughput:               " << std::fixed
				  << std::setprecision(0)
				  << (total_searched.load() / elapsed.count()) << " seq/sec\n";
	}
	std::cout << "===================================================\n";

	return 0;
}
