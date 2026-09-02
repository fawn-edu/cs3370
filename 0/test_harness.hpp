// test_harness.h
// ------------------------------------------------------------------
// A tiny, self-contained test harness for the CS-3370 C++ refresher
// (Rectangle / RectangularFrustum) assignment.
//
// It runs a handful of KNOWN-CORRECT volume & surface-area cases and a
// handful of INVALID-INPUT sanity checks, then prints a pass/fail
// summary. Nothing here reads from the keyboard, so it is safe to call
// from main() before (or instead of) your interactive section.
//
// ASSUMPTIONS -- adjust the marked spots if your code differs:
//   (1) Your frustum header is "rectangularfrustum.h" and the class is
//       named RectangularFrustum.
//   (2) The constructor takes (topLength, topWidth, bottomLength,
//       bottomWidth, height) and THROWS a std::exception-derived
//       exception (e.g. std::invalid_argument) when any value is <= 0.
//       ("Disallow non-positive values" in the assignment.)
//   (3) The frustum exposes volume() and surfaceArea() returning double.
// ------------------------------------------------------------------
#pragma once

#include <cmath>       // std::fabs
#include <string>
#include <iostream>
#include <functional>  // std::function

#include "frustum.hpp"   // <-- (1) rename to match your file

// Why is this a class? This is effectively a namespace. All state
// belongs to the runAll() function. What is this, Java?
class TestHarness {
public:
    // Runs every test and prints a summary. Returns true if all passed.
    bool runAll() {
        _passed = _failed = 0;
        std::cout << "=== RectangularFrustum test harness ===\n\n";
        testValidCases();
        testInvalidInputs();
        std::cout << "\n----------------------------------------\n";
        std::cout << "Passed: " << _passed
                  << "   Failed: " << _failed << "\n";
        return _failed == 0;
    }

private:
    int _passed = 0;
    int _failed = 0;

    // Floating-point values are almost never exactly equal, so compare
    // with a small tolerance instead of ==.
    static bool nearlyEqual(double a, double b, double eps = 1e-6) {
        return std::fabs(a - b) <= eps * (1.0 + std::fabs(a) + std::fabs(b));
    }

    // Print one result line and tally it.
    void report(const std::string& label, bool ok) {
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << label << "\n";
        (ok ? _passed : _failed)++;
    }

    // Check a numeric expectation (used for volume / surface area).
    void expectClose(const std::string& label,
                     double actual, double expected) {
        bool ok = nearlyEqual(actual, expected);
        report(label, ok);
        if (!ok) {
            std::cout << "          expected " << expected
                      << " but got " << actual << "\n";
        }
    }

    // Check that a piece of code throws (used for invalid-input checks).
    void expectThrow(const std::string& label,
                     const std::function<void()>& fn) {
        bool threw = false;
        try {
            fn();
        } catch (const std::exception&) {
            threw = true;   // constructor rejected the bad input -> good
        } catch (...) {
            threw = true;   // any other throw still counts as "rejected"
        }
        report(label, threw);
    }

    // --- Known-correct cases -------------------------------------
    // Values computed by hand from the assignment formulas:
    //   V  = (h/3) * (A1 + A2 + sqrt(A1 * A2))
    //   SA = l1*w1 + l2*w2 + h*(l1 + l2) + h*(w1 + w2)
    void testValidCases() {
        std::cout << "Valid cases:\n";

        // Case 1: top 2x3 (A1=6), bottom 4x6 (A2=24), h=9
        //   V  = 3 * (6 + 24 + sqrt(144)) = 3 * 42         = 126
        //   SA = 6 + 24 + 9*(2+4) + 9*(3+6) = 30 + 54 + 81 = 165
        {
            RectangularFrustum f(2, 3, 4, 6, 9);
            expectClose("Case 1 volume",       f.volume(),      126.0);
            expectClose("Case 1 surface area", f.surfaceArea(), 165.0);
        }

        // Case 2: top 5x5 (A1=25), bottom 5x5 (A2=25), h=10  (a box)
        //   V  = (10/3) * (25 + 25 + 25)              = 250
        //   SA = 25 + 25 + 10*(5+5) + 10*(5+5)        = 250
        {
            RectangularFrustum f(5, 5, 5, 5, 10);
            expectClose("Case 2 volume",       f.volume(),      250.0);
            expectClose("Case 2 surface area", f.surfaceArea(), 250.0);
        }

        // Case 3: top 1x1 (A1=1), bottom 3x3 (A2=9), h=6
        //   V  = 2 * (1 + 9 + sqrt(9)) = 2 * 13       = 26
        //   SA = 1 + 9 + 6*(1+3) + 6*(1+3)            = 58
        {
            RectangularFrustum f(1, 1, 3, 3, 6);
            expectClose("Case 3 volume",       f.volume(),      26.0);
            expectClose("Case 3 surface area", f.surfaceArea(), 58.0);
        }
    }

    // --- Invalid-input sanity checks -----------------------------
    // Each constructor call below uses a non-positive value and should
    // therefore be rejected (i.e. throw).
    void testInvalidInputs() {
        std::cout << "\nInvalid inputs (each should be rejected):\n";

        expectThrow("negative top length",
                    [] { RectangularFrustum(-2, 3, 4, 6, 9); });

        expectThrow("zero top width",
                    [] { RectangularFrustum(2, 0, 4, 6, 9); });

        expectThrow("negative bottom width",
                    [] { RectangularFrustum(2, 3, 4, -6, 9); });

        expectThrow("zero height",
                    [] { RectangularFrustum(2, 3, 4, 6, 0); });

        expectThrow("negative height",
                    [] { RectangularFrustum(2, 3, 4, 6, -9); });
    }
};
