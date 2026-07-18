#include "eztimer/eztimer.hpp"

#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <thread>
#include <algorithm>
#include <vector>
#include <functional>

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
    const std::size_t output_size = njobs * nthreads;

    std::vector<std::function<bool()> > funs;
    funs.reserve(6);
    std::vector<std::string> names;
    names.reserve(6);
    std::vector<double*> payload;

    std::vector<double> naive(output_size);
    payload.push_back(naive.data());
    names.push_back("naive"); 
    funs.emplace_back([&]() -> bool {
        run_naive(nthreads, njobs, niter, CONSTANT, naive);
        return true;
    });

    std::vector<double> spaced(output_size);
    payload.push_back(spaced.data());
    names.push_back("spaced"); 
    funs.emplace_back([&]() -> bool {
        run_spaced(nthreads, njobs, niter, CONSTANT, spaced);
        return true;
    });

    std::vector<double> aligned(output_size);
    payload.push_back(aligned.data());
    names.push_back("aligned");
    funs.emplace_back([&]() -> bool {
        run_aligned(nthreads, njobs, niter, CONSTANT, aligned);
        return true;
    });

    std::vector<double> new_aligned(output_size);
    payload.push_back(new_aligned.data());
    names.push_back("new-aligned");
    funs.emplace_back([&]() -> bool {
        run_new_aligned(nthreads, njobs, niter, CONSTANT, new_aligned);
        return true;
    });

    std::vector<double> new_vector(output_size);
    payload.push_back(new_vector.data());
    names.push_back("new-vector");
    funs.emplace_back([&]() -> bool {
        run_new_vector(nthreads, njobs, niter, CONSTANT, new_vector);
        return true;
    });

    const double* optr = NULL;
    eztimer::Options opt;
    opt.setup = [&]() -> void {
        optr = NULL;
        for (auto ptr : payload) {
            std::fill_n(ptr, output_size, 0);
        }
    };

    auto res = eztimer::time<bool>(
        funs,
        [&](const bool, std::size_t i) -> void {
            if (optr != NULL) {
                const auto target = payload[i];
                for (size_t o = 0; o < output_size; ++o) {
                    if (optr[o] != target[o]) {
                        throw std::runtime_error("oops that's not right");
                    }
                }
            } else {
                optr = payload[i];
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

    return 0;
}
