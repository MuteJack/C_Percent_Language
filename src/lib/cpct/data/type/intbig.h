// cpct/data/type/intbig.h
// cpct::IntBig — C%'s `intbig` type.
// Uses int64_t for values in range, auto-promotes to BigInt on overflow.
// Auto-demotes back to int64_t when value fits again.
// Unlike fixed types, arithmetic operators are required here to detect overflow.
#pragma once
#include "overflow.h"
#include "bigint.h"
#include <string>
#include <iostream>
#include <variant>
#include <limits>

namespace cpct {

class IntBig {
public:
    using small_type = int64_t;

    // Constructors
    IntBig() : val_(int64_t(0)) {}
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    IntBig(T v) : val_(static_cast<int64_t>(v)) {}
    IntBig(const BigInt& b) {
        if (b.fitsInt64()) {
            val_ = b.toInt64();
        } else {
            // Try string-based demote for edge cases normalize() misses
            std::string s = b.toString();
            try {
                long long v = std::stoll(s);
                val_ = static_cast<int64_t>(v);
            } catch (...) {
                val_ = b;
            }
        }
    }

    // Check state
    bool isSmall() const { return std::holds_alternative<int64_t>(val_); }
    bool isBig() const { return std::holds_alternative<BigInt>(val_); }

    // Conversion to int64 (throws if BigInt and doesn't fit)
    int64_t toInt64() const {
        if (isSmall()) return std::get<int64_t>(val_);
        auto& b = std::get<BigInt>(val_);
        if (b.fitsInt64()) return b.toInt64();
        throw std::overflow_error("IntBig value too large for int64");
    }

    // Implicit conversion to int64 for interop (only if small)
    operator int64_t() const { return toInt64(); }

    double toDouble() const {
        if (isSmall()) return static_cast<double>(std::get<int64_t>(val_));
        return std::get<BigInt>(val_).toDouble();
    }

    // C% built-in methods
    std::string type() const { return "intbig"; }
    std::size_t size() const {
        if (isSmall()) return 8;
        return std::get<BigInt>(val_).byteCount();
    }

    // Get BigInt representation (promotes if needed)
    BigInt toBigInt() const {
        if (isSmall()) return BigInt(std::get<int64_t>(val_));
        return std::get<BigInt>(val_);
    }

    // Arithmetic — detects overflow, auto-promotes
    IntBig operator+(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) {
            int64_t a = std::get<int64_t>(val_);
            int64_t b = std::get<int64_t>(rhs.val_);
            int64_t result;
            if (!__builtin_add_overflow(a, b, &result)) {
                return IntBig(result);
            }
        }
        return IntBig(safeBigInt() + rhs.safeBigInt());
    }

    IntBig operator-(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) {
            int64_t a = std::get<int64_t>(val_);
            int64_t b = std::get<int64_t>(rhs.val_);
            int64_t result;
            if (!__builtin_sub_overflow(a, b, &result)) {
                return IntBig(result);
            }
        }
        return IntBig(safeBigInt() - rhs.safeBigInt());
    }

    IntBig operator*(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) {
            int64_t a = std::get<int64_t>(val_);
            int64_t b = std::get<int64_t>(rhs.val_);
            int64_t result;
            if (!__builtin_mul_overflow(a, b, &result)) {
                return IntBig(result);
            }
        }
        return IntBig(safeBigInt() * rhs.safeBigInt());
    }

    IntBig operator/(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) {
            int64_t a = std::get<int64_t>(val_);
            int64_t b = std::get<int64_t>(rhs.val_);
            if (b == 0) throw std::runtime_error("Division by zero");
            if (a == INT64_MIN && b == -1) {
                return IntBig(safeBigInt() / rhs.safeBigInt());
            }
            return IntBig(a / b);
        }
        return IntBig(safeBigInt() / rhs.safeBigInt());
    }

    IntBig operator%(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) {
            int64_t a = std::get<int64_t>(val_);
            int64_t b = std::get<int64_t>(rhs.val_);
            if (b == 0) throw std::runtime_error("Modulo by zero");
            return IntBig(a % b);
        }
        return IntBig(safeBigInt() % rhs.safeBigInt());
    }

    IntBig operator-() const {
        if (isSmall()) {
            int64_t v = std::get<int64_t>(val_);
            if (v == INT64_MIN) return IntBig(-safeBigInt());
            return IntBig(-v);
        }
        return IntBig(-std::get<BigInt>(val_));
    }

    // Compound assignment
    IntBig& operator+=(const IntBig& rhs) { *this = *this + rhs; return *this; }
    IntBig& operator-=(const IntBig& rhs) { *this = *this - rhs; return *this; }
    IntBig& operator*=(const IntBig& rhs) { *this = *this * rhs; return *this; }
    IntBig& operator/=(const IntBig& rhs) { *this = *this / rhs; return *this; }
    IntBig& operator%=(const IntBig& rhs) { *this = *this % rhs; return *this; }

    // Increment/decrement
    IntBig& operator++() { *this = *this + IntBig(1); return *this; }
    IntBig operator++(int) { IntBig tmp = *this; ++(*this); return tmp; }
    IntBig& operator--() { *this = *this - IntBig(1); return *this; }
    IntBig operator--(int) { IntBig tmp = *this; --(*this); return tmp; }

    // Comparison
    bool operator==(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) return std::get<int64_t>(val_) == std::get<int64_t>(rhs.val_);
        return toBigInt() == rhs.toBigInt();
    }
    bool operator!=(const IntBig& rhs) const { return !(*this == rhs); }
    bool operator<(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) return std::get<int64_t>(val_) < std::get<int64_t>(rhs.val_);
        return toBigInt() < rhs.toBigInt();
    }
    bool operator>(const IntBig& rhs) const { return rhs < *this; }
    bool operator<=(const IntBig& rhs) const { return !(rhs < *this); }
    bool operator>=(const IntBig& rhs) const { return !(*this < rhs); }

    // Bitwise
    IntBig operator&(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) return IntBig(std::get<int64_t>(val_) & std::get<int64_t>(rhs.val_));
        return IntBig(toBigInt() & rhs.toBigInt());
    }
    IntBig operator|(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) return IntBig(std::get<int64_t>(val_) | std::get<int64_t>(rhs.val_));
        return IntBig(toBigInt() | rhs.toBigInt());
    }
    IntBig operator^(const IntBig& rhs) const {
        if (isSmall() && rhs.isSmall()) return IntBig(std::get<int64_t>(val_) ^ std::get<int64_t>(rhs.val_));
        return IntBig(toBigInt() ^ rhs.toBigInt());
    }
    IntBig operator~() const {
        if (isSmall()) return IntBig(~std::get<int64_t>(val_));
        return IntBig(~toBigInt());
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const IntBig& v) {
        if (v.isSmall()) return os << std::get<int64_t>(v.val_);
        return os << std::get<BigInt>(v.val_).toString();
    }

    // String representation
    std::string toString() const {
        if (isSmall()) return std::to_string(std::get<int64_t>(val_));
        return std::get<BigInt>(val_).toString();
    }

private:
    std::variant<int64_t, BigInt> val_;

    // Safe BigInt conversion — uses string for INT64_MIN edge case
    // (BigInt::promote() has UB for -INT64_MIN)
    BigInt safeBigInt() const {
        if (isBig()) return std::get<BigInt>(val_);
        return BigInt(std::to_string(std::get<int64_t>(val_)));
    }
};

} // namespace cpct
