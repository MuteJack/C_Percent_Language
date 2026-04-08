// cpct/data/cast.h
// C% type cast functions — int(x), float(x), string(x), bool(x), char(x), etc.
// Mirrors C%'s explicit type conversion: int(3.7) → 3, string(42) → "42", etc.
#pragma once
#include <string>
#include <cstdint>
#include <type_traits>
#include "type/int.h"
#include "type/int_fixed.h"
#include "type/float.h"
#include "type/string.h"
#include "type/bool.h"
#include "type/char.h"

namespace cpct {

namespace detail {
    // Extract native value from cpct wrapper or use as-is
    template<typename T>
    auto to_native(const T& val) {
        if constexpr (requires { typename T::native_type; }) {
            return static_cast<typename T::native_type>(val);
        } else {
            return val;
        }
    }

    // Convert any value to int64_t for integer casts
    template<typename T>
    int64_t to_int64(const T& val) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            return static_cast<int64_t>(std::stoll(val));
        } else if constexpr (std::is_same_v<std::decay_t<T>, String>) {
            return static_cast<int64_t>(std::stoll(val.str()));
        } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
            return val ? 1 : 0;
        } else {
            return static_cast<int64_t>(to_native(val));
        }
    }

    // Convert any value to double for float casts
    template<typename T>
    double to_double(const T& val) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            return std::stod(val);
        } else if constexpr (std::is_same_v<std::decay_t<T>, String>) {
            return std::stod(val.str());
        } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
            return val ? 1.0 : 0.0;
        } else {
            return static_cast<double>(to_native(val));
        }
    }

    // Convert any value to string
    template<typename T>
    std::string to_str(const T& val) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            return val;
        } else if constexpr (requires { val.str(); }) {
            return val.str();
        } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
            return val ? "true" : "false";
        } else if constexpr (std::is_same_v<std::decay_t<T>, char>) {
            return std::string(1, val);
        } else if constexpr (requires { typename T::native_type; }) {
            using N = typename T::native_type;
            if constexpr (std::is_same_v<N, bool>) {
                return static_cast<bool>(val) ? "true" : "false";
            } else if constexpr (std::is_same_v<N, char>) {
                return std::string(1, static_cast<char>(val));
            } else {
                return std::to_string(static_cast<N>(val));
            }
        } else {
            return std::to_string(val);
        }
    }

    // Convert any value to bool (C% truthy/falsy rules)
    template<typename T>
    bool to_bool(const T& val) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            return !val.empty(); // "" = false
        } else if constexpr (std::is_same_v<std::decay_t<T>, String>) {
            return !val.str().empty();
        } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
            return val;
        } else {
            return static_cast<bool>(to_native(val)); // 0 = false
        }
    }
}

// === int cast family ===

// int(x) → Int (int_fast16_t)
template<typename T>
Int to_int(const T& val) { return Int(static_cast<Int::native_type>(detail::to_int64(val))); }

// int8(x) → Int8
template<typename T>
Int8 to_int8(const T& val) { return Int8(static_cast<Int8::native_type>(detail::to_int64(val))); }

// int16(x) → Int16
template<typename T>
Int16 to_int16(const T& val) { return Int16(static_cast<Int16::native_type>(detail::to_int64(val))); }

// int32(x) → Int32
template<typename T>
Int32 to_int32(const T& val) { return Int32(static_cast<Int32::native_type>(detail::to_int64(val))); }

// int64(x) → Int64
template<typename T>
Int64 to_int64(const T& val) { return Int64(detail::to_int64(val)); }

// === uint cast family ===

// uint(x) → UInt
template<typename T>
UInt to_uint(const T& val) { return UInt(static_cast<UInt::native_type>(detail::to_int64(val))); }

// uint8(x) → UInt8
template<typename T>
UInt8 to_uint8(const T& val) { return UInt8(static_cast<UInt8::native_type>(detail::to_int64(val))); }

// uint16(x) → UInt16
template<typename T>
UInt16 to_uint16(const T& val) { return UInt16(static_cast<UInt16::native_type>(detail::to_int64(val))); }

// uint32(x) → UInt32
template<typename T>
UInt32 to_uint32(const T& val) { return UInt32(static_cast<UInt32::native_type>(detail::to_int64(val))); }

// uint64(x) → UInt64
template<typename T>
UInt64 to_uint64(const T& val) { return UInt64(static_cast<UInt64::native_type>(detail::to_int64(val))); }

// === float cast family ===

// float(x) → Float (double)
template<typename T>
Float to_float(const T& val) { return Float(detail::to_double(val)); }

// float32(x) → Float32
template<typename T>
Float32 to_float32(const T& val) { return Float32(static_cast<float>(detail::to_double(val))); }

// float64(x) → Float64
template<typename T>
Float64 to_float64(const T& val) { return Float64(detail::to_double(val)); }

// === string cast ===

// string(x) → String
template<typename T>
String to_string(const T& val) { return String(detail::to_str(val)); }

// === bool cast ===

// bool(x) → Bool (C% truthy/falsy)
template<typename T>
Bool to_bool(const T& val) { return Bool(detail::to_bool(val)); }

// === char cast ===

// char(x) → Char
template<typename T>
Char to_char(const T& val) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return Char(val.empty() ? '\0' : val[0]);
    } else if constexpr (std::is_same_v<std::decay_t<T>, String>) {
        return Char(val.str().empty() ? '\0' : val.str()[0]);
    } else {
        return Char(static_cast<char>(detail::to_int64(val)));
    }
}

} // namespace cpct
