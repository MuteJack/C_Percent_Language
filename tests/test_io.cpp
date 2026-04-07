// Test: cpct I/O library — simulates transpiled C% code
// This is what transpiled C% code would look like.
#include "include/cpct/types.h"
#include "include/cpct/io.h"
using namespace cpct;

int main() {
    // === print / println ===
    println("=== print/println ===");
    Int x = 42;
    Float pi = 3.14159;
    String name = "C%";

    print("x = ", x);
    println();
    println("pi = ", pi);
    println("name = ", name);
    println("multi: ", x, " ", pi, " ", name);

    // === f-string via std::format ===
    println("=== f-string ===");
    println(cpct::format("x = ", x, ", pi = ", pi));
    println(cpct::format("name = ", name));

    // === types with println ===
    println("=== types ===");
    Int8 small = 127;
    UInt64 big = 999999;
    Bool flag = true;
    Char ch = 'A';

    println("Int8: ", small);
    println("UInt64: ", big);
    println("Bool: ", flag);
    println("Char: ", ch);

    // === chained method + print ===
    println("=== string methods ===");
    String s = "hello world";
    println("upper: ", s.upper());
    println("len: ", s.len());
    println("reverse: ", s.reverse());

    // === input (skipped in automated test) ===
    // String user = input("Enter name: ");
    // println("Hello, ", user, "!");

    println("===== io test passed =====");
    return 0;
}
