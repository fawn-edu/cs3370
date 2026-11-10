#include <cmath>
#include <cstddef>
#include <format>
#include <fstream>
#include <utility>
#include "pulse_config.h"

using std::expected, std::unexpected, std::optional, std::string, std::string_view;

namespace {
	string_view strip_whitespace(string_view s) {
		// \n omitted since s should be a single line, split on \n
		// \f and \v omitted because the handout does not mention them,
		// but they would be checked by std::isspace()
		constexpr auto w = " \t\r";
		const auto first = s.find_first_not_of(w);
		if (first == string_view::npos) return string_view{};
		return s.substr(first, 1 + s.find_last_not_of(w) - first);
	}
	
	constexpr ConfigErrc p2c(ParseError e) {
		switch (e) {
		case ParseError::NotANumber: return ConfigErrc::NotANumber;
		case ParseError::OutOfRange: return ConfigErrc::OutOfRange;
		}
		std::unreachable();
	};
}

// ---- Layer 1: numbers -----
// Why don't std::stoi() and std::stod() have overloads accepting
// non-owning string_views? parse_int() has to allocate owning copies
// as a result. Wasteful.

expected<int, ParseError> parse_int(string_view s) {
	try {
		string s2{s};
		std::size_t pos;
		int result = std::stoi(s2, &pos);
		if (pos != s2.size()) return unexpected{ParseError::NotANumber};
		return result;
	} catch (const std::invalid_argument &e) {
		return unexpected{ParseError::NotANumber};
	} catch (const std::out_of_range &e) {
		return unexpected{ParseError::OutOfRange};
	}
}

expected<double, ParseError> parse_double(string_view s) {
	try {
		string s2{s};
		std::size_t pos;
		double result = std::stod(s2, &pos);
		if (pos != s2.size()) return unexpected{ParseError::NotANumber};
		return result;
	} catch (const std::invalid_argument &e) {
		return unexpected{ParseError::NotANumber};
	} catch (const std::out_of_range &e) {
		return unexpected{ParseError::OutOfRange};
	}
}

// ---- Layer 2: one line -----

expected<optional<KeyValue>, ConfigError> parse_line(string_view text, int line_no) {
	text = strip_whitespace(text);
	if (text.empty() || text.front() == '#') return {};
	const auto pos = text.find_first_of('=');
	if (pos == string_view::npos) return unexpected{ConfigError{ConfigErrc::MalformedLine, string{text}, line_no}};
	const auto key = strip_whitespace(text.substr(0, pos));
	if (key.empty()) return unexpected{ConfigError{ConfigErrc::MalformedLine, string{text}, line_no}};
	return KeyValue{
		string{key},
		string{strip_whitespace(text.substr(1 + pos))},
		line_no
	};
}

// ---- Layer 3: the whole file -----

expected<void, ConfigError> RawParams::set(const KeyValue& kv) {
	if (kv.key == "vt") {
		if (vt) return unexpected{ConfigError{ConfigErrc::DuplicateKey, kv.key}};
		vt.emplace(kv.value);
		return {};
	} else if (kv.key == "width") {
		if (width) return unexpected{ConfigError{ConfigErrc::DuplicateKey, kv.key}};
		width.emplace(kv.value);
		return {};
	} else if (kv.key == "pulse_delta") {
		if (pulse_delta) return unexpected{ConfigError{ConfigErrc::DuplicateKey, kv.key}};
		pulse_delta.emplace(kv.value);
		return {};
	} else if (kv.key == "drop_ratio") {
		if (drop_ratio) return unexpected{ConfigError{ConfigErrc::DuplicateKey, kv.key}};
		drop_ratio.emplace(kv.value);
		return {};
	} else if (kv.key == "below_drop_ratio") {
		if (below_drop_ratio) return unexpected{ConfigError{ConfigErrc::DuplicateKey, kv.key}};
		below_drop_ratio.emplace(kv.value);
		return {};
	}
	return unexpected{ConfigError{ConfigErrc::UnknownKey, kv.key}};
}

expected<RawParams, ConfigError> read_raw(std::istream& in) {
	string line;
	RawParams r{};
	for (auto i = 1; std::getline(in, line); ++i) {
		auto l = parse_line(line, i);
		if (!l) return unexpected{std::move(l.error())};
		if (*l) {
			auto e = r.set(**l);
			if (!e) return unexpected{std::move(e.error())};
		}
	}
	return r;
}

expected<PulseParams, ConfigError> validate(const RawParams& raw) {
	PulseParams p{};
	if (!raw.vt) return unexpected{ConfigError{ConfigErrc::MissingKeys, "vt"}};
	if (!raw.width) return unexpected{ConfigError{ConfigErrc::MissingKeys, "width"}};
	if (!raw.pulse_delta) return unexpected{ConfigError{ConfigErrc::MissingKeys, "pulse_delta"}};
	if (!raw.drop_ratio) return unexpected{ConfigError{ConfigErrc::MissingKeys, "drop_ratio"}};
	if (!raw.below_drop_ratio) return unexpected{ConfigError{ConfigErrc::MissingKeys, "below_drop_ratio"}};

	constexpr auto cki = [](const auto &in, int &out, const char *name, auto fn) -> optional<ConfigError> {
		auto v = parse_int(in).transform_error([&name](auto &&e){ return ConfigError{p2c(e), name}; });
		if (!v) return std::move(v.error());
		if (!fn(*v)) return ConfigError{ConfigErrc::OutOfRange, name};
		out = *v;
		return {};
	};

	constexpr auto ckd = [](const auto &in, double &out, const char *name, auto fn) -> optional<ConfigError> {
		auto v = parse_double(in).transform_error([&name](auto &&e){ return ConfigError{p2c(e), name}; });
		if (!v) return std::move(v.error());
		if (!fn(*v)) return ConfigError{ConfigErrc::OutOfRange, name};
		out = *v;
		return {};
	};

	// Was it worth it?
	if (auto e = cki(*raw.vt, p.vt, "vt", [](int n){ return n >= 1; })) return unexpected{std::move(*e)};
	if (auto e = cki(*raw.width, p.width, "width", [](int n){ return n >= 1; })) return unexpected{std::move(*e)};
	if (auto e = cki(*raw.pulse_delta, p.pulse_delta, "pulse_delta", [](int n){ return n >= 1; })) return unexpected{std::move(*e)};
	if (auto e = ckd(*raw.drop_ratio, p.drop_ratio, "drop_ratio", [](double n){ return !std::isnan(n) && 0 < n && n < 1; })) return unexpected{std::move(*e)};
	if (auto e = cki(*raw.below_drop_ratio, p.below_drop_ratio, "below_drop_ratio", [](int n){ return n >= 0; })) return unexpected{std::move(*e)};

	// The better version without nonsense obfuscating helpers
	/*
	auto vt = parse_int(*raw.vt);
	if (!vt) return unexpected{ConfigError{p2c(vt.error()), "vt"}};
	if (*vt < 1) return unexpected{ConfigError{ConfigErrc::OutOfRange, "vt"}};
	p.vt = *vt;

	auto width = parse_int(*raw.width);
	if (!width) return unexpected{ConfigError{p2c(width.error()), "width"}};
	if (*width < 1) return unexpected{ConfigError{ConfigErrc::OutOfRange, "width"}};
	p.width = *width;

	auto pulse_delta = parse_int(*raw.pulse_delta);
	if (!pulse_delta) return unexpected{ConfigError{p2c(pulse_delta.error()), "pulse_delta"}};
	if (*pulse_delta < 1) return unexpected{ConfigError{ConfigErrc::OutOfRange, "pulse_delta"}};
	p.pulse_delta = *pulse_delta;

	auto drop_ratio = parse_double(*raw.drop_ratio);
	if (!drop_ratio) return unexpected{ConfigError{p2c(drop_ratio.error()), "drop_ratio"}};
	if (std::isnan(*drop_ratio) || *drop_ratio <= 0.0 || *drop_ratio >= 1.0) return unexpected{ConfigError{ConfigErrc::OutOfRange, "drop_ratio"}};
	p.drop_ratio = *drop_ratio;

	auto below_drop_ratio = parse_int(*raw.below_drop_ratio);
	if (!below_drop_ratio) return unexpected{ConfigError{p2c(below_drop_ratio.error()), "below_drop_ratio"}};
	if (*below_drop_ratio < 0) return unexpected{ConfigError{ConfigErrc::OutOfRange, "below_drop_ratio"}};
	p.below_drop_ratio = *below_drop_ratio;
	*/

	return p;
}

expected<PulseParams, ConfigError> load_params(std::istream& in) {
	return read_raw(in).and_then(validate);
}

// ---- Layer 4: the program boundary -----

// In the header, this function is listed in layer 1, but in the handout
// it is listed in layer 4. Weird
string to_message(const ConfigError& err, string_view filename) {
	// "returns the exact text in Table 3"
	// "Your output must match Table 3 exactly."
	// This function expects a filename, and most ConfigErrors have a
	// meaningful line number. Why are these not printed in every error
	// case where they are useful?
	switch (err.code) {
	case ConfigErrc::CannotOpen: return std::format("Cannot open INI file: {}", filename);
	case ConfigErrc::MalformedLine: return std::format("Malformed line {} in {}", err.line, filename);
	// Why are UnknownKey and DuplicateKey required to print the same
	// message? It would be more helpful to print which since the
	// difference is already known up to this point. Also, it does not
	// print the line, which is helpful information.
	case ConfigErrc::UnknownKey: [[fallthrough]];
	case ConfigErrc::DuplicateKey: return std::format("Invalid or duplicate INI key: {}", err.detail);
	// err.detail contains the name of at least one key known to be
	// missing, why not print that?
	case ConfigErrc::MissingKeys: return std::format("Invalid INI file: missing one or more keys");
	case ConfigErrc::NotANumber: [[fallthrough]];
	case ConfigErrc::OutOfRange: return std::format("Invalid value for {} in {}", err.detail, filename);
	}

	// How I would have done it
	/*
	switch (err.code) {
	case ConfigErrc::CannotOpen: return std::format("{}: cannot open file", filename);
	case ConfigErrc::MalformedLine: return std::format("{}:{}: malformed line", filename, err.line);
	case ConfigErrc::UnknownKey: return std::format("{}:{}: unknown key ‘{}’", filename, err.line, err.detail);
	case ConfigErrc::DuplicateKey: return std::format("{}:{}: duplicate key ‘{}’", filename, err.line, err.detail);
	case ConfigErrc::MissingKeys: return std::format("{}: missing one or more keys (at least ‘{}’)", filename, err.detail);
	case ConfigErrc::NotANumber: return std::format("{}:{}: value of ‘{}’ is not a number", filename, err.line, err.detail);
	case ConfigErrc::OutOfRange: return std::format("{}:{}: value of ‘{}’ is out of range", filename, err.line, err.detail);
	}
	*/
	std::unreachable();
}

ConfigFileError::ConfigFileError(ConfigError err, string_view filename)
	: std::runtime_error{to_message(err, filename)}
	, err_{std::move(err)}
{}

PulseConfig::PulseConfig(const string& filename) {
	std::ifstream f{filename};
	if (!f.is_open()) throw ConfigFileError{ConfigError{ConfigErrc::CannotOpen, {}}, filename};
	auto r = load_params(f);
	if (!r) throw ConfigFileError{std::move(r.error()), filename};
	params_ = *r;
}

