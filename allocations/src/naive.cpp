#include <vector>
#include <thread>
#include <algorithm>
#include <numeric>

void run_naive(int nthreads, int njobs, int niter, int constant, std::vector<double>& output) {
    std::vector<std::thread> runners;
    runners.reserve(nthreads);
    for (int t = 0; t < nthreads; ++t) {
        runners.emplace_back([&](int t){
            int start = t * njobs, end = start + njobs;
            std::iota(output.begin() + start, output.begin() + end, constant + start);
            for (int i = 0; i < niter; ++i) {
                for (int x = start; x < end; ++x) {
                    output[x] += t; // do a very simple modification so that we're mostly memory-bound.
                }
            }
        }, t);
    }

    for (auto& tt : runners) {
        tt.join();
    }
}
