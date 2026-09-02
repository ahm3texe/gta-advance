    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ SetTextStyle,       0x0806434c
    .equ ResolveText,        0x0805e6e0
    .equ DrawText,           0x080643d8
    .equ DrawTextWithStyle,  0x0806435c
    .equ DivideSigned,       0x0806b858
    .equ DrawValueText,      0x08064460

    .section .text.draw_menu_items, "ax", %progbits
    .balign 4
    .global DrawMenuItems
    .type DrawMenuItems, %function
    .thumb_func

DrawMenuItems:
    push    {r4, r5, r6, r7, lr}
    mov     r7, r10
    mov     r6, r9
    mov     r5, r8
    push    {r5, r6, r7}
    sub     sp, #40
    adds    r4, r0, #0
    mov     r10, r1
    adds    r5, r2, #0

    ldr     r1, .Lreg_bg1vofs
    ldr     r6, .Lmenu_x_early
    mov     r2, r10
    subs    r0, r2, r5
    lsls    r0, r0, #4
    ldrb    r3, [r6]
    adds    r0, r3, r0
    negs    r0, r0
    strh    r0, [r1]

    movs    r0, #160
    bl      SetTextStyle
    ldr     r0, [r4]
    cmp     r0, #0
    beq     .Lread_item_count
    bl      ResolveText
    ldrb    r2, [r6]
    subs    r2, #32
    movs    r1, #120
    bl      DrawText

.Lread_item_count:
    ldr     r0, .Lactive_item_count_early
    ldrb    r0, [r0]
    mov     r9, r0
    cmp     r0, #8
    ble     .Lsetup_loop
    movs    r0, #8
    mov     r9, r0

.Lsetup_loop:
    adds    r7, r5, #0
    movs    r2, #0
    mov     r8, r2
    cmp     r8, r9
    blt     .Ldraw_next_item
    b       .Lreturn

.Ldraw_next_item:
    cmp     r7, r10
    bne     .Lunselected_style
    movs    r0, #160
    bl      SetTextStyle
    b       .Lload_item

.Lreg_bg1vofs:            .word 0x04000016
.Lmenu_x_early:           .word 0x02001414
.Lactive_item_count_early:.word 0x020011a0

.Lunselected_style:
    movs    r0, #192
    bl      SetTextStyle

.Lload_item:
    ldr     r0, .Lactive_item_list
    lsls    r1, r7, #2
    adds    r4, r1, r0
    ldr     r1, [r4]
    ldr     r0, [r1, #8]
    cmp     r0, #0
    bge     .Ldraw_numeric_item
    ldr     r0, [r1, #4]
    bl      ResolveText
    ldr     r1, .Lmenu_x_middle
    mov     r3, r8
    lsls    r2, r3, #4
    ldrb    r1, [r1]
    adds    r2, r1, r2
    movs    r1, #120
    bl      DrawText
    adds    r7, #1
    mov     r6, r8
    adds    r6, #1
    b       .Ladvance_item

    .hword 0
.Lactive_item_list: .word 0x020011b0
.Lmenu_x_middle:    .word 0x02001414

.Ldraw_numeric_item:
    ldr     r0, [r1, #4]
    bl      ResolveText
    ldr     r1, .Lmenu_x_late
    mov     r2, r8
    lsls    r6, r2, #4
    ldrb    r1, [r1]
    adds    r2, r1, r6
    movs    r1, #4
    bl      DrawTextWithStyle

    ldr     r0, [r4]
    ldr     r4, [r0, #8]
    adds    r0, r4, #0
    ldr     r1, .Lmillion
    bl      DivideSigned
    str     r0, [sp, #24]
    lsls    r2, r0, #5
    subs    r2, r2, r0
    lsls    r1, r2, #6
    subs    r1, r1, r2
    lsls    r1, r1, #3
    adds    r1, r1, r0
    lsls    r1, r1, #6
    subs    r4, r4, r1

    ldr     r5, .Lhundred_thousand
    adds    r0, r4, #0
    adds    r1, r5, #0
    bl      DivideSigned
    str     r0, [sp, #20]
    muls    r0, r5
    subs    r4, r4, r0

    ldr     r5, .Lten_thousand
    adds    r0, r4, #0
    adds    r1, r5, #0
    bl      DivideSigned
    str     r0, [sp, #16]
    muls    r0, r5
    subs    r4, r4, r0

    adds    r0, r4, #0
    movs    r1, #250
    lsls    r1, r1, #2
    bl      DivideSigned
    str     r0, [sp, #12]
    lsls    r1, r0, #5
    subs    r1, r1, r0
    lsls    r1, r1, #2
    adds    r1, r1, r0
    lsls    r1, r1, #3
    subs    r4, r4, r1

    adds    r0, r4, #0
    movs    r1, #100
    bl      DivideSigned
    str     r0, [sp, #8]
    movs    r1, #100
    muls    r0, r1
    subs    r4, r4, r0

    adds    r0, r4, #0
    movs    r1, #10
    bl      DivideSigned
    str     r0, [sp, #4]
    lsls    r1, r0, #2
    adds    r1, r1, r0
    lsls    r1, r1, #1
    subs    r4, r4, r1
    str     r4, [sp]

    add     r1, sp, #28
    movs    r0, #'$'
    strb    r0, [r1]
    movs    r2, #8
    mov     r12, r6
    adds    r5, r1, #0
    adds    r7, #1
    mov     r6, r8
    adds    r6, #1

    adds    r3, r5, #0
    movs    r1, #0
.Lclear_value_buffer:
    adds    r0, r3, r2
    strb    r1, [r0]
    subs    r2, #1
    cmp     r2, #0
    bne     .Lclear_value_buffer

    movs    r1, #0
    movs    r3, #1
    ldr     r0, .Lmenu_x_late
    mov     r8, r0
    adds    r4, r5, #0
    add     r2, sp, #24

.Lconvert_digit:
    cmp     r1, #1
    beq     .Lwrite_digit
    cmp     r1, #0
    bne     .Lnext_digit
    ldr     r0, [r2]
    cmp     r0, #0
    beq     .Lnext_digit
.Lwrite_digit:
    adds    r1, r4, r3
    ldrb    r0, [r2]
    adds    r0, #48
    strb    r0, [r1]
    adds    r3, #1
    movs    r1, #1
.Lnext_digit:
    subs    r2, #4
    .hword  0x456a  @ cmp r2, sp (ARMv4T encoding; avoids assembler deprecation warning)
    bge     .Lconvert_digit

    mov     r3, r8
    ldrb    r2, [r3]
    add     r2, r12
    adds    r0, r5, #0
    movs    r1, #236
    bl      DrawValueText

.Ladvance_item:
    mov     r8, r6
    cmp     r8, r9
    bge     .Lreturn
    b       .Ldraw_next_item

.Lreturn:
    add     sp, #40
    pop     {r3, r4, r5}
    mov     r8, r3
    mov     r9, r4
    mov     r10, r5
    pop     {r4, r5, r6, r7}
    pop     {r0}
    bx      r0
    .size DrawMenuItems, . - DrawMenuItems

    .hword 0
.Lmenu_x_late:      .word 0x02001414
.Lmillion:          .word 1000000
.Lhundred_thousand: .word 100000
.Lten_thousand:     .word 10000
