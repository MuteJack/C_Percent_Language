// cpct/data/type/float.h
// cpct::Float (double), cpct::Float32 (float) — C% floating-point types.
// Arithmetic: implicit conversion to native_type → C++ built-in operators.
#pragma once
#include <string>
#include <iostream>
#include <limits>

namespace cpct {

#define CPCT_DEFINE_FLOAT_CLASS(NAME, NATIVE, TYPE_STR)                        \
class NAME {                                                                   \
public:                                                                        \
    using native_type = NATIVE;                                                \
                                                                               \
    constexpr NAME() noexcept : val_(0.0) {}                                   \
    constexpr NAME(native_type v) noexcept : val_(v) {}                        \
                                                                               \
    constexpr operator native_type() const noexcept { return val_; }           \
                                                                               \
    std::string type() const { return TYPE_STR; }                              \
    std::size_t size() const { return sizeof(native_type); }                   \
    static constexpr native_type max() { return std::numeric_limits<native_type>::max(); } \
    static constexpr native_type min() { return std::numeric_limits<native_type>::lowest(); } \
                                                                               \
    NAME& operator++() noexcept { ++val_; return *this; }                      \
    NAME operator++(int) noexcept { NAME t = *this; ++val_; return t; }        \
    NAME& operator--() noexcept { --val_; return *this; }                      \
    NAME operator--(int) noexcept { NAME t = *this; --val_; return t; }        \
                                                                               \
    NAME& operator+=(native_type r) noexcept { val_ += r; return *this; }     \
    NAME& operator-=(native_type r) noexcept { val_ -= r; return *this; }     \
    NAME& operator*=(native_type r) noexcept { val_ *= r; return *this; }     \
    NAME& operator/=(native_type r) { val_ /= r; return *this; }             \
                                                                               \
    friend std::ostream& operator<<(std::ostream& os, NAME v) {               \
        return os << v.val_;                                                   \
    }                                                                          \
                                                                               \
private:                                                                       \
    native_type val_;                                                          \
};

CPCT_DEFINE_FLOAT_CLASS(Float,   double, "float")
CPCT_DEFINE_FLOAT_CLASS(Float32, float,  "float32")
CPCT_DEFINE_FLOAT_CLASS(Float64, double, "float64")

#undef CPCT_DEFINE_FLOAT_CLASS

} // namespace cpct
