// Test: type casts, max/min, shape
#include "include/cpct/types.h"
#include "include/cpct/io.h"
#include <cassert>
using namespace cpct;

int main() {
    // === max / min ===
    println("=== max/min ===");
    println("Int8 max=", Int8::max(), " min=", Int8::min());
    println("Int32 max=", Int32::max(), " min=", Int32::min());
    println("UInt8 max=", UInt8::max(), " min=", UInt8::min());
    println("Float32 max=", Float32::max(), " min=", Float32::min());
    println("Char max=", static_cast<int>(Char::max()), " min=", static_cast<int>(Char::min()));
    println("Bool max=", Bool::max(), " min=", Bool::min());

    // === shape ===
    println("=== shape ===");
    Array<Int, 5> arr = {1, 2, 3, 4, 5};
    auto sh = arr.shape();
    println("shape = [", sh[0], "]");
    assert(sh[0] == 5);

    // === int casts ===
    println("=== int casts ===");
    Int a = to_int(3.7);
    println("to_int(3.7) = ", a);
    assert(a == 3);

    Int b = to_int(String("42"));
    println("to_int(\"42\") = ", b);
    assert(b == 42);

    Int c = to_int(true);
    println("to_int(true) = ", c);
    assert(c == 1);

    Int8 d = to_int8(200);
    println("to_int8(200) = ", d);  // wrap-around

    // === float casts ===
    println("=== float casts ===");
    Float e = to_float(42);
    println("to_float(42) = ", e);

    Float f = to_float(String("3.14"));
    println("to_float(\"3.14\") = ", f);

    Float32 g = to_float32(3.14159265);
    println("to_float32(3.14159265) = ", g);

    // === string cast ===
    println("=== string cast ===");
    String s1 = to_string(42);
    println("to_string(42) = ", s1);
    assert(s1 == String("42"));

    String s2 = to_string(3.14);
    println("to_string(3.14) = ", s2);

    String s3 = to_string(true);
    println("to_string(true) = ", s3);
    assert(s3 == String("true"));

    String s4 = to_string(Char('A'));
    println("to_string('A') = ", s4);
    assert(s4 == String("A"));

    // === bool cast ===
    println("=== bool cast ===");
    Bool b1 = to_bool(0);
    println("to_bool(0) = ", b1);
    assert(!b1);

    Bool b2 = to_bool(42);
    println("to_bool(42) = ", b2);
    assert(b2);

    Bool b3 = to_bool(String(""));
    println("to_bool(\"\") = ", b3);
    assert(!b3);

    Bool b4 = to_bool(String("hello"));
    println("to_bool(\"hello\") = ", b4);
    assert(b4);

    // === char cast ===
    println("=== char cast ===");
    Char c1 = to_char(65);
    println("to_char(65) = ", c1);
    assert(c1 == 'A');

    Char c2 = to_char(String("hello"));
    println("to_char(\"hello\") = ", c2);
    assert(c2 == 'h');

    // === cross-type casts ===
    println("=== cross-type ===");
    Int32 x = to_int32(Float(3.99));
    println("to_int32(3.99) = ", x);
    assert(x == 3);

    Float y = to_float(Int8(127));
    println("to_float(Int8(127)) = ", y);

    println("===== cast test passed =====");
    return 0;
}
