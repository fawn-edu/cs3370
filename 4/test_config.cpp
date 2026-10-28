// test_config.cpp -- CS 3370 Assignment 4: Validated Config Loader
// Test harness (GIVEN -- do not change it).
//
// Build and run (one command line):
//   g++ -std=c++23 -Wall -Wextra test_config.cpp pulse_config.cpp -o tests
//   ./tests
//
// Every CHECK that fails prints its line number and the condition
// that was false, then the tests keep going. Fix the first failure
// in each section first: later ones often follow from it.
#include "pulse_config.h"

#include <cmath>
#include <cstdio>      // std::remove
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ---- A tiny test framework ------------------------------------------
namespace {

int checks   = 0;
int failures = 0;

// Returns the condition, so a test can write: if (CHECK(r)) { ... }
bool check(bool ok, const char* expr, int line) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cout << "  FAILED (line " << line << "): " << expr << '\n';
    }
    return ok;
}

#define CHECK(cond) check(static_cast<bool>(cond), #cond, __LINE__)

// Runs one group of tests. Nothing below the PulseConfig constructor
// may throw, so an exception that reaches here is itself a failure.
void run(const char* name, void (*tests)()) {
    std::cout << name << '\n';
    try {
        tests();
    } catch (const std::exception& e) {
        ++failures;
        std::cout << "  FAILED: exception escaped: " << e.what() << '\n';
    } catch (...) {
        ++failures;
        std::cout << "  FAILED: unknown exception escaped\n";
    }
}

// ---- Helpers ---------------------------------------------------------
const std::string good =
    "# Pulse parameters\n"
    "vt=100\n"
    "width=100\n"
    "pulse_delta=15\n"
    "drop_ratio=0.75\n"
    "below_drop_ratio=4\n";

std::expected<PulseParams, ConfigError> load(const std::string& text) {
    std::istringstream in{text};
    return load_params(in);
}

// `good`, with the line for `key` replaced by `line`
std::string good_with(const std::string& key, const std::string& line) {
    std::istringstream in{good};
    std::string text, out;
    while (std::getline(in, text))
        out += (text.starts_with(key + "=") ? line : text) + '\n';
    return out;
}

// Does r hold an error with this code? (Never reads error() on success.)
bool fails_with(const std::expected<PulseParams, ConfigError>& r,
                ConfigErrc code) {
    return !r && r.error().code == code;
}

// The message main would print for a failed load
std::string message(const std::string& text) {
    auto r = load(text);
    return r ? "(no error)" : to_message(r.error(), "test.ini");
}

// ---- Layer 1: numbers ------------------------------------------------
void test_layer1() {
    auto n = parse_int("42");
    CHECK(n && *n == 42);
    n = parse_int("-7");
    CHECK(n && *n == -7);
    CHECK(!parse_int("abc") && parse_int("abc").error()
                                   == ParseError::NotANumber);
    CHECK(!parse_int("") && parse_int("").error()
                                == ParseError::NotANumber);
    CHECK(!parse_int("100abc") && parse_int("100abc").error()
                                      == ParseError::NotANumber);
    CHECK(!parse_int("4.5") && parse_int("4.5").error()
                                   == ParseError::NotANumber);
    CHECK(!parse_int("1 00") && parse_int("1 00").error()
                                    == ParseError::NotANumber);
    CHECK(!parse_int("99999999999")
          && parse_int("99999999999").error() == ParseError::OutOfRange);

    auto d = parse_double("0.75");
    CHECK(d && *d == 0.75);
    d = parse_double("2");
    CHECK(d && *d == 2.0);
    CHECK(!parse_double("abc") && parse_double("abc").error()
                                      == ParseError::NotANumber);
    CHECK(!parse_double("0.5x") && parse_double("0.5x").error()
                                       == ParseError::NotANumber);
    CHECK(!parse_double("1e999") && parse_double("1e999").error()
                                        == ParseError::OutOfRange);
    d = parse_double("nan");        // a double -- Layer 3 rejects it
    CHECK(d && std::isnan(*d));
}

// ---- Layer 2: one line -----------------------------------------------
void test_layer2() {
    auto r = parse_line("", 1);                 // blank: skip
    CHECK(r && !*r);
    r = parse_line(" \t\r", 1);                 // only whitespace: skip
    CHECK(r && !*r);
    r = parse_line("   # a comment", 1);        // comment: skip
    CHECK(r && !*r);

    r = parse_line("  vt = 100 \r", 3);         // spaces and CR trimmed
    if (CHECK(r && *r)) {
        CHECK((*r)->key == "vt");
        CHECK((*r)->value == "100");
        CHECK((*r)->line == 3);
    }

    r = parse_line("vt=", 2);                   // empty value is a line;
    if (CHECK(r && *r))                         // Layer 3 rejects it
        CHECK((*r)->value.empty());

    r = parse_line("pulse_delta 15", 4);        // no '='
    CHECK(!r && r.error().code == ConfigErrc::MalformedLine);
    CHECK(!r && r.error().line == 4);
    r = parse_line("=100", 7);                  // empty key
    CHECK(!r && r.error().code == ConfigErrc::MalformedLine);
    CHECK(!r && r.error().line == 7);
}

// ---- Layer 3: RawParams::set -----------------------------------------
void test_set() {
    RawParams raw;
    CHECK(raw.set(KeyValue{"vt", "100", 1}));   // expected<void>: success
    CHECK(raw.vt && *raw.vt == "100");
    CHECK(!raw.width);                          // others still empty

    auto r = raw.set(KeyValue{"vt", "50", 2});
    CHECK(!r && r.error().code == ConfigErrc::DuplicateKey);
    CHECK(raw.vt && *raw.vt == "100");          // first value kept

    r = raw.set(KeyValue{"foo", "3", 3});
    CHECK(!r && r.error().code == ConfigErrc::UnknownKey);
    r = raw.set(KeyValue{"VT", "3", 4});        // keys are case-sensitive
    CHECK(!r && r.error().code == ConfigErrc::UnknownKey);
}

// ---- Layer 3: the whole file -----------------------------------------
void test_layer3() {
    auto r = load(good);
    if (CHECK(r)) {
        CHECK(r->vt == 100);
        CHECK(r->width == 100);
        CHECK(r->pulse_delta == 15);
        CHECK(std::abs(r->drop_ratio - 0.75) < 1e-12);
        CHECK(r->below_drop_ratio == 4);
    }

    // Messy but valid: blank lines, tabs, CRLF, keys out of order
    r = load("\n  # comment\r\n"
             "drop_ratio = 0.5\r\n"
             "\tvt\t=\t1\r\n"
             "below_drop_ratio=0\n"
             "  width =2\n"
             "pulse_delta = 3");             // no final newline
    if (CHECK(r)) {
        CHECK(r->vt == 1);                      // smallest allowed
        CHECK(r->width == 2 && r->pulse_delta == 3);
        CHECK(r->below_drop_ratio == 0);        // smallest allowed
        CHECK(std::abs(r->drop_ratio - 0.5) < 1e-12);
    }

    // One test for each error code
    CHECK(fails_with(load(good + "pulse_delta 15\n"),
                     ConfigErrc::MalformedLine));
    CHECK(fails_with(load(good + "foo=3\n"), ConfigErrc::UnknownKey));
    CHECK(fails_with(load(good + "width=5\n"), ConfigErrc::DuplicateKey));
    CHECK(fails_with(load("vt=100\nwidth=100\n"),
                     ConfigErrc::MissingKeys));
    CHECK(fails_with(load(""), ConfigErrc::MissingKeys));
    CHECK(fails_with(load(good_with("vt", "vt=abc")),
                     ConfigErrc::NotANumber));
    CHECK(fails_with(load(good_with("vt", "vt=99999999999")),
                     ConfigErrc::OutOfRange));

    // The line number of a malformed line
    r = load("# comment\n\nvt=100\nwidth 100\n");
    CHECK(!r && r.error().line == 4);
}

// ---- Layer 3: values and limits --------------------------------------
void test_values() {
    using E = ConfigErrc;
    CHECK(fails_with(load(good_with("vt", "vt=0")), E::OutOfRange));
    CHECK(fails_with(load(good_with("vt", "vt=-5")), E::OutOfRange));
    CHECK(fails_with(load(good_with("vt", "vt=")), E::NotANumber));
    CHECK(fails_with(load(good_with("width", "width=100abc")),
                     E::NotANumber));
    CHECK(fails_with(load(good_with("width", "width=0")),
                     E::OutOfRange));
    CHECK(fails_with(load(good_with("width", "width=4.5")),
                     E::NotANumber));
    CHECK(fails_with(load(good_with("pulse_delta", "pulse_delta=0")),
                     E::OutOfRange));
    CHECK(fails_with(load(good_with("below_drop_ratio",
                                    "below_drop_ratio=-1")),
                     E::OutOfRange));
    CHECK(load(good_with("below_drop_ratio", "below_drop_ratio=0")));

    const char* bad_ratios[] = {"0", "1", "1.5", "-0.5", "nan", "inf"};
    for (const char* ratio : bad_ratios) {
        auto r = load(good_with("drop_ratio",
                                std::string{"drop_ratio="} + ratio));
        if (!CHECK(fails_with(r, E::OutOfRange)))
            std::cout << "    (drop_ratio=" << ratio << ")\n";
    }
    CHECK(fails_with(load(good_with("drop_ratio", "drop_ratio=abc")),
                     E::NotANumber));
}

// ---- Layer 3: which error is reported first --------------------------
void test_error_order() {
    using E = ConfigErrc;
    // Pass 1 (every line) comes before pass 2 (missing keys)
    CHECK(fails_with(load("vt=100\nfoo=1\n"), E::UnknownKey));
    CHECK(fails_with(load("vt=100\nvt=200\n"), E::DuplicateKey));
    CHECK(fails_with(load("vt=abc\nbad line\n"), E::MalformedLine));
    // Within pass 1, the first bad line wins
    CHECK(fails_with(load("foo=1\nbad line\n"), E::UnknownKey));
    CHECK(fails_with(load("bad line\nfoo=1\n"), E::MalformedLine));
    // Pass 2 (missing keys) comes before pass 3 (values)
    CHECK(fails_with(load("vt=abc\n"), E::MissingKeys));
    // Pass 3 checks vt, width, pulse_delta, drop_ratio,
    // below_drop_ratio in that order, wherever they are in the file:
    // below_drop_ratio is bad and comes first, but width is reported
    const std::string two_bad = "below_drop_ratio=-1\n"
                                "vt=100\n"
                                "width=abc\n"
                                "pulse_delta=15\n"
                                "drop_ratio=0.75\n";
    CHECK(message(two_bad) == "Invalid value for width in test.ini");
}

// ---- Layer 4: messages -----------------------------------------------
void test_messages() {
    CHECK(message(good_with("vt", "vt=abc"))
          == "Invalid value for vt in test.ini");
    CHECK(message(good_with("drop_ratio", "drop_ratio=1.5"))
          == "Invalid value for drop_ratio in test.ini");
    CHECK(message("vt=100\n")
          == "Invalid INI file: missing one or more keys");
    CHECK(message(good + "foo=3\n")
          == "Invalid or duplicate INI key: foo");
    CHECK(message(good + "width=5\n")
          == "Invalid or duplicate INI key: width");
    CHECK(message("# one\n# two\n# three\npulse_delta 15\n")
          == "Malformed line 4 in test.ini");
    CHECK(to_message(ConfigError{ConfigErrc::CannotOpen, "a.ini", 0},
                     "a.ini")
          == "Cannot open INI file: a.ini");
}

// ---- Layer 4: the PulseConfig constructor ----------------------------
void test_layer4() {
    // A missing file throws ConfigFileError, carrying the code
    try {
        PulseConfig config{"no_such_file.ini"};
        CHECK(!"PulseConfig should have thrown for a missing file");
    } catch (const ConfigFileError& e) {
        CHECK(e.error().code == ConfigErrc::CannotOpen);
        CHECK(std::string{e.what()}
              == "Cannot open INI file: no_such_file.ini");
    }

    // A bad file throws too, and is also a std::runtime_error
    const char* temp = "harness_temp.ini";
    {
        std::ofstream out{temp};
        out << good_with("width", "width=0");
    }
    try {
        PulseConfig config{temp};
        CHECK(!"PulseConfig should have thrown for width=0");
    } catch (const std::runtime_error& e) {
        auto* ours = dynamic_cast<const ConfigFileError*>(&e);
        if (CHECK(ours != nullptr))
            CHECK(ours->error().code == ConfigErrc::OutOfRange);
        CHECK(std::string{e.what()}
              == "Invalid value for width in harness_temp.ini");
    }

    // A good file builds an object with the right parameters
    {
        std::ofstream out{temp};
        out << good;
    }
    try {
        PulseConfig config{temp};
        CHECK(config.params().vt == 100);
        CHECK(config.params().below_drop_ratio == 4);
    } catch (const std::exception& e) {
        CHECK(!"PulseConfig threw for a valid file");
        std::cout << "    (" << e.what() << ")\n";
    }
    std::remove(temp);
}

}  // namespace

int main() {
    run("Layer 1: parse_int, parse_double", test_layer1);
    run("Layer 2: parse_line", test_layer2);
    run("Layer 3: RawParams::set", test_set);
    run("Layer 3: load_params", test_layer3);
    run("Layer 3: values and limits", test_values);
    run("Layer 3: which error is reported first", test_error_order);
    run("Layer 4: to_message", test_messages);
    run("Layer 4: PulseConfig", test_layer4);

    std::cout << '\n' << checks - failures << " of " << checks
              << " checks passed\n";
    if (failures == 0)
        std::cout << "All tests passed\n";
    return failures == 0 ? 0 : 1;
}
