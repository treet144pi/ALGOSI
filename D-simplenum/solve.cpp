#include <iostream>
#include <vector>
#include <string>

using ull = unsigned long long;
using u128 = __uint128_t;

ull mul_mod(ull a, ull b, ull mod) {
    return (u128)a * b % mod;
}

ull pow_mod(ull a, ull e, ull mod) {
    ull res = 1;
    while (e > 0) {
        if (e & 1) {
            res = mul_mod(res, a, mod);
        }
        a = mul_mod(a, a, mod);
        e >>= 1;
    }
    return res;
}

bool check(ull n, ull a, ull d, int s) {
    ull x = pow_mod(a, d, n);

    if (x == 1 || x == n - 1) {
        return true;
    }

    for (int r = 1; r < s; ++r) {
        x = mul_mod(x, x, n);
        if (x == n - 1) {
            return true;
        }
    }

    return false;
}

bool isPrime(ull n) {
    if (n < 2) return false;

    for (ull p : {2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL, 23ULL}) {
        if (n % p == 0) return n == p;
    }

    ull d = n - 1;
    int s = 0;
    while ((d & 1) == 0) {
        d >>= 1;
        ++s;
    }

    for (ull a : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) {
        if (a % n == 0) continue;
        if (!check(n, a, d, s)) {
            return false;
        }
    }

    return true;
}

char to_hex(int x) {
    if (x < 10) return char('0' + x);
    return char('a' + x - 10);
}

int main() {
    int N;
    std::cin >> N;

    std::vector<int> ans((N + 3) / 4, 0);

    for (int i = 0; i < N; ++i) {
        std::string s;
        std::cin >> s;

        ull x = std::stoull(s, nullptr, 16);

        if (isPrime(x)) {
            ans[i / 4] |= (1 << (i % 4));
        }
    }

    int j = (int)ans.size() - 1;
    while (j > 0 && ans[j] == 0) {
        --j;
    }

    for (; j >= 0; --j) {
        std::cout << to_hex(ans[j]);
    }
    std::cout << '\n';

    return 0;
}
