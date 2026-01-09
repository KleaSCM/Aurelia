# Aurelia Assembly Language Guide

**(Revision 2.0 - Extended Technical Reference)**

## 1. Introduction

The Aurelia Assembler (`asm`) converts human-readable source code (`.s`) into machine-executable binary files (`.bin`).
It supports standard RISC syntax, labels, comments, and data directives.

## 2. Syntax Grammar (EBNF)

```ebnf
program      ::= { line }
line         ::= [ label_def ] [ instruction | directive ] [ comment ] NEWLINE
label_def    ::= IDENTIFIER ":"
instruction  ::= MNEMONIC operand_list
directive    ::= "." IDENTIFIER [ string_literal | number ]
operand_list ::= operand { "," operand }
operand      ::= REGISTER | IMMEDIATE | MEMORY_REF
comment      ::= ";" ANY_TEXT
```

## 3. Directives

* `.string "Text"`: Null-terminated ASCII.
* `.byte 0x00`: Raw byte insertion.

## 4. Advanced Optimization Guide

### 4.1 Instruction Scheduling (Pipeline Awareness)

The Aurelia CPU has a 3-stage pipeline. Certain instruction sequences cause **Stalls** (Bubbles).
A wise programmer schedules code to fill these bubbles.

**The Load-Use Hazard**:
Loading a value into a register and using it immediately causes a 1-cycle stall.

```asm
; BAD - Stall occurs after LDR
LDR R1, [R0, #0]
ADD R2, R1, #5   ; Pipeline waits for R1

; GOOD - interleaved independent instruction
LDR R1, [R0, #0]
ADD R3, R4, #1   ; Free instruction!
ADD R2, R1, #5   ; R1 is ready now
```

### 4.2 Branch Shadowing

Branches take 2 cycles if taken (Flush).
To minimize penalties:

1. Prefer **Fall-Through** logic for the common case (keep loop bodies contiguous).
2. Use `BNE` (Branch Not Equal) for loop tails.

### 4.3 Manual Calling Convention (ABI)

Since hardware has no `CALL`/`RET` instructions (Use `B` and `LR`), you must manage the Stack (`SP`) manually.

**Function Prologue**:

```asm
SUB SP, SP, #16     ; Allocate Frame
STR LR, [SP, #8]    ; Save Return Address
STR R16, [SP, #0]   ; Save Callee-Saved Reg
```

**Function Epilogue**:

```asm
LDR R16, [SP, #0]   ; Restore Reg
LDR LR, [SP, #8]    ; Restore Return Address
ADD SP, SP, #16     ; Deallocate Frame
MOV PC, LR          ; Return (Jump to LR)
```

## 5. Examples

### 5.1 Optimized Memcpy (Block Copy)

```asm
; R0 = Dest, R1 = Src, R2 = Count
memcpy:
    CMP R2, #0
    BEQ end
    
copy_loop:
    LDR R3, [R1, #0]  ; Load
    STR R3, [R0, #0]  ; Store
    
    ADD R0, R0, #8    ; Inc Dest
    ADD R1, R1, #8    ; Inc Src
    SUB R2, R2, #1    ; Dec Count
    
    CMP R2, #0
    BNE copy_loop
end:
    HALT
```

### 5.2 Arithmetic Kernel (Factorial)

```asm
fact:
    MOV R0, #1       ; Result
    MOV R1, #5       ; N
loop:
    MUL R0, R0, R1   ; Res = Res * N
    SUB R1, R1, #1   ; N--
    CMP R1, #1
    BNE loop
    HALT
```

---
**End of Guide.**
