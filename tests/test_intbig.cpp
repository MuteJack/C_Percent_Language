// Test: cpct::IntBig — auto-promoting integer
#include "include/cpct/types.h"
#include "include/cpct/io.h"
#include <cassert>
using namespace cpct;

int main() {
    // === small (int64) path ===
    println("=== small path ===");
    IntBig a = 42;
    IntBig b = 100;
    println("a = ", a, " type=", a.type(), " size=", a.size());
    assert(a.isSmall());

    IntBig c = a + b;
    println("42 + 100 = ", c);
    assert(c == IntBig(142));
    assert(c.isSmall());

    IntBig d = a * b;
    println("42 * 100 = ", d);
    assert(d == IntBig(4200));

    // === overflow → BigInt promotion ===
    println("=== overflow promotion ===");
    IntBig big = INT64_MAX;
    println("INT64_MAX = ", big);
    assert(big.isSmall());

    IntBig overflow = big + IntBig(1);
    println("INT64_MAX + 1 = ", overflow);
    assert(overflow.isBig()); // promoted to BigInt!

    IntBig overflow2 = big * IntBig(2);
    println("INT64_MAX * 2 = ", overflow2);
    assert(overflow2.isBig());

    // === demote back to small ===
    println("=== demote ===");
    IntBig back = overflow - IntBig(1);
    println("(INT64_MAX+1) - 1 = ", back);
    assert(back.isSmall()); // back to int64

    // === negative overflow ===
    println("=== negative overflow ===");
    IntBig neg(static_cast<int64_t>(INT64_MIN));
    IntBig negOver = neg - IntBig(1);
    println("INT64_MIN - 1 = ", negOver);
    assert(negOver.isBig());

    // === multiplication overflow ===
    println("=== large multiplication ===");
    IntBig x = 1000000000LL; // 10^9
    IntBig y = x * x;        // 10^18 — fits int64
    println("10^9 * 10^9 = ", y);
    assert(y.isSmall());

    IntBig z = y * x;        // 10^27 — overflows int64
    println("10^18 * 10^9 = ", z);
    assert(z.isBig());

    // === comparison ===
    println("=== comparison ===");
    assert(IntBig(10) < IntBig(20));
    assert(IntBig(10) == IntBig(10));
    assert(overflow > big);
    println("comparisons OK");

    // === increment ===
    println("=== increment ===");
    IntBig counter = INT64_MAX;
    counter++;
    println("INT64_MAX++ = ", counter);
    assert(counter.isBig());
    counter--;
    println("back-- = ", counter);
    assert(counter.isSmall());
    assert(counter == IntBig(INT64_MAX));

    println("===== intbig test passed =====");
    return 0;
}
