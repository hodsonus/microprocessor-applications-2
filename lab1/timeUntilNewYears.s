	.def timeUntilNewYears

	.ref Modulus

	.thumb
	.align 2
	.text

timeUntilNewYears:

	.asmfunc

	PUSH {R4 - R8} ; push what we use from R4 -> r9 here
	PUSH {LR}

	ldrb r4, [r0, #0] ; month
	ldrb r5, [r0, #1] ; day

	subi r4, r4, #1
	subi r5, r5, #1

	ldrb r6, [r0, #2] ; hour
	ldrb r7, [r0, #3] ; minute

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	mov r8, #30
	mul r4, r4, r8
	add r4, r4, r5 ; r4 now contains current day of the year

	mov r8, #360
	sub r4, r8, r4 ; r4 now contains the amount of days left in the year

	mov r8, #30
	udiv r0, r4, r8 ; r0 -> month return value

	push {r0}
	mov r0, r4 ; day remaining in year
	mov r1, #30 ; mod 30
	BL Modulus
	mov r1, r0 ; gives day remaining of month, put this into r1
	pop {r0}

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	mov r8, #60
	mul r6, r6, r8
	add r6, r6, r7 ; gives current minute of the day

	mov r8, #1440
	sub r6, r8, r6 ; minutes left in the day

	mov r8, #60
	udiv r2, r6, r8 ; r2 -> hour return value

	push {r0 - r2}
	mov r0, r6 ; minute of day
	mov r1, #60 ; mod 60
	BL Modulus
	mov r3, r0 ; gives minute remaining in day, put this into r3
	pop {r0 - r2}

end:

	lsl r3, r3, #24
	lsl r2, r2, #16
	lsl r1, r1, #8

	orr r0,r0,r1
	orr r0,r0,r2
	orr r0,r0,r3

	POP {LR}
	POP {R4 - R8}

	BX LR ; unconditional branch back to the return address

	.endasmfunc

	.align
	.end
