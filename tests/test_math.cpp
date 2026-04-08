// Test: cpct::pow and cpct::divmod
#include "include/cpct/types.h"
#include "include/cpct/io.h"
#include <cassert>
using namespace cpct;

int main() {
    // === integer pow ===
    println("=== integer pow ===");
    Int a = cpct::pow(Int(2), Int(10));
    println("2 ** 10 = ", a);           // 1024
    assert(a == 1024);

    Int b = cpct::pow(Int(3), Int(4));
    println("3 ** 4 = ", b);            // 81
    assert(b == 81);

    Int c = cpct::pow(Int(5), Int(0));
    println("5 ** 0 = ", c);            // 1
    assert(c == 1);

    // === float pow ===
    println("=== float pow ===");
    Float d = cpct::pow(Float(2.5), Float(3.0));
    println("2.5 ** 3.0 = ", d);        // 15.625

    Float e = cpct::pow(Float(9.0), Float(0.5));
    println("9.0 ** 0.5 = ", e);        // 3.0

    // === integer divmod ===
    println("=== integer divmod ===");
    auto [q1, r1] = cpct::divmod(Int(10), Int(3));
    println("10 /% 3 = ", q1, ", ", r1);  // 3, 1
    assert(q1 == 3);
    assert(r1 == 1);

    auto [q2, r2] = cpct::divmod(Int(17), Int(5));
    println("17 /% 5 = ", q2, ", ", r2);  // 3, 2
    assert(q2 == 3);
    assert(r2 == 2);

    // === float divmod ===
    println("=== float divmod ===");
    auto [q3, r3] = cpct::divmod(Float(10.0), Float(3.0));
    println("10.0 /% 3.0 = ", q3, ", ", r3);  // 3.0, 1.0

    auto [q4, r4] = cpct::divmod(Float(7.5), Float(2.0));
    println("7.5 /% 2.0 = ", q4, ", ", r4);   // 3.0, 1.5

    println("===== math test passed =====");
    return 0;
}
