#!/usr/bin/env python

from string import ascii_lowercase, digits


class DeBruijn:
    def __init__(self, n, k, alph=None):
        assert n >= 2 and k >= 2, "n and k must be >= 2"
        if alph:
            assert len(alph) == k, "Alphabet must be of length k"
            self.alph = alph
        else:
            assert k in range(2, 37), "Implicit alphabet is supported for 2 <= k <= 36"
            self.alph = (digits + ascii_lowercase)[:k]
        self.n = n
        self.k = k
        self.k_n_2 = k ** (n - 2)
        self.k_n_1 = self.k_n_2 * k
        self.k_n = self.k_n_1 * k
        self.wrap = self.k ** (self.k_n - self.n + 1)
        self.full_mask = (1 << self.k_n) - 1
        self.g = []
        self.alph_map = {c: i for i, c in enumerate(self.alph)}

    def __iter__(self):
        if not self.g:
            self.build_graph()
        for seq in self._euler_bin() if self.k == 2 else self._euler():
            assert self.is_valid_seq(seq), f"Invalid: {self.seq_str(seq, self.k_n)}"

            yield seq

    def is_valid_seq(self, seq):
        def _is_valid(x):
            x = x * self.k_n_1 + x // self.wrap
            subs = 0
            for _ in range(self.k_n):
                sub = x % self.k_n
                if (1 << sub) & subs:
                    return False
                subs ^= 1 << sub
                x //= self.k
            return subs == self.full_mask

        def _is_valid_bin(x):
            x = (x << (self.n - 1)) | (x >> (self.k_n - self.n + 1))
            subs = 0
            for _ in range(self.k_n):
                sub = x & (self.k_n - 1)
                if (1 << sub) & subs:
                    return False
                subs ^= 1 << sub
                x >>= 1
            return subs == self.full_mask

        if isinstance(seq, str):
            if len(seq) != self.k_n:
                return False
            x = 0
            for c in seq:
                try:
                    x = x * self.k + self.alph_map[c]
                except KeyError:
                    return False
        elif isinstance(seq, int):
            x = seq
        else:
            return False

        return _is_valid_bin(x) if self.k == 2 else _is_valid(x)

    def seq_str(self, seq, padding):
        if self.k == 2:
            return f"{seq:0{padding}b}"
        elif self.k == 8:
            return f"{seq:0{padding}o}"
        elif self.k == 16:
            return f"{seq:0{padding}x}"

        chars = []
        while seq > 0:
            chars.append(self.alph[seq % self.k])
            seq //= self.k

        return f"{''.join(reversed(chars)):>0{padding}}"

    def build_graph(self):
        self.g = tuple(
            tuple(v % self.k_n_2 * self.k + i for i in range(self.k))
            for v in range(self.k_n_1)
        )

    def print_graph(self):
        if not self.g:
            self.build_graph()
        for v in range(len(self.g)):
            adj = " ".join(f"{self.seq_str(u, self.n-1)}" for u in self.g[v])
            print(f"{self.seq_str(v, self.n-1)}: [{adj}]")

    def _euler(self):
        def _build_seq(path):
            seq = path[0] // self.k
            for v in path:
                seq = seq * self.k + v % self.k
            return seq // self.k_n_2

        def _dfs():
            v = path[-1]
            if not unused_edges[v]:
                path.pop()
                if len(path) == self.k_n:
                    yield _build_seq(path)
                return
            for i in range(self.k):
                if unused_edges[v] & (1 << i):
                    unused_edges[v] ^= 1 << i
                    path.append(self.g[v][i])
                    yield from _dfs()
                    unused_edges[v] ^= 1 << i
            path.pop()

        unused_edges = [(1 << self.k) - 1 for _ in range(self.k_n_1)]
        unused_edges[0] ^= 1
        path = [0, 0]

        yield from _dfs()

    def _euler_bin(self):
        def _build_seq(path):
            seq = path[0] >> 1
            for v in path:
                seq = (seq << 1) + (v & 1)
            return seq >> (self.n - 2)

        def _dfs():
            v = path[-1]
            if not unused_edges[v]:
                path.pop()
                if len(path) == self.k_n:
                    yield _build_seq(path)
                return
            if unused_edges[v] & 0b01:
                unused_edges[v] ^= 0b01
                path.append(g0[v])
                yield from _dfs()
                unused_edges[v] ^= 0b01
            if unused_edges[v] & 0b10:
                unused_edges[v] ^= 0b10
                path.append(g1[v])
                yield from _dfs()
                unused_edges[v] ^= 0b10
            path.pop()

        g0 = tuple(self.g[v][0] for v in range(self.k_n_1))
        g1 = tuple(self.g[v][1] for v in range(self.k_n_1))
        unused_edges = bytearray([0b11] * self.k_n_1)
        unused_edges[0] = 0b10
        path = [0, 0]

        yield from _dfs()


if __name__ == "__main__":
    db = DeBruijn(6, 2)
    cnt = 0
    for seq in db:
        cnt += 1
        if cnt >= 1_000_000:
            break
        # print(db.seq_str(seq, db.k_n), f"{seq:#010x}")
    print(cnt)
