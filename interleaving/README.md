# False sharing for interleaved stores

## Overview

Another common source of false sharing occurs when we want to write to a shared output array.
Typically each thread is responsible for writing to a single contiguous block of the output array,
such that false sharing can occur at shared cache lines across the block boundaries between threads.
In the vast majority of cases, this is not a concern as (i) the time should be dominated by the computation,
and (ii) the array is usually large enough that we can ignore the small proportion of cache lines that might be affected by false sharing.
Even if the array is small, execution should be so fast that any relatively high penalty from false sharing would be negligible on an absolute scale.

In some situations, each thread needs to write to multiple interleaved blocks of the output array.
For example, when performing a matrix multiplication, we might parallelize by splitting the LHS matrix into blocks of rows.
Each thread receives a block of rows, computes the product of that submatrix, and writes the result into an output array.
If the output array is column-major, each thread would have to write to multiple blocks, one per RHS column.
Such interleaving of the output introduces many boundaries between threads that increases the potential for false sharing.

This test suite measures the effect of false sharing by performing interleaved stores across increasing numbers of threads.
Specifically, it simulates a highly pathological scenario where the output array represents a double-precision column-major matrix with 8 rows.
Each thread is assigned a set of rows and will fill one row at a time; thus, all threads could be storing in the same cache line at once.

## Build instructions

To compile:

```console
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Results

Let's try with different numbers of columns in our output array.
All timings below are obtained on an Intel i7-8850H.

```console
$ ./build/interleaved -l 1000
serial              : 6.50728e-05 ± 8.69724e-06
parallel (2)        : 7.70021e-05 ± 1.16117e-05
parallel (4)        : 0.000120064 ± 1.7309e-05
parallel (8)        : 0.000212086 ± 3.09641e-05

$ ./build/interleaved -l 10000
serial              : 0.000202022 ± 9.44015e-06
parallel (2)        : 0.000171811 ± 1.27947e-05
parallel (4)        : 0.000197504 ± 1.01668e-05
parallel (8)        : 0.000329123 ± 2.15688e-05

$ ./build/interleaved -l 100000
serial              : 0.00388137 ± 0.000695538
parallel (2)        : 0.00159486 ± 2.77983e-05
parallel (4)        : 0.00120446 ± 1.68074e-05
parallel (8)        : 0.00087332 ± 4.70172e-05

$ ./build/interleaved -l 1000000
serial              : 0.0544323 ± 0.000350188
parallel (2)        : 0.0315513 ± 0.000270571
parallel (4)        : 0.0149194 ± 9.31318e-05
parallel (8)        : 0.00772734 ± 0.000122761
```

For a low number of columns, we see that increasing the number of threads will degrade performance, consistent with false sharing.
However, as the number of columns increases, a higher number of threads actually improves performance.
I think this is because, after some initial chaos, the threads get out of each other's way. 
Specifically, one thread obtains an exclusive lock on the cache lines it wants to write to and races ahead;
this delays the next thread, which will now always be a little bit behind and thus not have to contend for the locks of the first thread;
and so on for all the threads, akin to a staggered start.

## Conclusion

False sharing is a problem for interleaved writes to an output array, but only when the output array itself is small.
And frankly, if it's that small, the absolute time is negligible and can be ignored.
