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

    std::vector<double> naive(njobs * nthreads);
    ankerl::nanobench::Bench().run("naive", [&](){
        std::vector<std::thread> runners;
        runners.reserve(nthreads);
        for (int t = 0; t < nthreads; ++t) {
            runners.emplace_back([&](int t){
                int start = t * njobs, end = start + njobs;
                std::iota(naive.begin() + start, naive.begin() + end, CONSTANT + start);
                for (int i = 0; i < niter; ++i) {
                    for (int x = start; x < end; ++x) {
                        naive[x] += t; // do a very simple modification so that we're mostly memory-bound.
                    }
                }
            }, t);
        }

        for (auto& tt : runners) {
            tt.join();
        }
    });

    constexpr int CACHE_LINE_SIZE = 128;
    constexpr int DOUBLES_PER_LINE = CACHE_LINE_SIZE / sizeof(double);

    std::vector<double> spaced(njobs * nthreads);
    ankerl::nanobench::Bench().run("spaced", [&](){
        std::vector<std::thread> runners;
        runners.reserve(nthreads);
        std::vector<double> common((DOUBLES_PER_LINE + njobs) * nthreads);

        for (int t = 0; t < nthreads; ++t) {
            runners.emplace_back([&](int t){
                int start = t * njobs, end = start + njobs;
                auto ptr = common.data() + start + DOUBLES_PER_LINE * t;
                std::iota(ptr, ptr + njobs, CONSTANT + start);
                for (int i = 0; i < niter; ++i) {
                    for (int x = 0; x < njobs; ++x) {
                        ptr[x] += t; 
                    }
                }
                std::copy_n(ptr, njobs, spaced.begin() + start);
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
        std::vector<double> common((DOUBLES_PER_LINE + njobs) * nthreads);
        size_t used = 0;

        for (int t = 0; t < nthreads; ++t) {
            auto common_ptr_raw = reinterpret_cast<void*>(common.data() + used);
            size_t available = (common.size() - used) * sizeof(double);
            std::align(CACHE_LINE_SIZE, sizeof(double), common_ptr_raw, available);
            auto common_ptr = reinterpret_cast<double*>(common_ptr_raw);
            used = (common_ptr - common.data()) + njobs;

            runners.emplace_back([&](int t, double* ptr){
                int start = t * njobs, end = start + njobs;
                std::iota(ptr, ptr + njobs, CONSTANT + start);
                for (int i = 0; i < niter; ++i) {
                    for (int x = 0; x < njobs; ++x) {
                        ptr[x] += t; 
                    }
                }
                std::copy_n(ptr, njobs, aligned.begin() + start);
            }, t, common_ptr);
        }

        for (auto& tt : runners) {
            tt.join();
        }
    });

    std::vector<double> new_aligned(njobs * nthreads);
    ankerl::nanobench::Bench().run("new-aligned", [&](){
        std::vector<std::thread> runners;
        runners.reserve(nthreads);

        for (int t = 0; t < nthreads; ++t) {
            runners.emplace_back([&](int t){
                int start = t * njobs, end = start + njobs;
                auto ptr = new (std::align_val_t(CACHE_LINE_SIZE)) double[njobs];
                std::iota(ptr, ptr + njobs, CONSTANT + start);
                for (int i = 0; i < niter; ++i) {
                    for (int x = 0; x < njobs; ++x) {
                        ptr[x] += t;
                    }
                }
                std::copy_n(ptr, njobs, new_aligned.begin() + start);
                delete [] ptr;
            }, t);
        }

        for (auto& tt : runners) {
            tt.join();
        }
    });

    std::vector<double> new_vector(njobs * nthreads);
    ankerl::nanobench::Bench().run("new-vector", [&](){
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
                std::copy_n(ptr, njobs, new_vector.begin() + start);
            }, t);
        }

        for (auto& tt : runners) {
            tt.join();
        }
    });

    for (size_t i = 0; i < naive.size(); ++i) {
        if (naive[i] != new_vector[i] || aligned[i] != new_vector[i] || spaced[i] != new_vector[i] || aligned[i] != new_vector[i]) {
            throw std::runtime_error("oops");
        }
    }

    return 0;
}
