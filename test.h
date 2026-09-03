
#pragma once

#include <cstddef>
#include <iostream>
#include <iomanip>
using std::size_t;

// Unit Test Scaffolding: Users call test_, fail_, succeed_, throw_, nothrow_, and report_
// AUTHOR: Chuck Allison (Creative Commons License, 2001 - 2017)
// Updated 2020-11-19 by Peter Aldous: added reset_() (plus whitespace fixes)
// 13 Jan 2026 - Enhanced with descriptive test names

namespace {
    size_t nPass{0};
    size_t nFail{0};
    size_t testNumber{0};

    void do_fail(const char* text, const char* fileName, long lineNumber) {
        std::cout << "FAILURE: " << text << " in file " << fileName
                  << " on line " << lineNumber << std::endl;
        ++nFail;
    }

    void do_test(const char* condText, bool cond, const char* fileName, long lineNumber) {
        ++testNumber;
        std::cout << std::left << std::setw(4) << testNumber << " | ";

        if (!cond) {
            std::cout << "FAIL | " << condText << std::endl;
            std::cout << "       (in " << fileName << " on line " << lineNumber << ")" << std::endl;
            ++nFail;
        } else {
            std::cout << "PASS | " << condText << std::endl;
            ++nPass;
        }
    }

    void do_named_test(const char* testName, const char* condText, bool cond,
                       const char* fileName, long lineNumber) {
        ++testNumber;
        std::cout << std::left << std::setw(4) << testNumber << " | ";

        if (!cond) {
            std::cout << "FAIL | " << testName << std::endl;
            std::cout << "       Condition: " << condText << std::endl;
            std::cout << "       (in " << fileName << " on line " << lineNumber << ")" << std::endl;
            ++nFail;
        } else {
            std::cout << "PASS | " << testName << std::endl;
            ++nPass;
        }
    }

#ifdef succeed
    void succeed_() {
        ++nPass;
    }
#endif

    void report_() {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "Test Report:\n";
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "Number of Passes   = " << nPass << std::endl;
        std::cout << "Number of Failures = " << nFail << std::endl;
        std::cout << "Total Tests        = " << (nPass + nFail) << std::endl;

        if (nFail == 0) {
            std::cout << "\n✓ ALL TESTS PASSED!" << std::endl;
        } else {
            std::cout << "\n✗ Some tests failed." << std::endl;
        }
        std::cout << std::string(60, '=') << std::endl;
    }

    void reset_() {
        nPass = 0U;
        nFail = 0U;
        testNumber = 0U;
    }
}

#define test_(cond) do_test(#cond, cond, __FILE__, __LINE__)
#define test_named(name, cond) do_named_test(name, #cond, cond, __FILE__, __LINE__)
#define fail_(text) do_fail(text, __FILE__, __LINE__)

#define throw_(text,T)                      \
    try {                                   \
        text;                               \
        std::cout << "THROW ";              \
        do_fail(#text,__FILE__,__LINE__);   \
    } catch (T&) {                          \
        ++nPass;                            \
    } catch (...) {                         \
        std::cout << "THROW ";              \
        do_fail(#text,__FILE__,__LINE__);   \
    }

#define nothrow_(text)                      \
    try {                                   \
        text;                               \
        ++nPass;                            \
    } catch (...) {                         \
        std::cout << "NOTHROW ";            \
        do_fail(#text,__FILE__,__LINE__);   \
    }
