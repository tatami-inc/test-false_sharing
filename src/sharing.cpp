#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <thread>
#include <algorithm>
#include <vector>

void run_naive(int, int, int, int, std::vector<double>&);
void run_spaced(int, int, int, int, std::vector<double>&);
void run_aligned(int, int, int, int, std::vector<double>&);
void run_new_aligned(int, int, int, int, std::vector<double>&);
void run_new_vector(int, int, int, int, std::vector<double>&);

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
        run_naive(nthreads, njobs, niter, CONSTANT, naive);
    });

    std::vector<double> spaced(njobs * nthreads);
    ankerl::nanobench::Bench().run("spaced", [&](){
        run_spaced(nthreads, njobs, niter, CONSTANT, spaced);
    });

    std::vector<double> aligned(njobs * nthreads);
    ankerl::nanobench::Bench().minEpochIterations(10).run("aligned", [&](){
        run_aligned(nthreads, njobs, niter, CONSTANT, aligned);
    });

    std::vector<double> new_aligned(njobs * nthreads);
    ankerl::nanobench::Bench().run("new-aligned", [&](){
        run_new_aligned(nthreads, njobs, niter, CONSTANT, new_aligned);
    });

    std::vector<double> new_vector(njobs * nthreads);
    ankerl::nanobench::Bench().run("new-vector", [&](){
        run_new_vector(nthreads, njobs, niter, CONSTANT, new_vector);
    });

    for (size_t i = 0; i < naive.size(); ++i) {
        if (naive[i] != new_vector[i] || aligned[i] != new_vector[i] || spaced[i] != new_vector[i] || aligned[i] != new_vector[i]) {
            throw std::runtime_error("oops");
        }
    }

    return 0;
}
