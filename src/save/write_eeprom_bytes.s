    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ ProgramEepromDword, 0x0806beac
    .equ VerifyEepromDword,  0x0806c020

    .section .text.write_eeprom_bytes, "ax", %progbits
    .balign 4
    .global WriteEepromBytes
    .type WriteEepromBytes, %function
    .thumb_func

WriteEepromBytes:
    push    {r4, r5, r6, r7, lr}
    mov     r7, r10
    mov     r6, r9
    mov     r5, r8
    push    {r5, r6, r7}
    sub     sp, #20
    str     r0, [sp, #8]
    adds    r5, r1, #0
    adds    r6, r2, #0
    adds    r0, r5, #0
    cmp     r5, #0
    bge     .Lcalculate_block_count
    adds    r0, r5, #7
.Lcalculate_block_count:
    asrs    r0, r0, #3
    str     r0, [sp, #12]
    ldr     r2, .Ldma3
    ldr     r0, [r2, #8]
    movs    r1, #128
    lsls    r1, r1, #24
    cmp     r0, #0
    bge     .Ldisable_interrupts
.Lwait_for_dma3:
    ldr     r0, [r2, #8]
    ands    r0, r1
    cmp     r0, #0
    bne     .Lwait_for_dma3

.Ldisable_interrupts:
    ldr     r1, .Lreg_ime_early
    movs    r0, #0
    strh    r0, [r1]
    movs    r0, #0
    mov     r10, r0
    movs    r2, #0
    ldr     r1, [sp, #12]
    cmp     r2, r1
    bge     .Lenable_interrupts
    mov     r7, sp

.Lwrite_next_block:
    cmp     r5, #0
    blt     .Lprogram_block
    ldrb    r0, [r6]
    strb    r0, [r7, #7]
    adds    r6, #1
    subs    r5, #1
    cmp     r5, #0
    blt     .Lprogram_block
    ldrb    r0, [r6]
    strb    r0, [r7, #6]
    adds    r6, #1
    subs    r5, #1
    cmp     r5, #0
    blt     .Lprogram_block
    ldrb    r0, [r6]
    strb    r0, [r7, #5]
    adds    r6, #1
    subs    r5, #1
    cmp     r5, #0
    blt     .Lprogram_block
    ldrb    r0, [r6]
    strb    r0, [r7, #4]
    adds    r6, #1
    subs    r5, #1
    cmp     r5, #0
    blt     .Lprogram_block
    ldrb    r0, [r6]
    strb    r0, [r7, #3]
    adds    r6, #1
    subs    r5, #1
    cmp     r5, #0
    blt     .Lprogram_block
    ldrb    r0, [r6]
    strb    r0, [r7, #2]
    adds    r6, #1
    subs    r5, #1
    cmp     r5, #0
    blt     .Lprogram_block
    ldrb    r0, [r6]
    strb    r0, [r7, #1]
    adds    r6, #1
    subs    r5, #1
    cmp     r5, #0
    blt     .Lprogram_block
    ldrb    r0, [r6]
    strb    r0, [r7]
    adds    r6, #1
    subs    r5, #1

.Lprogram_block:
    ldr     r0, [sp, #8]
    adds    r4, r0, r2
    lsls    r0, r4, #16
    lsrs    r0, r0, #16
    mov     r1, sp
    str     r2, [sp, #16]
    bl      ProgramEepromDword
    mov     r8, r4
    ldr     r2, [sp, #16]
    adds    r2, #1
    mov     r9, r2
    b       .Lverify_block

.Ldma3:          .word 0x040000d4
.Lreg_ime_early: .word 0x04000208

.Lretry_program:
    adds    r0, r4, #0
    mov     r1, sp
    bl      ProgramEepromDword
    movs    r1, #1
    add     r10, r1

.Lverify_block:
    mov     r1, r8
    lsls    r0, r1, #16
    lsrs    r4, r0, #16
    adds    r0, r4, #0
    mov     r1, sp
    bl      VerifyEepromDword
    lsls    r0, r0, #16
    cmp     r0, #0
    beq     .Ladvance_block
    mov     r0, r10
    cmp     r0, #19
    ble     .Lretry_program

.Ladvance_block:
    mov     r2, r9
    ldr     r1, [sp, #12]
    cmp     r2, r1
    blt     .Lwrite_next_block

.Lenable_interrupts:
    ldr     r1, .Lreg_ime
    movs    r0, #1
    strh    r0, [r1]
    movs    r0, #1
    add     sp, #20
    pop     {r3, r4, r5}
    mov     r8, r3
    mov     r9, r4
    mov     r10, r5
    pop     {r4, r5, r6, r7}
    pop     {r1}
    bx      r1
    .size WriteEepromBytes, . - WriteEepromBytes

    .hword 0
.Lreg_ime: .word 0x04000208

