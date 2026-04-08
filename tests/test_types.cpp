// Test: cpct type library compilation and basic functionality
#include "include/cpct/types.h"
#include <iostream>
#include <cassert>

int main() {
    // === Int ===
    cpct::Int x = 42;
    std::cout << "Int: " << x << " type=" << x.type() << " size=" << x.size() << std::endl;
    x += 8;
    assert(x == 50);
    x++;
    assert(x == 51);

    // === Int32 ===
    cpct::Int32 y = 100;
    std::cout << "Int32: " << y << " type=" << y.type() << " size=" << y.size() << std::endl;

    // === Int8 ===
    cpct::Int8 small = 127;
    std::cout << "Int8: " << small << " type=" << small.type() << " size=" << small.size() << std::endl;

    // === UInt64 ===
    cpct::UInt64 big = 999999999;
    std::cout << "UInt64: " << big << " type=" << big.type() << " size=" << big.size() << std::endl;

    // === Fast types ===
    cpct::Int8f  f8  = 10;
    cpct::Int16f f16 = 1000;
    cpct::Int32f f32 = 100000;
    std::cout << "Int8f: " << f8 << " size=" << f8.size() << std::endl;
    std::cout << "Int16f: " << f16 << " size=" << f16.size() << std::endl;
    std::cout << "Int32f: " << f32 << " size=" << f32.size() << std::endl;

    // === Float ===
    cpct::Float pi = 3.14159;
    std::cout << "Float: " << pi << " type=" << pi.type() << " size=" << pi.size() << std::endl;
    cpct::Float32 f = 2.5f;
    std::cout << "Float32: " << f << " type=" << f.type() << " size=" << f.size() << std::endl;

    // === String ===
    cpct::String s = "hello";
    std::cout << "String: " << s << " type=" << s.type() << " len=" << s.len() << std::endl;
    std::cout << "upper: " << s.upper() << std::endl;
    std::cout << "lower: " << s.upper().lower() << std::endl;
    std::cout << "reverse: " << s.reverse() << std::endl;
    std::cout << "contains 'ell': " << s.is_contains("ell") << std::endl;
    std::cout << "starts_with 'he': " << s.is_starts_with("he") << std::endl;
    std::cout << "find 'lo': " << s.find("lo") << std::endl;

    cpct::String greeting = s + " world";
    std::cout << "concat: " << greeting << std::endl;

    auto parts = greeting.split(" ");
    std::cout << "split[0]: " << parts[0] << " split[1]: " << parts[1] << std::endl;

    cpct::String joined = cpct::join("-", parts);
    std::cout << "join: " << joined << std::endl;

    // === Bool ===
    cpct::Bool flag = true;
    std::cout << "Bool: " << flag << " type=" << flag.type() << std::endl;
    assert(flag == true);
    assert(!flag == false);

    // === Char ===
    cpct::Char ch = 'A';
    std::cout << "Char: " << ch << " type=" << ch.type() << " size=" << ch.size() << std::endl;
    ch++;
    assert(ch == 'B');

    // === Implicit conversion ===
    int native_int = x; // Int → int_fast16_t → int
    double native_float = pi;
    std::cout << "native_int: " << native_int << std::endl;
    std::cout << "native_float: " << native_float << std::endl;

    std::cout << "\n===== all tests passed =====" << std::endl;
    return 0;
}
