#include <algorithm> // max_element
#include <cstddef> // size_t
#include <exception> // exception
#include <filesystem> // directory_iterator
#include <fstream>
#include <functional> // negate
#include <iostream> // cerr
#include <iterator> // istream_iterator
#include <numeric> // accumulate
#include <optional>
#include <span>
#include <vector>
#include "pulse_config.h"

using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " <file.ini>\n";
		return 1;
	}
	try {
		const PulseConfig config{argv[1]};
		const PulseParams &params = config.params();
		for (const auto &f : filesystem::directory_iterator{"."}) {
			if (!f.is_regular_file() || f.path().extension() != ".dat") continue;
			ifstream file{f.path(), ios::in};
			vector<int> data{istream_iterator<int>{file}, {}};
			// Failbit gets set if there happens to be a trailing
			// newline; ignore such cases
			if (file.fail() && !file.eof()) {
				cerr << "Invalid file: " << f.path().filename().string() << "\n\n";
				continue;
			}
			ranges::transform(data, data.begin(), negate{});
			vector<int> smoothed{data};
			for (auto i = 3u; i < data.size() - 3; ++i) {
				smoothed[i] = (3 * (data[i - 1] + data[i] + data[i + 1])
					+ 2 * (data[i - 2] + data[i + 2])
					+ data[i - 3] + data[i + 3])
					/ 15;
			}
			// Detect pulses
			vector<size_t> pulses{};
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
			cout << f.path().filename().string() << ":\n";
			for (auto i = 0u; i < pulses.size() - 1; ++i) {
				const auto &it = pulses[i], &next = pulses[1 + i];
				const auto end = min<size_t>(it + params.width, next);
				// Check for piggyback
				if (next - it <= static_cast<size_t>(params.pulse_delta)) {
					const auto peakpos = ranges::max_element(span{&smoothed[it], &smoothed[end]});
					const auto thresh = params.drop_ratio * static_cast<double>(*peakpos);
					const auto nbelow = count_if(&*peakpos, &smoothed[next], [thresh](int n){ return n < thresh; });
					if (nbelow > params.below_drop_ratio) {
						cout << "Found piggyback at " << it << '\n';
						continue;
					}
				}
				// I would have used std::reduce() if the rubric didn't
				// explicitly call for std::accumulate()
				const auto a = accumulate(&data[it], &data[min(end, data.size())], 0);
				cout << it << " (" << a << ")\n";
			}
			// Final peak cannot be piggyback by definition
			const auto a = accumulate(&data[pulses.back()], &data[min(pulses.back() + params.width, data.size())], 0);
			cout << pulses.back() << " (" << a << ")\n\n";
		}
	} catch (const ConfigFileError &e) {
		cerr << e.what() << '\n';
		return 1;
	} catch (const exception &e) {
		cerr << "Unexpected error: " << e.what() << '\n';
		return 2;
	}
}
