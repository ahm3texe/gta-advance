    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ ReadEepromBytes,   0x0800091c
    .equ WriteEepromBytes,  0x080009ec
    .equ ReadSaveMetadata,  0x08000c00
    .equ WriteSaveMetadata, 0x08000c14

    .section .text.save_slots, "ax", %progbits
    .balign 4

    .global WriteSaveSlot
    .type WriteSaveSlot, %function
    .thumb_func
WriteSaveSlot:
    push    {r4, r5, r6, lr}
    adds    r5, r0, #0
    adds    r6, r1, #0
    ldr     r0, .Lwrite_slot_count
    ldr     r3, [r0]
    cmp     r3, #0
    beq     .Lwrite_fail
    ldr     r0, .Lwrite_payload_size
    ldr     r1, [r0]
    adds    r4, r0, #0
    cmp     r1, #0
    bne     .Lclamp_write_slot
.Lwrite_fail:
    movs    r0, #0
    b       .Lwrite_return

.Lwrite_slot_count:  .word 0x02000ec4
.Lwrite_payload_size:.word 0x02000ec0

.Lclamp_write_slot:
    cmp     r5, #0
    bge     .Lcheck_write_slot_max
    movs    r5, #0
    b       .Lclamp_write_length
.Lcheck_write_slot_max:
    cmp     r5, r3
    blt     .Lclamp_write_length
    subs    r5, r3, #1

.Lclamp_write_length:
    cmp     r2, #0
    beq     .Luse_full_write_length
    ldr     r0, [r4]
    cmp     r2, r0
    bls     .Lcalculate_write_block
.Luse_full_write_length:
    ldr     r2, [r4]

.Lcalculate_write_block:
    ldr     r0, [r4]
    cmp     r0, #0
    bge     .Ldivide_write_size
    adds    r0, #7
.Ldivide_write_size:
    asrs    r0, r0, #3
    muls    r0, r5
    adds    r0, #4
    adds    r1, r2, #0
    adds    r2, r6, #0
    bl      WriteEepromBytes
    ldr     r4, .Lwrite_metadata
    adds    r0, r4, #0
    bl      ReadSaveMetadata
    adds    r0, r5, #0
    adds    r0, #16
    adds    r0, r0, r4
    movs    r1, #1
    strb    r1, [r0]
    adds    r0, r4, #0
    bl      WriteSaveMetadata
    movs    r0, #1
.Lwrite_return:
    pop     {r4, r5, r6}
    pop     {r1}
    bx      r1
    .size WriteSaveSlot, . - WriteSaveSlot

.Lwrite_metadata: .word 0x02000ed0

    .global ReadSaveSlot
    .type ReadSaveSlot, %function
    .thumb_func
ReadSaveSlot:
    push    {r4, r5, r6, r7, lr}
    adds    r5, r0, #0
    adds    r7, r1, #0
    adds    r6, r2, #0
    ldr     r4, .Lread_metadata_early
    adds    r0, r4, #0
    bl      ReadSaveMetadata
    adds    r0, r5, #0
    adds    r0, #16
    adds    r0, r0, r4
    ldrb    r0, [r0]
    cmp     r0, #0
    bne     .Lclamp_read_slot
    movs    r0, #0
    b       .Lread_return

.Lread_metadata_early: .word 0x02000ed0

.Lclamp_read_slot:
    cmp     r5, #0
    bge     .Lcheck_read_slot_max
    movs    r5, #0
    b       .Lclamp_read_length
.Lcheck_read_slot_max:
    ldr     r0, .Lread_slot_count
    ldr     r0, [r0]
    cmp     r5, r0
    blt     .Lclamp_read_length
    subs    r5, r0, #1

.Lclamp_read_length:
    ldr     r1, .Lread_payload_size
    cmp     r6, #0
    beq     .Luse_full_read_length
    ldr     r0, [r1]
    cmp     r6, r0
    bls     .Lcalculate_read_block
.Luse_full_read_length:
    ldr     r6, [r1]

.Lcalculate_read_block:
    ldr     r0, [r1]
    cmp     r0, #0
    bge     .Ldivide_read_size
    adds    r0, #7
.Ldivide_read_size:
    asrs    r0, r0, #3
    muls    r0, r5
    adds    r0, #4
    adds    r1, r6, #0
    adds    r2, r7, #0
    bl      ReadEepromBytes
    movs    r0, #1
.Lread_return:
    pop     {r4, r5, r6, r7}
    pop     {r1}
    bx      r1
    .size ReadSaveSlot, . - ReadSaveSlot

    .hword 0
.Lread_slot_count:   .word 0x02000ec4
.Lread_payload_size: .word 0x02000ec0

