# False sharing performance testing

This repository contains a few programs to test the performance degradation from [false sharing](https://docs.kernel.org/kernel-hacking/false-sharing.html).
We are mostly interested in false sharing in the context of `tatami::parallelize()`.

- [`allocations`](allocations/) tests a bunch of different mitigations based on aligned or per-thread memory allocations.
- [`interleaving`](interleaving/) tests the effect of false sharing in the presence of highly interleaved arrays.
