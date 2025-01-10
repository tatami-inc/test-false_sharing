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
- In the _spaced_ approach, we create a common buffer that has spacing between blocks equal to the cache line (here, assumed to be 128 bytes).
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
