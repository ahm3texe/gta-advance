    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ InitCartridgeLibrary,  0x0806b864
    .equ InitMemorySubsystem,   0x08005f5c
    .equ WaitForDma3,          0x08063b74
    .equ InitSubsystem,        0x0803251c
    .equ WaitForVBlank,        0x0800cae4
    .equ InitInterrupts,       0x0800038c
    .equ Func_08001a00,        0x08001a00
    .equ Func_080087f4,        0x080087f4
    .equ Func_08000c28,        0x08000c28
    .equ Func_08005fa8,        0x08005fa8
    .equ Func_0800858c,        0x0800858c
    .equ Func_08002600,        0x08002600
    .equ Func_080337a8,        0x080337a8
    .equ Func_0803004c,        0x0803004c
    .equ Func_0805b1c0,        0x0805b1c0
    .equ Func_08008108,        0x08008108
    .equ Func_080664c4,        0x080664c4
    .equ Func_08013824,        0x08013824
    .equ Func_08012248,        0x08012248
    .equ Func_08012198,        0x08012198
    .equ Func_080100d0,        0x080100d0
    .equ Func_0800db7c,        0x0800db7c
    .equ Func_08011cf4,        0x08011cf4
    .equ Func_08012a98,        0x08012a98
    .equ Func_08013098,        0x08013098
    .equ Func_08013434,        0x08013434
    .equ Func_080138e8,        0x080138e8
    .equ Func_08014fa0,        0x08014fa0
    .equ Func_08008e68,        0x08008e68
    .equ Func_08037e7c,        0x08037e7c
    .equ Func_08042784,        0x08042784
    .equ Func_08050918,        0x08050918
    .equ Func_08051300,        0x08051300
    .equ Func_08051960,        0x08051960
    .equ Func_08019c04,        0x08019c04
    .equ Func_08001dc0,        0x08001dc0
    .equ Func_08061dd4,        0x08061dd4
    .equ Func_080389c0,        0x080389c0
    .equ Func_080357cc,        0x080357cc
    .equ Func_0805e118,        0x0805e118
    .equ Func_08033c94,        0x08033c94
    .equ Func_0805e168,        0x0805e168
    .equ Func_0802fe44,        0x0802fe44
    .equ Func_08063d3c,        0x08063d3c

    .section .text.game_init, "ax", %progbits
    .balign 4
    .global GameInit
    .type GameInit, %function
    .thumb_func

GameInit:
    push    {r4, r5, r6, r7, lr}
    mov     r7, r10
    mov     r6, r9
    mov     r5, r8
    push    {r5, r6, r7}
    sub     sp, #16

    ldr     r0, .Lreg_waitcnt
    mov     r8, r0
    ldrh    r0, [r0]
    ldr     r1, .Lwaitcnt_bits
    orrs    r0, r1
    mov     r2, r8
    strh    r0, [r2]

    movs    r5, #0
    str     r5, [sp]
    ldr     r4, .Ldma3_early
    mov     r0, sp
    str     r0, [r4]
    movs    r1, #128
    lsls    r1, r1, #18
    mov     r9, r1
    str     r1, [r4, #4]
    ldr     r0, .Lclear_ewram_control
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]

    str     r5, [sp]
    mov     r2, sp
    str     r2, [r4]
    movs    r6, #192
    lsls    r6, r6, #18
    str     r6, [r4, #4]
    ldr     r0, .Lclear_iwram_control
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]

    add     r0, sp, #4
    strh    r5, [r0]
    str     r0, [r4]
    movs    r7, #192
    lsls    r7, r7, #19
    str     r7, [r4, #4]
    ldr     r0, .Lclear_vram_control
    mov     r10, r0
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]

    movs    r0, #1
    bl      InitCartridgeLibrary

    ldr     r0, .Lewram_fill_value
    str     r0, [sp]
    mov     r1, sp
    str     r1, [r4]
    mov     r2, r9
    str     r2, [r4, #4]
    ldr     r0, .Lewram_fill_control
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]

    ldr     r0, .Liwram_fill_value
    str     r0, [sp]
    str     r1, [r4]
    str     r6, [r4, #4]
    ldr     r0, .Liwram_fill_control
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]

    str     r5, [sp]
    str     r1, [r4]
    str     r6, [r4, #4]
    ldr     r0, .Lclear_iwram_tail_control
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]

    ldr     r1, .Lwaitcnt_bits
    mov     r0, r8
    strh    r1, [r0]
    bl      InitMemorySubsystem
    bl      WaitForDma3

    add     r6, sp, #8
    strh    r5, [r6]
    str     r6, [r4]
    str     r7, [r4, #4]
    mov     r2, r10
    str     r2, [r4, #8]
    ldr     r0, [r4, #8]

    strh    r5, [r6]
    str     r6, [r4]
    movs    r0, #224
    lsls    r0, r0, #19
    str     r0, [r4, #4]
    ldr     r0, .Lclear_oam_control
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]

    ldr     r1, .Lvblank_state_early
    movs    r0, #1
    strb    r0, [r1]
    ldr     r0, .Ldisplay_state_early
    str     r5, [r0]
    ldr     r0, .Lsubsystem_argument_early
    bl      InitSubsystem
    bl      WaitForVBlank
    ldr     r5, .Lbios_irq_flags
    movs    r0, #1
    ldrh    r1, [r5]
    orrs    r0, r1
    strh    r0, [r5]
    bl      InitInterrupts
    bl      Func_08001a00
    bl      Func_080087f4

    movs    r2, #0
    str     r2, [sp, #12]
    adds    r7, r6, #0
    mov     r10, r5
    mov     r8, r4
.Louter_loop:
    movs    r5, #1
    ldr     r0, .Louter_state
    movs    r1, #0
    str     r1, [r0]
    bl      Func_08000c28
    bl      Func_08005fa8
    ldr     r2, [sp, #12]
    cmp     r2, #0
    bne     .Lafter_optional_init
    bl      Func_0800858c
.Lafter_optional_init:
    ldr     r0, [sp, #12]
    bl      Func_08002600
    movs    r0, #0
    str     r0, [sp, #12]
    bl      Func_080337a8
    bl      Func_0803004c
    movs    r0, #0
    bl      Func_0805b1c0
    bl      Func_08008108
    ldr     r0, .Lloop_state
    mov     r1, sp
    ldrb    r1, [r1, #12]
    strb    r1, [r0]
    mov     r9, r0
    movs    r6, #0
    ldr     r4, .Ldma3_early
    b       .Linner_frame_start

.Lreg_waitcnt:                .word 0x04000204
.Lwaitcnt_bits:               .word 0x00004014
.Ldma3_early:                 .word 0x040000d4
.Lclear_ewram_control:        .word 0x85010000
.Lclear_iwram_control:        .word 0x85001f80
.Lclear_vram_control:         .word 0x8100c000
.Lewram_fill_value:           .word 0xcdcdcdcd
.Lewram_fill_control:         .word 0x85402000
.Liwram_fill_value:           .word 0xefefefef
.Liwram_fill_control:         .word 0x85002000
.Lclear_iwram_tail_control:   .word 0x85000040
.Lclear_oam_control:          .word 0x81000200
.Lvblank_state_early:         .word 0x02000130
.Ldisplay_state_early:        .word 0x020004bc
.Lsubsystem_argument_early:   .word 0x000002fd
.Lbios_irq_flags:             .word 0x03007ff8
.Louter_state:                .word 0x0200049c
.Lloop_state:                 .word 0x02000494

.Lcheck_inner_exit:
    bl      Func_080664c4
    cmp     r0, #0
    bne     .Linner_loop_done

.Linner_frame_start:
    ldr     r0, .Lframe_state_source
    mov     r2, r9
    ldrb    r1, [r2]
    strb    r1, [r0]
    ldr     r0, .Lframe_counter
    str     r6, [r0]
    bl      WaitForVBlank
    cmp     r5, #0
    bne     .Lrun_frame

    bl      WaitForDma3
    strh    r5, [r7]
    str     r7, [r4]
    movs    r0, #192
    lsls    r0, r0, #19
    str     r0, [r4, #4]
    ldr     r0, .Lclear_vram_control_late
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]
    strh    r5, [r7]
    str     r7, [r4]
    movs    r0, #224
    lsls    r0, r0, #19
    str     r0, [r4, #4]
    ldr     r0, .Lclear_oam_control_late
    str     r0, [r4, #8]
    ldr     r0, [r4, #8]
    ldr     r1, .Lvblank_state_late
    movs    r0, #1
    strb    r0, [r1]
    ldr     r0, .Ldisplay_state_late
    str     r5, [r0]
    ldr     r0, .Lsubsystem_argument_late
    bl      InitSubsystem
    bl      WaitForVBlank
    movs    r0, #1
    mov     r1, r10
    ldrh    r1, [r1]
    orrs    r0, r1
    mov     r2, r10
    strh    r0, [r2]
    bl      InitInterrupts

.Lrun_frame:
    mov     r0, r9
    strb    r6, [r0]
    ldr     r0, .Lframe_reset
    str     r6, [r0]
    bl      Func_08013824
    bl      Func_08012248
    bl      Func_08012198
    bl      Func_080100d0
    bl      Func_0800db7c
    bl      Func_08011cf4
    bl      Func_08012a98
    bl      Func_08013098
    bl      Func_08013434
    bl      Func_080138e8
    bl      Func_08014fa0
    bl      Func_08008e68
    bl      Func_08037e7c
    bl      Func_08042784
    bl      Func_08050918
    bl      Func_08051300
    bl      Func_08051960
    bl      Func_08019c04
    bl      Func_08001dc0
    bl      Func_08061dd4
    bl      Func_080389c0
    bl      Func_080357cc
    bl      Func_0805e118
    bl      Func_08033c94
    bl      Func_0805e168
    ldr     r0, .Lpost_frame_state
    str     r6, [r0]
    bl      Func_0802fe44
    bl      Func_08063d3c
    ldr     r0, .Lreg_ime
    strh    r6, [r0]
    bl      Func_080337a8
    movs    r5, #0
    mov     r1, r9
    ldrb    r1, [r1]
    cmp     r1, #1
    bne     .Lcheck_inner_exit

.Linner_loop_done:
    bl      Func_080664c4
    cmp     r0, #0
    beq     .Lnormalize_game_state
    movs    r2, #1
    str     r2, [sp, #12]

.Lnormalize_game_state:
    ldr     r1, .Lgame_state
    ldrb    r0, [r1, #12]
    subs    r0, #1
    lsls    r0, r0, #24
    lsrs    r0, r0, #24
    cmp     r0, #1
    bhi     .Lreset_display
    movs    r0, #0
    strb    r0, [r1, #12]

.Lreset_display:
    bl      WaitForDma3
    movs    r1, #0
    strh    r1, [r7]
    mov     r2, r8
    str     r7, [r2]
    movs    r0, #192
    lsls    r0, r0, #19
    str     r0, [r2, #4]
    ldr     r0, .Lclear_vram_control_late
    str     r0, [r2, #8]
    ldr     r0, [r2, #8]
    strh    r1, [r7]
    str     r7, [r2]
    movs    r0, #224
    lsls    r0, r0, #19
    str     r0, [r2, #4]
    ldr     r0, .Lclear_oam_control_late
    str     r0, [r2, #8]
    ldr     r0, [r2, #8]
    ldr     r1, .Lvblank_state_late
    movs    r0, #1
    strb    r0, [r1]
    ldr     r0, .Ldisplay_state_late
    movs    r1, #0
    str     r1, [r0]
    ldr     r0, .Lsubsystem_argument_late
    bl      InitSubsystem
    bl      WaitForVBlank
    movs    r0, #1
    mov     r2, r10
    ldrh    r2, [r2]
    orrs    r0, r2
    mov     r1, r10
    strh    r0, [r1]
    bl      InitInterrupts
    b       .Louter_loop

.Lframe_state_source:        .word 0x02000cf8
.Lframe_counter:             .word 0x02000eb4
.Lclear_vram_control_late:   .word 0x8100c000
.Lclear_oam_control_late:    .word 0x81000200
.Lvblank_state_late:         .word 0x02000130
.Ldisplay_state_late:        .word 0x020004bc
.Lsubsystem_argument_late:   .word 0x000002fd
.Lframe_reset:               .word 0x020003e0
.Lpost_frame_state:          .word 0x02000ebc
.Lreg_ime:                   .word 0x04000208
.Lgame_state:                .word 0x02000ce0

    .size GameInit, . - GameInit
