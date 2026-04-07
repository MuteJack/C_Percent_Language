// Test: Map hash collision handling and sort methods
#include "include/cpct/types.h"
#include "include/cpct/io.h"
#include <cassert>
using namespace cpct;

int main() {
    // === sortkey ===
    println("=== sortkey ===");
    Map<String, Int> scores;
    scores["charlie"] = 92;
    scores["alice"] = 95;
    scores["bob"] = 87;
    println("before: ", scores);

    scores.sortkey();
    println("sortkey asc: ", scores);
    auto ks = scores.keys();
    assert(ks[0] == String("alice"));
    assert(ks[1] == String("bob"));
    assert(ks[2] == String("charlie"));

    scores.sortkey(true);
    println("sortkey desc: ", scores);

    // === sortval ===
    println("=== sortval ===");
    scores.sortval();
    println("sortval asc: ", scores);
    auto vs = scores.values();
    assert(vs[0] == 87);
    assert(vs[1] == 92);
    assert(vs[2] == 95);

    scores.sortval(true);
    println("sortval desc: ", scores);

    // === sortvk ===
    println("=== sortvk ===");
    Map<String, Int> tied;
    tied["bob"] = 90;
    tied["alice"] = 90;
    tied["charlie"] = 85;
    tied.sortvk();
    println("sortvk: ", tied);
    auto tk = tied.keys();
    assert(tk[0] == String("charlie")); // 85
    assert(tk[1] == String("alice"));   // 90, alphabetically first
    assert(tk[2] == String("bob"));     // 90, alphabetically second

    // === hash collision safety ===
    println("=== hash collision ===");
    Map<Int, String> m;
    // Insert many keys — some may collide
    for (int i = 0; i < 100; i++) {
        m[Int(i)] = cpct::format("val_", i);
    }
    assert(m.len() == 100);
    assert(m.has(Int(0)));
    assert(m.has(Int(99)));
    assert(m[Int(50)] == String("val_50"));
    println("100 entries OK, m[50] = ", m[Int(50)]);

    // Remove and verify
    m.remove(Int(50));
    assert(m.len() == 99);
    assert(!m.has(Int(50)));
    println("after remove 50, len = ", m.len());

    println("===== map sort test passed =====");
    return 0;
}
