    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ Func_080512b0, 0x080512b0
    .equ Func_08004280, 0x08004280

    .section .text.menu_helpers, "ax", %progbits
    .balign 4

    .global ResetMenuState
    .type ResetMenuState, %function
    .thumb_func
ResetMenuState:
    push    {r4, lr}
    ldr     r0, .Lmenu_x
    movs    r1, #0
    strb    r1, [r0]
    ldr     r0, .Lactive_item_count
    strb    r1, [r0]
    ldr     r3, .Lmenu_flags
    ldr     r4, .Lmenu_runtime
    ldr     r1, .Lactive_item_list
    movs    r2, #0
    adds    r0, r1, #0
    adds    r0, #76
.Lclear_active_items:
    str     r2, [r0]
    subs    r0, #4
    cmp     r0, r1
    bge     .Lclear_active_items
    movs    r0, #0
    str     r0, [r3]
    str     r0, [r4]
    pop     {r4}
    pop     {r0}
    bx      r0
    .size ResetMenuState, . - ResetMenuState

.Lmenu_x:            .word 0x02001414
.Lactive_item_count: .word 0x020011a0
.Lmenu_flags:        .word 0x02001410
.Lmenu_runtime:      .word 0x02010f44
.Lactive_item_list:  .word 0x020011b0

    .global IsMenuFlagSet
    .type IsMenuFlagSet, %function
    .thumb_func
IsMenuFlagSet:
    ldr     r2, .Lmenu_flags_check
    movs    r1, #1
    lsls    r1, r0
    ldr     r0, [r2]
    ands    r0, r1
    cmp     r0, #0
    bne     .Lflag_set
    movs    r0, #0
    b       .Lflag_return
    .hword 0
.Lmenu_flags_check: .word 0x02001410
.Lflag_set:
    movs    r0, #1
.Lflag_return:
    bx      lr
    .size IsMenuFlagSet, . - IsMenuFlagSet

    .global FinalizeMenuLayout
    .type FinalizeMenuLayout, %function
    .thumb_func
FinalizeMenuLayout:
    push    {lr}
    bl      Func_080512b0
    adds    r0, #108
    movs    r1, #0
    bl      Func_08004280
    pop     {r0}
    bx      r0
    .size FinalizeMenuLayout, . - FinalizeMenuLayout
    .hword 0

