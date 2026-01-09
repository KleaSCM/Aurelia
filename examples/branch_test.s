; Branch and loop test program
; Counts from 0 to 10 using a loop

	MOV R0, #0       ; Initialize counter to 0

loop:
	ADD R0, R0, #1   ; Increment counter
	CMP R0, #10      ; Compare counter with 10
	BNE loop         ; Branch to loop if not equal

	HALT             ; Stop when counter reaches 10
