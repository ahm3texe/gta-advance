    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ IdentifyEeprom,     0x0806bd34
    .equ DivideSigned,       0x0806c0f4
    .equ ReadSaveMetadata,   0x08000c00
    .equ WriteSaveMetadata,  0x08000c14

    .section .text.init_save_system, "ax", %progbits
    .balign 4
    .global InitSaveSystem
    .type InitSaveSystem, %function
    .thumb_func

InitSaveSystem:
    push    {r4, r5, lr}
    adds    r5, r0, #0
    ldr     r4, .Lreg_ime
    movs    r0, #0
    strh    r0, [r4]
    movs    r0, #4
    bl      IdentifyEeprom
    movs    r0, #1
    strh    r0, [r4]
    ldr     r1, .Lslot_count_early
    str     r5, [r1]
    cmp     r5, #0
    bgt     .Lpositive_slot_count
    movs    r0, #1
    b       .Lstore_slot_count

.Lreg_ime:
    .word   0x04000208
.Lslot_count_early:
    .word   0x02000ec4

.Lpositive_slot_count:
    cmp     r5, #15
    ble     .Lread_metadata
    movs    r0, #16
.Lstore_slot_count:
    str     r0, [r1]

.Lread_metadata:
    ldr     r4, .Lmetadata
    adds    r0, r4, #0
    bl      ReadSaveMetadata

    ldrb    r0, [r4]
    cmp     r0, #'C'
    bne     .Linitialize_metadata
    ldrb    r0, [r4, #1]
    cmp     r0, #'R'
    bne     .Linitialize_metadata
    ldrb    r0, [r4, #2]
    cmp     r0, #'A'
    bne     .Linitialize_metadata
    ldrb    r0, [r4, #3]
    cmp     r0, #'W'
    bne     .Linitialize_metadata
    ldrb    r0, [r4, #4]
    cmp     r0, #'S'
    bne     .Linitialize_metadata
    ldrb    r0, [r4, #5]
    cmp     r0, #'A'
    bne     .Linitialize_metadata
    ldrb    r0, [r4, #6]
    cmp     r0, #'V'
    bne     .Linitialize_metadata
    ldrb    r0, [r4, #7]
    cmp     r0, #'E'
    bne     .Linitialize_metadata
    ldr     r0, .Lslot_count
    ldrb    r4, [r4, #8]
    ldrb    r0, [r0]
    cmp     r4, r0
    beq     .Lcalculate_payload_size

.Linitialize_metadata:
    ldr     r1, .Lmetadata
    movs    r0, #'C'
    strb    r0, [r1]
    movs    r0, #'R'
    strb    r0, [r1, #1]
    movs    r2, #'A'
    strb    r2, [r1, #2]
    movs    r0, #'W'
    strb    r0, [r1, #3]
    movs    r0, #'S'
    strb    r0, [r1, #4]
    strb    r2, [r1, #5]
    movs    r0, #'V'
    strb    r0, [r1, #6]
    movs    r0, #'E'
    strb    r0, [r1, #7]
    ldr     r0, .Lslot_count
    ldr     r0, [r0]
    strb    r0, [r1, #8]
    adds    r0, r1, #0
    movs    r4, #0
    movs    r3, #15
    adds    r2, r0, #0
    adds    r2, #31
.Lclear_slot_flags:
    strb    r4, [r2]
    subs    r2, #1
    subs    r3, #1
    cmp     r3, #0
    bge     .Lclear_slot_flags
    bl      WriteSaveMetadata

.Lcalculate_payload_size:
    ldr     r4, .Lpayload_size
    ldr     r0, .Lslot_count
    ldr     r1, [r0]
    movs    r0, #240
    lsls    r0, r0, #1
    bl      DivideSigned
    adds    r1, r0, #0
    str     r1, [r4]
    movs    r0, #7
    ands    r0, r1
    cmp     r0, #0
    beq     .Lreturn_payload_size
    adds    r3, r4, #0
    movs    r2, #7
.Lalign_payload_size:
    subs    r1, #1
    adds    r0, r1, #0
    ands    r0, r2
    cmp     r0, #0
    bne     .Lalign_payload_size
    str     r1, [r3]

.Lreturn_payload_size:
    ldr     r0, [r4]
    pop     {r4, r5}
    pop     {r1}
    bx      r1
    .size InitSaveSystem, . - InitSaveSystem

    .hword  0
.Lmetadata:       .word 0x02000ed0
.Lslot_count:     .word 0x02000ec4
.Lpayload_size:   .word 0x02000ec0

