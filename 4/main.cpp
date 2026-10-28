// main.cpp -- CS 3370 Assignment: Validated Config Loader
// (Given to students as-is; they write pulse_config.cpp.)
#include "pulse_config.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file.ini>\n";
        return 1;
    }
    try {
        PulseConfig config{argv[1]};
        const PulseParams& p = config.params();
        std::cout << "Loaded " << argv[1] << '\n'
                  << "vt=" << p.vt << '\n'
                  << "width=" << p.width << '\n'
                  << "pulse_delta=" << p.pulse_delta << '\n'
                  << "drop_ratio=" << p.drop_ratio << '\n'
                  << "below_drop_ratio=" << p.below_drop_ratio
                  << '\n';
    } catch (const ConfigFileError& e) {      // our errors first
        std::cerr << e.what() << '\n';
        return 1;
    } catch (const std::exception& e) {       // anything else last
        std::cerr << "Unexpected error: " << e.what() << '\n';
        return 2;
    }
}
