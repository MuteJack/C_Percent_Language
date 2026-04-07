// Test: find(), is_contains(), Vector sort(descending)
#include "include/cpct/types.h"
#include "include/cpct/io.h"
#include <cassert>
using namespace cpct;

int main() {
    // === Vector find/contains ===
    println("=== Vector find/contains ===");
    Vector<Int> v = {10, 20, 30, 40, 50};

    assert(v.find(Int(30)) == 2);
    assert(v.find(Int(99)) == -1);
    println("find(30) = ", v.find(Int(30)));
    println("find(99) = ", v.find(Int(99)));

    assert(v.is_contains(Int(20)));
    assert(!v.is_contains(Int(99)));
    println("contains(20) = ", v.is_contains(Int(20)));
    println("contains(99) = ", v.is_contains(Int(99)));

    // === Vector sort descending ===
    println("=== Vector sort descending ===");
    Vector<Int> v2 = {3, 1, 4, 1, 5};
    v2.sort();
    println("sort asc: ", v2);
    assert(v2[0] == 1);

    v2.sort(true);
    println("sort desc: ", v2);
    assert(v2[0] == 5);

    // === Array find/contains ===
    println("=== Array find/contains ===");
    Array<Int, 5> arr = {10, 20, 30, 40, 50};

    assert(arr.find(Int(30)) == 2);
    assert(arr.find(Int(99)) == -1);
    println("find(30) = ", arr.find(Int(30)));
    println("find(99) = ", arr.find(Int(99)));

    assert(arr.is_contains(Int(20)));
    assert(!arr.is_contains(Int(99)));
    println("contains(20) = ", arr.is_contains(Int(20)));
    println("contains(99) = ", arr.is_contains(Int(99)));

    // === String vector ===
    println("=== String vector find ===");
    Vector<String> sv = {"apple", "banana", "cherry"};
    assert(sv.find(String("banana")) == 1);
    assert(sv.find(String("grape")) == -1);
    assert(sv.is_contains(String("cherry")));
    println("find banana = ", sv.find(String("banana")));
    println("contains cherry = ", sv.is_contains(String("cherry")));

    println("===== find test passed =====");
    return 0;
}
