#include <vector>
#include <thread>
#include <algorithm>
#include <numeric>

void run_new_vector(int nthreads, int njobs, int niter, int constant, std::vector<double>& output) {
    std::vector<std::thread> runners;
    runners.reserve(nthreads);

    for (int t = 0; t < nthreads; ++t) {
        runners.emplace_back([&](int t){
            int start = t * njobs, end = start + njobs;
            std::vector<double> payload(njobs);
            auto ptr = payload.data();
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
