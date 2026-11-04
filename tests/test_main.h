#pragma once
#include <cstdio>
#include <string>
#include <vector>

struct Test {
    const char* name;
    int (*fn)();
};

// Runs all tests; returns the number of failures.
inline int run_all(const std::vector<Test>& tests) {
    int failed = 0;
    for (auto& t : tests) {
        int r = t.fn();
        if (r == 0)
            std::printf("  PASS  %s\n", t.name);
        else {
            std::printf("  FAIL  %s (code %d)\n", t.name, r);
            ++failed;
        }
    }
    std::printf("%zu tests, %d failed\n", tests.size(), failed);
    return failed ? 1 : 0;
}
