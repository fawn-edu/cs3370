#include <algorithm> // copy_if, max_element
#include <cstddef> // size_t
#include <exception> // exception
#include <filesystem> // directory_iterator
#include <fstream>
#include <functional> // negate
#include <iostream> // cerr
#include <iterator> // istream_iterator
#include <numeric> // accumulate
#include <optional>
#include <print>
#include <span>
#include <vector>
#include "pulse_config.h"
#include "stage_timer.hpp" // slop

using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		println(cerr, "Usage: {} <file.ini>", argv[0]);
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
			vector<int> data;
			{
				StageTimer t{"reading"};
				ifstream file{f.path(), ios::in};
				data = vector<int>{istream_iterator<int>{file}, {}};
				// Failbit gets set if there happens to be a trailing
				// newline; ignore such cases
				if ((file.fail() && !file.eof())) {
					println(cerr, "Invalid file: {}\n", f.path().filename().string());
					continue;
				}
				// Skip files containing less than 3 samples
				// Pulses can only occur over 3 samples so files with
				// any fewer can't have pulses anyways
				if (data.size() < 3) continue;
			}
			vector<int> smoothed;
			{
				StageTimer t{"smoothing"};
				ranges::transform(data, data.begin(), negate{});
				smoothed = vector<int>{data};
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
			{
				StageTimer t{"printing"};
				println("{}:", f.path().filename().string());
			}
			for (auto i = 0u; i < pulses.size() - 1; ++i) {
				const auto &it = pulses[i], &next = pulses[1 + i];
				const auto end = min<size_t>(it + params.width, next);
				{
					StageTimer t{"piggyback test"};
					// Check for piggyback
					if (next - it <= static_cast<size_t>(params.pulse_delta)) {
						const auto peakpos = ranges::max_element(span{&smoothed[it], &smoothed[end]});
						const auto thresh = params.drop_ratio * static_cast<double>(*peakpos);
						const auto nbelow = count_if(&*peakpos, &smoothed[next], [thresh](int n){ return n < thresh; });
						if (nbelow > params.below_drop_ratio) {
							{
								StageTimer t{"printing"};
								println("Found piggyback at {}", it);
							}
							continue;
						}
					}
				}
				// I would have used std::reduce() if the rubric didn't
				// explicitly call for std::accumulate()
				int a;
				{
					StageTimer t{"area"};
					a = accumulate(&data[it], &data[min(end, data.size())], 0);
				}
				{
					StageTimer t{"printing"};
					println("{} ({})", it, a);
				}
			}
			// Final peak cannot be piggyback by definition
			int a;
			{
				StageTimer t{"area"};
				a = accumulate(&data[pulses.back()], &data[min(pulses.back() + params.width, data.size())], 0);
			}
			{
				StageTimer t{"printing"};
				println("{} ({})\n", pulses.back(), a);
			}
		}
		print_stage_report();
	} catch (const ConfigFileError &e) {
		println(cerr, "{}", e.what());
		return 1;
	} catch (const exception &e) {
		println(cerr, "Unexpected error: {}", e.what());
		return 2;
	}
}
