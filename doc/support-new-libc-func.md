# How to add support for libc functions

When **ravel** see instructions like `call malloc`, it will invoke a piece of C++ code which simulate the corresponding
libc function. In other words, **ravel** doesn't have assembly implementation for the libc function in it. More 
precisely, this is implemented as follows. 

Recall that `call` is a pseudo-instruction, and it will be translated by the assembler (and also **ravel**) into
`auipc` and `jalr`. In **ravel**, we assign each libc function a special dummy address. When the interpreter find the
`pc` is pointing to one of these addresses, it knows a libc function has been invoked and then simulate it.

A **ravel** interpretable consists of two parts, a header and the actual instructions. The first `12` bytes of the 
header correspond the assembly code
```asm
         .text
_start:
         call main  ; this  will be decomposed into 2 instructions
         nop        ; stands for the end of interpretation
```
The other bytes are placeholders for libc functions. The size of the header is determined by 
`ravel::LibcFuncEndAddr`, whose default value is `64` (cf. `src/linker/interpretable.h`). If the needed libc functions 
are too many to be fitted into the header, one can adjust `ravel::LibcFuncEndAddr` to any larger integer divisible by
`16`. 

In order to add support for a new libc function. One needs to first assign it an address in `ravel::libc::Func` in
`src/linker/interpretable.h`, and update the function `ravel::libc::getName2Pos`, which is in the same file. Then
one needs to implement it in `ravel::Interpreter::simulateLibCFunc` in `src/interpreter/interpreter.cpp`. It's 
encouraged to mimic they way the other libc functions are implemented and put the actual implementation in 
`src/interpreter/libc_sim.h` and `src/interpreter/libc_sim.cpp`.
