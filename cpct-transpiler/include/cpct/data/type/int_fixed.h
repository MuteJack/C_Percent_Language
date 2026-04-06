// cpct/int_fixed.h
// Fixed-size integer types: Int8, Int16, Int32, Int64, UInt8~UInt64, UInt.
// Each wraps the corresponding C++ fixed-width type with C%-compatible methods.
#pragma once
#include <cstdint>
#include <string>
#include <iostream>
#include <limits>

namespace cpct {

// Macro to generate a fixed-size integer wrapper class.
// NAME: class name (e.g., Int8)
// NATIVE: underlying C++ type (e.g., int8_t)
// TYPE_STR: C% type name string (e.g., "int8")
#define CPCT_DEFINE_INT_CLASS(NAME, NATIVE, TYPE_STR)                          \
class NAME {                                                                   \
public:                                                                        \
    using native_type = NATIVE;                                                \
                                                                               \
    constexpr NAME() noexcept : val_(0) {}                                     \
    constexpr NAME(native_type v) noexcept : val_(v) {}                        \
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>> \
    constexpr NAME(T v) noexcept : val_(static_cast<native_type>(v)) {}        \
                                                                               \
    constexpr operator native_type() const noexcept { return val_; }           \
                                                                               \
    std::string type() const { return TYPE_STR; }                              \
    std::size_t size() const { return sizeof(native_type); }                   \
    static constexpr native_type max() { return std::numeric_limits<native_type>::max(); } \
    static constexpr native_type min() { return std::numeric_limits<native_type>::min(); } \
                                                                               \
    constexpr NAME operator+(NAME r) const noexcept { return NAME(val_ + r.val_); } \
    constexpr NAME operator-(NAME r) const noexcept { return NAME(val_ - r.val_); } \
    constexpr NAME operator*(NAME r) const noexcept { return NAME(val_ * r.val_); } \
    constexpr NAME operator/(NAME r) const { return NAME(val_ / r.val_); }    \
    constexpr NAME operator%(NAME r) const { return NAME(val_ % r.val_); }    \
    constexpr NAME operator-() const noexcept { return NAME(-val_); }          \
                                                                               \
    NAME& operator+=(NAME r) noexcept { val_ += r.val_; return *this; }       \
    NAME& operator-=(NAME r) noexcept { val_ -= r.val_; return *this; }       \
    NAME& operator*=(NAME r) noexcept { val_ *= r.val_; return *this; }       \
    NAME& operator/=(NAME r) { val_ /= r.val_; return *this; }               \
    NAME& operator%=(NAME r) { val_ %= r.val_; return *this; }               \
                                                                               \
    NAME& operator++() noexcept { ++val_; return *this; }                      \
    NAME operator++(int) noexcept { NAME t = *this; ++val_; return t; }        \
    NAME& operator--() noexcept { --val_; return *this; }                      \
    NAME operator--(int) noexcept { NAME t = *this; --val_; return t; }        \
                                                                               \
    /* Comparison: handled by implicit conversion to native_type */             \
                                                                               \
    constexpr NAME operator&(NAME r) const noexcept { return NAME(val_ & r.val_); } \
    constexpr NAME operator|(NAME r) const noexcept { return NAME(val_ | r.val_); } \
    constexpr NAME operator^(NAME r) const noexcept { return NAME(val_ ^ r.val_); } \
    constexpr NAME operator~() const noexcept { return NAME(~val_); }          \
    constexpr NAME operator<<(NAME r) const noexcept { return NAME(val_ << r.val_); } \
    constexpr NAME operator>>(NAME r) const noexcept { return NAME(val_ >> r.val_); } \
    NAME& operator&=(NAME r) noexcept { val_ &= r.val_; return *this; }       \
    NAME& operator|=(NAME r) noexcept { val_ |= r.val_; return *this; }       \
    NAME& operator^=(NAME r) noexcept { val_ ^= r.val_; return *this; }       \
    NAME& operator<<=(NAME r) noexcept { val_ <<= r.val_; return *this; }     \
    NAME& operator>>=(NAME r) noexcept { val_ >>= r.val_; return *this; }     \
                                                                               \
    friend std::ostream& operator<<(std::ostream& os, NAME v) {               \
        return os << +v.val_;                                                  \
    }                                                                          \
                                                                               \
private:                                                                       \
    native_type val_;                                                          \
};

// Signed fixed-size
CPCT_DEFINE_INT_CLASS(Int8,  int8_t,  "int8")
CPCT_DEFINE_INT_CLASS(Int16, int16_t, "int16")
CPCT_DEFINE_INT_CLASS(Int32, int32_t, "int32")
CPCT_DEFINE_INT_CLASS(Int64, int64_t, "int64")

// Unsigned fixed-size
CPCT_DEFINE_INT_CLASS(UInt8,  uint8_t,  "uint8")
CPCT_DEFINE_INT_CLASS(UInt16, uint16_t, "uint16")
CPCT_DEFINE_INT_CLASS(UInt32, uint32_t, "uint32")
CPCT_DEFINE_INT_CLASS(UInt64, uint64_t, "uint64")

// UInt = uint_fast16_t (alias for default unsigned)
CPCT_DEFINE_INT_CLASS(UInt, uint_fast16_t, "uint")

// Fast types
CPCT_DEFINE_INT_CLASS(Int8f,   int_fast8_t,   "int8f")
CPCT_DEFINE_INT_CLASS(Int16f,  int_fast16_t,  "int16f")
CPCT_DEFINE_INT_CLASS(Int32f,  int_fast32_t,  "int32f")
CPCT_DEFINE_INT_CLASS(UInt8f,  uint_fast8_t,  "uint8f")
CPCT_DEFINE_INT_CLASS(UInt16f, uint_fast16_t, "uint16f")
CPCT_DEFINE_INT_CLASS(UInt32f, uint_fast32_t, "uint32f")

#undef CPCT_DEFINE_INT_CLASS

} // namespace cpct
