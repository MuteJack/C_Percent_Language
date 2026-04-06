// cpct/data/type/int_fixed.h
// Fixed-size integer types: Int8~Int64, UInt8~UInt64.
// Fast types: Int8f~Int32f, UInt8f~UInt32f, UInt — use platform.h for fixed mapping.
// Arithmetic: implicit conversion to native_type → C++ built-in operators.
// Only increment/decrement and compound assignment are class-level.
#pragma once
#include "../../platform.h"
#include <cstdint>
#include <string>
#include <iostream>
#include <limits>

namespace cpct {

// Macro for integer wrapper — no arithmetic operators, relies on implicit conversion.
#define CPCT_DEFINE_INT_CLASS(NAME, NATIVE, TYPE_STR)                          \
class NAME {                                                                   \
public:                                                                        \
    using native_type = NATIVE;                                                \
                                                                               \
    constexpr NAME() noexcept : val_(0) {}                                     \
    constexpr NAME(native_type v) noexcept : val_(v) {}                        \
                                                                               \
    constexpr operator native_type() const noexcept { return val_; }           \
                                                                               \
    std::string type() const { return TYPE_STR; }                              \
    std::size_t size() const { return sizeof(native_type); }                   \
    static constexpr native_type max() { return std::numeric_limits<native_type>::max(); } \
    static constexpr native_type min() { return std::numeric_limits<native_type>::min(); } \
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
    NAME& operator%=(native_type r) { val_ %= r; return *this; }             \
    NAME& operator&=(native_type r) noexcept { val_ &= r; return *this; }    \
    NAME& operator|=(native_type r) noexcept { val_ |= r; return *this; }    \
    NAME& operator^=(native_type r) noexcept { val_ ^= r; return *this; }    \
    NAME& operator<<=(native_type r) noexcept { val_ <<= r; return *this; }  \
    NAME& operator>>=(native_type r) noexcept { val_ >>= r; return *this; }  \
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

// UInt = platform-configured unsigned fast16
CPCT_DEFINE_INT_CLASS(UInt, uint_t, "uint")

// Fast types — use platform-configured fixed sizes
CPCT_DEFINE_INT_CLASS(Int8f,   fast8_t,   "int8f")
CPCT_DEFINE_INT_CLASS(Int16f,  fast16_t,  "int16f")
CPCT_DEFINE_INT_CLASS(Int32f,  fast32_t,  "int32f")
CPCT_DEFINE_INT_CLASS(UInt8f,  ufast8_t,  "uint8f")
CPCT_DEFINE_INT_CLASS(UInt16f, ufast16_t, "uint16f")
CPCT_DEFINE_INT_CLASS(UInt32f, ufast32_t, "uint32f")

#undef CPCT_DEFINE_INT_CLASS

} // namespace cpct
