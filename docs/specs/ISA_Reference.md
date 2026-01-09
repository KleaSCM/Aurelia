# Aurelia V2 Architecture - Instruction Set Reference

**(Revision 2.0 - Extended Technical Reference)**

## 1. Overview

The Aurelia V2 ISA is a RISC-based, Load/Store architecture designed for implementation simplicity and educational clarity. It features a fixed 32-bit instruction width and follows a Harvard-style organizational logic.

## 2. Register File Specification

Total Storage: $32 \times 64\text{-bits} = 2048 \text{ bits}$.

| Register | Name | ABI Role | Preserved? |
|:---:|:---:|:---|:---:|
| **R0** | **Accumulator** | Return Values / Math | No |
| **R1-R7** | **Arguments** | Function Arguments 1-7 | No |
| **R8-R15** | **Temporaries** | Scratch registers | No |
| **R16-R29**| **Saved** | Long-term storage | Yes |
| **R30** | **SP** | Stack Pointer | Yes |
| **R31** | **LR** | Link Register | No |

## 3. Instruction Encoding Reference

All instructions are **32-bits** (Little Endian).

### 3.1 Format Types

* **R-Type (Register)**: `[Op:6 | Rd:5 | Rn:5 | Rm:5 | Res:11]`
* **I-Type (Immediate)**: `[Op:6 | Rd:5 | Rn:5 | Imm:11]`
* **B-Type (Branch)**: `[Op:6 | Imm:26]` (Conceptually 26, implemented as 11 currently)

---

## 4. Operation Reference (Micro-Ops)

Legend:

* `SR`: Status Register
* `SExt(x)`: Sign Extend x to 64-bits.
* `PC`: Program Counter

### Arithmetic Group

#### `ADD` - Addition

* **Opcode**: `0x01`
* **Cycles**: 1
* **RTL**:

    ```verilog
    OpA = (Type==Reg) ? R[Rn] : R[Rn];
    OpB = (Type==Reg) ? R[Rm] : SExt(Imm);
    Res = OpA + OpB;
    R[Rd] = Res;
    UpdateFlags(Res);
    ```

* **Flags**:
  * `Z = (Res == 0)`
  * `N = Res[63]`
  * `C = (OpA + OpB) > UINT64_MAX`
  * `V = (OpA[63] == OpB[63]) && (Res[63] != OpA[63])`

#### `SUB` - Subtraction

* **Opcode**: `0x02`
* **Cycles**: 1
* **RTL**:

    ```verilog
    OpA = R[Rn];
    OpB = (Type==Reg) ? R[Rm] : SExt(Imm);
    Res = OpA - OpB;
    R[Rd] = Res;
    UpdateFlags(Res);
    ```

* **Flags**: Z, N, C (Borrow), V.

#### `MUL` - Multiplication (Proposed)

* **Opcode**: Reserved
* **Cycles**: 4 (Simulated Booth's Algorithm)
* **RTL**:

    ```verilog
    Res = (R[Rn] * R[Rm]);
    R[Rd] = Res[63:0];
    ```

---

### Logic Group

#### `AND` - Logical AND

* **Opcode**: `0x03`
* **Cycles**: 1
* **RTL**:

    ```verilog
    R[Rd] = R[Rn] & OpB;
    ```

* **Flags**: Z, N. C=0, V=0.

#### `OR` - Logical OR

* **Opcode**: `0x04`
* **Cycles**: 1
* **RTL**:

    ```verilog
    R[Rd] = R[Rn] | OpB;
    ```

* **Flags**: Z, N.

#### `LSL` - Logical Shift Left

* **Opcode**: `0x06`
* **Cycles**: 1
* **RTL**:

    ```verilog
    ShiftAmt = OpB & 0x3F;
    R[Rd] = R[Rn] << ShiftAmt;
    ```

* **Flags**:
  * `C = R[Rn][64 - ShiftAmt]` (Last bit shifted out)
  * Z, N.

---

### Memory Group

#### `LDR` - Load Word

* **Opcode**: `0x10`
* **Cycles**: 3 (Address Calc + Bus Req + Data Wait)
* **RTL**:

    ```verilog
    BE: Addr = R[Rn] + SExt(Imm);
    MEM: Bus.Read(Addr);
    WB: R[Rd] = Bus.Data;
    ```

* **Hazards**: Introduces 1-cycle stall if next instruction reads `Rd`.

#### `STR` - Store Word

* **Opcode**: `0x11`
* **Cycles**: 2 (Address Calc + Bus Write)
* **RTL**:

    ```verilog
    BE: Addr = R[Rn] + SExt(Imm);
    MEM: Bus.Write(Addr, R[Rd]);
    ```

---

### Control Flow Group

#### `B` - Unconditional Branch

* **Opcode**: `0x30`
* **Cycles**: 2 (Flush Pipeline)
* **RTL**:

    ```verilog
    PC = PC + SExt(Imm);
    Flush(IF_Stage);
    ```

#### `BEQ` - Branch if Equal (Z=1)

* **Opcode**: `0x31`
* **Cycles**: 1 (Not Taken) / 2 (Taken)
* **RTL**:

    ```verilog
    if (SR.Z == 1) {
        PC = PC + SExt(Imm);
        Flush(IF_Stage);
    }
    ```

#### `CMP` - Compare

* **Opcode**: `0x09`
* **Cycles**: 1
* **RTL**:

    ```verilog
    Temp = R[Rn] - OpB;
    UpdateFlags(Temp);
    // No Register Writeback
    ```

---
**End of Specification.**
