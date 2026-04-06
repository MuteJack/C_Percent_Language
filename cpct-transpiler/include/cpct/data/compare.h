// cpct/data/compare.h
// Safe comparison functions — uses std::cmp_* for integers (signed/unsigned safe),
// falls back to native operators for non-integer types (float, string, etc.).
#pragma once
#include <utility>
#include <type_traits>

namespace cpct {

namespace detail {
    // Extract native value: if cpct wrapper, get native_type; otherwise use as-is
    template<typename T>
    auto native_val(const T& v) {
        if constexpr (requires { typename T::native_type; })
            return static_cast<typename T::native_type>(v);
        else
            return v;
    }

    // std::cmp_* only works with "standard integer types" — char/bool excluded
    template<typename T>
    constexpr bool is_std_integer() {
        using U = std::decay_t<T>;
        if constexpr (requires { typename U::native_type; }) {
            using N = typename U::native_type;
            return std::is_integral_v<N> && !std::is_same_v<N, bool> && !std::is_same_v<N, char>;
        } else {
            return std::is_integral_v<U> && !std::is_same_v<U, bool> && !std::is_same_v<U, char>;
        }
    }
}

template<typename A, typename B>
bool cmp_less(const A& a, const B& b) {
    if constexpr (detail::is_std_integer<A>() && detail::is_std_integer<B>())
        return std::cmp_less(detail::native_val(a), detail::native_val(b));
    else
        return a < b;
}

template<typename A, typename B>
bool cmp_greater(const A& a, const B& b) {
    if constexpr (detail::is_std_integer<A>() && detail::is_std_integer<B>())
        return std::cmp_greater(detail::native_val(a), detail::native_val(b));
    else
        return a > b;
}

template<typename A, typename B>
bool cmp_less_equal(const A& a, const B& b) {
    if constexpr (detail::is_std_integer<A>() && detail::is_std_integer<B>())
        return std::cmp_less_equal(detail::native_val(a), detail::native_val(b));
    else
        return a <= b;
}

template<typename A, typename B>
bool cmp_greater_equal(const A& a, const B& b) {
    if constexpr (detail::is_std_integer<A>() && detail::is_std_integer<B>())
        return std::cmp_greater_equal(detail::native_val(a), detail::native_val(b));
    else
        return a >= b;
}

template<typename A, typename B>
bool cmp_equal(const A& a, const B& b) {
    if constexpr (detail::is_std_integer<A>() && detail::is_std_integer<B>())
        return std::cmp_equal(detail::native_val(a), detail::native_val(b));
    else
        return a == b;
}

template<typename A, typename B>
bool cmp_not_equal(const A& a, const B& b) {
    if constexpr (detail::is_std_integer<A>() && detail::is_std_integer<B>())
        return std::cmp_not_equal(detail::native_val(a), detail::native_val(b));
    else
        return a != b;
}

} // namespace cpct
