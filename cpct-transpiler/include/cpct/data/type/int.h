// cpct/data/type/int.h
// cpct::Int — C%'s default `int` type.
// Maps to platform-configured fixed-size type (int64_t by default).
// Arithmetic uses implicit conversion to native_type → C++ built-in operators.
#pragma once
#include "../../platform.h"
#include <string>
#include <iostream>
#include <limits>

namespace cpct {

class Int {
public:
    using native_type = int_t; // from platform.h

    constexpr Int() noexcept : val_(0) {}
    constexpr Int(native_type v) noexcept : val_(v) {}

    // Implicit conversion to native type — enables built-in arithmetic/comparison
    constexpr operator native_type() const noexcept { return val_; }

    // C% built-in methods
    std::string type() const { return "int"; }
    std::size_t size() const { return sizeof(native_type); }
    static constexpr native_type max() { return std::numeric_limits<native_type>::max(); }
    static constexpr native_type min() { return std::numeric_limits<native_type>::min(); }

    // Increment/decrement (must be on class — built-in can't return Int)
    Int& operator++() noexcept { ++val_; return *this; }
    Int operator++(int) noexcept { Int tmp = *this; ++val_; return tmp; }
    Int& operator--() noexcept { --val_; return *this; }
    Int operator--(int) noexcept { Int tmp = *this; --val_; return tmp; }

    // Compound assignment (must be on class — modifies val_)
    Int& operator+=(native_type rhs) noexcept { val_ += rhs; return *this; }
    Int& operator-=(native_type rhs) noexcept { val_ -= rhs; return *this; }
    Int& operator*=(native_type rhs) noexcept { val_ *= rhs; return *this; }
    Int& operator/=(native_type rhs) { val_ /= rhs; return *this; }
    Int& operator%=(native_type rhs) { val_ %= rhs; return *this; }
    Int& operator&=(native_type rhs) noexcept { val_ &= rhs; return *this; }
    Int& operator|=(native_type rhs) noexcept { val_ |= rhs; return *this; }
    Int& operator^=(native_type rhs) noexcept { val_ ^= rhs; return *this; }
    Int& operator<<=(native_type rhs) noexcept { val_ <<= rhs; return *this; }
    Int& operator>>=(native_type rhs) noexcept { val_ >>= rhs; return *this; }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, Int v) {
        return os << v.val_;
    }

private:
    native_type val_;
};

} // namespace cpct
