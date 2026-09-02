    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ SaveIoEnter,          0x080337a8
    .equ SaveIoLeave,          0x08033b74
    .equ IdentifyEeprom,       0x0806bd34
    .equ ProgramEepromDwordEx, 0x0806c078

    .section .text.write_eeprom_range, "ax", %progbits
    .balign 4
    .global WriteEepromRange
    .type WriteEepromRange, %function
    .thumb_func

WriteEepromRange:
    push    {r4, r5, r6, r7, lr}
    mov     r7, r10
    mov     r6, r9
    mov     r5, r8
    push    {r5, r6, r7}
    sub     sp, #20
    adds    r6, r0, #0
    adds    r5, r1, #0
    adds    r4, r2, #0
    bl      SaveIoEnter

    subs    r0, r4, #1
    lsrs    r0, r0, #3
    adds    r0, #1
    mov     r10, r0
    movs    r7, #7
    ands    r7, r6
    lsrs    r6, r6, #3

    ldr     r1, .Lreg_ime_early
    movs    r0, #0
    strh    r0, [r1]
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
    str     r0, [sp, #8]
    movs    r0, #4
    bl      IdentifyEeprom
    lsls    r0, r0, #16
    cmp     r0, #0
    beq     .Lvalidate_range
    movs    r1, #0
    str     r1, [sp, #8]
    b       .Lfinish_write

.Lreg_ime_early:          .word 0x04000208
.Ldma3:                   .word 0x040000d4
.Leeprom_available_early: .word 0x02000eb8

.Lvalidate_range:
    mov     r1, r10
    adds    r0, r6, r1
    cmp     r0, #64
    bhi     .Lwrite_failed
    movs    r1, #0
    mov     r9, r1
    cmp     r9, r10
    bge     .Lfinish_write
    mov     r3, sp
    lsls    r6, r6, #16
    mov     r8, r6

.Lprepare_block:
    cmp     r7, #0
    ble     .Lcopy_byte_7
    subs    r7, #1
    b       .Lnext_byte_6
.Lcopy_byte_7:
    cmp     r4, #0
    beq     .Lnext_byte_6
    ldrb    r0, [r5]
    strb    r0, [r3, #7]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_6:
    cmp     r7, #0
    ble     .Lcopy_byte_6
    subs    r7, #1
    b       .Lnext_byte_5
.Lcopy_byte_6:
    cmp     r4, #0
    beq     .Lnext_byte_5
    ldrb    r0, [r5]
    strb    r0, [r3, #6]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_5:
    cmp     r7, #0
    ble     .Lcopy_byte_5
    subs    r7, #1
    b       .Lnext_byte_4
.Lcopy_byte_5:
    cmp     r4, #0
    beq     .Lnext_byte_4
    ldrb    r0, [r5]
    strb    r0, [r3, #5]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_4:
    cmp     r7, #0
    ble     .Lcopy_byte_4
    subs    r7, #1
    b       .Lnext_byte_3
.Lcopy_byte_4:
    cmp     r4, #0
    beq     .Lnext_byte_3
    ldrb    r0, [r5]
    strb    r0, [r3, #4]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_3:
    cmp     r7, #0
    ble     .Lcopy_byte_3
    subs    r7, #1
    b       .Lnext_byte_2
.Lcopy_byte_3:
    cmp     r4, #0
    beq     .Lnext_byte_2
    ldrb    r0, [r5]
    strb    r0, [r3, #3]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_2:
    cmp     r7, #0
    ble     .Lcopy_byte_2
    subs    r7, #1
    b       .Lnext_byte_1
.Lcopy_byte_2:
    cmp     r4, #0
    beq     .Lnext_byte_1
    ldrb    r0, [r5]
    strb    r0, [r3, #2]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_1:
    cmp     r7, #0
    ble     .Lcopy_byte_1
    subs    r7, #1
    b       .Lnext_byte_0
.Lcopy_byte_1:
    cmp     r4, #0
    beq     .Lnext_byte_0
    ldrb    r0, [r5]
    strb    r0, [r3, #1]
    adds    r5, #1
    subs    r4, #1
.Lnext_byte_0:
    cmp     r7, #0
    ble     .Lcopy_byte_0
    subs    r7, #1
    b       .Lprogram_block
.Lcopy_byte_0:
    cmp     r4, #0
    beq     .Lprogram_block
    ldrb    r0, [r5]
    strb    r0, [r3]
    adds    r5, #1
    subs    r4, #1

.Lprogram_block:
    movs    r6, #0
    mov     r2, r8
.Lretry_program:
    lsrs    r0, r2, #16
    mov     r1, sp
    str     r2, [sp, #12]
    str     r3, [sp, #16]
    bl      ProgramEepromDwordEx
    lsls    r0, r0, #16
    adds    r6, #1
    ldr     r2, [sp, #12]
    ldr     r3, [sp, #16]
    cmp     r0, #0
    beq     .Lprogram_succeeded
    cmp     r6, #9
    ble     .Lretry_program

.Lwrite_failed:
    movs    r0, #0
    str     r0, [sp, #8]
    b       .Lfinish_write

.Lprogram_succeeded:
    movs    r1, #128
    lsls    r1, r1, #9
    add     r8, r1
    movs    r0, #1
    add     r9, r0
    cmp     r9, r10
    blt     .Lprepare_block

.Lfinish_write:
    ldr     r1, .Leeprom_available
    movs    r0, #0
    str     r0, [r1]
    ldr     r1, .Lreg_ime
    movs    r0, #1
    strh    r0, [r1]
    bl      SaveIoLeave
    ldr     r0, [sp, #8]
    add     sp, #20
    pop     {r3, r4, r5}
    mov     r8, r3
    mov     r9, r4
    mov     r10, r5
    pop     {r4, r5, r6, r7}
    pop     {r1}
    bx      r1
    .size WriteEepromRange, . - WriteEepromRange

.Leeprom_available: .word 0x02000eb8
.Lreg_ime:          .word 0x04000208

