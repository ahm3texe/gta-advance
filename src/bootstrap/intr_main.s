    .syntax unified
    .cpu arm7tdmi
    .arm

    .section .text.intr, "ax", %progbits
    .global IntrMain
    .type IntrMain, %function

IntrMain:
    mov     r3, #0x04000000
    add     r3, r3, #0x200
    ldr     r2, [r3]
    ldrh    r1, [r3, #8]
    mrs     r0, spsr
    stmdb   sp!, {r0-r3, lr}
    mov     r0, #1
    strh    r0, [r3, #8]
    and     r1, r2, r2, lsr #16

    mov     ip, #0
    add     ip, ip, #4
    ands    r0, r1, #0x0001
    bne     .Ldispatch

    /* Handler table retains slots for HBlank/VCount ordering. */
    add     ip, ip, #4
    add     ip, ip, #4
    ands    r0, r1, #0x0004
    bne     .Ldispatch

    add     ip, ip, #4
    ands    r0, r1, #0x0002
    bne     .Ldispatch

    add     ip, ip, #4
    ands    r0, r1, #0x0008
    bne     .Ldispatch

    add     ip, ip, #4
    ands    r0, r1, #0x0100
    bne     .Ldispatch

    add     ip, ip, #4
    ands    r0, r1, #0x0200
    bne     .Ldispatch

    add     ip, ip, #4
    ands    r0, r1, #0x0400
    bne     .Ldispatch

    add     ip, ip, #4
    ands    r0, r1, #0x0800
    bne     .Ldispatch

    add     ip, ip, #4
    ands    r0, r1, #0x1000
    bne     .Ldispatch

    add     ip, ip, #4
    ands    r0, r1, #0x2000
    strbne  r0, [r3, #-0x17c]
.Lunsupported_irq:
    bne     .Lunsupported_irq

.Ldispatch:
    strh    r0, [r3, #2]
    mov     r1, #0x20c0
    bic     r2, r2, r0
    and     r1, r1, r2
    strh    r1, [r3]

    mrs     r3, cpsr
    bic     r3, r3, #0xdf
    orr     r3, r3, #0x1f
    msr     cpsr_fc, r3

    ldr     r1, .Lhandler_table
    ldr     r0, .Lactive_irq_slot
    str     ip, [r0]
    add     r1, r1, ip
    ldr     r0, [r1]
    stmdb   sp!, {lr}
    add     lr, pc, #0
    bx      r0
    ldmia   sp!, {lr}

    mrs     r3, cpsr
    bic     r3, r3, #0xdf
    orr     r3, r3, #0x92
    msr     cpsr_fc, r3

    ldmia   sp!, {r0-r3, lr}
    strh    r2, [r3]
    strh    r1, [r3, #8]
    msr     spsr_fc, r0
    bx      lr

    .size IntrMain, . - IntrMain

.Lhandler_table:
    .word   0x02000170
.Lactive_irq_slot:
    .word   0x02000284

