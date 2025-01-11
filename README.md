# False sharing for blocked multi-threading 

## Introduction

When processing an array, we can split it up into contiguous blocks and assign each block to a thread.
However, if each thread is repeatedly writing to the block, false sharing could occur at the block boundaries.
To avoid this, we instruct each thread to write to its own buffer before copying it back to the original array.
The question is, does this make a difference?

## Setup

We try a few different approaches to characterize the performance of our system:

- In the _naive_ approach, each thread directly modifies the values in its block across multiple iterations.
  This is most likely to be subject to false sharing at block boundaries, provided that the blocks are not inadvertently aligned to cache lines.
- In the _spaced_ approach, we create a common buffer that has spacing between blocks equal to the cache line size
  ([128 bytes for Macs, 64 bytes for x86-64](https://lemire.me/blog/2023/12/12/measuring-the-size-of-the-cache-line-empirically/)).
  This should avoid any false sharing as threads can never write to the same cache line.
- In the _aligned_ approach, we create a common buffer where each block is aligned to the next cache line boundary.
  This should also avoid any false sharing as well as checking the effect of aligned memory access.
- In the _new-aligned_ approach, each thread allocates a buffer that is aligned to the cache line size and modifies the values in that thread-specific buffer.
  This is similar to the _aligned_ approach but also checks for potential optimizations when the compiler knows that memory is local to a thread.
- In the _new-vector_ approach, each thread allocates a temporary `std::vector` and modifies the values in that thread-specific vector.
  This hopes to mitigate false sharing by allowing heap memory to be allocated in thread-specific arenas.

To compile:

```console
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

We can now test all approaches with different sizes of the blocks (i.e., jobs per thread) as well as varying numbers of threads. 

```console
# block size of 1000 with 4 threads
./build/sharing -n 1000 -t 4
```

## Results

We run the test binary with `./build/sharing -n <BLOCK SIZE> -t 4`, using block sizes that aren't a power of 2.
This avoids unintended alignment to cache lines in the naive approach, which would eliminate false sharing.
The milliseconds per operation is shown for each approach in the tables below.

First, on an Apple M2 running Sonoma 14.6.1 with Apple clang 15.0.0:

| block size | naive | spaced | aligned | new-aligned | new-vector | 
|------------|-------|--------|---------|-------------|------------|
|         11 |     8 |      5 |       5 |           4 |          4 |
|         21 |    11 |      5 |       5 |           5 |          5 |
|         51 |    17 |      9 |       7 |           7 |          7 |
|        101 |    26 |     14 |      12 |          12 |         12 |
|        201 |    42 |     31 |      23 |          24 |         23 |
|        501 |   136 |     95 |      51 |          51 |         52 |
|       1001 |   292 |    173 |     105 |         103 |        105 |
|       2001 |   388 |    327 |     207 |         207 |        209 |

Then on Intel i7 running Ubuntu 22.04 with GCC 11.4.0:

| block size | naive | spaced | aligned | new-aligned | new-vector | 
|------------|-------|--------|---------|-------------|------------|
|         11 |    13 |      2 |       2 |           2 |          2 |
|         21 |    18 |      2 |       2 |           2 |          2 |
|         51 |    28 |     13 |       8 |           5 |          5 |
|        101 |    49 |     16 |      12 |           9 |          9 |
|        201 |    61 |     26 |      17 |          16 |         16 |
|        501 |    78 |     42 |      42 |          42 |         41 |
|       1001 |   159 |     83 |      92 |          81 |         80 |
|       2001 |   226 |    168 |     158 |         157 |        156 |

These results show that false sharing is responsible for a considerable performance loss, based on the differences in timings between `naive` and `spaced`.
Some additional performance is achieved by aligning to the cache line boundaries, possibly from aligned SIMD loads.

Interestingly, the new-vector approach is good in all scenarios despite the lack of any guarantees with respect to false sharing or alignment.
This is a nice outcome as it implies that idiomatic C++ - namely, the creation of a vector in thread's scope - is performant.
It is also a reminder to not be overly prescriptive as this can block compiler optimizations.
