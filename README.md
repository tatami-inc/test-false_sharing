# False sharing for blocked multi-threading 

## Introduction

When processing an array, we can split it up into contiguous blocks and assign each block to a thread.
However, if each thread is repeatedly writing to the block, false sharing could occur at the block boundaries.
To avoid this, we instruct each thread to write to its own buffer before copying it back to the original array.
The question is, does this make a difference?

## Setup

In the _naive_ approach, each thread directly modifies the values in its block across multiple iterations.
This is most likely to be subject to false sharing at block boundaries.

In the _separate_ approach, each thread allocates a temporary `std::vector` and modifies the values in that thread-specific buffer.
The final values are copied to the original array when all calculations are done.
This mitigates false sharing by allowing heap memory to be allocated in thread-specific arenas.

The _aligned_ approach is similar to the separate approach, except that the thread-specific buffers are aligned to a conservative estimate of the cache line size, i.e., 128 bytes.
This guarantees that no false sharing can occur.

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

Sepcifications:

- Apple M2 running Sonoma 14.6.1 with Apple clang 15.0.0

We run the test binary with `./build/sharing -n <BLOCK SIZE> -t 4`, using block sizes that aren't a power of 2 to avoid accidental alignment in the naive approach.
The milliseconds per operation is shown for each approach:

| block size | M2 naive | M2 separate | M2 aligned | Intel naive | Intel separate | Intel aligned |
|------------|----------|-------------|------------|-------------|----------------|---------------|
|         11 |        8 |           5 |          5 |             |                |               |
|         21 |       11 |           5 |          5 |             |                |               |
|         51 |       17 |           6 |          6 |             |                |               |
|        101 |       26 |          12 |         12 |             |                |               |
|        201 |       45 |          24 |         24 |             |                |               |
|        501 |      137 |          52 |         52 |             |                |               |
|       1001 |      292 |         105 |        105 |             |                |               |
