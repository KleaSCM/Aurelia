; Memory access test program
; Tests load and store instructions

	MOV R0, #100     ; Base address simulation
	MOV R1, #42      ; Value to store
	STR R1, [R0, #4] ; Store R1 to memory[R0 + 4]
	LDR R2, [R0, #4] ; Load from memory[R0 + 4] into R2
	HALT
