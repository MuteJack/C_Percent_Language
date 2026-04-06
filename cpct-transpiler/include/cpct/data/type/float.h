// cpct/float.h
// cpct::Float (double), cpct::Float32 (float) — C% floating-point types.
#pragma once
#include <string>
#include <iostream>
#include <type_traits>
#include <limits>

namespace cpct {

#define CPCT_DEFINE_FLOAT_CLASS(NAME, NATIVE, TYPE_STR)                        \
class NAME {                                                                   \
public:                                                                        \
    using native_type = NATIVE;                                                \
                                                                               \
    constexpr NAME() noexcept : val_(0.0) {}                                   \
    constexpr NAME(native_type v) noexcept : val_(v) {}                        \
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>> \
    constexpr NAME(T v) noexcept : val_(static_cast<native_type>(v)) {}        \
                                                                               \
    constexpr operator native_type() const noexcept { return val_; }           \
                                                                               \
    std::string type() const { return TYPE_STR; }                              \
    std::size_t size() const { return sizeof(native_type); }                   \
    static constexpr native_type max() { return std::numeric_limits<native_type>::max(); } \
    static constexpr native_type min() { return std::numeric_limits<native_type>::lowest(); } \
                                                                               \
    constexpr NAME operator+(NAME r) const noexcept { return NAME(val_ + r.val_); } \
    constexpr NAME operator-(NAME r) const noexcept { return NAME(val_ - r.val_); } \
    constexpr NAME operator*(NAME r) const noexcept { return NAME(val_ * r.val_); } \
    constexpr NAME operator/(NAME r) const { return NAME(val_ / r.val_); }    \
    constexpr NAME operator-() const noexcept { return NAME(-val_); }          \
                                                                               \
    NAME& operator+=(NAME r) noexcept { val_ += r.val_; return *this; }       \
    NAME& operator-=(NAME r) noexcept { val_ -= r.val_; return *this; }       \
    NAME& operator*=(NAME r) noexcept { val_ *= r.val_; return *this; }       \
    NAME& operator/=(NAME r) { val_ /= r.val_; return *this; }               \
                                                                               \
    NAME& operator++() noexcept { ++val_; return *this; }                      \
    NAME operator++(int) noexcept { NAME t = *this; ++val_; return t; }        \
    NAME& operator--() noexcept { --val_; return *this; }                      \
    NAME operator--(int) noexcept { NAME t = *this; --val_; return t; }        \
                                                                               \
    /* Comparison: handled by implicit conversion to native_type */             \
                                                                               \
    friend std::ostream& operator<<(std::ostream& os, NAME v) {               \
        return os << v.val_;                                                   \
    }                                                                          \
                                                                               \
private:                                                                       \
    native_type val_;                                                          \
};

// C% float = double (64-bit), float64 alias
CPCT_DEFINE_FLOAT_CLASS(Float,   double, "float")
CPCT_DEFINE_FLOAT_CLASS(Float32, float,  "float32")
CPCT_DEFINE_FLOAT_CLASS(Float64, double, "float64")

#undef CPCT_DEFINE_FLOAT_CLASS

} // namespace cpct
