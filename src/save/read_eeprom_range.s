    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ SaveIoEnter,     0x080337a8
    .equ SaveIoLeave,     0x08033b74
    .equ IdentifyEeprom,  0x0806bd34
    .equ ReadEepromDword, 0x0806bdfc

    .section .text.read_eeprom_range, "ax", %progbits
    .balign 4
    .global ReadEepromRange
    .type ReadEepromRange, %function
    .thumb_func

ReadEepromRange:
    push    {r4, r5, r6, r7, lr}
    mov     r7, r8
    push    {r7}
    sub     sp, #12
    mov     r8, r0
    adds    r5, r1, #0
    adds    r4, r2, #0
    bl      SaveIoEnter
    movs    r6, #7
    mov     r0, r8
    ands    r6, r0
    lsrs    r0, r0, #3
    mov     r8, r0

    ldr     r0, .Lreg_ime_early
    movs    r1, #0
    strh    r1, [r0]
    ldr     r2, .Ldma3
    ldr     r0, [r2, #8]
    movs    r1, #128
    lsls    r1, r1, #24
    cmp     r0, #0
    bge     .Lmark_eeprom_available
.Lwait_for_dma3:
    ldr     r0, [r2, #8]
    ands    r0, r1
    cmp     r0, #0
    bne     .Lwait_for_dma3

.Lmark_eeprom_available:
    ldr     r1, .Leeprom_available_early
    movs    r0, #1
    str     r0, [r1]
    movs    r0, #4
    bl      IdentifyEeprom
    lsls    r0, r0, #16
    cmp     r0, #0
    bne     .Lfinish_read
    movs    r2, #0
    cmp     r4, #0
    beq     .Lfinish_read
    mov     r7, sp

.Lread_next_block:
    mov     r1, r8
    adds    r0, r1, r2
    lsls    r0, r0, #16
    lsrs    r0, r0, #16
    mov     r1, sp
    str     r2, [sp, #8]
    bl      ReadEepromDword
    lsls    r0, r0, #16
    ldr     r2, [sp, #8]
    cmp     r0, #0
    bne     .Lfinish_read

    cmp     r6, #0
    ble     .Lcopy_byte_7
    subs    r6, #1
    b       .Lnext_byte_6

.Lreg_ime_early:          .word 0x04000208
.Ldma3:                   .word 0x040000d4
.Leeprom_available_early: .word 0x02000eb8

.Lcopy_byte_7:
    cmp     r4, #0
    beq     .Lnext_byte_6
    ldrb    r0, [r7, #7]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_6:
    cmp     r6, #0
    ble     .Lcopy_byte_6
    subs    r6, #1
    b       .Lnext_byte_5
.Lcopy_byte_6:
    cmp     r4, #0
    beq     .Lnext_byte_5
    ldrb    r0, [r7, #6]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_5:
    cmp     r6, #0
    ble     .Lcopy_byte_5
    subs    r6, #1
    b       .Lnext_byte_4
.Lcopy_byte_5:
    cmp     r4, #0
    beq     .Lnext_byte_4
    ldrb    r0, [r7, #5]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_4:
    cmp     r6, #0
    ble     .Lcopy_byte_4
    subs    r6, #1
    b       .Lnext_byte_3
.Lcopy_byte_4:
    cmp     r4, #0
    beq     .Lnext_byte_3
    ldrb    r0, [r7, #4]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_3:
    cmp     r6, #0
    ble     .Lcopy_byte_3
    subs    r6, #1
    b       .Lnext_byte_2
.Lcopy_byte_3:
    cmp     r4, #0
    beq     .Lnext_byte_2
    ldrb    r0, [r7, #3]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_2:
    cmp     r6, #0
    ble     .Lcopy_byte_2
    subs    r6, #1
    b       .Lnext_byte_1
.Lcopy_byte_2:
    cmp     r4, #0
    beq     .Lnext_byte_1
    ldrb    r0, [r7, #2]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_1:
    cmp     r6, #0
    ble     .Lcopy_byte_1
    subs    r6, #1
    b       .Lnext_byte_0
.Lcopy_byte_1:
    cmp     r4, #0
    beq     .Lnext_byte_0
    ldrb    r0, [r7, #1]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_0:
    cmp     r6, #0
    ble     .Lcopy_byte_0
    subs    r6, #1
    b       .Ladvance_block
.Lcopy_byte_0:
    cmp     r4, #0
    beq     .Ladvance_block
    ldrb    r0, [r7]
    strb    r0, [r5]
    adds    r5, #1
    subs    r4, #1
.Ladvance_block:
    adds    r2, #1
    cmp     r4, #0
    bne     .Lread_next_block

.Lfinish_read:
    ldr     r1, .Leeprom_available
    movs    r0, #0
    str     r0, [r1]
    ldr     r1, .Lreg_ime
    movs    r0, #1
    strh    r0, [r1]
    bl      SaveIoLeave
    movs    r0, #1
    add     sp, #12
    pop     {r3}
    mov     r8, r3
    pop     {r4, r5, r6, r7}
    pop     {r1}
    bx      r1
    .size ReadEepromRange, . - ReadEepromRange

.Leeprom_available: .word 0x02000eb8
.Lreg_ime:          .word 0x04000208

