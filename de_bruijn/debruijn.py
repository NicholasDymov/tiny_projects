#!/usr/bin/env python

from string import ascii_lowercase, digits


class DeBruijn:
    @staticmethod
    def build_graph(n, k):
        g = [[] for _ in range(k ** (n - 1))]

        mod = k ** (n - 2) if n >= 2 else 1
        for v in range(len(g)):
            for i in range(k):
                g[v].append(v % mod * k + i)

        return g

    @staticmethod
    def is_valid_seq(seq, n, k, alph=None):
        seq_len = k**n
        if isinstance(seq, str):
            if len(seq) != seq_len:
                raise ValueError(f"str sequence must be of length {seq_len}")
            if alph is None:
                raise ValueError("Alphabet cannot be None with str sequence")
            if len(alph) != k:
                raise ValueError("Alphabet must be of length k")
            x = 0
            for c in seq:
                try:
                    x = x * k + alph.index(c)
                except ValueError:
                    raise ValueError(f"{c} not in alphabet")
        elif isinstance(seq, int):
            x = seq
        else:
            raise TypeError(f"Sequence of type {type(seq)} unsupported")

        x = x * k ** (n - 1) + x // k ** (seq_len - n + 1)
        subs = set()
        for _ in range(seq_len):
            sub = x % seq_len
            if sub in subs:
                return False
            subs.add(sub)
            x //= k
        return True

    def seq_str(self, seq, padding):
        s = ""
        while seq > 0:
            s = self.alph[seq % self.k] + s
            seq //= self.k

        return f"{s:>0{padding}}"

    def __init__(self, n, k, alph=None):
        if alph:
            assert len(alph) == k, "Alphabet must be of length k"
            self.alph = alph
        else:
            assert k in range(2, 37), "Implicit alphabet is supported for 2 <= k <= 36"
            self.alph = (digits + ascii_lowercase)[: self.k]
        self.n = n
        self.k = k
        self.g = DeBruijn.build_graph(n, k)

    def __iter__(self):
        for seq in self._euler():
            assert self.is_valid_seq(
                seq, self.n, self.k
            ), f"Invalid: {self.seq_str(seq, self.k ** self.n)}"

            yield seq

    def _euler(self):
        yield 0

    def print_graph(self):
        for v in range(len(self.g)):
            adj = " ".join(f"{self.seq_str(u, self.n-1)}" for u in self.g[v])
            print(f"{self.seq_str(v, self.n-1)}: [{adj}]")


if __name__ == "__main__":
    db = DeBruijn(5, 2)
    db.print_graph()
