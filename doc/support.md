# Support 

Reference: https://rv8.io/asm.html

## Directives

Unsupported directives will be ignored. 
In `.section` directives, arguments like `.rodata.str1.4,"aMS",@progbits,1`
will be treated as `.rodata`. Section names such as `.sdata` will be treated 
as `.data`

| Directive | Arguments  
|:---       |:---        
|`.text`    |
|`.data`    |
|`.rodata`  | 
|`.bss`     |
|`.section` | {`.text`, `.data`, `.rodata`, `.bss`}
|`.align`   | {`2`, `4`, `8`, `16`}
|`.p2align` | {`1`, `2`, `3`, `4`}
|`.globl`   | symbol
|`.comm`    | symbol, size, align
|`.zero`    | integer
|`.string`  | string
|`.asciz`   | string
|`.word`    | integer

## Labels

Local labels are not supported.

## Relocation Functions

`%hi(symbol)`, `%lo(symbol)`, `%pcrel_hi(symbol)` and `%pcrel_lo(symbol)`
are supported. 

## Pseudo-instructions

`offset` can be a label.

| Pseudo-instruction |
|:---|
| `la rd, symbol`
| `l{b, h, w} rd, symbol`
| `s{b, h, w} rd, symbol, rt`
| `nop`
| `li  rd, imm`
| `mv  rd, rs`
| `not rd, rs`
| `neg rd, rs`
| `{seqz, snez, sltz, sgtz} rd, rs`
| `{beqz, bnez, blez, bgez, bltz, btz} rs, offset`
| `{bgt, ble, bgtu, bleu} rs, rt, offset`
| `j   offset`
| `jr  offset`
| `ret`
| `call offset`
| `tail offset` 

## Instructions

RV32I base instruction set and RV32M standard extension are supported except 
`FENCE`, `FENCE.I`, `ECALL`, `EBREAK`, and CSR instructions. 

## Libc Functions
Currently, the following libc functions are supported. If you need other libc
functions, you may open an issue.

| Function signature |
|:---|
|`puts`
|`scanf` (partially supported)
|`sscanf` (partially supported)
|`printf` (partially supported)
|`sprintf` (partially supported)
|`putchar`
|`malloc`
|`free`
|`memcpy`
|`strlen`
|`strcpy`
|`strcat`
|`strcmp`
|`memset`

