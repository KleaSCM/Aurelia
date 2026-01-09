; Simple arithmetic test program
; Tests basic data processing instructions

	MOV R0, #10      ; Load immediate 10 into R0
	MOV R1, #20      ; Load immediate 20 into R1
	ADD R2, R0, R1   ; R2 = R0 + R1 (30)
	SUB R3, R1, R0   ; R3 = R1 - R0 (10)
	HALT             ; Stop execution
