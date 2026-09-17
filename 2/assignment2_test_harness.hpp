// =====================================================================
// CS 3370 -- Programming Assignment 2: NPC Memory Pool
// SELF-TEST HARNESS
// =====================================================================
//
// WHAT THIS IS
//   A self-contained set of checks you can run against YOUR NPC and
//   Pool implementation before you submit. It talks to your code only
//   through the public interface the assignment requires:
//
//       NPC(name, type, x, y, health)      -- the constructor
//       operator<<(ostream&, const NPC&)   -- the output operator
//       new NPC(...) / delete npc          -- your operator new/delete
//       NPC::profile()                     -- optional; detected below
//
//   It does NOT need to see inside Pool, and it does NOT require any
//   of the bonus features.
//

// HOW TO READ THE RESULTS
//   [PASS]  that requirement is satisfied.
//   [FAIL]  something the assignment requires is wrong. Fix it.
//   [WARN]  legal, but probably not what was intended -- read the note.
//   [INFO]  just reporting a number so you can sanity-check it.
//   The program's exit code is 0 if there were no failures, 1 otherwise.
//


// =====================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include "NPC.hpp"

// ---------------------------------------------------------------------
// The pool size declared in the assignment: inline static Pool pool{100, true};
// If you intentionally built a different-sized pool, change this to match.
// ---------------------------------------------------------------------
constexpr std::size_t EXPECTED_CAPACITY = 100;

// ---------------------------------------------------------------------
// Single point of change for BONUS 2 (factory pattern).
// If you made NPC's constructor private and added spawn(), comment out
// the `new NPC(...)` line and uncomment the NPC::spawn(...) line. The
// rest of the harness does not care which one you use.
// ---------------------------------------------------------------------
static NPC* makeNPC(const std::string& name, const std::string& type,
                    int x, int y, int health = 100) {
    // return new NPC(name, type, x, y, health);
    return NPC::spawn(name, type, x, y, health);   // Bonus 2
}

// ---------------------------------------------------------------------
// Detect whether NPC::profile() exists and is callable. If you didn't
// write it (it is optional), the harness silently skips that section
// instead of failing to compile.
// ---------------------------------------------------------------------
template <typename T>
concept HasProfile = requires { T::profile(); };

// =====================================================================
// A very small check framework -- no external test library needed.
// =====================================================================
static int g_pass = 0;
static int g_fail = 0;
static int g_warn = 0;

static void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}
static void pass(const std::string& what) {
    std::cout << "  [PASS] " << what << '\n';
    ++g_pass;
}
static void fail(const std::string& what, const std::string& detail = "") {
    std::cout << "  [FAIL] " << what << '\n';
    if (!detail.empty()) std::cout << "         " << detail << '\n';
    ++g_fail;
}
static void warn(const std::string& what, const std::string& detail = "") {
    std::cout << "  [WARN] " << what << '\n';
    if (!detail.empty()) std::cout << "         " << detail << '\n';
    ++g_warn;
}
static void info(const std::string& what) {
    std::cout << "  [INFO] " << what << '\n';
}
static void check(bool condition, const std::string& what,
                  const std::string& detail = "") {
    if (condition) pass(what); else fail(what, detail);
}

static std::uintptr_t addrOf(const void* p) {
    return reinterpret_cast<std::uintptr_t>(p);
}

// Pool geometry, discovered in test 3 and reused by later tests.
static bool          g_geometryKnown = false;
static std::uintptr_t g_poolFirst = 0;   // address of the lowest slot
static std::uintptr_t g_poolLast  = 0;   // address of the highest slot
static std::size_t    g_stride    = 0;   // bytes between adjacent slots

static bool inPool(const void* p) {
    if (!g_geometryKnown) return true;   // nothing to compare against yet
    const std::uintptr_t a = addrOf(p);
    return a >= g_poolFirst && a <= g_poolLast;
}

// Render an NPC through its operator<< so we can inspect the text.
static std::string render(const NPC* npc) {
    std::ostringstream os;
    os << *npc;
    return os.str();
}

static std::string trimmed(const std::string& s) {
    const std::size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const std::size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// =====================================================================
// TEST 1 -- object size and alignment (informational)
// =====================================================================
static void test_size_and_alignment() {
    section("1. NPC size and alignment");
    info("sizeof(NPC)  = " + std::to_string(sizeof(NPC)) + " bytes");
    info("alignof(NPC) = " + std::to_string(alignof(NPC)) + " bytes");
    info("A 100-slot pool therefore needs at least " +
         std::to_string(EXPECTED_CAPACITY * sizeof(NPC)) + " bytes.");
    info("Remember: sizeof(NPC) is fixed at compile time, but a long "
         "std::string member");
    info("       still heap-allocates its characters OUTSIDE the pool. "
         "Keep test names short.");
}

// =====================================================================
// TEST 2 -- constructor and output operator
// =====================================================================
static void test_output_operator() {
    section("2. Constructor and output operator");

    NPC* npc = makeNPC("Goblin_247", "enemy", 5, 10, 100);
    const std::string actual   = render(npc);
    const std::string expected =
        "NPC[name='Goblin_247', type='enemy', pos=(5,10), hp=100]";

    if (actual == expected) {
        pass("operator<< matches the format in the handout");
    } else if (trimmed(actual) == expected) {
        warn("operator<< is correct except for surrounding whitespace",
             "Don't print a trailing '\\n' or std::endl inside operator<< -- "
             "let the caller decide.");
    } else {
        fail("operator<< does not match the format in the handout",
             "expected: " + expected + "\n         actual:   " + actual);
    }

    // Default health argument should be 100.
    NPC* dflt = makeNPC("Shopkeeper", "merchant", 25, 25);
    const std::string d = render(dflt);
    check(d.find("hp=100") != std::string::npos,
          "default health parameter is 100",
          "actual: " + d);

    delete npc;
    delete dflt;
}

// =====================================================================
// TEST 3 -- allocations really come from the pool
//
// If operator new was overridden correctly, filling the pool should
// produce exactly EXPECTED_CAPACITY addresses that sit in one
// contiguous block, evenly spaced. The global heap essentially never
// hands back a run of 100 evenly-spaced addresses, so this is a strong
// check that the pool -- not ::operator new -- served the request.
// =====================================================================
static void test_pool_geometry() {
    section("3. Allocations come from the pool");

    std::vector<NPC*> live;
    live.reserve(EXPECTED_CAPACITY);
    bool ranOutEarly = false;

    try {
        for (std::size_t i = 0; i < EXPECTED_CAPACITY; ++i)
            live.push_back(makeNPC("N" + std::to_string(i), "test",
                                   static_cast<int>(i), static_cast<int>(i), 100));
    } catch (const std::bad_alloc&) {
        ranOutEarly = true;
    }

    check(!ranOutEarly,
          "pool supplies all " + std::to_string(EXPECTED_CAPACITY) + " slots",
          "bad_alloc was thrown after only " + std::to_string(live.size()) +
              " allocations -- is the pool smaller than " +
              std::to_string(EXPECTED_CAPACITY) + " slots?");

    if (live.size() < 2) {
        fail("not enough allocations succeeded to analyse the pool layout");
        for (NPC* p : live) delete p;
        return;
    }

    std::vector<std::uintptr_t> addrs;
    addrs.reserve(live.size());
    for (NPC* p : live) addrs.push_back(addrOf(p));

    // -- all distinct? (two live objects sharing a slot is the worst bug here)
    std::vector<std::uintptr_t> sorted = addrs;
    std::sort(sorted.begin(), sorted.end());
    const bool allDistinct =
        std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
    check(allDistinct, "every live NPC has its own address",
          "two allocations returned the same slot -- allocate() is handing out "
          "an index that is still in use.");

    // -- alignment
    bool aligned = true;
    for (std::uintptr_t a : sorted)
        if (a % alignof(NPC) != 0) { aligned = false; break; }
    check(aligned, "every slot is correctly aligned for NPC",
          "at least one slot address is not a multiple of alignof(NPC) = " +
              std::to_string(alignof(NPC)) +
              " -- see the alignof(T) note in Bonus 1.");

    // -- uniform stride
    std::vector<std::uintptr_t> gaps;
    for (std::size_t i = 1; i < sorted.size(); ++i)
        gaps.push_back(sorted[i] - sorted[i - 1]);
    const bool uniform =
        std::adjacent_find(gaps.begin(), gaps.end(), std::not_equal_to<>()) ==
        gaps.end();

    if (uniform && !gaps.empty()) {
        g_stride = static_cast<std::size_t>(gaps.front());
        pass("slots are evenly spaced -- allocations come from one block");
        info("observed slot stride = " + std::to_string(g_stride) + " bytes");
        if (g_stride == sizeof(NPC)) {
            pass("stride equals sizeof(NPC)");
        } else if (g_stride > sizeof(NPC)) {
            warn("stride is larger than sizeof(NPC)",
                 "That is fine if you padded slots for alignment; it just "
                 "means the pool uses more memory than the minimum.");
        } else {
            fail("stride is SMALLER than sizeof(NPC)",
                 "Slots overlap -- one NPC's bytes sit on top of the next "
                 "one's. get_slot() is multiplying by the wrong size.");
        }
    } else {
        fail("slot addresses are not evenly spaced",
             "Allocations look like they came from the global heap rather "
             "than your pool. Did NPC::operator new call pool.allocate()?");
    }

    if (allDistinct) {
        g_poolFirst     = sorted.front();
        g_poolLast      = sorted.back();
        g_geometryKnown = true;
        info("pool block spans " +
             std::to_string(g_poolLast - g_poolFirst + (g_stride ? g_stride : 0)) +
             " bytes");
    }

    for (NPC* p : live) delete p;
}

// =====================================================================
// TEST 4 -- exhaustion throws std::bad_alloc
// =====================================================================
static void test_exhaustion() {
    section("4. Exhausting the pool throws std::bad_alloc");

    std::vector<NPC*> live;
    try {
        for (std::size_t i = 0; i < EXPECTED_CAPACITY; ++i)
            live.push_back(makeNPC("F" + std::to_string(i), "fill",
                                   0, 0, 100));
    } catch (const std::bad_alloc&) {
        // handled by test 3's reporting; keep going with what we got
    }

    NPC* overflow = nullptr;
    bool threwBadAlloc = false;
    bool threwSomethingElse = false;

    try {
        overflow = makeNPC("OneTooMany", "overflow", 0, 0, 100);
    } catch (const std::bad_alloc&) {
        threwBadAlloc = true;
    } catch (...) {
        threwSomethingElse = true;
    }

    if (threwBadAlloc) {
        pass("allocating one past capacity throws std::bad_alloc");
    } else if (threwSomethingElse) {
        fail("allocating past capacity threw, but not std::bad_alloc",
             "The handout asks specifically for std::bad_alloc so that "
             "callers can catch the standard allocation failure type.");
    } else {
        fail("allocating past capacity did NOT throw",
             "An empty free list must throw std::bad_alloc. Returning a "
             "garbage pointer (or popping from an empty vector, which is "
             "undefined behavior) is exactly how a pool becomes a security "
             "bug.");
        delete overflow;
    }

    for (NPC* p : live) delete p;
}

// =====================================================================
// TEST 5 -- freed slots are reused
// =====================================================================
static void test_slot_reuse() {
    section("5. Freed slots go back on the free list");

    std::vector<NPC*> live;
    for (int i = 0; i < 5; ++i)
        live.push_back(makeNPC("R" + std::to_string(i), "reuse", i, i, 100));

    const std::uintptr_t freedAddr = addrOf(live[2]);
    delete live[2];
    live[2] = nullptr;

    NPC* replacement = makeNPC("Replacement", "reuse", 99, 99, 100);
    const std::uintptr_t newAddr = addrOf(replacement);

    if (newAddr == freedAddr) {
        pass("the next allocation reuses the slot that was just freed");
    } else if (inPool(replacement)) {
        warn("the next allocation reused a different free slot",
             "Legal, but with vector::pop_back() the most recently freed "
             "index should come back first. Check that deallocate() pushes "
             "onto the same end allocate() pops from.");
    } else {
        fail("the replacement was not allocated from the pool at all",
             "Its address falls outside the block the pool handed out "
             "earlier.");
    }

    live[2] = replacement;
    for (NPC* p : live) delete p;
}

// =====================================================================
// TEST 6 -- interleaved free/allocate in arbitrary order, and object
//           integrity across slot reuse
//
// This is the "can we reuse spaces released in random order?" question
// from the lecture. It also catches overlapping slots: if two live NPCs
// share bytes, one of them will no longer render its own name.
// =====================================================================
static void test_interleaved_and_integrity() {
    section("6. Out-of-order release, and objects survive slot reuse");

    constexpr int FIRST_WAVE = 20;
    std::vector<NPC*>        live;
    std::vector<std::string> expectedName;

    for (int i = 0; i < FIRST_WAVE; ++i) {
        const std::string n = "W" + std::to_string(i);
        live.push_back(makeNPC(n, "wave1", i, i, 100));
        expectedName.push_back(n);
    }

    // Release every other one -- i.e. deliberately NOT in allocation order.
    for (int i = 0; i < FIRST_WAVE; i += 2) {
        delete live[i];
        live[i] = nullptr;
        expectedName[i].clear();
    }

    // Refill the holes.
    bool refillThrew = false;
    try {
        for (int i = 0; i < FIRST_WAVE; i += 2) {
            const std::string n = "X" + std::to_string(i);
            live[i] = makeNPC(n, "wave2", i, i, 50);
            expectedName[i] = n;
        }
    } catch (const std::bad_alloc&) {
        refillThrew = true;
    }

    check(!refillThrew, "slots released out of order become available again",
          "bad_alloc while refilling holes -- deallocate() is not returning "
          "indices to the free list.");

    // All live objects distinct, in the pool, and still holding their own data.
    std::vector<std::uintptr_t> addrs;
    bool allInPool = true;
    bool contentIntact = true;
    std::string firstBadRender;

    for (std::size_t i = 0; i < live.size(); ++i) {
        if (!live[i]) continue;
        addrs.push_back(addrOf(live[i]));
        if (!inPool(live[i])) allInPool = false;

        const std::string text = render(live[i]);
        if (text.find("name='" + expectedName[i] + "'") == std::string::npos) {
            contentIntact = false;
            if (firstBadRender.empty())
                firstBadRender = "expected name '" + expectedName[i] +
                                 "', rendered: " + text;
        }
    }

    std::sort(addrs.begin(), addrs.end());
    check(std::adjacent_find(addrs.begin(), addrs.end()) == addrs.end(),
          "no two live NPCs share a slot after out-of-order reuse",
          "An index was handed out while it was still in use -- check that "
          "deallocate() cannot push the same index twice.");
    check(allInPool, "every reused allocation stays inside the pool block");
    check(contentIntact,
          "each NPC still holds its own data after slots were recycled",
          firstBadRender);

    for (NPC* p : live) delete p;
}

// =====================================================================
// TEST 7 -- the pool is fully restored after everything is freed
// =====================================================================
static void test_full_cycle() {
    section("7. Free list is fully restored after a complete cycle");

    for (int round = 1; round <= 2; ++round) {
        std::vector<NPC*> live;
        bool threw = false;
        try {
            for (std::size_t i = 0; i < EXPECTED_CAPACITY; ++i)
                live.push_back(makeNPC("C" + std::to_string(i), "cycle",
                                       0, 0, 100));
        } catch (const std::bad_alloc&) {
            threw = true;
        }

        check(!threw && live.size() == EXPECTED_CAPACITY,
              "round " + std::to_string(round) + ": filled the pool to " +
                  std::to_string(EXPECTED_CAPACITY) + " slots",
              "only " + std::to_string(live.size()) +
                  " allocations succeeded -- slots from the previous round "
                  "were never returned to the free list (a slot leak).");

        for (NPC* p : live) delete p;
    }
}

// =====================================================================
// TEST 8 -- churn: many allocate/deallocate cycles, no drift, no crash
// =====================================================================
static void test_churn() {
    section("8. Sustained churn");

    constexpr int ROUNDS    = 200;
    constexpr int BATCH     = 10;
    bool          threw     = false;
    bool          strayed   = false;

    for (int r = 0; r < ROUNDS && !threw; ++r) {
        std::vector<NPC*> batch;
        try {
            for (int i = 0; i < BATCH; ++i)
                batch.push_back(makeNPC("K", "churn", i, r, 100));
        } catch (const std::bad_alloc&) {
            threw = true;
        }

        for (NPC* p : batch)
            if (!inPool(p)) strayed = true;

        // Release in a scrambled (but deterministic) order.
        const int n = static_cast<int>(batch.size());
        for (int i = 0; i < n; ++i) {
            const int idx = (i * 7 + 3) % n;
            delete batch[idx];
            batch[idx] = nullptr;
        }
        for (NPC* p : batch) delete p;   // no-ops; all already null
    }

    check(!threw,
          std::to_string(ROUNDS) + " rounds of " + std::to_string(BATCH) +
              " allocate/free cycles never exhaust the pool",
          "The free list is shrinking over time -- deallocate() is losing "
          "indices.");
    check(!strayed, "every churned allocation stayed inside the pool block");
}

// =====================================================================
// TEST 9 -- profile() output (informational; skipped if not implemented)
// =====================================================================
static void test_profile() {
    section("9. profile() output");

    if constexpr (HasProfile<NPC>) {
        info("Pool should be completely empty at this point "
             "(0 live, all slots free):");
        NPC::profile();

        std::vector<NPC*> live;
        for (int i = 0; i < 3; ++i)
            live.push_back(makeNPC("P" + std::to_string(i), "profile",
                                   i, i, 100));

        info("Now 3 NPCs are live -- your profile() should say so:");
        NPC::profile();

        for (NPC* p : live) delete p;

        info("And back to empty:");
        NPC::profile();
        info("Check those three dumps by eye: the live/free counts should "
             "read 0, 3, 0.");
    } else {
        info("NPC::profile() not found (or not public) -- skipping. "
             "It is recommended but optional.");
    }
}

// =====================================================================
// COPY THIS CODE INTO YOUR MAIN FUNCTION AS WELL AS ANYTHING ELSE YOU WISH TO RUN
/*
    std::cout << "=====================================================\n"
              << " CS 3370 Assignment 2 -- NPC Memory Pool self-test\n"
              << "=====================================================\n";

    test_size_and_alignment();
    test_output_operator();
    test_pool_geometry();
    test_exhaustion();
    test_slot_reuse();
    test_interleaved_and_integrity();
    test_full_cycle();
    test_churn();
    test_profile();

    std::cout << "\n=====================================================\n"
              << " SUMMARY: " << g_pass << " passed, "
              << g_fail << " failed, " << g_warn << " warnings\n";
    if (g_fail == 0)
        std::cout << " All required checks passed. Re-read your Pool "
                     "destructor\n for leaks, then submit.\n";
    else
        std::cout << " Fix the [FAIL] items above before submitting.\n";
    std::cout << "=====================================================\n";

    return g_fail == 0 ? 0 : 1;
*/
