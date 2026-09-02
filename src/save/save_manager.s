    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ SaveIoEnter,      0x080337a8
    .equ SaveIoLeave,      0x08033b74
    .equ IdentifyEeprom,   0x0806bd34
    .equ ReadEepromRange,  0x08000ddc
    .equ WriteEepromRange, 0x08000f1c
    .equ PrepareSaveWrite, 0x0802fe44
    .equ GetSaveNonce,     0x08032548

    .section .text.save_manager, "ax", %progbits
    .balign 4

    .global InitSaveManager
    .type InitSaveManager, %function
    .thumb_func
InitSaveManager:
    push    {r4, r5, r6, r7, lr}
    mov     r7, r9
    mov     r6, r8
    push    {r6, r7}
    sub     sp, #4
    bl      SaveIoEnter
    ldr     r0, .Lstate_0
    movs    r1, #0
    str     r1, [r0]
    ldr     r0, .Lstate_1
    str     r1, [r0]
    ldr     r0, .Lstate_2
    strb    r1, [r0]
    ldr     r0, .Lstate_3
    strh    r1, [r0]
    ldr     r0, .Lreg_ime_early
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
    mov     r9, r0

    ldr     r1, .Lslot_headers_early
    movs    r2, #0
    adds    r0, r1, #0
    adds    r0, #24
.Lclear_slot_headers:
    strb    r2, [r0]
    subs    r0, #12
    cmp     r0, r1
    bge     .Lclear_slot_headers

    movs    r0, #4
    bl      IdentifyEeprom
    lsls    r0, r0, #16
    cmp     r0, #0
    beq     .Lscan_slot_headers
    movs    r0, #0
    mov     r9, r0
    b       .Lfinish_init

    .hword 0
.Lstate_0:                .word 0x02000f70
.Lstate_1:                .word 0x02000f0c
.Lstate_2:                .word 0x02001050
.Lstate_3:                .word 0x02001130
.Lreg_ime_early:          .word 0x04000208
.Ldma3:                   .word 0x040000d4
.Leeprom_available_early: .word 0x02000eb8
.Lslot_headers_early:     .word 0x02000460

.Lscan_slot_headers:
    movs    r7, #0
    movs    r5, #0
    movs    r1, #156
    negs    r1, r1
    mov     r8, r1
    movs    r6, #2
.Lscan_next_slot:
    ldr     r0, .Lslot_headers
    adds    r4, r7, r0
    adds    r0, r5, #0
    adds    r1, r4, #0
    movs    r2, #12
    bl      ReadEepromRange
    cmp     r0, #0
    beq     .Lstore_slot_status
    mov     r1, r8
    subs    r0, r5, r1
    mov     r1, sp
    movs    r2, #1
    bl      ReadEepromRange
    cmp     r0, #0
    beq     .Lstore_slot_status
    mov     r0, sp
    ldrb    r0, [r0]
    ldrb    r1, [r4]
    adds    r0, r0, r1
    cmp     r0, #255
    beq     .Ladvance_slot
    movs    r0, #0
.Lstore_slot_status:
    strb    r0, [r4]
.Ladvance_slot:
    adds    r7, #12
    adds    r5, #160
    subs    r6, #1
    cmp     r6, #0
    bge     .Lscan_next_slot

.Lfinish_init:
    ldr     r1, .Leeprom_available
    movs    r0, #0
    str     r0, [r1]
    ldr     r1, .Lreg_ime
    movs    r0, #1
    strh    r0, [r1]
    bl      SaveIoLeave
    mov     r0, r9
    add     sp, #4
    pop     {r3, r4}
    mov     r8, r3
    mov     r9, r4
    pop     {r4, r5, r6, r7}
    pop     {r1}
    bx      r1
    .size InitSaveManager, . - InitSaveManager

.Lslot_headers:     .word 0x02000460
.Leeprom_available: .word 0x02000eb8
.Lreg_ime:          .word 0x04000208

    .global LoadSaveSlot
    .type LoadSaveSlot, %function
    .thumb_func
LoadSaveSlot:
    push    {r4, lr}
    adds    r2, r0, #0
    cmp     r2, #2
    bhi     .Lload_fail
    ldr     r0, .Lload_headers
    lsls    r1, r2, #1
    adds    r1, r1, r2
    lsls    r1, r1, #2
    adds    r1, r1, r0
    ldrb    r0, [r1]
    cmp     r0, #0
    beq     .Lload_fail
    cmp     r1, #0
    beq     .Lload_fail
    lsls    r0, r2, #2
    adds    r0, r0, r2
    lsls    r0, r0, #5
    ldr     r4, .Lsave_buffer
    adds    r1, r4, #0
    movs    r2, #160
    bl      ReadEepromRange
    adds    r2, r0, #0
    cmp     r2, #0
    beq     .Lload_delay
    adds    r0, r4, #0
    adds    r0, #156
    ldrb    r1, [r4]
    ldrb    r0, [r0]
    adds    r0, r0, r1
    cmp     r0, #255
    bne     .Lload_fail
    cmp     r1, #0
    bne     .Lload_delay
.Lload_fail:
    movs    r0, #0
    b       .Lload_return

.Lload_headers: .word 0x02000460
.Lsave_buffer:  .word 0x02000d50

.Lload_delay:
    movs    r0, #3
.Lload_delay_loop:
    subs    r0, #1
    cmp     r0, #0
    bge     .Lload_delay_loop
    adds    r0, r2, #0
.Lload_return:
    pop     {r4}
    pop     {r1}
    bx      r1
    .size LoadSaveSlot, . - LoadSaveSlot

    .global WriteGameSaveSlot
    .type WriteGameSaveSlot, %function
    .thumb_func
WriteGameSaveSlot:
    push    {r4, r5, r6, r7, lr}
    adds    r4, r0, #0
    bl      PrepareSaveWrite
    cmp     r4, #2
    bls     .Lprepare_save_buffer
    movs    r0, #0
    b       .Lwrite_game_return

.Lprepare_save_buffer:
    lsls    r6, r4, #1
    lsls    r7, r4, #2
    ldr     r5, .Lwrite_save_buffer
.Lget_nonzero_nonce:
    bl      GetSaveNonce
    strb    r0, [r5]
    lsls    r0, r0, #24
    cmp     r0, #0
    beq     .Lget_nonzero_nonce

    ldr     r1, .Lwrite_save_buffer
    ldrb    r0, [r1]
    mvns    r2, r0
    adds    r0, r1, #0
    adds    r0, #156
    strb    r2, [r0]

    adds    r2, r6, r4
    lsls    r2, r2, #2
    ldr     r0, .Lwrite_headers
    adds    r2, r2, r0
    adds    r0, r1, #0
    ldmia   r0!, {r3, r5, r6}
    stmia   r2!, {r3, r5, r6}

    adds    r0, r7, r4
    lsls    r0, r0, #5
    movs    r2, #160
    bl      WriteEepromRange
    movs    r1, #3
.Lwrite_delay_loop:
    subs    r1, #1
    cmp     r1, #0
    bge     .Lwrite_delay_loop
.Lwrite_game_return:
    pop     {r4, r5, r6, r7}
    pop     {r1}
    bx      r1
    .size WriteGameSaveSlot, . - WriteGameSaveSlot

.Lwrite_save_buffer: .word 0x02000d50
.Lwrite_headers:     .word 0x02000460

