    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ FinalizeMenuLayout, 0x08001e1c

    .section .text.build_active_menu_items, "ax", %progbits
    .balign 4
    .global BuildActiveMenuItems
    .type BuildActiveMenuItems, %function
    .thumb_func

BuildActiveMenuItems:
    push    {r4, r5, r6, r7, lr}
    mov     r7, r8
    push    {r7}
    adds    r4, r0, #0
    ldr     r1, .Lactive_item_count
    movs    r0, #0
    strb    r0, [r1]
    movs    r6, #0
    ldr     r0, [r4, #20]
    ldr     r2, .Lmenu_x
    mov     r8, r2
    cmp     r6, r0
    bge     .Lposition_menu

    ldr     r0, .Lactive_item_list
    mov     r12, r0
    adds    r7, r1, #0
    adds    r5, r4, #0
    adds    r5, #24
    adds    r3, r4, #0
    adds    r3, #36

.Lscan_item:
    lsls    r2, r6, #5
    ldr     r1, [r3]
    cmp     r1, #0
    beq     .Ladd_item
    adds    r0, r4, #0
    adds    r0, #40
    adds    r0, r0, r2
    ldr     r1, [r1]
    ldr     r0, [r0]
    ands    r1, r0
    cmp     r1, #0
    beq     .Lskip_item

.Ladd_item:
    ldrb    r1, [r7]
    lsls    r0, r1, #2
    add     r0, r12
    str     r5, [r0]
    ldrb    r0, [r7]
    adds    r0, #1
    strb    r0, [r7]

.Lskip_item:
    adds    r5, #32
    adds    r3, #32
    adds    r6, #1
    ldr     r0, [r4, #20]
    cmp     r6, r0
    blt     .Lscan_item

.Lposition_menu:
    ldr     r1, [r4, #20]
    cmp     r1, #8
    ble     .Lclamped_count
    movs    r1, #8
.Lclamped_count:
    lsls    r1, r1, #4
    ldr     r0, [r4]
    cmp     r0, #0
    beq     .Lposition_left
    asrs    r1, r1, #1
    movs    r0, #96
    subs    r0, r0, r1
    mov     r2, r8
    strb    r0, [r2]
    b       .Lfinish_layout

    .hword 0
.Lactive_item_count: .word 0x020011a0
.Lmenu_x:            .word 0x02001414
.Lactive_item_list:  .word 0x020011b0

.Lposition_left:
    asrs    r1, r1, #1
    movs    r0, #80
    subs    r0, r0, r1
    mov     r1, r8
    strb    r0, [r1]

.Lfinish_layout:
    adds    r0, r4, #0
    bl      FinalizeMenuLayout
    adds    r0, r4, #0
    pop     {r3}
    mov     r8, r3
    pop     {r4, r5, r6, r7}
    pop     {r1}
    bx      r1
    .size BuildActiveMenuItems, . - BuildActiveMenuItems

