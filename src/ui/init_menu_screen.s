    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ Func_08063cf0, 0x08063cf0
    .equ Func_080353bc, 0x080353bc
    .equ Func_08012248, 0x08012248
    .equ Func_080104c0, 0x080104c0
    .equ Func_0800dd50, 0x0800dd50
    .equ Func_0802f55c, 0x0802f55c
    .equ Func_0800ab6c, 0x0800ab6c
    .equ Func_0803493c, 0x0803493c
    .equ Func_08063b98, 0x08063b98
    .equ Func_0803c49c, 0x0803c49c
    .equ Func_080348b4, 0x080348b4

    .section .text.init_menu_screen, "ax", %progbits
    .balign 4
    .global InitMenuScreen
    .type InitMenuScreen, %function
    .thumb_func

InitMenuScreen:
    push    {r4, r5, lr}
    sub     sp, #4
    ldr     r5, .Lpalette_source
    bl      Func_08063cf0
    bl      Func_080353bc
    bl      Func_08012248

    ldr     r1, .Lreg_bldcnt
    movs    r0, #255
    strh    r0, [r1]
    ldr     r0, .Lreg_bldalpha
    movs    r4, #0
    strh    r4, [r0]

    ldr     r3, .Lreg_ime
    ldrh    r2, [r3]
    strh    r4, [r3]
    adds    r1, #132
    str     r5, [r1]
    movs    r0, #160
    lsls    r0, r0, #19
    str     r0, [r1, #4]
    ldr     r0, .Lpalette_dma_control
    str     r0, [r1, #8]
    ldr     r0, [r1, #8]
    strh    r2, [r3]

    ldrh    r2, [r3]
    strh    r4, [r3]
    mov     r0, sp
    strh    r4, [r0]
    str     r0, [r1]
    movs    r0, #192
    lsls    r0, r0, #19
    str     r0, [r1, #4]
    ldr     r0, .Lvram_dma_control
    str     r0, [r1, #8]
    ldr     r0, [r1, #8]
    strh    r2, [r3]

    bl      Func_080104c0
    bl      Func_0800dd50
    bl      Func_0802f55c
    bl      Func_0800ab6c
    bl      Func_0803493c
    bl      Func_08063b98

    ldr     r1, .Lframe_counter
    movs    r0, #1
    str     r0, [r1]
    bl      Func_0803c49c
    cmp     r0, #0
    beq     .Lreturn
    adds    r0, #55
    ldrb    r0, [r0]
    cmp     r0, #0
    beq     .Lreturn
    ldr     r0, .Lmenu_resource_id
    bl      Func_080348b4

.Lreturn:
    add     sp, #4
    pop     {r4, r5}
    pop     {r0}
    bx      r0
    .size InitMenuScreen, . - InitMenuScreen

    .hword 0
.Lpalette_source:      .word 0x02001210
.Lreg_bldcnt:          .word 0x04000050
.Lreg_bldalpha:        .word 0x04000052
.Lreg_ime:             .word 0x04000208
.Lpalette_dma_control: .word 0x80000100
.Lvram_dma_control:    .word 0x81004b00
.Lframe_counter:       .word 0x03000004
.Lmenu_resource_id:    .word 0x0000011b

