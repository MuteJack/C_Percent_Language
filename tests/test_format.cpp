// Test: cpct::format — f-string transpilation target
#include "include/cpct/types.h"
#include "include/cpct/io.h"
#include <cassert>
using namespace cpct;

int main() {
    // === basic types ===
    println("=== basic format ===");
    std::string s1 = cpct::format("hello world");
    println(s1);
    assert(s1 == "hello world");

    // === int ===
    Int x = 42;
    std::string s2 = cpct::format("x = ", x);
    println(s2);
    assert(s2 == "x = 42");

    // === float ===
    Float pi = 3.140000;
    std::string s3 = cpct::format("pi = ", pi);
    println(s3);

    // === string ===
    String name = "C%";
    std::string s4 = cpct::format("name = ", name);
    println(s4);
    assert(s4 == "name = C%");

    // === bool ===
    Bool flag = true;
    std::string s5 = cpct::format("flag = ", flag);
    println(s5);
    assert(s5 == "flag = true");

    // === char ===
    Char ch = 'A';
    std::string s6 = cpct::format("ch = ", ch);
    println(s6);
    assert(s6 == "ch = A");

    // === mixed types (simulates f-string) ===
    println("=== f-string simulation ===");
    // f"hello {name}, x={x}, flag={flag}"
    std::string msg = cpct::format("hello ", name, ", x=", x, ", flag=", flag);
    println(msg);
    assert(msg == "hello C%, x=42, flag=true");

    // === native types ===
    println("=== native types ===");
    std::string s7 = cpct::format("int=", 100, " float=", 2.5, " bool=", true);
    println(s7);

    // === use in comparison ===
    println("=== comparison ===");
    String expected = "error: 404";
    Int code = 404;
    std::string result = cpct::format("error: ", code);
    assert(result == std::string(expected));
    println("match: ", result);

    // === use as variable ===
    println("=== variable ===");
    String greeting = cpct::format("Hello, ", name, "!");
    println(greeting);
    assert(greeting == String("Hello, C%!"));

    // === empty format ===
    std::string empty = cpct::format();
    assert(empty.empty());

#if CPCT_HAS_STD_FORMAT
    println("=== format_spec (std::format available) ===");
    std::string spec = cpct::format_spec("pi = {:.2f}", static_cast<double>(pi));
    println(spec);
#else
    println("=== format_spec skipped (std::format not available) ===");
#endif

    println("===== format test passed =====");
    return 0;
}
