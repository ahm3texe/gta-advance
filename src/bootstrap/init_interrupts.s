    .syntax unified
    .cpu arm7tdmi
    .thumb

    .section .text.init_interrupts, "ax", %progbits
    .balign 4
    .global InitInterrupts
    .type InitInterrupts, %function
    .thumb_func

InitInterrupts:
    push    {r4, lr}
    ldr     r4, .Lreg_ime
    movs    r3, #0
    strh    r3, [r4]

    ldr     r0, .Lhandler_table
    ldr     r1, .Ldummy_handler
    str     r1, [r0, #0]
    ldr     r2, .Lvblank_handler
    str     r2, [r0, #4]
    str     r1, [r0, #8]
    ldr     r2, .Ltimer0_handler
    str     r2, [r0, #12]
    str     r1, [r0, #16]
    str     r1, [r0, #20]
    str     r1, [r0, #24]
    str     r1, [r0, #28]
    str     r1, [r0, #32]
    str     r1, [r0, #36]
    str     r1, [r0, #40]
    str     r1, [r0, #44]
    str     r1, [r0, #48]

    ldrh    r2, [r4]
    strh    r3, [r4]
    ldr     r1, .Ldma3
    ldr     r0, .Lintr_main_rom
    str     r0, [r1, #0]
    ldr     r3, .Lintr_main_ewram
    str     r3, [r1, #4]
    ldr     r0, .Ldma_copy_control
    str     r0, [r1, #8]
    ldr     r0, [r1, #8]
    strh    r2, [r4]

    ldr     r0, .Lbios_irq_vector
    str     r3, [r0]
    ldr     r0, .Lirq_depth
    movs    r1, #0
    str     r1, [r0]
    ldr     r0, .Lvblank_state
    str     r1, [r0]

    ldr     r1, .Lreg_dispstat
    ldr     r2, .Ldispstat_value
    adds    r0, r2, #0
    strh    r0, [r1]
    ldr     r1, .Lreg_ie
    movs    r0, #5
    strh    r0, [r1]
    movs    r0, #1
    strh    r0, [r4]

    pop     {r4}
    pop     {r0}
    bx      r0

    .size InitInterrupts, . - InitInterrupts
    /* Original compiler used a zero halfword for literal-pool alignment. */
    .hword  0

.Lreg_ime:           .word 0x04000208
.Lhandler_table:     .word 0x02000170
.Ldummy_handler:     .word 0x08000735
.Lvblank_handler:    .word 0x08000221
.Ltimer0_handler:    .word 0x0800079d
.Ldma3:              .word 0x040000d4
.Lintr_main_rom:     .word 0x08000104
.Lintr_main_ewram:   .word 0x020004d0
.Ldma_copy_control:  .word 0x84000200
.Lbios_irq_vector:   .word 0x03007ffc
.Lirq_depth:         .word 0x02000eb8
.Lvblank_state:      .word 0x02000e74
.Lreg_dispstat:      .word 0x04000004
.Ldispstat_value:    .word 0x00003228
.Lreg_ie:            .word 0x04000200
