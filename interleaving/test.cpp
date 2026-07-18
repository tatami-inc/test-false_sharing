#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <thread>
#include <algorithm>
#include <vector>
#include <iostream>

#include "eztimer/eztimer.hpp"

constexpr int limit = 8;

double parallel_store(int njobs, double* results, int nthreads) {
    std::vector<std::thread> threads;
    threads.reserve(nthreads);
    for (int t = 0; t < nthreads; ++t) {
        threads.emplace_back([&](int t) -> void {
            const int stride = limit / nthreads;
            const int start = stride * t, end = start + stride;
            for (int x = start; x < end; ++x) {
                for (int j = 0; j < njobs; ++j) {
                    results[x + j * limit] = j;
                }
            }
        }, t);
    }
    for (auto& th : threads) {
        th.join();
    }
    return results[0] + results[njobs * limit - 1];
}

int main(int argc, char* argv[]) {
    CLI::App app{"Interleaved stores"};
    int njobs;
    app.add_option("-l,--length", njobs, "Array length")->default_val(10000);
    CLI11_PARSE(app, argc, argv);

    std::vector<std::function<double()> > funs;
    std::vector<std::string> names;
    funs.reserve(4);
    names.reserve(4);

    auto results1 = new (std::align_val_t(CACHE_LINE_SIZE)) double[njobs * limit];
    names.push_back("serial");
    funs.push_back([&]() -> double {
        // We still use parallel_store() to make sure that we include the thread start-up cost.
        // Otherwise we're not purely measuring the effect of false sharing.
        return parallel_store(njobs, results1, 1);
    });

    auto results2 = new (std::align_val_t(CACHE_LINE_SIZE)) double[njobs * limit];
    names.push_back("parallel (2)");
    funs.push_back([&]() -> double {
        return parallel_store(njobs, results2, 2);
    });

    auto results4 = new (std::align_val_t(CACHE_LINE_SIZE)) double[njobs * limit];
    names.push_back("parallel (4)");
    funs.push_back([&]() -> double {
        return parallel_store(njobs, results4, 4);
    });

    auto results8 = new (std::align_val_t(CACHE_LINE_SIZE)) double[njobs * limit];
    names.push_back("parallel (8)");
    funs.push_back([&]() -> double {
        return parallel_store(njobs, results8, 8);
    });

    eztimer::Options opt;
    opt.setup = [&]() -> void {
        std::fill_n(results1, njobs, 0);
        std::fill_n(results2, njobs, 0);
        std::fill_n(results4, njobs, 0);
        std::fill_n(results8, njobs, 0);
    };

    std::optional<double> expected;
    auto res = eztimer::time<double>(
        funs,
        [&](const double& res, std::size_t i) -> void {
            //std::cout << res << "\t" << i << std::endl;
            if (expected.has_value()) {
                if (*expected != res) {
                    throw std::runtime_error("oops that's not right");
                }
            } else {
                expected = res;
            }
        },
        opt
    );

    const std::size_t num_methods = names.size();
    for (std::size_t m = 0; m < num_methods; ++m) {
        auto name = names[m];
        constexpr std::size_t pad_to = 20;
        if (name.size() < pad_to) {
            name.insert(name.end(), pad_to - name.size(), ' ');
        }
        std::cout << name << ": " << res[m].mean.count() << " ± " << res[m].sd.count() / std::sqrt(res[m].times.size()) << std::endl;
    }

    ::operator delete [](results1, std::align_val_t(CACHE_LINE_SIZE));
    ::operator delete [](results2, std::align_val_t(CACHE_LINE_SIZE));
    ::operator delete [](results4, std::align_val_t(CACHE_LINE_SIZE));
    ::operator delete [](results8, std::align_val_t(CACHE_LINE_SIZE));
    return 0;
}
