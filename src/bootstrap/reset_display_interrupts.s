    .syntax unified
    .cpu arm7tdmi
    .thumb

    .equ WaitForDma3,      0x08063b74
    .equ InitSubsystem,    0x0803251c
    .equ WaitForVBlank,    0x0800cae4
    .equ InitInterrupts,   0x0800038c

    .section .text.reset_display_interrupts, "ax", %progbits
    .balign 4
    .global ResetDisplayAndInterrupts
    .type ResetDisplayAndInterrupts, %function
    .thumb_func

ResetDisplayAndInterrupts:
    push    {lr}
    sub     sp, #4
    bl      WaitForDma3

    mov     r0, sp
    movs    r2, #0
    strh    r2, [r0]
    ldr     r1, .Ldma3
    str     r0, [r1, #0]
    movs    r0, #0xc0
    lsls    r0, r0, #19
    str     r0, [r1, #4]
    ldr     r0, .Lclear_vram_control
    str     r0, [r1, #8]
    ldr     r0, [r1, #8]

    mov     r0, sp
    strh    r2, [r0]
    str     r0, [r1, #0]
    movs    r0, #0xe0
    lsls    r0, r0, #19
    str     r0, [r1, #4]
    ldr     r0, .Lclear_oam_control
    str     r0, [r1, #8]
    ldr     r0, [r1, #8]

    ldr     r1, .Lvblank_state
    movs    r0, #1
    strb    r0, [r1]
    ldr     r1, .Ldisplay_state
    movs    r0, #0
    str     r0, [r1]
    ldr     r0, .Lsubsystem_argument
    bl      InitSubsystem
    bl      WaitForVBlank

    ldr     r1, .Lbios_irq_flags
    movs    r0, #1
    ldrh    r2, [r1]
    orrs    r0, r2
    strh    r0, [r1]
    bl      InitInterrupts

    add     sp, #4
    pop     {r0}
    bx      r0
    .size ResetDisplayAndInterrupts, . - ResetDisplayAndInterrupts

    .hword  0
.Ldma3:                  .word 0x040000d4
.Lclear_vram_control:    .word 0x8100c000
.Lclear_oam_control:     .word 0x81000200
.Lvblank_state:          .word 0x02000130
.Ldisplay_state:         .word 0x020004bc
.Lsubsystem_argument:    .word 0x000002fd
.Lbios_irq_flags:        .word 0x03007ff8

