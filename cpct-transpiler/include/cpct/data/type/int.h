// cpct/int.h
// cpct::Int — wraps int_fast16_t (C%'s default `int` type).
// Provides C%-compatible methods: type(), size().
// Implicitly converts to/from the underlying native type.
#pragma once
#include <cstdint>
#include <string>
#include <iostream>
#include <limits>

namespace cpct {

class Int {
public:
    using native_type = int_fast16_t;

    // Constructors
    constexpr Int() noexcept : val_(0) {}
    constexpr Int(native_type v) noexcept : val_(v) {}
    constexpr Int(int v) noexcept : val_(static_cast<native_type>(v)) {}
    constexpr Int(long v) noexcept : val_(static_cast<native_type>(v)) {}
    constexpr Int(long long v) noexcept : val_(static_cast<native_type>(v)) {}

    // Implicit conversion to native type
    constexpr operator native_type() const noexcept { return val_; }

    // C% built-in methods
    std::string type() const { return "int"; }
    std::size_t size() const { return sizeof(native_type); }
    static constexpr native_type max() { return std::numeric_limits<native_type>::max(); }
    static constexpr native_type min() { return std::numeric_limits<native_type>::min(); }

    // Arithmetic operators (return Int to keep wrapping)
    constexpr Int operator+(Int rhs) const noexcept { return Int(val_ + rhs.val_); }
    constexpr Int operator-(Int rhs) const noexcept { return Int(val_ - rhs.val_); }
    constexpr Int operator*(Int rhs) const noexcept { return Int(val_ * rhs.val_); }
    constexpr Int operator/(Int rhs) const { return Int(val_ / rhs.val_); }
    constexpr Int operator%(Int rhs) const { return Int(val_ % rhs.val_); }
    constexpr Int operator-() const noexcept { return Int(-val_); }

    // Compound assignment
    Int& operator+=(Int rhs) noexcept { val_ += rhs.val_; return *this; }
    Int& operator-=(Int rhs) noexcept { val_ -= rhs.val_; return *this; }
    Int& operator*=(Int rhs) noexcept { val_ *= rhs.val_; return *this; }
    Int& operator/=(Int rhs) { val_ /= rhs.val_; return *this; }
    Int& operator%=(Int rhs) { val_ %= rhs.val_; return *this; }

    // Increment/decrement
    Int& operator++() noexcept { ++val_; return *this; }
    Int operator++(int) noexcept { Int tmp = *this; ++val_; return tmp; }
    Int& operator--() noexcept { --val_; return *this; }
    Int operator--(int) noexcept { Int tmp = *this; --val_; return tmp; }

    // Comparison: handled by implicit conversion to native_type (no class-level operators needed)

    // Bitwise operators
    constexpr Int operator&(Int rhs) const noexcept { return Int(val_ & rhs.val_); }
    constexpr Int operator|(Int rhs) const noexcept { return Int(val_ | rhs.val_); }
    constexpr Int operator^(Int rhs) const noexcept { return Int(val_ ^ rhs.val_); }
    constexpr Int operator~() const noexcept { return Int(~val_); }
    constexpr Int operator<<(Int rhs) const noexcept { return Int(val_ << rhs.val_); }
    constexpr Int operator>>(Int rhs) const noexcept { return Int(val_ >> rhs.val_); }
    Int& operator&=(Int rhs) noexcept { val_ &= rhs.val_; return *this; }
    Int& operator|=(Int rhs) noexcept { val_ |= rhs.val_; return *this; }
    Int& operator^=(Int rhs) noexcept { val_ ^= rhs.val_; return *this; }
    Int& operator<<=(Int rhs) noexcept { val_ <<= rhs.val_; return *this; }
    Int& operator>>=(Int rhs) noexcept { val_ >>= rhs.val_; return *this; }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, Int v) {
        return os << v.val_;
    }

private:
    native_type val_;
};

} // namespace cpct
