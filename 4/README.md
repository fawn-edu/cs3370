# Programming assignment 4: validated config loader

Fawn Sannar <10725695@uvu.edu> | `10725695`

The test program can be compiled and ran by invoking `make`. Both binaries can be compiled at once with `make all`.

## Reflection

> 1. `parse_line` returns `std::expected<std::optional<KeyValue>, ConfigError>`. What would go wrong if it returned only `std::optional<KeyValue>`, or only `std::expected<KeyValue, ConfigError>`?

There would be no way to distinguish between a malformed line and an empty or comment line.

> 2. Why does `PulseConfig`’s constructor throw, while every function it calls returns an `expected`? What can a caller do with `ConfigFileError::error()` that it cannot do with `what()`?

A constructor does not really return a value, and as such it is unable to return an `expected`. For a constructor to report an error state, it must throw an exception.

`ConfigFileError::error()` returns the underlying `ConfigError`, which can be used to convert back into an `expected`, whereas `std::exception::what()` just returns a null-terminated string.

## Regarding the rubric

> `ConfigFileError` derives from `runtime_error` and is caught by `const&`

The former requirement is already fulfilled in `pulse_config.h:79`, and the latter requirement does not seem applicable to this assignment, as nowhere in the student-implemented functions is a `ConfigFileError` ever intended to be caught. Are these left over from a previous draft of this assignment, or perhaps intended to apply to the next assignment that builds upon this one?

> A helper removes the repetition in `validate`

This rubric item upsets me. `validate()` is not a complicated function. I have done as the rubric specifies, but I strongly feel this *decreases* readability for no benefit.
