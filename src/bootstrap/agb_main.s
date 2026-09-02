    .syntax unified
    .cpu arm7tdmi
    .arm

    .section .text.boot, "ax", %progbits
    .global AgbMain
    .type AgbMain, %function

AgbMain:
    mov     r0, #0x12
    msr     cpsr_fc, r0
    ldr     sp, .Lirq_stack
    mov     r0, #0x1f
    msr     cpsr_fc, r0
    ldr     sp, .Lsystem_stack
    ldr     r1, .Lirq_vector
    add     r0, pc, #0x20
    str     r0, [r1]
    ldr     r1, .Lgame_init
    mov     lr, pc
    bx      r1
    b       AgbMain

.Lsystem_stack:
    .word   0x03007e00
.Lirq_stack:
    .word   0x03007fa0
.Lirq_vector:
    .word   0x03007ffc
.Lgame_init:
    .word   0x08000431

    .size AgbMain, . - AgbMain
