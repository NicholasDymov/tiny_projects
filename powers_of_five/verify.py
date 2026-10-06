#!/usr/bin/env python

import sys


def get_64bit(n: int) -> tuple[int, int]:
    if n == 0:
        return 0, 0
    if n < 0:
        raise ValueError("n must be non-negative")

    bits = n.bit_length()

    if bits <= 64:
        shift = 64 - bits
        return n << shift, -shift

    shift = bits - 64
    significand = n >> shift
    exponent = shift

    round_bit = (n >> (shift - 1)) & 1
    sticky_bit = int((n & ((1 << (shift - 1)) - 1)) != 0)
    lsb = significand & 1

    if round_bit and (sticky_bit or lsb):
        significand += 1

    if significand == (1 << 64):
        significand >>= 1
        exponent += 1

    return significand, exponent


powers = []

for line in sys.stdin:
    x, y = line.split()
    powers.append((int(x, 16), int(y)))

pos, neg = powers[: len(powers) // 2], powers[len(powers) // 2 :]

x = 5

low, high = 0, 0
for i, power in enumerate(pos):
    expected = get_64bit(x)
    if power != expected:
        print(f"5^2^{i}")
        print(f"Expected: {expected=}")
        print(f"Got: {power=}")
        high = i - 1
        break
    x *= x

print(f"Table is precise with exponent in [2 ^ {low}, 2 ^ {high}]")
