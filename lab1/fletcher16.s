	.def fletcher16s

	.ref Modulus255

	.thumb
	.align 2
	.text

fletcher16s:

	.asmfunc

	PUSH {R4 - R8}
	PUSH {LR}

	MOV R4, R1 ; count
	MOV R5, #0 ; sum1
	MOV R6, #0 ; sum2
	MOV R7, R0 ; data
	MOV R8, #0 ; i

loop:
	CBZ R4, end

	LDRB R0, [R7, R8]
	ADD R0, R5, R0
	BL Modulus255
	MOV R5, R0

	ADD R0, R6, R5
	BL Modulus255
	MOV R6, R0

	ADD R8, R8, #1
	SUB R4, R4, #1

	B loop

end:

	LSL R6, R6, #8
	ORR R0, R6, R5

	POP {LR}
	POP {R4 - R8}

	BX LR ; unconditional branch back to the return address

	.endasmfunc

	.align
	.end
