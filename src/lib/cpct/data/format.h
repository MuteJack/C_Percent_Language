// cpct/data/format.h
// cpct::format() — string formatting for f-string transpilation.
// Uses std::format when available (C++20), falls back to to_string + append.
// Usage: cpct::format("hello ", name, ", age=", age)
#pragma once
#include <string>
#include <type_traits>

// Detect std::format availability
#if __has_include(<format>)
#include <format>
#if defined(__cpp_lib_format) && __cpp_lib_format >= 202110L
#define CPCT_HAS_STD_FORMAT 1
#endif
#endif

#ifndef CPCT_HAS_STD_FORMAT
#define CPCT_HAS_STD_FORMAT 0
#endif

namespace cpct {

namespace detail {
    // Detects cpct wrapper types (have native_type typedef)
    template<typename T, typename = void>
    struct has_native_type : std::false_type {};

    template<typename T>
    struct has_native_type<T, std::void_t<typename T::native_type>> : std::true_type {};

    // Detects types with .str() method (cpct::String)
    template<typename T, typename = void>
    struct has_str_method : std::false_type {};

    template<typename T>
    struct has_str_method<T, std::void_t<decltype(std::declval<T>().str())>> : std::true_type {};

    // Convert any value to std::string
    template<typename T>
    std::string stringify(const T& val) {
        // std::string — return as-is
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            return val;
        }
        // const char* / string literal
        else if constexpr (std::is_convertible_v<T, const char*>) {
            return std::string(val);
        }
        // bool (must be before is_arithmetic to avoid "0"/"1")
        else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
            return val ? "true" : "false";
        }
        // cpct::String (has .str())
        else if constexpr (has_str_method<T>::value) {
            return val.str();
        }
        // cpct wrapper types (have native_type)
        else if constexpr (has_native_type<T>::value) {
            using N = typename T::native_type;
            if constexpr (std::is_same_v<N, bool>) {
                return static_cast<bool>(val) ? "true" : "false";
            } else if constexpr (std::is_same_v<N, char>) {
                return std::string(1, static_cast<char>(val));
            } else {
                return std::to_string(static_cast<N>(val));
            }
        }
        // char
        else if constexpr (std::is_same_v<std::decay_t<T>, char>) {
            return std::string(1, val);
        }
        // Native arithmetic types
        else if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
            return std::to_string(val);
        }
        // Fallback
        else {
            return std::to_string(val);
        }
    }
}

// format(args...) — concatenate all arguments into a single string.
// This is the transpilation target for C% f-strings:
//   f"hello {name}, age={age}" → cpct::format("hello ", name, ", age=", age)
template<typename... Args>
std::string format(Args&&... args) {
    std::string result;
    (result.append(detail::stringify(std::forward<Args>(args))), ...);
    return result;
}

#if CPCT_HAS_STD_FORMAT
// format_spec(fmt, args...) — std::format wrapper for format specifiers.
// Used when f-string contains format specs like {:.2f}
//   f"pi = {pi:.2f}" → cpct::format_spec("pi = {:.2f}", pi)
template<typename... Args>
std::string format_spec(std::format_string<Args...> fmt, Args&&... args) {
    return std::format(fmt, std::forward<Args>(args)...);
}
#endif

} // namespace cpct
