// cpct/data/math.h
// C% math functions — pow (**), divmod (/%).
// Uses if constexpr for zero-overhead integer/float dispatch at compile time.
#pragma once
#include <cmath>
#include <utility>
#include <type_traits>
#include <stdexcept>

namespace cpct {

namespace detail {
    // Detects whether T is an integer-like type (native or cpct wrapper with integral native_type).
    template<typename T, typename = void>
    struct is_integral_like : std::is_integral<T> {};

    template<typename T>
    struct is_integral_like<T, std::void_t<typename T::native_type>>
        : std::is_integral<typename T::native_type> {};

    template<typename T>
    inline constexpr bool is_integral_like_v = is_integral_like<T>::value;
}

// pow(base, exp) — C%'s ** operator.
// Integer path: loop-based exponentiation with overflow awareness.
// Float path: delegates to std::pow().
template<typename Base, typename Exp>
auto pow(Base base, Exp exp) {
    if constexpr (detail::is_integral_like_v<Base> && detail::is_integral_like_v<Exp>) {
        // Integer exponentiation
        using R = decltype(base * base); // result type
        if (exp < 0) {
            throw std::runtime_error("Negative exponent in integer pow");
        }
        R result = 1;
        R b = base;
        auto e = static_cast<int64_t>(exp);
        while (e > 0) {
            if (e & 1) result *= b;
            b *= b;
            e >>= 1;
        }
        return result;
    } else {
        // Float exponentiation
        return static_cast<Base>(std::pow(
            static_cast<double>(base),
            static_cast<double>(exp)
        ));
    }
}

// divmod(a, b) — C%'s /% operator.
// Returns {quotient, remainder} as std::pair.
// Integer path: uses native / and % operators.
// Float path: uses std::floor and std::fmod.
template<typename T>
std::pair<T, T> divmod(T a, T b) {
    if constexpr (detail::is_integral_like_v<T>) {
        if (b == 0) {
            throw std::runtime_error("Division by zero in divmod");
        }
        return { a / b, a % b };
    } else {
        if (static_cast<double>(b) == 0.0) {
            throw std::runtime_error("Division by zero in divmod");
        }
        double da = static_cast<double>(a);
        double db = static_cast<double>(b);
        return { static_cast<T>(std::floor(da / db)),
                 static_cast<T>(std::fmod(da, db)) };
    }
}

} // namespace cpct
