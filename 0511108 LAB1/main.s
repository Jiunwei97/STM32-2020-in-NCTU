	.syntax unified
	.cpu cortex-m4
	.thumb

.data

	str: .asciz "Hello World!"
.text
	.global main

main:
	movs r1, #0x20 ;//20H
	movs r2, #0x10 ;//10H
	SUBS r3, r1,r2 ;//20H-10H=10H

	movs r4, #0X5 //5H
	movs r5, #0x9 //9H
	MUL r6, r4,r5 //5H*9H=45H



