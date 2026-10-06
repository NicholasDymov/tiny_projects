#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <vector>

using uint128_t = unsigned __int128;

class FloatingPoint {
  private:
	uint64_t significand;
	int exponent;

  public:
	FloatingPoint(uint64_t s = 0, int e = 0) : significand(s), exponent(e) {}

	FloatingPoint operator*(const FloatingPoint &other) const {
		uint128_t product =
			static_cast<uint128_t>(significand) * other.significand;
		int shift = 63 + ((product >> 127) & 1);
		product += static_cast<uint128_t>(1) << (shift - 1);
		product >>= shift;
		return {static_cast<uint64_t>(product),
				exponent + other.exponent + shift};
	}

	FloatingPoint &operator*=(const FloatingPoint &other) {
		*this = *this * other;
		return *this;
	}

	uint64_t get_significand() const {
		return significand;
	}

	int get_exponent() const {
		return exponent;
	}
};

using fp = FloatingPoint;

std::vector<fp> build_table_positive(int n) {
	std::vector<fp> table(n + 1);
	table[0] = {5ULL << 61, -61};
	for (int i = 1; i <= n; i++) {
		table[i] = table[i - 1] * table[i - 1];
	}
	return table;
}

std::vector<fp> build_table_negative(int n) {
	std::vector<fp> table(n + 1);
	return table;
}

void print_table(std::vector<fp> table) {
	for (auto x : table) {
		std::cout << "0x" << std::hex << std::uppercase << x.get_significand()
				  << std::dec << ' ' << x.get_exponent() << '\n';
	}
}

int main() {
	auto powers_positive = build_table_positive(8);
	auto powers_negative = build_table_negative(8);
	print_table(powers_positive);
	print_table(powers_negative);
	return 0;
}
