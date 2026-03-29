#include <iostream>
#include <vector>
#include <string>
#include <compare>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <iomanip>

class Integer {
private:
    static constexpr uint32_t BASE = 1000000000U;
    static constexpr uint32_t SHIFT_CHUNK = 29;

    bool neg = false;
    std::vector<uint32_t> digits; // little-endian, base 1e9

    bool is_zero() const {
        return digits.empty();
    }

    void normalize() {
        while (!digits.empty() && digits.back() == 0) {
            digits.pop_back();
        }
        if (digits.empty()) {
            neg = false;
        }
    }

    static int cmp_abs(const Integer& a, const Integer& b) {
        if (a.digits.size() != b.digits.size()) {
            return (a.digits.size() < b.digits.size() ? -1 : 1);
        }
        for (std::size_t i = a.digits.size(); i > 0; --i) {
            if (a.digits[i - 1] != b.digits[i - 1]) {
                return (a.digits[i - 1] < b.digits[i - 1] ? -1 : 1);
            }
        }
        return 0;
    }

    static Integer add_abs(const Integer& a, const Integer& b) {
        Integer res;
        std::size_t n = std::max(a.digits.size(), b.digits.size());
        res.digits.resize(n);

        uint64_t carry = 0;
        for (std::size_t i = 0; i < n; ++i) {
            uint64_t cur = carry;
            if (i < a.digits.size()) cur += a.digits[i];
            if (i < b.digits.size()) cur += b.digits[i];
            res.digits[i] = static_cast<uint32_t>(cur % BASE);
            carry = cur / BASE;
        }
        if (carry) {
            res.digits.push_back(static_cast<uint32_t>(carry));
        }
        return res;
    }

    static Integer sub_abs(const Integer& a, const Integer& b) {
        Integer res;
        res.digits.resize(a.digits.size());

        int64_t carry = 0;
        for (std::size_t i = 0; i < a.digits.size(); ++i) {
            int64_t cur = static_cast<int64_t>(a.digits[i]) - carry;
            if (i < b.digits.size()) cur -= static_cast<int64_t>(b.digits[i]);
            if (cur < 0) {
                cur += BASE;
                carry = 1;
            } else {
                carry = 0;
            }
            res.digits[i] = static_cast<uint32_t>(cur);
        }
        res.normalize();
        return res;
    }

    static Integer mul_abs(const Integer& a, const Integer& b) {
        Integer res;
        if (a.is_zero() || b.is_zero()) {
            return res;
        }

        res.digits.assign(a.digits.size() + b.digits.size(), 0);

        for (std::size_t i = 0; i < a.digits.size(); ++i) {
            uint64_t carry = 0;
            for (std::size_t j = 0; j < b.digits.size() || carry; ++j) {
                uint64_t cur = res.digits[i + j] + carry;
                if (j < b.digits.size()) {
                    cur += static_cast<uint64_t>(a.digits[i]) * b.digits[j];
                }
                res.digits[i + j] = static_cast<uint32_t>(cur % BASE);
                carry = cur / BASE;
            }
        }

        res.normalize();
        return res;
    }

    static Integer mul_uint32(const Integer& a, uint32_t m) {
        Integer res;
        if (a.is_zero() || m == 0) {
            return res;
        }

        res.digits.resize(a.digits.size());
        uint64_t carry = 0;

        for (std::size_t i = 0; i < a.digits.size(); ++i) {
            uint64_t cur = static_cast<uint64_t>(a.digits[i]) * m + carry;
            res.digits[i] = static_cast<uint32_t>(cur % BASE);
            carry = cur / BASE;
        }

        if (carry) {
            res.digits.push_back(static_cast<uint32_t>(carry));
        }
        return res;
    }

    void mul_uint32_inplace(uint32_t m) {
        if (is_zero() || m == 1) return;
        if (m == 0) {
            digits.clear();
            neg = false;
            return;
        }

        uint64_t carry = 0;
        for (std::size_t i = 0; i < digits.size(); ++i) {
            uint64_t cur = static_cast<uint64_t>(digits[i]) * m + carry;
            digits[i] = static_cast<uint32_t>(cur % BASE);
            carry = cur / BASE;
        }
        if (carry) {
            digits.push_back(static_cast<uint32_t>(carry));
        }
    }

    void add_uint32_inplace(uint32_t x) {
        uint64_t carry = x;
        std::size_t i = 0;
        while (carry) {
            if (i == digits.size()) {
                digits.push_back(0);
            }
            uint64_t cur = static_cast<uint64_t>(digits[i]) + carry;
            digits[i] = static_cast<uint32_t>(cur % BASE);
            carry = cur / BASE;
            ++i;
        }
    }

    uint32_t div_uint32_inplace(uint32_t v) {
        uint64_t rem = 0;
        for (std::size_t i = digits.size(); i > 0; --i) {
            uint64_t cur = rem * BASE + digits[i - 1];
            digits[i - 1] = static_cast<uint32_t>(cur / v);
            rem = cur % v;
        }
        normalize();
        return static_cast<uint32_t>(rem);
    }

    static void div_mod_abs(const Integer& a, const Integer& b, Integer& q, Integer& r) {
        if (b.is_zero()) {
            throw std::runtime_error("division by zero");
        }

        if (cmp_abs(a, b) < 0) {
            q = Integer(0);
            r = a;
            return;
        }

        q.digits.assign(a.digits.size(), 0);
        q.neg = false;
        r = Integer(0);

        for (std::size_t i = a.digits.size(); i > 0; --i) {
            r.digits.insert(r.digits.begin(), a.digits[i - 1]);
            r.normalize();

            uint32_t left = 0;
            uint32_t right = BASE - 1;
            uint32_t best = 0;

            while (left <= right) {
                uint32_t mid = left + (right - left) / 2;
                Integer t = mul_uint32(b, mid);
                int cmp = cmp_abs(t, r);
                if (cmp <= 0) {
                    best = mid;
                    left = mid + 1;
                } else {
                    if (mid == 0) break;
                    right = mid - 1;
                }
            }

            q.digits[i - 1] = best;
            if (best != 0) {
                r = sub_abs(r, mul_uint32(b, best));
            }
        }

        q.normalize();
        r.normalize();
    }

public:
    Integer() = default;

    template<
        class I,
        class = std::enable_if_t<
            std::is_integral_v<I> &&
            !std::is_same_v<std::remove_cv_t<I>, bool>
        >
    >
    Integer(I value) {
        using D = std::decay_t<I>;
        if constexpr (std::is_signed_v<D>) {
            int64_t x = static_cast<int64_t>(value);
            if (x < 0) {
                neg = true;
                uint64_t ux = static_cast<uint64_t>(-(x + 1)) + 1;
                while (ux > 0) {
                    digits.push_back(static_cast<uint32_t>(ux % BASE));
                    ux /= BASE;
                }
            } else {
                uint64_t ux = static_cast<uint64_t>(x);
                while (ux > 0) {
                    digits.push_back(static_cast<uint32_t>(ux % BASE));
                    ux /= BASE;
                }
            }
        } else {
            uint64_t ux = static_cast<uint64_t>(value);
            while (ux > 0) {
                digits.push_back(static_cast<uint32_t>(ux % BASE));
                ux /= BASE;
            }
        }
        normalize();
    }

    template<class T>
    bool fits() const {
        if constexpr (std::is_same_v<T, uint64_t>) {
            if (neg) return false;
            __uint128_t val = 0;
            for (std::size_t i = digits.size(); i > 0; --i) {
                val = val * BASE + digits[i - 1];
                if (val > std::numeric_limits<uint64_t>::max()) {
                    return false;
                }
            }
            return true;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            __uint128_t val = 0;
            for (std::size_t i = digits.size(); i > 0; --i) {
                val = val * BASE + digits[i - 1];
                if (!neg && val > static_cast<__uint128_t>(std::numeric_limits<int64_t>::max())) {
                    return false;
                }
                if (neg && val > (static_cast<__uint128_t>(std::numeric_limits<int64_t>::max()) + 1)) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    operator bool() const {
        return !is_zero();
    }

    explicit operator uint64_t() const {
        __uint128_t val = 0;
        for (std::size_t i = digits.size(); i > 0; --i) {
            val = val * BASE + digits[i - 1];
        }
        return static_cast<uint64_t>(val);
    }

    explicit operator int64_t() const {
        __uint128_t val = 0;
        for (std::size_t i = digits.size(); i > 0; --i) {
            val = val * BASE + digits[i - 1];
        }

        if (!neg) {
            return static_cast<int64_t>(static_cast<uint64_t>(val));
        }

        __uint128_t lim = static_cast<__uint128_t>(std::numeric_limits<int64_t>::max()) + 1;
        if (val == lim) {
            return std::numeric_limits<int64_t>::min();
        }
        return -static_cast<int64_t>(static_cast<uint64_t>(val));
    }

    Integer operator+() const {
        return *this;
    }

    Integer operator-() const {
        Integer res = *this;
        if (!res.is_zero()) {
            res.neg = !res.neg;
        }
        return res;
    }

    Integer& operator+=(const Integer& other) {
        Integer res;

        if (neg == other.neg) {
            res = add_abs(*this, other);
            res.neg = neg;
        } else {
            int cmp = cmp_abs(*this, other);
            if (cmp == 0) {
                res = Integer(0);
            } else if (cmp > 0) {
                res = sub_abs(*this, other);
                res.neg = neg;
            } else {
                res = sub_abs(other, *this);
                res.neg = other.neg;
            }
        }

        res.normalize();
        *this = res;
        return *this;
    }

    Integer& operator-=(const Integer& other) {
        return *this += (-other);
    }

    Integer& operator*=(const Integer& other) {
        bool sign = (neg != other.neg);
        Integer res = mul_abs(*this, other);
        if (!res.is_zero()) {
            res.neg = sign;
        }
        *this = res;
        return *this;
    }

    Integer& operator/=(const Integer& other) {
        if (other.is_zero()) {
            throw std::runtime_error("division by zero");
        }

        bool sign = (neg != other.neg);

        Integer a = *this;
        Integer b = other;
        a.neg = false;
        b.neg = false;

        Integer q, r;
        div_mod_abs(a, b, q, r);

        if (!q.is_zero()) {
            q.neg = sign;
        }
        *this = q;
        return *this;
    }

    Integer& operator%=(const Integer& other) {
        if (other.is_zero()) {
            throw std::runtime_error("division by zero");
        }

        bool sign = neg;

        Integer a = *this;
        Integer b = other;
        a.neg = false;
        b.neg = false;

        Integer q, r;
        div_mod_abs(a, b, q, r);

        if (!r.is_zero()) {
            r.neg = sign;
        }
        *this = r;
        return *this;
    }

    Integer& operator+=(int64_t x) { return *this += Integer(x); }
    Integer& operator-=(int64_t x) { return *this -= Integer(x); }
    Integer& operator*=(int64_t x) { return *this *= Integer(x); }
    Integer& operator/=(int64_t x) { return *this /= Integer(x); }
    Integer& operator%=(int64_t x) { return *this %= Integer(x); }

    Integer& operator+=(uint64_t x) { return *this += Integer(x); }
    Integer& operator-=(uint64_t x) { return *this -= Integer(x); }
    Integer& operator*=(uint64_t x) { return *this *= Integer(x); }
    Integer& operator/=(uint64_t x) { return *this /= Integer(x); }
    Integer& operator%=(uint64_t x) { return *this %= Integer(x); }

    Integer& operator<<=(uint64_t k) {
        while (k > 0) {
            uint32_t take = static_cast<uint32_t>(std::min<uint64_t>(k, SHIFT_CHUNK));
            mul_uint32_inplace(static_cast<uint32_t>(1U << take));
            k -= take;
        }
        return *this;
    }

    Integer& operator>>=(uint64_t k) {
        while (k > 0) {
            uint32_t take = static_cast<uint32_t>(std::min<uint64_t>(k, SHIFT_CHUNK));
            uint32_t divv = static_cast<uint32_t>(1U << take);

            if (!neg) {
                div_uint32_inplace(divv);
            } else if (!is_zero()) {
                uint32_t rem = div_uint32_inplace(divv);
                if (rem != 0) {
                    add_uint32_inplace(1);
                }
                neg = !is_zero();
            }

            k -= take;
        }
        normalize();
        return *this;
    }

    Integer operator<<(uint64_t k) const {
        Integer res = *this;
        res <<= k;
        return res;
    }

    Integer operator>>(uint64_t k) const {
        Integer res = *this;
        res >>= k;
        return res;
    }

    Integer& operator++() {
        *this += 1;
        return *this;
    }

    Integer& operator--() {
        *this -= 1;
        return *this;
    }

    Integer operator++(int) {
        Integer old = *this;
        ++(*this);
        return old;
    }

    Integer operator--(int) {
        Integer old = *this;
        --(*this);
        return old;
    }

    friend Integer operator+(Integer a, const Integer& b) {
        a += b;
        return a;
    }

    friend Integer operator-(Integer a, const Integer& b) {
        a -= b;
        return a;
    }

    friend Integer operator*(Integer a, const Integer& b) {
        a *= b;
        return a;
    }

    friend Integer operator/(Integer a, const Integer& b) {
        a /= b;
        return a;
    }

    friend Integer operator%(Integer a, const Integer& b) {
        a %= b;
        return a;
    }

    friend bool operator==(const Integer& a, const Integer& b) {
        return a.neg == b.neg && a.digits == b.digits;
    }

    friend std::strong_ordering operator<=>(const Integer& a, const Integer& b) {
        if (a.neg != b.neg) {
            return a.neg ? std::strong_ordering::less : std::strong_ordering::greater;
        }

        int cmp = cmp_abs(a, b);
        if (cmp == 0) return std::strong_ordering::equal;

        if (!a.neg) {
            return (cmp < 0 ? std::strong_ordering::less : std::strong_ordering::greater);
        } else {
            return (cmp < 0 ? std::strong_ordering::greater : std::strong_ordering::less);
        }
    }

    friend std::ostream& operator<<(std::ostream& out, const Integer& x) {
        if (x.is_zero()) {
            out << '0';
            return out;
        }

        if (x.neg) {
            out << '-';
        }

        out << x.digits.back();
        for (std::size_t i = x.digits.size() - 1; i > 0; --i) {
            out << std::setw(9) << std::setfill('0') << x.digits[i - 1];
        }
        return out;
    }

    friend std::istream& operator>>(std::istream& in, Integer& x) {
        std::string s;
        in >> s;
        if (!in) return in;

        x = Integer(0);

        std::size_t pos = 0;
        bool sign = false;

        if (!s.empty() && (s[0] == '+' || s[0] == '-')) {
            sign = (s[0] == '-');
            pos = 1;
        }

        if (pos == s.size()) {
            in.setstate(std::ios::failbit);
            return in;
        }

        x.digits.clear();
        x.neg = false;

        for (std::size_t i = s.size(); i > pos;) {
            std::size_t start = (i >= pos + 9 ? i - 9 : pos);
            uint32_t block = 0;
            for (std::size_t j = start; j < i; ++j) {
                if (s[j] < '0' || s[j] > '9') {
                    in.setstate(std::ios::failbit);
                    return in;
                }
                block = block * 10 + static_cast<uint32_t>(s[j] - '0');
            }
            x.digits.push_back(block);
            i = start;
        }

        x.neg = sign;
        x.normalize();
        return in;
    }
};
