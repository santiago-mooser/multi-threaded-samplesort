# Multithreaded C implementation of the Samplesort algorithm

## Introduction

This is a proof-of-concept implementation of the samplesort algorithm using C. It is simply used to demonstrate my current ability to code in C and to do basic memory management.

This project was written as a programming assignment as part of The University of Hong Kong's Principles of Operating Systems course (COMP3230). The aim of hte assignment was to teach students how to use threading, memory management and synchronization primitives such as mutex locks, condition variables and semaphores to coordinate between threads.

You can find more information about the assignment in the file [2022-Programming-Ass2.pdf](./2022-Programming-Ass2.pdf)

## Compiling the binary

A simple gcc command should suffice:

```bash
gcc -o psort psort.c -lpthread
```


## Benchmarking the binary

You can use the included bash script [benchmark.sh](./benchmark.sh) to benchmark the compiled binary. It will run the script with different numbers of threads and different sizes of input arrays (which are automaticall generated) and output the time taken for each run.

An excel file with the results is included already: [benchmark.xlsx](./benchmark.xlsx)