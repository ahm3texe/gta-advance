    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ SAVE_METADATA,    0x02000ed0
    .equ ReadEepromBytes,  0x0800091c
    .equ WriteEepromBytes, 0x080009ec

    .section .text.save_wrappers, "ax", %progbits
    .balign 4

    .global IsSaveSlotValid
    .type IsSaveSlotValid, %function
    .thumb_func
IsSaveSlotValid:
    push    {r4, r5, lr}
    adds    r4, r0, #0
    ldr     r5, .Lsave_metadata
    adds    r0, r5, #0
    bl      ReadSaveMetadata
    adds    r4, #16
    adds    r4, r4, r5
    ldrb    r0, [r4]
    pop     {r4, r5}
    pop     {r1}
    bx      r1
    .size IsSaveSlotValid, . - IsSaveSlotValid

.Lsave_metadata:
    .word SAVE_METADATA

    .global ReadSaveMetadata
    .type ReadSaveMetadata, %function
    .thumb_func
ReadSaveMetadata:
    push    {lr}
    adds    r2, r0, #0
    movs    r0, #0
    movs    r1, #32
    bl      ReadEepromBytes
    movs    r0, #1
    pop     {r1}
    bx      r1
    .size ReadSaveMetadata, . - ReadSaveMetadata
    .hword  0

    .global WriteSaveMetadata
    .type WriteSaveMetadata, %function
    .thumb_func
WriteSaveMetadata:
    push    {lr}
    adds    r2, r0, #0
    movs    r0, #0
    movs    r1, #32
    bl      WriteEepromBytes
    movs    r0, #1
    pop     {r1}
    bx      r1
    .size WriteSaveMetadata, . - WriteSaveMetadata
    .hword  0
