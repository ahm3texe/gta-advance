    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ LoadGraphicsResource, 0x0806430c

    .section .text.menu_graphics, "ax", %progbits
    .balign 4

    .global LoadMenuGraphics
    .type LoadMenuGraphics, %function
    .thumb_func
LoadMenuGraphics:
    push    {r4, r5, lr}
    sub     sp, #8
    movs    r1, #192
    lsls    r1, r1, #19
    adds    r0, r0, r1
    ldr     r2, .Lresource_a
    ldr     r3, .Lresource_b
    movs    r1, #160
    str     r1, [sp]
    movs    r4, #0
    str     r4, [sp, #4]
    movs    r1, #30
    bl      LoadGraphicsResource

    ldr     r3, .Lreg_ime
    ldrh    r2, [r3]
    strh    r4, [r3]
    ldr     r1, .Ldma3
    ldr     r0, .Lpalette_source_0
    str     r0, [r1]
    ldr     r0, .Lpalette_dest_0
    str     r0, [r1, #4]
    ldr     r5, .Lpalette_dma_control
    str     r5, [r1, #8]
    ldr     r0, [r1, #8]
    strh    r2, [r3]

    ldrh    r2, [r3]
    strh    r4, [r3]
    ldr     r0, .Lpalette_source_1
    str     r0, [r1]
    ldr     r0, .Lpalette_dest_1
    str     r0, [r1, #4]
    str     r5, [r1, #8]
    ldr     r0, [r1, #8]
    strh    r2, [r3]

    add     sp, #8
    pop     {r4, r5}
    pop     {r0}
    bx      r0
    .size LoadMenuGraphics, . - LoadMenuGraphics

    .hword 0
.Lresource_a:           .word 0x08831880
.Lresource_b:           .word 0x0883f880
.Lreg_ime:              .word 0x04000208
.Ldma3:                 .word 0x040000d4
.Lpalette_source_0:     .word 0x08831680
.Lpalette_dest_0:       .word 0x05000140
.Lpalette_dma_control:  .word 0x80000010
.Lpalette_source_1:     .word 0x08ec7a44
.Lpalette_dest_1:       .word 0x05000180

    .global ClearMenuVram
    .type ClearMenuVram, %function
    .thumb_func
ClearMenuVram:
    push    {r4, lr}
    sub     sp, #4
    ldr     r3, .Lclear_reg_ime
    ldrh    r2, [r3]
    movs    r0, #0
    strh    r0, [r3]
    mov     r1, sp
    ldr     r4, .Lclear_value
    adds    r0, r4, #0
    strh    r0, [r1]
    ldr     r1, .Lclear_dma3
    mov     r0, sp
    str     r0, [r1]
    movs    r0, #192
    lsls    r0, r0, #19
    str     r0, [r1, #4]
    ldr     r0, .Lclear_vram_control
    str     r0, [r1, #8]
    ldr     r0, [r1, #8]
    strh    r2, [r3]
    add     sp, #4
    pop     {r4}
    pop     {r0}
    bx      r0
    .size ClearMenuVram, . - ClearMenuVram

.Lclear_reg_ime:       .word 0x04000208
.Lclear_value:         .word 0x00009090
.Lclear_dma3:          .word 0x040000d4
.Lclear_vram_control:  .word 0x81004b00

    .global EnableMenuDisplay
    .type EnableMenuDisplay, %function
    .thumb_func
EnableMenuDisplay:
    movs    r1, #128
    lsls    r1, r1, #19
    ldr     r2, .Ldisplay_control_0
    adds    r0, r2, #0
    strh    r0, [r1]
    bx      lr
    .size EnableMenuDisplay, . - EnableMenuDisplay
.Ldisplay_control_0: .word 0x00000101

    .global EnableMenuDisplayAlt
    .type EnableMenuDisplayAlt, %function
    .thumb_func
EnableMenuDisplayAlt:
    movs    r1, #128
    lsls    r1, r1, #19
    ldr     r2, .Ldisplay_control_1
    adds    r0, r2, #0
    strh    r0, [r1]
    bx      lr
    .size EnableMenuDisplayAlt, . - EnableMenuDisplayAlt
.Ldisplay_control_1: .word 0x00000101

