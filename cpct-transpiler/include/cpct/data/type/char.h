// cpct/char.h
// cpct::Char — wraps char with C%-compatible methods.
// In C%, char is a numeric type (like int8) that displays as a character.
#pragma once
#include <string>
#include <iostream>
#include <limits>

namespace cpct {

class Char {
public:
    using native_type = char;

    constexpr Char() noexcept : val_(0) {}
    constexpr Char(char v) noexcept : val_(v) {}

    constexpr operator char() const noexcept { return val_; }

    std::string type() const { return "char"; }
    std::size_t size() const { return 1; }
    static constexpr char max() { return std::numeric_limits<char>::max(); }
    static constexpr char min() { return std::numeric_limits<char>::min(); }

    // Increment/decrement
    Char& operator++() noexcept { ++val_; return *this; }
    Char operator++(int) noexcept { Char t = *this; ++val_; return t; }
    Char& operator--() noexcept { --val_; return *this; }
    Char operator--(int) noexcept { Char t = *this; --val_; return t; }

    // Compound assignment
    Char& operator+=(char rhs) noexcept { val_ += rhs; return *this; }
    Char& operator-=(char rhs) noexcept { val_ -= rhs; return *this; }

    // Comparison
    // Comparison: handled by implicit conversion to char

    // Stream: outputs as character
    friend std::ostream& operator<<(std::ostream& os, Char v) {
        return os << v.val_;
    }

private:
    char val_;
};

} // namespace cpct
