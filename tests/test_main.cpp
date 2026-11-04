// Test runner: collects tests from test_config.cpp and test_library.cpp.
#include "test_main.h"

#include <cstdio>

// Defined in the individual test files.
std::vector<Test> config_tests();
std::vector<Test> library_tests();

int main() {
    std::vector<Test> all;
    for (auto& t : config_tests()) all.push_back(t);
    for (auto& t : library_tests()) all.push_back(t);
    return run_all(all);
}
