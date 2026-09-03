@ BIOS syscall thunk'lari — 0x0806B84C-0x0806B88B
@
@ GBA BIOS cagrilari Thumb `swi` komutuyla yapiliyor. Bunlar C'ye
@ cevrilemez: inline asm projede yasak (docs/WORKFLOW.md), asm'siz de
@ `swi` uretilemez. Bu yuzden agb_main.s ve intr_main.s gibi KALICI
@ assembly kaynagi.
@
@ Dogrulama:  make bios-match

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

	@ Kesmeleri kapatip yigini IWRAM'in tepesine kurar, RAM'i sifirlar
	@ ve yumusak sifirlama yapar. Geri donmez.
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
