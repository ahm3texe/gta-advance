    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ UpdateTransferSubsystem0, 0x08012b9c
    .equ UpdateTransferSubsystem1, 0x080133a8
    .equ UpdateTransferSubsystem2, 0x080130f4
    .equ UpdateTransferSubsystem3, 0x08013900
    .equ UpdateTransferSubsystem4, 0x080101d8
    .equ UpdateVCountSubsystem,    0x080327c8

    .section .text.irq_helpers, "ax", %progbits
    .balign 4

    .global NoOpVBlankFinalize
    .type NoOpVBlankFinalize, %function
    .thumb_func
NoOpVBlankFinalize:
    bx      lr
    .size NoOpVBlankFinalize, . - NoOpVBlankFinalize
    .hword  0

    .global DummyIntr
    .type DummyIntr, %function
    .thumb_func
DummyIntr:
    bx      lr
    .size DummyIntr, . - DummyIntr
    .hword  0

    .global RunVBlankTransfers
    .type RunVBlankTransfers, %function
    .thumb_func
RunVBlankTransfers:
    push    {lr}
    bl      UpdateTransferSubsystem0
    bl      UpdateTransferSubsystem1
    bl      UpdateTransferSubsystem2
    bl      UpdateTransferSubsystem3
    ldr     r0, .Lasync_state
    ldr     r0, [r0]
    cmp     r0, #0
    bne     .Lcopy_delay
    bl      UpdateTransferSubsystem4

.Lcopy_delay:
    movs    r1, #0xc0
    lsls    r1, r1, #18
    ldr     r2, .Liwram_frame_counter
    ldr     r0, [r2]
    str     r0, [r1]
    cmp     r0, #5
    bls     .Lcheck_game_state
    movs    r0, #5
    str     r0, [r1]

.Lcheck_game_state:
    ldr     r0, .Lgame_state
    ldrb    r0, [r0, #12]
    subs    r0, #1
    lsls    r0, r0, #24
    lsrs    r0, r0, #24
    cmp     r0, #1
    bhi     .Lreset_counter
    movs    r0, #5
    str     r0, [r1]

.Lreset_counter:
    movs    r0, #0
    str     r0, [r2]
    ldr     r1, .Lvblank_state
    movs    r0, #1
    strb    r0, [r1]
    pop     {r0}
    bx      r0

.Lasync_state:         .word 0x02000e74
.Liwram_frame_counter: .word 0x03000004
.Lgame_state:          .word 0x02000ce0
.Lvblank_state:        .word 0x02000130

    .global NoOpInterruptHelper
    .type NoOpInterruptHelper, %function
    .thumb_func
NoOpInterruptHelper:
    bx      lr
    .size NoOpInterruptHelper, . - NoOpInterruptHelper
    .hword  0

    .global VCountIntr
    .type VCountIntr, %function
    .thumb_func
VCountIntr:
    push    {lr}
    bl      UpdateVCountSubsystem
    ldr     r0, .Lreg_if
    ldrh    r1, [r0]
    movs    r2, #4
    orrs    r1, r2
    strh    r1, [r0]
    pop     {r0}
    bx      r0
    .size VCountIntr, . - VCountIntr

.Lreg_if:              .word 0x04000202

