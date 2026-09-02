    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ ReadEepromDword, 0x0806bdfc

    .section .text.read_eeprom_bytes, "ax", %progbits
    .balign 4
    .global ReadEepromBytes
    .type ReadEepromBytes, %function
    .thumb_func

ReadEepromBytes:
    push    {r4, r5, r6, r7, lr}
    mov     r7, r9
    mov     r6, r8
    push    {r6, r7}
    sub     sp, #8
    mov     r9, r0
    adds    r4, r1, #0
    adds    r5, r2, #0
    adds    r0, r4, #0
    cmp     r4, #0
    bge     .Lcalculate_block_count
    adds    r0, r4, #7
.Lcalculate_block_count:
    asrs    r0, r0, #3
    mov     r8, r0
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
    ldr     r1, .Lreg_ime
    movs    r0, #0
    strh    r0, [r1]
    movs    r7, #0
    cmp     r7, r8
    bge     .Lenable_interrupts

    mov     r6, sp
.Lread_next_block:
    mov     r1, r9
    adds    r0, r1, r7
    lsls    r0, r0, #16
    lsrs    r0, r0, #16
    mov     r1, sp
    bl      ReadEepromDword
    cmp     r4, #0
    blt     .Ladvance_block
    ldrb    r0, [r6, #7]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
    cmp     r4, #0
    blt     .Ladvance_block
    ldrb    r0, [r6, #6]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
    cmp     r4, #0
    blt     .Ladvance_block
    ldrb    r0, [r6, #5]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
    cmp     r4, #0
    blt     .Ladvance_block
    ldrb    r0, [r6, #4]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
    cmp     r4, #0
    blt     .Ladvance_block
    ldrb    r0, [r6, #3]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
    cmp     r4, #0
    blt     .Ladvance_block
    ldrb    r0, [r6, #2]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
    cmp     r4, #0
    blt     .Ladvance_block
    ldrb    r0, [r6, #1]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
    cmp     r4, #0
    blt     .Ladvance_block
    ldrb    r0, [r6]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1

.Ladvance_block:
    adds    r7, #1
    cmp     r7, r8
    blt     .Lread_next_block

.Lenable_interrupts:
    ldr     r1, .Lreg_ime
    movs    r0, #1
    strh    r0, [r1]
    movs    r0, #1
    add     sp, #8
    pop     {r3, r4}
    mov     r8, r3
    mov     r9, r4
    pop     {r4, r5, r6, r7}
    pop     {r1}
    bx      r1
    .size ReadEepromBytes, . - ReadEepromBytes

.Ldma3:    .word 0x040000d4
.Lreg_ime: .word 0x04000208
