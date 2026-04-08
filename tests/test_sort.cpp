// Test: sort/reverse for Array, Vector, Dict
#include "include/cpct/types.h"
#include "include/cpct/io.h"
#include <cassert>
using namespace cpct;

int main() {
    // === Array sort ===
    println("=== Array sort ===");
    Array<Int, 5> arr = {3, 1, 4, 1, 5};
    println("before: ", arr);
    arr.sort();
    println("sort asc: ", arr);
    assert(arr[0] == 1);
    assert(arr[4] == 5);

    arr.sort(true);
    println("sort desc: ", arr);
    assert(arr[0] == 5);
    assert(arr[4] == 1);

    // === Array reverse ===
    println("=== Array reverse ===");
    Array<Int, 4> arr2 = {1, 2, 3, 4};
    arr2.reverse();
    println("reversed: ", arr2);
    assert(arr2[0] == 4);
    assert(arr2[3] == 1);

    // === Vector reverse ===
    println("=== Vector reverse ===");
    Vector<Int> v = {10, 20, 30, 40, 50};
    v.reverse();
    println("reversed: ", v);
    assert(v[0] == 50);
    assert(v[4] == 10);

    // === Dict sortkey ===
    println("=== Dict sortkey ===");
    Dict d;
    d.set("charlie", 92);
    d.set("alice", 95);
    d.set("bob", 87);

    d.sortkey();
    println("sortkey asc keys: ");
    for (auto& k : d) print(k, " ");
    println();
    auto ks = d.keys();
    assert(ks[0] == "alice");
    assert(ks[1] == "bob");
    assert(ks[2] == "charlie");

    d.sortkey(true);
    println("sortkey desc keys: ");
    for (auto& k : d) print(k, " ");
    println();

    // === Dict sortval ===
    println("=== Dict sortval ===");
    d.sortval<int>();
    println("sortval asc keys: ");
    for (auto& k : d) print(k, " ");
    println();
    ks = d.keys();
    assert(ks[0] == "bob");     // 87
    assert(ks[1] == "charlie"); // 92
    assert(ks[2] == "alice");   // 95

    d.sortval<int>(true);
    println("sortval desc keys: ");
    for (auto& k : d) print(k, " ");
    println();

    // === Dict sortvk ===
    println("=== Dict sortvk ===");
    Dict d2;
    d2.set("bob", 90);
    d2.set("alice", 90);
    d2.set("charlie", 85);

    d2.sortvk<int>();
    ks = d2.keys();
    println("sortvk keys: ");
    for (auto& k : d2) print(k, " ");
    println();
    assert(ks[0] == "charlie"); // 85
    assert(ks[1] == "alice");   // 90, alphabetically first
    assert(ks[2] == "bob");     // 90, alphabetically second

    println("===== sort test passed =====");
    return 0;
}
