// cpct/platform.h
// Platform configuration — maps fast types to fixed-size types.
// Define CPCT_PLATFORM before including this header to select a platform.
// Default: x64-linux (64-bit fast types).
//
// Usage:
//   #define CPCT_PLATFORM_AVR
//   #include <cpct/types.h>
//
// Or let the translator set it based on --platform flag.
#pragma once
#include <cstdint>

namespace cpct {

// Platform presets — each defines the fixed-size type for fast8/fast16/fast32.
// These replace int_fast*_t to avoid cross-platform ambiguity.

#if defined(CPCT_PLATFORM_AVR)
    // AVR (8-bit): fast = minimum size
    using fast8_t  = int8_t;
    using fast16_t = int16_t;
    using fast32_t = int32_t;
    using ufast8_t  = uint8_t;
    using ufast16_t = uint16_t;
    using ufast32_t = uint32_t;

#elif defined(CPCT_PLATFORM_X86) || defined(CPCT_PLATFORM_X64_WIN) || defined(CPCT_PLATFORM_ESP32)
    // x86 / x64-win / esp32: fast = 32-bit
    using fast8_t  = int8_t;
    using fast16_t = int32_t;
    using fast32_t = int32_t;
    using ufast8_t  = uint8_t;
    using ufast16_t = uint32_t;
    using ufast32_t = uint32_t;

#elif defined(CPCT_PLATFORM_ARM32) || defined(CPCT_PLATFORM_RISCV32)
    // ARM32 / RISC-V 32: fast = 32-bit (all)
    using fast8_t  = int32_t;
    using fast16_t = int32_t;
    using fast32_t = int32_t;
    using ufast8_t  = uint32_t;
    using ufast16_t = uint32_t;
    using ufast32_t = uint32_t;

#else
    // Default / x64-linux / arm64: fast = 64-bit
    using fast8_t  = int8_t;
    using fast16_t = int64_t;
    using fast32_t = int64_t;
    using ufast8_t  = uint8_t;
    using ufast16_t = uint64_t;
    using ufast32_t = uint64_t;

#endif

// int/uint = fast16 (C%'s default integer type)
using int_t  = fast16_t;
using uint_t = ufast16_t;

} // namespace cpct
