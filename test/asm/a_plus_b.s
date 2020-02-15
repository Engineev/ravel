# #include <stdio.h>
#
# int main() {
#   int a, b;
#   scanf("%d %d", &a, &b);
#   printf("%d\n", a + b);
#   return 0;
# }
        .file   "main.c"
        .option nopic
        .text
        .section        .rodata
        .align  2
.LC0:
        .string "%d %d"
        .align  2
.LC1:
        .string "%d\n"
        .text
        .align  2
        .globl  main
        .type   main, @function
main:
        addi    sp,sp,-32
        sw      ra,28(sp)
        sw      s0,24(sp)
        addi    s0,sp,32
        addi    a4,s0,-24
        addi    a5,s0,-20
        mv      a2,a4
        mv      a1,a5
        lui     a5,%hi(.LC0)
        addi    a0,a5,%lo(.LC0)
        call    __isoc99_scanf
        lw      a4,-20(s0)
        lw      a5,-24(s0)
        add     a5,a4,a5
        mv      a1,a5
        lui     a5,%hi(.LC1)
        addi    a0,a5,%lo(.LC1)
        call    printf
        li      a5,0
        mv      a0,a5
        lw      ra,28(sp)
        lw      s0,24(sp)
        addi    sp,sp,32
        jr      ra
        .size   main, .-main
        .ident  "GCC: (GNU) 9.2.0"
        .section        .note.GNU-stack,"",@progbits
