#include <algorithm> // copy_if, max_element
#include <cassert>
#include <charconv>
#include <cstddef> // size_t
#include <exception> // exception
#include <filesystem> // directory_iterator
#include <cstdio>
#include <iterator> // istream_iterator
#include <numeric> // accumulate
#include <optional>
#include <span>
#include <vector>
#include "pulse_config.h"
#include "stage_timer.hpp" // slop

using namespace std;

// Returns a span<char> containing the entire contents of a file using
// C-style IO
span<char> read_file(const char *name) {
	FILE *file = fopen(name, "rb");
	if (!file) return {};
	[[maybe_unused]] int err = 0;
	err = fseek(file, 0, SEEK_END); assert(err == 0);
	long fsize = ftell(file); assert(fsize != -1);
	auto fsz = static_cast<size_t>(fsize);
	err = fseek(file, 0, SEEK_SET); assert(err == 0);
	char *buf = new char[fsize];
	[[maybe_unused]] size_t cnt = fread(buf, sizeof buf[0], fsize, file); assert(cnt == fsz);
	(void)fclose(file);
	return {buf, fsz};
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file.ini>\n", argv[0]);
		return 1;
	}
	try {
		const PulseConfig config{argv[1]};
		const PulseParams &params = config.params();
		// Sort list of files first so output is consistent
		// I did not do this for P5 since it was not specified anywhere
		// in the instructions or rubric, but here I need to diff the
		// output of different runs so they have to be in the same order
		vector<filesystem::directory_entry> files{};
		copy_if(
			filesystem::directory_iterator{"."},
			{},
			back_inserter(files),
			[](auto &f){ return f.is_regular_file() && f.path().extension() == ".dat"; }
		);
		ranges::sort(files);
		for (const auto &f : files) {
			vector<long long> data;
			{
				StageTimer t{"reading"};
				auto buf = read_file(f.path().string().c_str());
				if (buf.empty()) {
					fprintf(stderr, "Invalid file: %s\n", f.path().filename().string().c_str());
					continue;
				}
				// Parse integers, separated by whitespace
				bool bad = false;
				for (char *c = buf.data(), *end = buf.data() + buf.size(); c < end;) {
					while (isspace(*c) && c < end) ++c;
					if (c == end) break;
					char *tail = c;
					while (!isspace(*c) && c < end) ++c;
					char *head = c;
					int v;
					auto res = from_chars(tail, head, v);
					if (res.ec != errc{}) {
						bad = true;
						break;
					}
					// Negate sample before storing in the first place
					data.emplace_back(-static_cast<long long>(v));
				}
				delete[] buf.data();
				if (bad) {
					fprintf(stderr, "Invalid file: %s\n\n", f.path().filename().string().c_str());
					continue;
				}
				// Skip files containing less than 3 samples
				// Pulses can only occur over 3 samples so files with
				// any fewer can't have pulses anyways
				if (data.size() < 3) continue;
			}
			vector<long long> smoothed;
			{
				StageTimer t{"smoothing"};
				smoothed = vector<long long>{data};
				for (auto i = 3u; i < data.size() - 3; ++i) {
					smoothed[i] = (3 * (data[i - 1] + data[i] + data[i + 1])
						+ 2 * (data[i - 2] + data[i + 2])
						+ data[i - 3] + data[i + 3])
						/ 15;
				}
			}
			// Detect pulses
			vector<size_t> pulses{};
			{
				StageTimer t{"detection"};
				optional<size_t> j = nullopt;
				for (auto i = 0u; i < data.size() - 2; ++i) {
					const auto dv = smoothed[i + 2] - smoothed[i];
					if (!j && dv > params.vt) { // Pulse begins
						j = i;
					} else if (j && dv < 0) { // Pulse ends
						pulses.emplace_back(*j);
						j = nullopt;
					}
				}
				if (j) pulses.emplace_back(*j); // Pulse began but did not end
				if (pulses.empty()) continue; // Skip files with no pulses
			}
			auto msg = format("{}:\n", f.path().filename().string());
			for (auto i = 0u; i < pulses.size() - 1; ++i) {
				const auto &it = pulses[i], &next = pulses[1 + i];
				const auto end = min<size_t>(it + params.width, next);
				{
					StageTimer t{"piggyback test"};
					// Check for piggyback
					if (next - it <= static_cast<size_t>(params.pulse_delta)) {
						const auto peakpos = ranges::max_element(span{&smoothed[it], &smoothed[end]});
						const auto thresh = params.drop_ratio * static_cast<double>(*peakpos);
						const auto nbelow = count_if(&*peakpos, &smoothed[next], [thresh](long long n){ return n < thresh; });
						if (nbelow > params.below_drop_ratio) {
							msg.append(format("Found piggyback at {}\n", it));
							continue;
						}
					}
				}
				// I would have used std::reduce() if the rubric didn't
				// explicitly call for std::accumulate()
				long long a;
				{
					StageTimer t{"area"};
					a = accumulate(&data[it], &data[end], 0ll);
				}
				msg.append(format("{} ({})\n", it, a));
			}
			// Final peak cannot be piggyback by definition
			long long a;
			{
				StageTimer t{"area"};
				a = accumulate(&data[pulses.back()], &data[min(pulses.back() + params.width, data.size())], 0ll);
			}
			msg.append(format("{} ({})\n", pulses.back(), a));
			{
				StageTimer t{"printing"};
				puts(msg.c_str());
			}
		}
		print_stage_report();
	} catch (const ConfigFileError &e) {
		fprintf(stderr, "%s", e.what());
		return 1;
	} catch (const exception &e) {
		fprintf(stderr, "Unexpected error: %s", e.what());
		return 2;
	}
}
