// cpct/bool.h
// cpct::Bool — wraps bool with C%-compatible methods.
#pragma once
#include <string>
#include <iostream>

namespace cpct {

class Bool {
public:
    using native_type = bool;

    constexpr Bool() noexcept : val_(false) {}
    constexpr Bool(bool v) noexcept : val_(v) {}

    constexpr operator bool() const noexcept { return val_; }

    std::string type() const { return "bool"; }
    std::size_t size() const { return sizeof(bool); }
    static constexpr bool max() { return true; }
    static constexpr bool min() { return false; }

    // Comparison: handled by implicit conversion to bool
    constexpr Bool operator!() const noexcept { return Bool(!val_); }
    // && and || handled by implicit conversion to bool

    friend std::ostream& operator<<(std::ostream& os, Bool v) {
        return os << (v.val_ ? "true" : "false");
    }

private:
    bool val_;
};

} // namespace cpct
