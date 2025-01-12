#include <vector>
#include <thread>
#include <algorithm>
#include <numeric>
#include <memory>

void run_aligned(int nthreads, int njobs, int niter, int constant, std::vector<double>& output) {
    std::vector<std::thread> runners;
    runners.reserve(nthreads);
    constexpr int DOUBLES_PER_CACHE_LINE = CACHE_LINE_SIZE/sizeof(double);
    std::vector<double> common((DOUBLES_PER_CACHE_LINE + njobs) * nthreads);
    size_t used = 0;

    for (int t = 0; t < nthreads; ++t) {
        auto common_ptr_raw = reinterpret_cast<void*>(common.data() + used);
        size_t available = (common.size() - used) * sizeof(double);
        std::align(CACHE_LINE_SIZE, sizeof(double), common_ptr_raw, available);
        auto common_ptr = reinterpret_cast<double*>(common_ptr_raw);
        used = (common_ptr - common.data()) + njobs;

        runners.emplace_back([&](int t, double* ptr){
            int start = t * njobs, end = start + njobs;
            std::iota(ptr, ptr + njobs, constant + start);
            for (int i = 0; i < niter; ++i) {
                for (int x = 0; x < njobs; ++x) {
                    ptr[x] += t; 
                }
            }
            std::copy_n(ptr, njobs, output.begin() + start);
        }, t, common_ptr);
    }

    for (auto& tt : runners) {
        tt.join();
    }
}
