    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ Memset,            0x0806dcc0
    .equ WriteEepromRange,  0x08000f1c

    .section .text.save_helpers, "ax", %progbits
    .balign 4

    .global EraseSaveSlot
    .type EraseSaveSlot, %function
    .thumb_func
EraseSaveSlot:
    push    {r4, lr}
    sub     sp, #8
    adds    r4, r0, #0
    cmp     r4, #2
    bhi     .Lerase_invalid
    ldr     r1, .Lslot_headers_erase
    lsls    r0, r4, #1
    adds    r0, r0, r4
    lsls    r0, r0, #2
    adds    r0, r0, r1
    movs    r1, #0
    strb    r1, [r0]
    mov     r0, sp
    movs    r2, #8
    bl      Memset
    lsls    r0, r4, #2
    adds    r0, r0, r4
    lsls    r0, r0, #5
    mov     r1, sp
    movs    r2, #8
    bl      WriteEepromRange
    b       .Lerase_return

.Lslot_headers_erase: .word 0x02000460

.Lerase_invalid:
    movs    r0, #0
.Lerase_return:
    add     sp, #8
    pop     {r4}
    pop     {r1}
    bx      r1
    .size EraseSaveSlot, . - EraseSaveSlot
    .hword 0

    .global GetSaveSlotHeader
    .type GetSaveSlotHeader, %function
    .thumb_func
GetSaveSlotHeader:
    adds    r2, r0, #0
    cmp     r2, #2
    bhi     .Lheader_invalid
    ldr     r0, .Lslot_headers_get
    lsls    r1, r2, #1
    adds    r1, r1, r2
    lsls    r1, r1, #2
    adds    r1, r1, r0
    ldrb    r0, [r1]
    cmp     r0, #0
    beq     .Lheader_invalid
    adds    r0, r1, #0
    b       .Lheader_return
    .hword 0
.Lslot_headers_get: .word 0x02000460
.Lheader_invalid:
    movs    r0, #0
.Lheader_return:
    bx      lr
    .size GetSaveSlotHeader, . - GetSaveSlotHeader

    .global ReadU8
    .type ReadU8, %function
    .thumb_func
ReadU8:
    ldrb    r0, [r0]
    bx      lr
    .size ReadU8, . - ReadU8

    .global ReadU16LE
    .type ReadU16LE, %function
    .thumb_func
ReadU16LE:
    adds    r1, r0, #0
    ldrb    r2, [r1, #1]
    lsls    r0, r2, #8
    ldrb    r1, [r1]
    orrs    r0, r1
    bx      lr
    .size ReadU16LE, . - ReadU16LE

    .global ReadU32LE
    .type ReadU32LE, %function
    .thumb_func
ReadU32LE:
    adds    r1, r0, #0
    ldrb    r2, [r1, #1]
    lsls    r0, r2, #8
    ldrb    r3, [r1]
    orrs    r0, r3
    ldrb    r3, [r1, #2]
    lsls    r2, r3, #16
    orrs    r0, r2
    ldrb    r1, [r1, #3]
    lsls    r1, r1, #24
    orrs    r0, r1
    bx      lr
    .size ReadU32LE, . - ReadU32LE

    .global WriteU8
    .type WriteU8, %function
    .thumb_func
WriteU8:
    strb    r1, [r0]
    bx      lr
    .size WriteU8, . - WriteU8

    .global WriteU16LE
    .type WriteU16LE, %function
    .thumb_func
WriteU16LE:
    lsls    r1, r1, #16
    lsrs    r1, r1, #16
    strb    r1, [r0]
    lsrs    r1, r1, #8
    strb    r1, [r0, #1]
    bx      lr
    .size WriteU16LE, . - WriteU16LE

    .global WriteU32LE
    .type WriteU32LE, %function
    .thumb_func
WriteU32LE:
    strb    r1, [r0]
    movs    r2, #255
    lsls    r2, r2, #8
    ands    r2, r1
    lsrs    r2, r2, #8
    strb    r2, [r0, #1]
    movs    r2, #255
    lsls    r2, r2, #16
    ands    r2, r1
    lsrs    r2, r2, #16
    strb    r2, [r0, #2]
    lsrs    r1, r1, #24
    strb    r1, [r0, #3]
    bx      lr
    .size WriteU32LE, . - WriteU32LE

