// This file was written by a clanker and I have made no modifications
// to it beyond this comment. I am morally opposed to the development
// and use of so-called "AI" (especially in an educational environment)
// and I am ONLY doing this because the assignment specifically requires
// me to.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	// Special edge-case test generation mode.
	if (argc == 3 && std::string(argv[1]) == "--edge") {
		try {
			const fs::path dir = argv[2];
			fs::create_directories(dir);

			// Empty file.
			{
				std::ofstream f(dir / "empty.dat");
			}

			// Exactly 5 values.
			{
				std::ofstream f(dir / "five.dat");
				f << "0 -10 15 -20 5\n";
			}

			// Exactly 7 values.
			{
				std::ofstream f(dir / "seven.dat");
				f << "0 -10 15 -20 5 -3 8\n";
			}

			// Invalid non-integer token.
			{
				std::ofstream f(dir / "invalid.dat");
				f << "0 -10 15 abc -20 5\n";
			}

			// Flat noise: no pulses.
			{
				std::mt19937 rng(12345);
				std::normal_distribution<double> noise(0.0, 20.0);
				std::ofstream f(dir / "flat.dat");

				for (int i = 0; i < 1000; ++i) {
					f << static_cast<int>(std::round(noise(rng)))
					  << '\n';
				}
			}

			std::cout << "Generated 5 edge-case files in "
			          << dir << '\n';
			return 0;

		} catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << '\n';
			return 1;
		}
	}

	// Normal generation mode.
	if (argc != 5) {
		std::cerr << "Usage: gen_data <out_dir> <num_files> "
		             "<samples_per_file> <seed>\n"
		             "       gen_data --edge <out_dir>\n";
		return 1;
	}

	try {
		const fs::path out_dir = argv[1];
		const int num_files = std::stoi(argv[2]);
		const int samples = std::stoi(argv[3]);
		const unsigned int seed =
			static_cast<unsigned int>(std::stoul(argv[4]));

		if (num_files <= 0 || samples < 100) {
			throw std::invalid_argument(
				"num_files must be positive and samples_per_file >= 100");
		}

		fs::create_directories(out_dir);

		std::mt19937 rng(seed);

		std::normal_distribution<double> noise(0.0, 20.0);
		std::uniform_int_distribution<int> amplitude(500, 5000);
		std::uniform_int_distribution<int> rise_dist(5, 10);
		std::uniform_int_distribution<int> decay_dist(30, 80);
		std::uniform_int_distribution<int> gap_dist(5, 15);
		std::uniform_int_distribution<int> spacing_dist(100, 300);
		std::uniform_int_distribution<int> pair_chance(0, 99);

		std::ofstream truth(out_dir / "truth.txt");
		if (!truth) {
			throw std::runtime_error("Cannot create truth.txt");
		}

		for (int file_num = 0; file_num < num_files; ++file_num) {
			std::vector<double> data(samples);

			// Generate baseline Gaussian noise.
			for (double& value : data) {
				value = noise(rng);
			}

			std::vector<int> starts;

			// Leave room for a complete pulse at the end.
			const int max_start = samples - 100;
			int position = 20;

			while (position < max_start) {
				// Generate a pulse.
				const int height = amplitude(rng);
				const int rise = rise_dist(rng);
				const int decay = decay_dist(rng);

				starts.push_back(position);

				// Hardware stores negated values:
				// pulses go downward.
				for (int i = 0; i < rise + decay; ++i) {
					const double pulse =
						(i < rise)
							? static_cast<double>(i + 1) / rise
							: std::exp(
								-3.0 * (i - rise + 1) / decay);

					const int index = position + i;
					if (index < samples) {
						data[index] -= height * pulse;
					}
				}

				position += rise + decay + spacing_dist(rng);

				// Occasionally add a piggyback pulse.
				if (position < max_start &&
				    pair_chance(rng) < 30) {

					position = starts.back() +
					           rise + decay + gap_dist(rng);

					const int second_height = amplitude(rng);
					const int second_rise = rise_dist(rng);
					const int second_decay = decay_dist(rng);

					starts.push_back(position);

					for (int i = 0;
					     i < second_rise + second_decay; ++i) {

						const double pulse =
							(i < second_rise)
								? static_cast<double>(i + 1) /
								      second_rise
								: std::exp(
									-3.0 * (i - second_rise + 1) /
									second_decay);

						const int index = position + i;
						if (index < samples) {
							data[index] -= second_height * pulse;
						}
					}

					position += second_rise + second_decay +
					            spacing_dist(rng);
				}
			}

			// Write the data file.
			const fs::path filename =
				out_dir / ("data_" + std::to_string(file_num) + ".dat");

			std::ofstream output(filename);
			if (!output) {
				throw std::runtime_error(
					"Cannot create " + filename.string());
			}

			for (int i = 0; i < samples; ++i) {
				output << static_cast<long long>(
					std::llround(data[i])) << '\n';
			}

			// Record ground-truth pulse positions.
			truth << filename.filename().string() << ":";
			for (int start : starts) {
				truth << " " << start;
			}
			truth << '\n';

			std::cout << "Generated " << filename << " ("
			          << starts.size() << " pulses)\n";
		}

		std::cout << "Truth file written to "
		          << (out_dir / "truth.txt") << '\n';

	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << '\n';
		return 1;
	}

	return 0;
}
