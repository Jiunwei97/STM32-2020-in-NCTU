	.syntax unified
	.cpu cortex-m4
	.thumb
.data
	result: .word 0 //TODO: store result
	N: .word 0      //fib(N),function input
.text
	.global main

	.equ RCC_AHB2ENR, 0x4002104C
	.equ GPIOA_MODER, 0x48000000
	.equ GPIOA_OTYPER, 0x48000004
	.equ GPIOA_OSPEEDR, 0x48000008
	.equ GPIOA_PUPDR, 0x4800000C
	.equ GPIOA_ODR, 0x48000014
	.equ GPIOC_MODER, 0x48000800
	.equ GPIOC_IDR, 0x48000810
	.equ DECODE_MODE, 0x09
	.equ SCAN_LIMIT, 0x0B
	.equ DISPLAY_TEST, 0x0F
	.equ SHUTDOWN, 0x0C
	.equ INTENSITY,0x0A
	.equ DATA, 0x20 //PA5
	.equ LOAD, 0x40 //PA6
	.equ CLOCK, 0x80 //PA7
	.equ LEN, 0x7
main:
	BL GPIO_init
	BL max7219_init
	BL ITOA					   //Display xxxxxxx0
LOOP:
		ldr r2, =GPIOC_IDR
		movs r10, #1       //mask PC13(button)
		lsl r10, #13
		ldr r9, [r2]		//button input
		ands r9, r10
		cmp r9, r10
		beq LOOP			//no pressed
		bl do_pushed		//pressed

		cmp r4,#2			//r4==2(long pressed)
		beq RESET

		cmp r4,#1			//r4==1(short pressed)
		beq INCR
        b LOOP
		RESET:
		ldr r4,=N			//reset result and N
		movs r5,#0
		str r5,[r4]
		ldr r4,=result
		str r5,[r4]
		B SEND

		INCR:
		ldr r4,=N			//N increment
		ldr r5,[r4]
		adds r5,#1
		str r5,[r4]
		BL fib				//fibonacci

		SEND:
		BL ITOA				//send result to max7219
		B LOOP


GPIO_init:
    movs r0,#0x5
	ldr r1, =RCC_AHB2ENR
	str r0, [r1]

	movs r0,#0x5400
	ldr r1,=GPIOA_MODER
	ldr r2,[r1]
	and r2,#0xFFFF03FF
	orrs r2,r2,r0
	str r2,[r1]

	movs r0,#0xA800
	ldr r1,=GPIOA_OSPEEDR
	strh r0,[r1]
	ldr r3,=GPIOA_ODR

	ldr r1, =GPIOC_MODER
	ldr r0, [r1]
	ldr r2, =#0xF3FFFFFF
	and r0, r2
	str r0, [r1]

	ldr r2, =GPIOC_IDR
	ldr r12,=#50000
	BX LR


MAX7219Send:
	ldr r3,=GPIOA_ODR
	lsl r0, r0, #8
	add r0, r0, r1
	ldr r2, =#LOAD
	ldr r5, =#DATA
	ldr r4, =#CLOCK
	mov r7, #15//r7 = i
	mov r1, #0
	str r1,[r3]

	max7219send_loop:
	mov r1, #1
	lsl r1, r7
	ands r1,r0
	cmp r1,#0
	beq low
	b high

	low:
	movs r1,#0
	b cont

	high:
	movs r1,r5

	cont:
	orr r1,r4
	str r1,[r3]

	ands r1,r5
	str r1,[r3]

	subs r7, r7, #1
	bge max7219send_loop
	movs r1,r2
	orr r1,r4
	str r1, [r3]
	movs r1,#0
	str r1, [r3]
	BX LR


max7219_init:
	push {lr}
	ldr r0, =#DECODE_MODE
	ldr r1, =#0xFF
	BL MAX7219Send
	ldr r0, =#DISPLAY_TEST
	ldr r1, =#0x0
	BL MAX7219Send
	ldr r0, =#SCAN_LIMIT
	ldr r1, =0x7			//Display digits 0~7
	BL MAX7219Send
	ldr r0, =#INTENSITY
	ldr r1, =#0xA
	BL MAX7219Send
	ldr r0, =#SHUTDOWN
	ldr r1, =#0x1
	BL MAX7219Send
	pop {pc}


ITOA:
	push {lr}
	ldr r6,=result
	ldr r6,[r6]
	ldr r10,=#10000000		//divisor
	mov r9,#10
	mov r11,#8				//ith digit

	cmp r6,#0             //if result==0
	beq zero

	cmp r6,#-1				//if result==-1
	beq error
	b EMP					//else

	error:					//result==-1
	mov r0,r11
	mov r1,#0xF				//digit 7~2 all off
	BL MAX7219Send
	subs r11,#1
	cmp r11,#2
	bne error
	mov r0,r11				//Digit 1
	mov r1,#0XA				//'-'
	BL MAX7219Send
	subs r11,#1
	mov r0,r11				//Digit 0
	mov r1,#1				//'1'
	BL MAX7219Send
	pop {pc}

	zero:					//result==0
	mov r0,r11
	mov r1,#0xF				//digit 7~1 all off
	BL MAX7219Send
	subs r11,#1
	cmp r11,#1
	bne zero
	mov r0,r11				//Digit 0
	mov r1,#0				//'0'
	BL MAX7219Send
	pop {pc}

	EMP:
	mov r0,r11				//check digit i is empty
	mov r8,r6
	udiv r1,r8,r10
	cmp r1,#0
	bne ILOOP				//if not, branch ILOOP
	mov r1,#0xF				//if empty
	BL MAX7219Send			//digit i display black
	udiv r10,r10,r9
	subs r11,#1
	bgt EMP


	ILOOP:
	mov r0,r11				//Display ith digit
	mov r8,r6
	udiv r1,r8,r10
	mul r8,r1,r10
	subs r6,r8
	BL MAX7219Send
	udiv r10,r10,r9
	subs r11,#1
	bgt ILOOP
	pop {pc}

do_pushed:
	ldr r12,=#50000			//short pressed target
	ldr r2, =GPIOC_IDR
	movs r10, #1
	lsl r10, #13
	movs r4,#0
	//BL Delay
	DLOOP:					//loop until no pressed
	adds r4,#1
	ldr r9, [r2]
	ands r9, r10
	cmp r9, r10
	bne DLOOP

	ldr r9,=#500000		//long pressed target

	cmp r4,r12			//if less than
	ble nope				//short pressed target
	b spressed

	nope:
	movs r4, #0
	BX LR

	spressed:
	cmp r4,r9			//if less than
	bge lpressed			//long pressed target
	movs r4, #1
	BX LR

	lpressed:
	movs r4, #2        //if greater than
	BX LR					//long pressed target





fib:
	ldr r0,=N
	ldr r0,[r0]
	cmp R0,#0
	beq noact			//N==0
	cmp R0,#1
	beq noact			//N==1
	mov R4,#-1
	cmp R0,#100
	bgt exit
	mov R1,#0
	mov R4,#1
    mov R3,0x1
floop:
	add R3,R3,#1		//i
	add R5,R1,R4		//temp=F[n]+F[n+1];
	mov R1,R4			//F[n]=F[n+1]
	mov R4,R5			//F[n+1]=temp
	cmp R3,R0			//i==N?
	beq exit
	b floop
noact:
	mov r4,r0
	b exit
overflow:
	mov R4,#-1
	ldr r5,=result
	str r4,[r5]
	bx lr
exit:
    ldr r3,=0x5F5E100	//fib(N)>=100000000
	cmp r4,r3
	bge overflow		//Overflow
	ldr r5,=result
	str r4,[r5]
	bx lr
