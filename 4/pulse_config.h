// pulse_config.h -- CS 3370 Assignment: Validated Config Loader
// DO NOT change any declaration in this file.
#pragma once

#include <expected>
#include <istream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

// ---- Layer 1: numbers -----
enum class ParseError { NotANumber, OutOfRange };

std::expected<int, ParseError>    parse_int(std::string_view s);
std::expected<double, ParseError> parse_double(std::string_view s);

// ---- The loader's error type -----
enum class ConfigErrc {
    CannotOpen,     // file could not be opened
    MalformedLine,  // a non-blank, non-comment line with no key=
    UnknownKey,     // key is not one of the five parameters
    DuplicateKey,   // key appears more than once
    MissingKeys,    // one or more parameters never appeared
    NotANumber,     // value is not a number of the right type
    OutOfRange      // value does not fit the type or its limits
};

struct ConfigError {
    ConfigErrc  code;
    std::string detail;    // the key name, or the bad line's text
    int         line = 0;  // 1-based line number; 0 = no line
};

// The exact text main prints for an error (see the handout)
std::string to_message(const ConfigError& err,
                       std::string_view filename);

// ---- Layer 2: one line -----
struct KeyValue {
    std::string key;
    std::string value;
    int         line = 0;
};

// value holding a KeyValue -> a key=value setting
// value holding nullopt    -> blank line or comment: skip it
// error                    -> MalformedLine
std::expected<std::optional<KeyValue>, ConfigError>
parse_line(std::string_view text, int line_no);

// ---- Layer 3: the whole file -----
struct RawParams {
    std::optional<std::string> vt;
    std::optional<std::string> width;
    std::optional<std::string> pulse_delta;
    std::optional<std::string> drop_ratio;
    std::optional<std::string> below_drop_ratio;

    // Fails with UnknownKey or DuplicateKey
    std::expected<void, ConfigError> set(const KeyValue& kv);
};

struct PulseParams {
    int    vt               = 0;
    int    width            = 0;
    int    pulse_delta      = 0;
    double drop_ratio       = 0.0;
    int    below_drop_ratio = 0;
};

std::expected<RawParams, ConfigError> read_raw(std::istream& in);
std::expected<PulseParams, ConfigError>
validate(const RawParams& raw);
std::expected<PulseParams, ConfigError>
load_params(std::istream& in);

// ---- Layer 4: the program boundary -----
class ConfigFileError : public std::runtime_error {
public:
    ConfigFileError(ConfigError err, std::string_view filename);
    const ConfigError& error() const noexcept { return err_; }
private:
    ConfigError err_;
};

class PulseConfig {
public:
    // Throws ConfigFileError if the file can't be loaded
    explicit PulseConfig(const std::string& filename);
    const PulseParams& params() const noexcept { return params_; }
private:
    PulseParams params_;
};
