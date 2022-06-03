# ravel

| master | dev|
|---     |--- |
|[![Build Status](https://travis-ci.com/Engineev/ravel.svg?token=t7LhMb4BZCM8Q58kCnsH&branch=master)](https://travis-ci.com/Engineev/ravel) |[![Build Status](https://travis-ci.com/Engineev/ravel.svg?token=t7LhMb4BZCM8Q58kCnsH&branch=dev)](https://travis-ci.com/Engineev/ravel)


## Introduction

**ravel** is a RISC-V simulator written for the compiler course taught at 
Shanghai Jiao Tong University, and the students in this course are usually second-year
undergraduates at ACM Honers Class. In this course, students are required to
implement a toy compiler that turns Mx* (a toy language used in the course) to
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
to `stdout`. This is equivalent to
```shell script
ravel --input-file=test.in --output-file=test.out test.s builtin.s
# By default, cache is disabled. You can use --enable-cache to turn it on.
```

If you'd like to see the instructions being executed, you may pass in command
line option `--print-instructions`, but note that this will significantly slow down the 
simulation. Also, if `--keep-debug-info` is passed in, **ravel** will perform more checks on 
memory accesses and will print additional information like the call stack when an error occurred.

## Ravel as a static library
It's possible to use **ravel** as a static library. In fact, `make insatll` will also install the library 
`libravel-sim.a` into `${CMAKE_INSTALL_PREFIX}/lib` and the corresponding headers into 
`${CMAKE_INSTALL_PREFIX}/include`. See [./test/test-ravel-sim.cpp](./test/test-ravel-sim.cpp) for a minimal example.
If you installed **ravel** into `/usr/local/opt`, then you can build the test with
```shell script
g++ -std=c++17 -I/usr/local/opt/include -L/usr/local/lib/ -lravel-sim ./test/test-ravel-sim.cpp
```


## Support

In short, if the assembly resembles the one generated with the following 
command,
```shell script
riscv32-unknown-linux-gnu-gcc -S -std=c99 -fno-section-anchors main.c
```
where the build of `gcc` is configured with
```shell script
./configure --prefix=/opt/riscv --with-arch=rv32ima --with-abi=ilp32
```
then in most cases it is supported by the simulator.

[comment]: <> (For LLVM users, the command should be )

[comment]: <> (```shell script)

[comment]: <> (llc --march=riscv32 --mattr=+m main.ll)

[comment]: <> (```)
See [this](https://github.com/riscv/riscv-gnu-toolchain) for information on the
risc-v toolchain. For detail on supported directives, instructions and libc
functions, see [doc/support.md](./doc/support.md).  

## Computing the running time
The output of **ravel** contains a `time` filed. This is computed in the 
following way. For each type of instructions, the number of execution is 
recorded during the interpretation, and `time` is computed by a weighted 
summation. The default weights are listed in the following table.
You can change the weights by passing in command line options like 
`-wsimple=2`. By default, cache is disabled. You may enable it by passing in
`--enable-cache`. 

| Type   | Weight |
|---     |---     |
|simple  | 1
|cache   | 4
|mul     | 4
|br      | 8
|div     | 8
|mem     | 64
|libcIO  | 64
|libcMem | function-dependent

Note: Unconditional jumps are viewed as simple instructions.

## Pronunciation

This project is named after the French composer Maurice Ravel, so the most correct way to pronounce the project name
in English is /rəˈvɛl/ or /ræˈvɛl/, though I'm also OK with the pronunciation /ˈravəl/. 

In any case, how about checking this colorful [piano concerto](https://youtu.be/cJOW5mlhH_Y) by Ravel?

