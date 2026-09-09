@ BIOS syscall thunks — 0x0806B84C-0x0806B88B
@
@ GBA BIOS calls are made with the Thumb `swi` instruction. These cannot be
@ converted to C: inline asm is forbidden in this project (docs/WORKFLOW.md),
@ and `swi` cannot be produced without it. So this is PERMANENT assembly
@ source, like agb_main.s and intr_main.s.
@
@ FULL COVERAGE: the ROM was scanned, and these are the only real thunks of
@ the `swi` + `bx lr` shape. These ten functions are the game's entire contact
@ surface with the BIOS; swi is used nowhere else.
@
@ In particular: there is NO thunk where `swi 6` is followed by
@ `adds r0, r1, #0`, so the game never uses the remainder (r1) of the BIOS
@ Div, and the `int Div(int, int)` declaration on the C side is complete.
@ The BIOS returns three values (r0 quotient, r1 remainder, r3 absolute
@ quotient); a C call can only take r0, but that is not needed here.
@
@ Verification:  make bios-match

	.text
	.thumb
	.align 1

	.global ArcTan2
	.thumb_func
ArcTan2:                        @ 0x0806B84C
	swi 10
	bx lr

	.global CpuFastSet
	.thumb_func
CpuFastSet:                     @ 0x0806B850
	swi 12
	bx lr

	.global CpuSet
	.thumb_func
CpuSet:                         @ 0x0806B854
	swi 11
	bx lr

	.global Div
	.thumb_func
Div:                            @ 0x0806B858
	swi 6
	bx lr

	.global LZ77UnCompWram
	.thumb_func
LZ77UnCompWram:                 @ 0x0806B85C
	swi 17
	bx lr

	.global RLUnCompVram
	.thumb_func
RLUnCompVram:                   @ 0x0806B860
	swi 21
	bx lr

	.global RegisterRamReset
	.thumb_func
RegisterRamReset:               @ 0x0806B864
	swi 1
	bx lr

	@ Disables interrupts, sets the stack to the top of IWRAM, clears RAM
	@ and performs a soft reset. Does not return.
	.global SoftResetSystem
	.thumb_func
SoftResetSystem:                @ 0x0806B868
	ldr r3, =0x04000208
	movs r2, #0
	strb r2, [r3]
	ldr r1, =0x03007F00
	mov sp, r1
	swi 1
	swi 0
	.align 2, 0
	.ltorg

	.global Sqrt
	.thumb_func
Sqrt:                           @ 0x0806B880
	swi 8
	bx lr

	.global VBlankIntrWait
	.thumb_func
VBlankIntrWait:                 @ 0x0806B884
	movs r2, #0
	swi 5
	bx lr

	.align 2, 0
