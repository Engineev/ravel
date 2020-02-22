# ravel
master: [![Build Status](https://travis-ci.com/Engineev/ravel.svg?token=t7LhMb4BZCM8Q58kCnsH&branch=master)](https://travis-ci.com/Engineev/ravel)

dev: [![Build Status](https://travis-ci.com/Engineev/ravel.svg?token=t7LhMb4BZCM8Q58kCnsH&branch=dev)](https://travis-ci.com/Engineev/ravel)

## Introduction

**ravel** is a RISC-V simulator written for the compiler course taught at 
Shanghai Jiao Tong University, where the students are typically second-year
undergraduates at ACM Honers Class. In this course, students are required to
implement a toy compiler that turn Mx* (a toy language used in the course) to
RISC-V assembly. This simulator is used to test the correctness of the 
implementation and measure the quality of the optimization.  

## Getting started
The only prerequisites are CMake (>= 3.10) and a cpp-17 supporting compiler.
You can install the simulator with the following commands.
```shell script
git clone https://github.com/Engineev/ravel.git
cd ravel
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/opt  # or any directory you want
make 
make install
export PATH="/usr/local/opt/bin:$PATH"  # optional
```

The easiest way to start a simulation is to use the OJ mode, that is,
```shell script
ravel --oj-mode
```
Under this mode, the simulator takes `test.s` and `builtin.s` as the source
code, `test.in` as the input and redirect the program output into `test.out`.
The results (e.g. exit code) of the simulation will still be outputted directly
to `stdout`.

## Support

In short, if the assembly resembles the one generated with the following 
command,
```shell script
riscv32-unknown-linux-gnu-gcc -S -std=c99 -msmall-data-limit=0 -fno-section-anchors main.c
```
where the build of `gcc` is configured with
```shell script
./configure --prefix=/opt/riscv --with-arch=rv32ima --with-abi=ilp32
```
then in most cases it is supported by the simulator.
See [this](https://github.com/riscv/riscv-gnu-toolchain) for information on the
risc-v toolchain. For detail on supported directives, instructions and libc
functions, see [this](./doc/support.md).  



