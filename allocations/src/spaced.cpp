#include <vector>
#include <thread>
#include <algorithm>
#include <numeric>

void run_spaced(int nthreads, int njobs, int niter, int constant, std::vector<double>& output) {
    std::vector<std::thread> runners;
    runners.reserve(nthreads);
    constexpr int DOUBLES_PER_CACHE_LINE = CACHE_LINE_SIZE/sizeof(double);
    std::vector<double> common((DOUBLES_PER_CACHE_LINE + njobs) * nthreads);

    for (int t = 0; t < nthreads; ++t) {
        runners.emplace_back([&](int t){
            int start = t * njobs, end = start + njobs;
            auto ptr = common.data() + start + DOUBLES_PER_CACHE_LINE * t;
            std::iota(ptr, ptr + njobs, constant + start);
            for (int i = 0; i < niter; ++i) {
                for (int x = 0; x < njobs; ++x) {
                    ptr[x] += t; 
                }
            }
            std::copy_n(ptr, njobs, output.begin() + start);
        }, t);
    }

    for (auto& tt : runners) {
        tt.join();
    }
}
