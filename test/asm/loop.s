# int main() {
#   int s = 0;
#   for (int i = 0; i < 10; ++i) {
#     s += i;
#   }
#   return s;
# }
        .file   "main.c"
        .option nopic
        .text
        .align  2
        .globl  main
        .type   main, @function
main:
        addi    sp,sp,-32
        sw      s0,28(sp)
        addi    s0,sp,32
        sw      zero,-20(s0)
        sw      zero,-24(s0)
        j       .L2
.L3:
        lw      a4,-20(s0)
        lw      a5,-24(s0)
        add     a5,a4,a5
        sw      a5,-20(s0)
        lw      a5,-24(s0)
        addi    a5,a5,1
        sw      a5,-24(s0)
.L2:
        lw      a4,-24(s0)
        li      a5,9
        ble     a4,a5,.L3
        lw      a5,-20(s0)
        mv      a0,a5
        lw      s0,28(sp)
        addi    sp,sp,32
        jr      ra
        .size   main, .-main
        .ident  "GCC: (GNU) 9.2.0"
        .section        .note.GNU-stack,"",@progbits
