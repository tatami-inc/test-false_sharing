#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <thread>
#include <algorithm>
#include <vector>

int main(int argc, char* argv[]) {
    CLI::App app{"Expanded testing checks"};
    int niter;
    app.add_option("-i,--iter", niter, "Number of iterations")->default_val(1000000);
    int nthreads;
    app.add_option("-t,--thread", nthreads, "Number of threads")->default_val(4);
    int njobs;
    app.add_option("-n,--number", njobs, "Number of jobs per thread")->default_val(13);
    CLI11_PARSE(app, argc, argv);

    // Using a constant so that the data aren't known at compiler time, avoid
    // an overly smart compiler from optimizing out everything.
    const int CONSTANT = niter * nthreads + njobs;

    std::vector<double> shared(njobs * nthreads);
    ankerl::nanobench::Bench().run("naive", [&](){
        std::vector<std::thread> runners;
        runners.reserve(nthreads);
        for (int t = 0; t < nthreads; ++t) {
            runners.emplace_back([&](int t){
                int start = t * njobs, end = start + njobs;
                std::iota(shared.begin() + start, shared.begin() + end, CONSTANT + start);
                for (int i = 0; i < niter; ++i) {
                    for (int x = start; x < end; ++x) {
                        shared[x] += t; // do a very simple modification so that we're mostly memory-bound.
                    }
                }
            }, t);
        }

        for (auto& tt : runners) {
            tt.join();
        }
    });

    std::vector<double> separate(njobs * nthreads);
    ankerl::nanobench::Bench().run("separate", [&](){
        std::vector<std::thread> runners;
        runners.reserve(nthreads);

        for (int t = 0; t < nthreads; ++t) {
            runners.emplace_back([&](int t){
                int start = t * njobs, end = start + njobs;
                std::vector<double> payload(njobs);
                auto ptr = payload.data();
                std::iota(ptr, ptr + njobs, CONSTANT + start);
                for (int i = 0; i < niter; ++i) {
                    for (int x = 0; x < njobs; ++x) {
                        ptr[x] += t; 
                    }
                }
                std::copy_n(ptr, njobs, separate.begin() + start);
            }, t);
        }

        for (auto& tt : runners) {
            tt.join();
        }
    });

    std::vector<double> aligned(njobs * nthreads);
    ankerl::nanobench::Bench().run("aligned", [&](){
        std::vector<std::thread> runners;
        runners.reserve(nthreads);

        for (int t = 0; t < nthreads; ++t) {
            runners.emplace_back([&](int t){
                int start = t * njobs, end = start + njobs;
                auto ptr = new (std::align_val_t(128)) double[njobs];
                std::iota(ptr, ptr + njobs, CONSTANT + start);
                for (int i = 0; i < niter; ++i) {
                    for (int x = 0; x < njobs; ++x) {
                        ptr[x] += t;
                    }
                }
                std::copy_n(ptr, njobs, aligned.begin() + start);
                delete [] ptr;
            }, t);
        }

        for (auto& tt : runners) {
            tt.join();
        }
    });

    for (size_t i = 0; i < shared.size(); ++i) {
        if (shared[i] != separate[i] || aligned[i] != separate[i]) {
            throw std::runtime_error("oops");
        }
    }

    return 0;
}
