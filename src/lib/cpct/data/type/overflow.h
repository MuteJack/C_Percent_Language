// cpct/data/type/overflow.h
// Portable overflow-checked arithmetic for MSVC (replaces GCC/Clang __builtin_*_overflow)
#pragma once
#include <cstdint>
#include <climits>

#if defined(_MSC_VER) && !defined(__clang__)
static inline bool __builtin_add_overflow(int64_t a, int64_t b, int64_t* result) {
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return true;
    *result = a + b;
    return false;
}
static inline bool __builtin_sub_overflow(int64_t a, int64_t b, int64_t* result) {
    if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) return true;
    *result = a - b;
    return false;
}
static inline bool __builtin_mul_overflow(int64_t a, int64_t b, int64_t* result) {
    if (a == 0 || b == 0) { *result = 0; return false; }
    if (a > 0) {
        if (b > 0) { if (a > INT64_MAX / b) return true; }
        else { if (b < INT64_MIN / a) return true; }
    } else {
        if (b > 0) { if (a < INT64_MIN / b) return true; }
        else { if (a != 0 && b < INT64_MAX / a) return true; }
    }
    *result = a * b;
    return false;
}
#endif
