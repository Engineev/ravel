# Support 

Reference: https://rv8.io/asm.html

## Directives

Unsupported directives will be ignored. 
In `.section` directives, arguments like `.rodata.str1.4,"aMS",@progbits,1`
will be treated as `.rodata`. 

| Directive | Arguments  
|:---       |:---        
|`.text`    |
|`.data`    |
|`.rodata`  | 
|`.bss`     |
|`.section` | {`.text`, `.data`, `.rodata`, `.bss`}
|`.align`   | {`2`, `4`, `8`, `16`}
|`.globl`   | symbol
|`.comm`    | symbol, size, align
|`.zero`    | integer
|`.string`  | string
|`.word`    | integer

## Labels

Local labels are not supported.

## Relocation Functions

`%hi(symbol)` and `%lo(symbol)` are supported. 

## Pseudo-instructions

`offset` can be a label.

| Pseudo-instruction |
|:---|
| `nop`
| `li  rd, imm`
| `mv  rd, rs`
| `not rd, rs`
| `neg rd, rs`
| `{bgt, ble, bgtu, bleu} rs, rt, offset`
| `j   offset`
| `jr  offset`
| `ret`
| `call offset` 

## Instructions

RV32I base instruction set and RV32M standard extension are supported except 
`FENCE`, `FENCE.I`, `ECALL`, `EBREAK`, and CSR instructions. 



