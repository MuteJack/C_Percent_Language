// Test: cpct data structures — Array, Vector, Dict, Map
#include "include/cpct/types.h"
#include "include/cpct/io.h"
#include <cassert>
using namespace cpct;

int main() {
    // === Array ===
    println("=== Array ===");
    Array<Int, 5> arr = {1, 2, 3, 4, 5};
    println("arr = ", arr);
    println("len = ", arr.len());
    println("front = ", arr.front());
    println("back = ", arr.back());
    println("arr[-1] = ", arr[-1]);
    assert(arr[0] == 1);
    assert(arr[-1] == 5);

    print("foreach: ");
    for (auto& x : arr) print(x, " ");
    println();

    auto sliced = arr.slice(1, 4);
    print("slice[1:4]: ");
    for (auto& x : sliced) print(x, " ");
    println();

    // === Vector ===
    println("=== Vector ===");
    Vector<Int> v = {10, 20, 30};
    println("v = ", v);

    v.push(40);
    v.push(50);
    println("after push: ", v);

    Int popped = v.pop();
    println("popped: ", popped);

    v.insert(1, 99);
    println("insert(1, 99): ", v);

    v.erase(1);
    println("erase(1): ", v);

    println("v[-1] = ", v[-1]);
    println("is_empty = ", v.is_empty());

    Vector<Int> unsorted = {3, 1, 4, 1, 5, 9, 2};
    unsorted.sort();
    println("sorted: ", unsorted);

    // === Dict — multi-type keys ===
    println("=== Dict ===");
    Dict d;
    d.set("alice", 95);
    d.set("bob", 87);
    d.set(int64_t(1), std::string("one"));         // int key
    d.set_bool(true, std::string("yes"));           // bool key
    d.set_char('X', std::string("char X"));         // char key

    println("len = ", d.len());
    assert(d.len() == 5);
    assert(d.has("alice"));
    assert(d.has(int64_t(1)));
    assert(d.has_bool(true));
    assert(d.has_char('X'));
    assert(!d.has("unknown"));
    assert(!d.has(int64_t(999)));

    println("alice = ", d.get<int>("alice"));
    println("key 1 = ", d.get<std::string>(int64_t(1)));
    println("key true = ", d.get_bool<std::string>(true));
    println("key X = ", d.get_char<std::string>('X'));

    // Dict — string "1" vs int 1 are different keys
    d.set("1", std::string("string one"));
    assert(d.has("1"));
    assert(d.has(int64_t(1)));
    assert(d.get<std::string>("1") == "string one");
    assert(d.get<std::string>(int64_t(1)) == "one");
    println("string '1' vs int 1: OK");

    // Dict — remove preserves order
    println("keys before remove: ");
    for (auto& k : d) print(k, " ");
    println();

    d.remove("bob");
    println("after remove bob, len = ", d.len());
    println("keys after remove: ");
    for (auto& k : d) print(k, " ");
    println();

    // === Map — basic ===
    println("=== Map ===");
    Map<String, Int> scores;
    scores.set("alice", 95);
    scores.set("bob", 87);
    scores["charlie"] = 92;
    println("map = ", scores);
    println("len = ", scores.len());
    println("size = ", scores.size());

    // Map — type coercion (float → int)
    scores.set("dave", 88.7);  // float coerced to Int
    println("dave (coerced from 88.7) = ", scores["dave"]);

    // Map — remove preserves order
    scores.remove("bob");
    println("after remove bob: ", scores);
    auto ks = scores.keys();
    assert(ks[0] == String("alice"));
    assert(ks[1] == String("charlie"));
    assert(ks[2] == String("dave"));
    println("order preserved after remove: OK");

    // Map — sortkey
    scores.sortkey();
    println("sortkey: ", scores);

    // Map — sortval
    scores.sortval();
    println("sortval: ", scores);

    // Map — int keys
    Map<Int, String> names;
    names[1] = "one";
    names[2] = "two";
    names[3] = "three";
    println("int-key map = ", names);
    assert(names[2] == String("two"));

    println("===== ds test passed =====");
    return 0;
}
