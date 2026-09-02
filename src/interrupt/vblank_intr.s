    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ UpdateVBlankSubsystem0, 0x08033264
    .equ UpdateVBlankSubsystem1, 0x08011ec4
    .equ UpdateVBlankSubsystem2, 0x0806686c
    .equ UpdateVBlankSubsystem3, 0x08012b9c
    .equ UpdateVBlankSubsystem4, 0x080133a8
    .equ UpdateVBlankSubsystem5, 0x080130f4
    .equ UpdateVBlankSubsystem6, 0x08013900
    .equ UpdateVBlankSubsystem7, 0x080101d8
    .equ UpdateVBlankSubsystem8, 0x080108f4
    .equ FinishVBlank,          0x08000730

    .section .text.vblank_intr, "ax", %progbits
    .balign 4
    .global VBlankIntr
    .type VBlankIntr, %function
    .thumb_func

VBlankIntr:
    push    {r4-r7, lr}
    mov     r7, r9
    mov     r6, r8
    push    {r6, r7}

    ldr     r1, .Lframe_counter
    ldr     r0, [r1]
    adds    r0, #1
    str     r0, [r1]

    ldr     r0, .Liwram_frame_counter
    mov     r8, r0
    ldr     r0, [r0]
    adds    r0, #1
    mov     r1, r8
    str     r0, [r1]

    ldr     r1, .Lirq_depth
    ldr     r0, [r1]
    adds    r5, r0, #1
    str     r5, [r1]
    cmp     r5, #1
    beq     .Lrun_updates
    b       .Lepilogue

.Lrun_updates:
    bl      UpdateVBlankSubsystem0
    bl      UpdateVBlankSubsystem1
    ldr     r0, .Lvblank_enabled
    ldrh    r0, [r0]
    cmp     r0, #0
    beq     .Lskip_optional
    bl      UpdateVBlankSubsystem2

.Lskip_optional:
    ldr     r2, .Lreg_vcount
    mov     r9, r2
    ldrh    r0, [r2]
    subs    r0, #160
    lsls    r0, r0, #16
    lsrs    r0, r0, #16
    cmp     r0, #10
    bhi     .Lfinish_updates

    ldr     r4, .Lvblank_state
    ldrb    r0, [r4]
    cmp     r0, #2
    bne     .Lstate_zero_path

    bl      UpdateVBlankSubsystem3
    bl      UpdateVBlankSubsystem4
    bl      UpdateVBlankSubsystem5
    bl      UpdateVBlankSubsystem6
    ldr     r0, .Lasync_state
    ldr     r0, [r0]
    cmp     r0, #0
    bne     .Lcopy_delay_a
    bl      UpdateVBlankSubsystem7

.Lcopy_delay_a:
    movs    r1, #0xc0
    lsls    r1, r1, #18
    mov     r2, r8
    ldr     r0, [r2]
    str     r0, [r1]
    cmp     r0, #5
    bls     .Lcheck_mode_a
    movs    r0, #5
    str     r0, [r1]

.Lcheck_mode_a:
    ldr     r0, .Lgame_state_a
    ldrb    r0, [r0, #12]
    subs    r0, #1
    lsls    r0, r0, #24
    lsrs    r0, r0, #24
    cmp     r0, #1
    bhi     .Lreset_counter_a
    movs    r0, #5
    str     r0, [r1]

.Lreset_counter_a:
    movs    r0, #0
    mov     r1, r8
    str     r0, [r1]
    strb    r5, [r4]
    b       .Lfinish_updates

    .balign 4
.Lframe_counter:       .word 0x02035ca8
.Liwram_frame_counter: .word 0x03000004
.Lirq_depth:           .word 0x02000eb8
.Lvblank_enabled:      .word 0x02000d08
.Lreg_vcount:          .word 0x04000006
.Lvblank_state:        .word 0x02000130
.Lasync_state:         .word 0x02000e74
.Lgame_state_a:        .word 0x02000ce0

.Lstate_zero_path:
    ldrb    r0, [r4]
    adds    r7, r0, #0
    cmp     r7, #0
    bne     .Lother_state
    ldr     r6, .Lasync_state_b
    ldr     r0, [r6]
    cmp     r0, #0
    bne     .Lcheck_vcount_wide
    bl      UpdateVBlankSubsystem8

.Lcheck_vcount_wide:
    mov     r2, r9
    ldrh    r0, [r2]
    subs    r0, #160
    lsls    r0, r0, #16
    lsrs    r0, r0, #16
    cmp     r0, #19
    bls     .Lrun_full_updates
    movs    r0, #2
    b       .Lstore_state

    .hword  0
.Lasync_state_b:       .word 0x02000e74

.Lrun_full_updates:
    bl      UpdateVBlankSubsystem3
    bl      UpdateVBlankSubsystem4
    bl      UpdateVBlankSubsystem5
    bl      UpdateVBlankSubsystem6
    ldr     r0, [r6]
    cmp     r0, #0
    bne     .Lcopy_delay_b
    bl      UpdateVBlankSubsystem7

.Lcopy_delay_b:
    movs    r1, #0xc0
    lsls    r1, r1, #18
    mov     r2, r8
    ldr     r0, [r2]
    str     r0, [r1]
    cmp     r0, #5
    bls     .Lcheck_mode_b
    movs    r0, #5
    str     r0, [r1]

.Lcheck_mode_b:
    ldr     r0, .Lgame_state_b
    ldrb    r0, [r0, #12]
    subs    r0, #1
    lsls    r0, r0, #24
    lsrs    r0, r0, #24
    cmp     r0, #1
    bhi     .Lreset_counter_b
    movs    r0, #5
    str     r0, [r1]

.Lreset_counter_b:
    mov     r0, r8
    str     r7, [r0]
    strb    r5, [r4]
    b       .Lfinish_updates

    .hword  0
.Lgame_state_b:        .word 0x02000ce0

.Lother_state:
    ldrb    r0, [r4]
    cmp     r0, #1
    beq     .Lfinish_updates
    movs    r0, #0
.Lstore_state:
    strb    r0, [r4]

.Lfinish_updates:
    bl      FinishVBlank

.Lepilogue:
    ldr     r1, .Lirq_depth_end
    ldr     r0, [r1]
    subs    r0, #1
    str     r0, [r1]
    ldr     r1, .Lbios_irq_flags
    movs    r0, #1
    ldrh    r2, [r1]
    orrs    r0, r2
    strh    r0, [r1]

    pop     {r3, r4}
    mov     r8, r3
    mov     r9, r4
    pop     {r4-r7}
    pop     {r0}
    bx      r0

    .size VBlankIntr, . - VBlankIntr

.Lirq_depth_end:       .word 0x02000eb8
.Lbios_irq_flags:      .word 0x03007ff8

