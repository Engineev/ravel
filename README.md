# ravel
master: [![Build Status](https://travis-ci.com/Engineev/ravel.svg?token=t7LhMb4BZCM8Q58kCnsH&branch=master)](https://travis-ci.com/Engineev/ravel)

dev: [![Build Status](https://travis-ci.com/Engineev/ravel.svg?token=t7LhMb4BZCM8Q58kCnsH&branch=dev)](https://travis-ci.com/Engineev/ravel)

A RISC-V simulator

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

