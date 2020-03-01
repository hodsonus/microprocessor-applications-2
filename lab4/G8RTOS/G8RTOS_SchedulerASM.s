; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler, StartCriticalSection, EndCriticalSection

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc

	; Loads the currently running thread’s context into the CPU
	ldr r0, RunningPtr ; point to the beginning of the running thread's struct
	ldr r1, [r0] ; follow the pointer to the object and load it
	ldr sp, [r1] ; restore the sp with the value that was stored in the tcb

	; Pops registers from thread stack
	pop {r4-r11, r0-r3, r12}

	; Skip loading LR (R14) and pop PC (R15) into LR
	add sp, sp, #4
	pop {lr}

	; Enable interrupts
	cpsie i

	; Branch to the new function
	bx lr

	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:

	.asmfunc

	push {r1-r3, r12, lr}
	bl StartCriticalSection ; r0 contains IBit_State
	pop {r1-r3, r12, lr}

	; Saves remaining registers into thread stack
	push {r4-r11}

	; Saves current stack pointer to tcb
	ldr r1, RunningPtr ; point to the beginning of the running thread's struct
	ldr r1, [r1] ; follow the pointer to the object and load it
	str sp, [r1] ; update the value of the memory that sp is pointing to as the current sp

	; Calls G8RTOS_Scheduler to get new tcb
	push {r0-r3, r12, lr}
	bl G8RTOS_Scheduler
	pop {r0-r3, r12, lr}

	; Set stack pointer to new stack pointer from new tcb
	ldr r1, RunningPtr ; point to the beginning of the **new** running thread's struct
	ldr r1, [r1] ; follow the pointer to the object and load it
	ldr sp, [r1] ; restore the sp with the value that was stored in the tcb

	; Pops registers from thread stack
	pop {r4-r11}
	; Popping r0-r3, r12-r15, psr is automatic when returning from this handler

	push {r1-r3, r12, lr}
	bl EndCriticalSection ; r0 contains IBit_State
	pop {r1-r3, r12, lr}

	bx lr

	.endasmfunc
	
	; end of the asm file
	.align
	.end
