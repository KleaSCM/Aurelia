**Aurelia** is a cycle-accurate ecosystem emulator modeling a custom 64-bit RISC architecture. It simulates the entire stack: from the physics of NAND flash tunneling to the micro-operations of the CPU pipeline.

# Aurelia V2 Architecture - Hardware Specification

## 1. System Bus Protocol (The Nervous System)

**(Revision 4.1 - Critical Interconnect Standard)**

The Aurelia System Bus (ASB) is a high-bandwidth, low-latency synchronous interconnect designed to arbitrate data flow between the Main Processing Unit (MPU), Direct Memory Access (DMA) engines, and peripheral blocks. It implements a split-transaction protocol with independent Read and Write channels to maximize throughput.

### 1.1 Signal Interface Definitions

The Physical Layer (PHY) consists of 142 discrete signal lines grouped into Control, Address, and Data planes.

#### 1.1.1 Global Signals

| Signal | Width | Driver | Description |
| :--- | :---: | :---: | :--- |
| **ACLK** | 1 | Clock | Global reference clock. All signals sampled on rising edge. |
| **ARESETn** | 1 | Reset | Global async reset (Active Low). Schmitt Trigger input. |

#### 1.1.2 Master Channel Signals

| Signal | Width | Direction | Role | Logic Definition |
| :--- | :---: | :---: | :--- | :--- |
| **ADDR** | 64 | Master $\to$ Slave | Target Address | Physical Byte Address. Must be aligned to `SIZE`. Unaligned access results in `RESP_ERROR`. |
| **TRANS** | 2 | Master $\to$ Slave | Transaction Type | `00`=Idle, `01`=Busy (Insert Wait), `10`=Non-Sequential (Start), `11`=Sequential (Burst). |
| **WRITE** | 1 | Master $\to$ Slave | Transfer Direction | `0`=Read (Slave Drives Data), `1`=Write (Master Drives Data). |
| **SIZE** | 3 | Master $\to$ Slave | Transfer Size | `000`=8b, `001`=16b, `010`=32b, `011`=64b, `100`=128b (Cache Line). |
| **BURST** | 3 | Master $\to$ Slave | Burst Type | `000`=Fixed, `001`=Incr, `010`=Wrap. Defines address calculation for subsequent beats. |
| **PROT** | 4 | Master $\to$ Slave | Protection Attributes | `[0]`=Privileged/User, `[1]`=Bufferable, `[2]`=Cacheable, `[3]`=Instruction/Data. |
| **WDATA** | 64 | Master $\to$ Slave | Write Data | Driver active only during Data Phase of Write cycles. Tristated otherwise. |
| **WSTRB** | 8 | Master $\to$ Slave | Write Strobes | Byte lane enables. `Bit[n]` corresponds to `Data[8n+7:8n]`. Used for sub-word writes. |

#### 1.1.3 Slave Channel Signals

| Signal | Width | Direction | Role | Logic Definition |
| :--- | :---: | :---: | :--- | :--- |
| **READY** | 1 | Slave $\to$ Master | Transfer Complete | High = Slave accepts command / drives valid data. Low = Insert Wait State. |
| **RESP** | 2 | Slave $\to$ Master | Response Status | `00`=OKAY, `01`=ERROR (Bus Fault), `10`=RETRY, `11`=SPLIT (Not Implemented). |
| **RDATA** | 64 | Slave $\to$ Master | Read Data | Valid only when `READY=1` during Read cycles. Slave must drive 0s on unused lanes. |

---

### 1.2 Electrical Specifications

The ASB is designed for integration in 7nm finFET processes.

#### 1.2.1 DC Characteristics ($V_{DD} = 1.0V \pm 5\%$)

| Parameter | Symbol | Min | Max | Unit | Condition |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Input High Voltage** | $V_{IH}$ | $0.65 \times V_{DD}$ | $V_{DD} + 0.3$ | V | LVCMOS Compliant |
| **Input Low Voltage** | $V_{IL}$ | $-0.3$ | $0.35 \times V_{DD}$ | V | |
| **Output High Voltage** | $V_{OH}$ | $V_{DD} - 0.2$ | - | V | $I_{OH} = -8mA$ |
| **Output Low Voltage** | $V_{OL}$ | - | $0.2$ | V | $I_{OL} = 8mA$ |

#### 1.2.2 AC Timing Characteristics ($F_{clk} = 1.0 \text{GHz}$)

| Parameter | Symbol | Min | Max | Unit | Description |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Clock Period** | $t_{cyc}$ | 1.0 | - | ns | Target Frequency |
| **Input Setup Time** | $t_{su}$ | 200 | - | ps | Data valid before Clock Rise |
| **Input Hold Time** | $t_{hold}$ | 100 | - | ps | Data stable after Clock Rise |
| **Output Valid Delay** | $t_{ov}$ | - | 450 | ps | Clock Rise to Output Valid |

---

### 1.3 Protocol State Machines

The bus logic is governed by two interacting Finite State Machines (FSM).

#### 1.3.1 Master FSM

1. **IDLE**: Reset state. Drives `TRANS=Idle`.
    * Transition to **ADDR** on internal Request.
2. **ADDR**: Drives `ADDR`, `SIZE`, `WRITE`, `TRANS=NonSeq`.
    * If `READY=1`, transition to **DATA** (or **IDLE** if Write Buffered).
    * If `READY=0`, Stay in **ADDR** (Wait State).
3. **DATA**:
    * **Read**: Sample `RDATA` when `READY=1`.
    * **Write**: Drive `WDATA`. Use `WSTRB`.
    * If Burst > 1, Loop in **DATA** for $N$ beats (SEQ).

#### 1.3.2 Slave FSM

1. **WAIT**: Default state. Monitors `ADDR` and `TRANS`.
    * If `TRANS != Idle` AND `Address Decode Match`: Transition to **ACCESS**.
2. **ACCESS**:
    * Perform internal read/write (e.g., DRAM Row activation).
    * Assert `READY=0` while internal busy.
    * Assert `READY=1` when data available.
3. **RESPONSE**:
    * Drive `RESP` status. Return to **WAIT**.

---

### 1.4 Electrical Characteristics (Lumped Model)

For simulation fidelity, the bus assumes a trace impedance $Z_0 = 50\Omega$ and propagation delay $t_{pd} = 150 \text{ps/inch}$.

* **Capacitance ($C_{load}$)**: Sum of all connected input gates + trace capability.
    $$ C_{total} = \sum C_{in} + (L_{trace} \times C_{per\_unit}) $$
* **Dynamic Power**:
    $$ P_{dyn} = \alpha \cdot f_{clk} \cdot C_{total} \cdot V_{dd}^2 $$
    Where $\alpha$ is the Activity Factor (Average toggle rate).
  * `ADDR` bus $\alpha \approx 0.15$ (Due to locality).
  * `DATA` bus $\alpha \approx 0.50$ (High entropy).

---

## 2. Microarchitecture (Classic RISC Pipeline)

## 2. Microarchitecture (Classic RISC Pipeline)

**(Revision 4.0 - Pipeline Control Logic)**

The Aurelia CPU implements a scalar, 3-stage pipeline optimized for high-frequency emulation. While identifying as a "Classic RISC", it employs simplified Scoreboarding logic to handle multi-cycle operations (like `MUL`).

### 2.1 Pipeline Stages & Datapath

The logical stages are: **Fetch (IF)**, **Decode (ID)**, and **Execute/Writeback (EX)**.

#### 2.1.1 Stage 1: Instruction Fetch (IF)

**Primary Function**: Fetch 32-bit instruction word from Instruction Memory ($IMEM$) at address $PC$.

**RTL Logic**:
$$ PC_{next} = (Branch_{taken}) \ ? \ PC_{branch} \ : \ PC + 4 $$
$$ IR \leftarrow Mem[PC] $$

**Structural Hazards**:
Since Aurelia uses a Unified Memory Architecture (Von Neumann), the IF stage must arbitrate with the LSU (Load/Store Unit) in the EX stage.

* **Priority Logic**: `LSU_Request > Fetch_Request`.
* **Stall Conditions**: $\text{Stall}_{IF} = \text{Bus}_{Busy} \lor \text{LSU}_{Active}$.

#### 2.1.2 Stage 2: Instruction Decode (ID) & Operand Fetch

**Primary Function**: Decodes the 32-bit instruction, reads register operands, and generates immediate values.

**Register File Specification**:

* **Ports**: 2 Read Ports ($R_a, R_b$), 1 Write Port ($W_d$).
* **Access Time**: Single-cycle asynchronous read, synchronous write on falling edge (to allow forwarding from EX).

**Control Unit Logic**:
The Control Unit is a hardwired combinatorial block mapping OpCodes to control signals.

| Opcode | `RegDst` | `ALUSrc` | `MemtoReg` | `RegWrite` | `MemRead` | `MemWrite` | `Branch` | `ALUOp` |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **R-Type** | 1 | 0 | 0 | 1 | 0 | 0 | 0 | `10` |
| **LDR** | 0 | 1 | 1 | 1 | 1 | 0 | 0 | `00` |
| **STR** | X | 1 | X | 0 | 0 | 1 | 0 | `00` |
| **BEQ** | X | 0 | X | 0 | 0 | 0 | 1 | `01` |
| **ADDI** | 0 | 1 | 0 | 1 | 0 | 0 | 0 | `00` |

#### 2.1.3 Stage 3: Execute (EX) & Writeback (WB)

**Primary Function**: Performs ALU operations, Address Calculation, and updates the Register File.

**ALU Logic**:
The ALU receives operands from the Forwarding Unit (muxed between Register data, Immediate data, or forwarded Results).
$$ ZeroFlag = (Result == 0) $$

**Writeback Logic**:
$$ W_{data} = (MemtoReg) \ ? \ Mem_{data} \ : \ ALU_{result} $$
$$ R[Rd] \leftarrow W_{data} \ (\text{if } RegWrite) $$

---

### 2.2 Hazard Detection & Resolution

To maintain 1.0 CPI, the pipeline MUST handle data and control hazards without software NOPs where possible.

#### 2.2.1 Data Hazards (Load-Use)

A Load-Use hazard occurs when an instruction attempts to read a register that the immediately preceding instruction is loading from memory. This requires a **Stall**.

**Detection Logic**:

```verilog
if (ID/EX.MemRead and
   ((ID/EX.Rt == IF/ID.Rs) or (ID/EX.Rt == IF/ID.Rt))) {
       stall_pipeline();
}
```

**Action**:

1. Verify control signals to 0 (Bubbling).
2. Disable PC update.
3. Disable IF/ID latch update.

#### 2.2.2 Control Hazards (Branch Penalty)

Branches are resolved in the **EX Stage**. This results in a 2-cycle penalty if the branch is taken.

**Optimization**:

* **Not Taken**: No penalty (Predict Not Taken).
* **Taken**: Flush IF/ID and ID/EX registers.

**Misprediction Logic**:
$$ Flush_{pipe} = (Branch_{signal} \land ZeroFlag) $$

---

### 2.3 Forwarding Unit (Bypassing)

The Forwarding Unit allows the ALU to use results from the EX/MEM or MEM/WB stages before they are written back to the Register File, eliminating stalls for R-Type dependencies.

**Conditions**:

1. **EX Hazard**:

    ```verilog
    if (EX/MEM.RegWrite
        and (EX/MEM.Rd != 0)
        and (EX/MEM.Rd == ID/EX.Rs)) {
            ForwardA = 10; // Forward from EX
    }
    ```

2. **MEM Hazard**:

    ```verilog
    if (MEM/WB.RegWrite
        and (MEM/WB.Rd != 0)
        and not (EX/MEM.Rd == ID/EX.Rs) // EX Hazard has priority
        and (MEM/WB.Rd == ID/EX.Rs)) {
            ForwardA = 01; // Forward from MEM
    }
    ```

**Mux Control**:

* `ForwardA = 00`: Use Register File Output.
* `ForwardA = 10`: Use ALU Result (from previous cycle).
* `ForwardA = 01`: Use Memory/Writeback Result (from 2 cycles ago).

---

The Aurelia CPU implements a scalar, 3-stage pipeline optimized for high-frequency emulation. While identifying as a "Classic RISC", it employs simplified Scoreboarding logic to handle multi-cycle operations (like `MUL`).

### 2.1 Pipeline Stages & Datapath

The logical stages are: **Fetch (IF)**, **Decode (ID)**, and **Execute/Writeback (EX)**.

#### 2.1.1 Stage 1: Instruction Fetch (IF)

**Primary Function**: Fetch 32-bit instruction word from Instruction Memory ($IMEM$) at address $PC$.

**RTL Logic**:
$$ PC_{next} = (Branch_{taken}) \ ? \ PC_{branch} \ : \ PC + 4 $$
$$ IR \leftarrow Mem[PC] $$

**Structural Hazards**:
Since Aurelia uses a Unified Memory Architecture (Von Neumann), the IF stage must arbitrate with the LSU (Load/Store Unit) in the EX stage.

* **Priority Logic**: `LSU_Request > Fetch_Request`.
* **Stall Conditions**: $\text{Stall}_{IF} = \text{Bus}_{Busy} \lor \text{LSU}_{Active}$.

#### 2.1.2 Stage 2: Decode & Operand Fetch (ID)

**Primary Function**: Decode opcode, read Register File ($RF$), and Generate Control Signals.

**Register File Specification**:

* Ports: 2 Read ($R_n, R_m$), 1 Write ($R_d$).
* Bypass: Internal forwarding guarantees Write-after-Read (WAR) consistency within the same cycle.

**Control Unit Truth Table**:

| Opcode | ALU Op | RegWrite | MemRead | MemWrite | Branch |
| :--- | :---: | :---: | :---: | :---: | :---: |
| `ADD` | `001` | 1 | 0 | 0 | 0 |
| `LDR` | `010` | 1 | 1 | 0 | 0 |
| `STR` | `010` | 0 | 0 | 1 | 0 |
| `BEQ` | `110` | 0 | 0 | 0 | 1 |

#### 2.1.3 Stage 3: Execute / Writeback (EX)

**Primary Function**: ALU Execution, Memory Access (LSU), and Result Commit.

**ALU Logic**:
The Arithmetic Logic Unit performs operations based on `ALU_Op` and `ConditionCode`.
$$ Result = f_{ALU}(OpA, OpB) $$

* **Zero Flag ($Z$)**: $Result == 0$.
* **Negative Flag ($N$)**: $Result[63] == 1$.
* **Carry Flag ($C$)**: Unsigned Overflow ($A+B < A$).
* **Overflow Flag ($V$)**: Signed Overflow ($(A>0 \land B>0 \land R<0) \lor (A<0 \land B<0 \land R>0)$).

**Writeback Logic**:
$$ R[R_d] \leftarrow (MemToReg) \ ? \ Mem_{Data} \ : \ ALU_{Result} $$

---

### 2.2 Hazard Detection & Resolution

To maintain Cycle-Accurate fidelity, hazards are resolved via Hardware Interlocks (Stalling) rather than software NOPs.

#### 2.2.1 Data Hazards (RAW)

Since the pipeline is short (3-stage), data dependencies are typically resolved via RF Bypassing. However, a **Load-Use** hazard requires a stall.

**Detection Logic**:
$$ \text{LoadUse} = (ID.Op == Load) \land ((ID.Rd == IF.Rn) \lor (ID.Rd == IF.Rm)) $$
**Action**:

1. Assert `Stall_IF`, `Stall_ID`.
2. Insert `BUBBLE` into EX stage.

#### 2.2.2 Control Hazards (Branch Penalty)

Branches are resolved in the **ID** stage (Optimization).

* **Misprediction Penalty**: 1 Cycle.
* **Action**: If `BranchTaken`:
    1. Flush IF (Discard fetched instruction).
    2. Set `PC = Target`.

---

### 2.3 Exception & Interrupt Handling

Precise exceptions are supported via the **Exception Program Counter (EPC)**.

**Vector Table**:

| ID | Exception Type | Priority | Vector Offset |
| :---: | :--- | :---: | :---: |
| 0 | Reset | 0 | `0x0000_0000` |
| 1 | Undefined Instruction | 1 | `0x0000_0004` |
| 2 | Software Interrupt (SWI) | 2 | `0x0000_0008` |
| 3 | Prefetch Abort | 3 | `0x0000_000C` |
| 4 | Data Abort | 4 | `0x0000_0010` |

**Exception FSM**:

1. **Fault**: Logic detects illegal state (e.g., `Opcode > MAX`).
2. **Trap**: Pipeline Flush. `EPC <= PC`. `SR.Mode <= Supervisor`.
3. **Vector**: `PC <= VectorTable[ID]`.

---

### 2.4 Performance Counters (Telemetry)

The microarchitecture exposes Model-Specific Registers (MSRs) for profiling.

* `MSR_CYCLE_COUNT`: Total Clock Cycles.
* `MSR_INST_RETIRED`: Instructions successfully committed.
* `MSR_L1_MISS`: I-Cache / D-Cache misses (Simulated).
* `MSR_BRANCH_MISS`: Branch mispredictions.

$$ IPC = \frac{InstructionCount}{CycleCount} $$

---

## 3. Interrupt Controller (APIC) Specification

**(Revision 3.5 - Advanced Programmable Interrupt Controller)**

The Aurelia PIC is a low-latency, vectored interrupt controller supporting up to 64 distinct interrupt sources with programmable priority levels.

### 3.1 Interrupt Vector Logical Model

The controller implements a **Vectored Priority** model. When multiple interrupts assert simultaneously, the highest priority request is serviced first.

**Vector Table Layout**:
The Vector Table Base Address ($VTBA$) is stored in System Control Register `SCB_VTOR`.

$$ VectorAddr = VTBA + (IRQ\_ID \times 4) $$

### 3.2 Signal Path Latency

The total Interrupt Latency ($T_{lat}$) is the sum of:

1. **Synchronization Delay**: 2 cycles (Double-flop synchronizer for async inputs).
2. **Arbitration Delay**: 1 cycle (Priority Encoder).
3. **Vector Fetch**: 1 cycle (Table lookup).
4. **Context Save**: 4 cycles (Push PC, SR, R0-R3).
$$ T_{lat} = 8 \text{ cycles (Best Case)} $$

### 3.3 Register Map (Base: 0xE0002000)

| Offset | Name | Type | Width | Description |
| :--- | :--- | :---: | :---: | :--- |
| `0x000` | `ISR` | RO | 64 | **Interrupt Status Register**. Raw line status. |
| `0x008` | `IER` | RW | 64 | **Interrupt Enable Register**. 1=Enabled. |
| `0x010` | `IAR` | WO | 64 | **Interrupt Acknowledge**. Write 1 to clear Edge-triggered IRQs. |
| `0x018` | `IPR[0..7]` | RW | 64 | **Interrupt Priority Registers**. 8 bits per IRQ ID. |
| `0x100` | `SWT` | WO | 32 | **Software Trigger**. Write ID to manually assert IRQ. |

### 3.4 Arbitration Logic (Hardware)

The Priority Resolver selects the active interrupt $I_{win}$ such that:
$$ I_{win} = \text{argmax}_{i} (Priority[i] \mid (ISR[i] \land IER[i])) $$
If $Priority[I_{win}] < CPU_{Threshold}$, the interrupt is masked.

**Nesting Logic**:
The APIC supports **Preemption**. If a higher-priority IRQ arrives while servicing a lower-priority one:

1. Current ISR is suspended.
2. Review context is pushed to Stack.
3. Higher priority ISR executes.
4. Standard "Tail Chaining" optimization is used to switch contexts without unstacking/restacking if pending IRQs remain.

---

## 4. Global System Timer (GST) Specification

**(Revision 2.2 - High Precision Event Timer)**

The GST provides a monotonically increasing 64-bit counter for OS scheduling and high-resolution profiling.

### 4.1 Timer Model

The GST operates on a fixed frequency derived from the system bus clock ($F_{bus}$).
$$ F_{timer} = F_{bus} / (Prescaler + 1) $$

* **Counter Width**: 64-bit.
* **Comparator**: 64-bit Match Register.
* **Interrupt**: Level-triggered when `Counter >= Match`.

### 4.2 Watchdog Functionality

The Watchdog Timer (WDT) is a separate down-counter designed to reset the system in case of software lockups.

* **Kick Mechanism**: Software must write `0x55AA` followed by `0xAA55` to the `WDT_KICK` register to reset the counter.
* **Timeout Action**: If counter reaches 0, the warm release signal `nRESET` is asserted.

### 4.3 Register Map (Base: 0xE0003000)

| Offset | Name | Type | Description |
| :--- | :--- | :---: | :--- |
| `0x00` | `GST_LO` | RO | **Counter Low**. Bits [31:0]. |
| `0x04` | `GST_HI` | RO | **Counter High**. Bits [63:32]. |
| `0x08` | `CMP_LO` | RW | **Comparator Low**. Bits [31:0]. |
| `0x0C` | `CMP_HI` | RW | **Comparator High**. Bits [63:32]. |
| `0x10` | `CTRL` | RW | **Control**. [0]=Enable, [1]=IRQ_Mask. |
| `0x20` | `WDT_VAL` | RW | **Watchdog Reload Value**. |
| `0x24` | `WDT_KICK` | WO | **Watchdog Kick Sequence**. |

**Atomic Read Logic**:
To read the 64-bit counter on a 32-bit bus without tearing:

1. Read `GST_LO`: Hardware latches current `GST_HI` into a shadow register.
2. Read `GST_HI`: Returns the latched shadow value.

---
**End of Specification.**

# Volume 5: Instruction Set Architecture (ISA) Reference

**(Revision 4.0 - Architecture Definition Document)**

The Aurelia V2 ISA is a 32-bit width, fixed-length instruction set following strict Load/Store semantics. It is designed to expose the microarchitectural constraints of the pipeline to the compiler/programmer.

## 6.1 Programmers Model

### 6.1.1 Register File

The execution state consists of 32 general-purpose registers (GPRs), each 64-bits wide.

| Register | Alias | ABI Usage Rules | Preservation |
| :--- | :---: | :--- | :---: |
| **R0** | `A0` | Accumulator / Return Value | Caller-Save |
| **R1-R7** | `A1-A7` | Function Arguments | Caller-Save |
| **R8-R15** | `T0-T7` | Temporaries | Caller-Save |
| **R16-R29** | `S0-S13` | Saved Registers | Callee-Save |
| **R30** | `SP` | Stack Pointer | Callee-Save |
| **R31** | `LR` | Link Register | Caller-Save |

### 6.1.2 Status Register (CPSR)

The Current Program Status Register holds the ALU flags and execution mode.

* **Bit [31] N**: Negative Result.
* **Bit [30] Z**: Zero Result.
* **Bit [29] C**: Carry / Unsigned Overflow.
* **Bit [28] V**: Signed Overflow.
* **Bit [7] I**: IRQ Mask (1=Disabled).

## 6.2 Instruction Formats

The ISA uses 3 primary encoding formats.

### 6.2.1 R-Type (Register Operations)

Used for ALU operations dependent on register operands.
`[31:26 Opcode] [25:21 Rd] [20:16 Rn] [15:11 Rm] [10:0 Reserved]`

### 6.2.2 I-Type (Immediate Operations)

Used for ALU operations with constants and Memory Access commands.
`[31:26 Opcode] [25:21 Rd] [20:16 Rn] [10:0 Immediate]`

* **Immediate Field**: 11-bit signed integer ($\pm 1024$).

### 6.2.3 B-Type (Branch Operations)

Used for Control Flow.
`[31:26 Opcode] [25:0 Offset]`

* **Offset Field**: 26-bit signed offset, shifted left by 2 (Word Aligned). Range: $\pm 32$MB.

## 6.3 Operation Reference

### 6.3.1 Arithmetic Logic Unit (ALU) Operations

The ALU performs fundamental integer arithmetic. Unless otherwise noted, all operations are 64-bit.

#### **ADD** : Add without Carry

##### 1. Description

The `ADD` instruction adds the value of a register to either a second register or an immediate value, and stores the result in the destination register.

##### 2. Encoding

* **I-Type (Immediate)**: `[Op: 0x01] [Rd: 5] [Rn: 5] [Imm: 11]`
* **R-Type (Register)**: `[Op: 0x01] [Rd: 5] [Rn: 5] [Rm: 5] [Res: 11]`

##### 3. Operation (RTL)

```verilog
// Stage: EXECUTE
Operand1 = R[Rn];
Operand2 = (Type == Immediate) ? SignExtend(Imm11) : R[Rm];

(Result, CarryOut) = Operand1 + Operand2;

R[Rd] = Result;

if (UpdateFlags) {
    N = Result[63];
    Z = (Result == 0);
    C = CarryOut;
    V = OverflowFrom(Operand1 + Operand2);
}
```

##### 4. Exceptions

* None.

##### 5. Latency & Throughput

* **Latency**: 1 Cycle
* **Throughput**: 1 instruction per cycle

##### 6. Usage Examples

```asm
ADD R0, R1, #4       ; R0 = R1 + 4
ADD R0, R1, R2       ; R0 = R1 + R2
ADD R0, R0, #-1      ; Decrement R0
```

---

#### **SUB** : Subtract without Carry

##### 1. Description

The `SUB` instruction subtracts a register or immediate value from a register.

##### 2. Encoding

* **I-Type (Immediate)**: `[Op: 0x02] [Rd: 5] [Rn: 5] [Imm: 11]`
* **R-Type (Register)**: `[Op: 0x02] [Rd: 5] [Rn: 5] [Rm: 5] [Res: 11]`

##### 3. Operation (RTL)

```verilog
// Stage: EXECUTE
Operand1 = R[Rn];
Operand2 = (Type == Immediate) ? SignExtend(Imm11) : R[Rm];

(Result, Borrow) = Operand1 - Operand2;

R[Rd] = Result;

if (UpdateFlags) {
    N = Result[63];
    Z = (Result == 0);
    C = !Borrow; // Carry flag acts as Not-Borrow in ARM/Aurelia
    V = OverflowFrom(Operand1 - Operand2);
}
```

##### 4. Exceptions

* None.

##### 5. Usage Examples

```asm
SUB R0, R1, #10      ; R0 = R1 - 10
SUB RSP, RSP, #16    ; Allocate Stack Frame
```

---

#### **MUL** : Multiply (64-bit x 64-bit)

##### 1. Description

The `MUL` instruction multiplies two 64-bit register values. It produces the lower 64-bits of the result.
*Note: High 64-bits are currently discarded (No `MULH` instruction yet).*

##### 2. Encoding

* **R-Type (Register)**: `[Op: 0x1A] [Rd: 5] [Rn: 5] [Rm: 5] [Res: 11]`

##### 3. Operation (RTL)

```verilog
// Stage: EXECUTE (Multi-cycle)
Operand1 = R[Rn];
Operand2 = R[Rm];

FullResult[127:0] = Operand1 * Operand2;

R[Rd] = FullResult[63:0];

if (UpdateFlags) {
    N = Result[63];
    Z = (Result == 0);
    C = Unpredictable;
    V = Unpredictable;
}
```

##### 4. Pipeline Hazards

* **Stall**: This instruction asserts `BUSY` for 3 additional cycles (Total 4). The Fetch Unit is stalled during execution.

##### 5. Usage Examples

```asm
MUL R0, R1, R2       ; R0 = R1 * R2
```

---

#### **UDIV** : Unsigned Divide

##### 1. Description

The `UDIV` instruction performs unsigned integer division.

##### 2. Encoding

* **R-Type (Register)**: `[Op: 0x1B] [Rd: 5] [Rn: 5] [Rm: 5] [Res: 11]`

##### 3. Operation (RTL)

```verilog
// Stage: EXECUTE (Multi-cycle)
Dividend = R[Rn];
Divisor  = R[Rm];

if (Divisor == 0) {
    // Division by Zero Handling
    Result = 0;
    // Optional: Trigger DivisionByZero Exception
} else {
    Result = UInt(Dividend) / UInt(Divisor);
}

R[Rd] = Result;
```

##### 4. Latency

* **Latency**: 12-64 Cycles (Data Dependent mechanism).
  * *Micro-Architecture Note*: Implemented as iterative restoring division.

---

#### **CMP** : Compare

##### 1. Description

The `CMP` instruction subtracts the value of the second operand from the first operand and updates the status flags. The result is discarded.

##### 2. Encoding

* **I-Type**: `[Op: 0x09] [Rd: 0] [Rn: 5] [Imm: 11]`
* **R-Type**: `[Op: 0x09] [Rd: 0] [Rn: 5] [Rm: 5] [Res: 11]`

##### 3. Operation (RTL)

```verilog
// Stage: EXECUTE
Operand1 = R[Rn];
Operand2 = (Type==Imm) ? SExt(Imm) : R[Rm];

AluResult = Operand1 - Operand2;

// CRITICAL: Writeback to Rd is DISABLED.
UpdateFlags(AluResult);
```

##### 4. Usage Examples

```asm
CMP R0, #0       ; Check if R0 is zero
BEQ Label        ; Branch if Z flag set
```

---

### 6.3.2 Logical Operations Unit (LOU)

The LOU performs bitwise boolean operations.

#### **AND** : Bitwise AND

##### 1. Description

The `AND` instruction performs a bitwise AND between the operands.

##### 2. Encoding

* **I-Type**: `[Op: 0x03] [Rd: 5] [Rn: 5] [Imm: 11]`
* **R-Type**: `[Op: 0x03] [Rd: 5] [Rn: 5] [Rm: 5]`

##### 3. Operation (RTL)

```verilog
Operand1 = R[Rn];
Operand2 = (Type==Imm) ? SExt(Imm) : R[Rm];
Result = Operand1 & Operand2;
R[Rd] = Result;
UpdateFlags_Logic(Result); // Updates N, Z. Clears C, V.
```

##### 4. Usage Examples

```asm
AND R0, R1, #0xFF    ; Mask lower byte
```

---

#### **ORR** : Bitwise OR

##### 1. Description

The `ORR` instruction performs a bitwise Inclusive OR.

##### 2. Encoding

* **I-Type**: `[Op: 0x04] [Rd: 5] [Rn: 5] [Imm: 11]`
* **R-Type**: `[Op: 0x04] [Rd: 5] [Rn: 5] [Rm: 5]`

##### 3. Operation (RTL)

```verilog
Result = Operand1 | Operand2;
R[Rd] = Result;
```

---

#### **EOR** : Bitwise Exclusive OR (XOR)

##### 1. Description

The `EOR` instruction performs a bitwise Exclusive OR.

##### 2. Encoding

* **I-Type**: `[Op: 0x05] [Rd: 5] [Rn: 5] [Imm: 11]`
* **R-Type**: `[Op: 0x05] [Rd: 5] [Rn: 5] [Rm: 5]`

##### 3. Examples

```asm
EOR R0, R0, R0       ; Clear R0 (Set Z flag)
```

---

#### **BIC** : Bit Clear (AND NOT)

##### 1. Description

The `BIC` instruction performs an AND with the complement of the second operand.
$Rd = Rn \ \& \ (\sim Op2)$.

##### 2. Encoding

* **R-Type**: `[Op: 0x1C] [Rd: 5] [Rn: 5] [Rm: 5]`

---

### 6.3.3 Barrel Shifter Unit (BSU)

The Shifter typically executes in parallel with the ALU (in Stage 3).

#### **LSL** : Logical Shift Left

##### 1. Description

Shifts bits left, filling with zeros. The carry flag is updated to the last bit shifted out.

##### 2. Encoding

* **I-Type**: `[Op: 0x06] [Rd: 5] [Rn: 5] [Imm: 11]`
* **R-Type**: `[Op: 0x06] [Rd: 5] [Rn: 5] [Rm: 5]`

##### 3. Operation (RTL)

```verilog
ShiftAmt = (Type==Imm) ? Imm[5:0] : R[Rm][5:0];
Result = R[Rn] << ShiftAmt;
R[Rd] = Result;

if (UpdateFlags) {
    C = R[Rn][64 - ShiftAmt]; // Last bit shifted out
    N = Result[63];
    Z = (Result == 0);
}
```

---

#### **LSR** : Logical Shift Right

##### 1. Description

Shifts bits right, filling with zeros (Unsigned shift).

##### 2. Encoding

* **I-Type**: `[Op: 0x07] [Rd: 5] [Rn: 5] [Imm: 11]`

##### 3. Operation (RTL)

```verilog
Result = R[Rn] >> ShiftAmt;
R[Rd] = Result;
```

---

#### **ASR** : Arithmetic Shift Right

##### 1. Description

Shifts bits right, replicating the sign bit (Signed shift).

##### 2. Encoding

* **I-Type**: `[Op: 0x08] [Rd: 5] [Rn: 5] [Imm: 11]`

##### 3. Operation (RTL)

```verilog
Result = Int64(R[Rn]) >> ShiftAmt;
R[Rd] = Result;
```

---

### 6.3.4 Memory Access Unit (LSU)

The LSU handles all data movement between the Register File and Main Memory. It supports Base+Offset addressing.

#### **LDR** : Load Register (64-bit)

##### 1. Description

Calculates an effective address by adding an immediate offset to a base register, reads a 64-bit word from memory, and writes it to the destination register.

##### 2. Encoding

* **I-Type**: `[Op: 0x10] [Rd: 5] [Rn: 5] [Imm: 11]`

##### 3. Operation (RTL)

```verilog
// Stage: EXECUTE (Address Calc)
Address = R[Rn] + SignExtend(Imm);

// Stage: MEMORY
Data = Memory.Read64(Address);

// Stage: WRITEBACK
R[Rd] = Data;
```

##### 4. Exceptions

* **Data Abort**: Raised if `Address` is not 8-byte aligned.

##### 5. Latency

* **Latency**: 3 Cycles (Load-Use Penalty)

---

#### **STR** : Store Register (64-bit)

##### 1. Description

Calculates an effective address and stores a 64-bit word from a register to memory.

##### 2. Encoding

* **I-Type**: `[Op: 0x11] [Rd: 5] [Rn: 5] [Imm: 11]`

##### 3. Operation (RTL)

```verilog
Address = R[Rn] + SignExtend(Imm);
Memory.Write64(Address, R[Rd]);
```

##### 4. Latency

* **Latency**: 1 Cycle (Posted Write).

---

#### **LDRB** : Load Register Byte

##### 1. Description

Loads a single byte from memory and zero-extends it to 64-bits.

##### 2. Encoding

* **I-Type**: `[Op: 0x12] [Rd: 5] [Rn: 5] [Imm: 11]`

##### 3. Operation

```verilog
Address = R[Rn] + SignExtend(Imm);
Data = Memory.Read8(Address);
R[Rd] = ZeroExtend(Data);
```

---

#### **STRB** : Store Register Byte

##### 1. Description

Stores the least significant byte (LSB) of a register to memory.

##### 2. Encoding

* **I-Type**: `[Op: 0x13] [Rd: 5] [Rn: 5] [Imm: 11]`

##### 3. Operation

```verilog
Address = R[Rn] + SignExtend(Imm);
Memory.Write8(Address, R[Rd][7:0]);
```

---

### 6.3.5 Control Flow Unit (BRU)

The BRU manages Program Counter (PC) updates. All branches have a **Delay Slot** or **Pipeline Flush** penalty.

#### **B** : Unconditional Branch

##### 1. Description

Performs a PC-relative jump.

##### 2. Encoding

* **B-Type**: `[Op: 0x30] [Offset: 26]`

##### 3. Operation

```verilog
Target = PC + (SignExtend(Offset) << 2);
PC = Target;
FlushPipeline(IF);
```

##### 4. Range

$\pm 128$ MB from current PC.

---

#### **BL** : Branch with Link (Subroutine Call)

##### 1. Description

Writes `PC + 4` to the Link Register (LR) and then jumps to the target.

##### 2. Encoding

* **B-Type**: `[Op: 0x31] [Offset: 26]`

##### 3. Operation

```verilog
R[31] = PC + 4; // Save Return Address
PC = Target;
```

---

#### **BX** : Branch Exchange (Indirect Jump)

##### 1. Description

Jumps to an absolute address contained in a register. Used for returning from functions (`BX LR`) or jump tables.

##### 2. Encoding

* **R-Type**: `[Op: 0x32] [Rd: 0] [Rn: 5] [Rm: 0]`

##### 3. Operation

```verilog
PC = R[Rn] & 0xFFFFFFFFFFFFFFFC; // Align to word boundary
```

---

#### **B.cond** : Conditional Branch

##### 1. Description

Branches only if the specific Condition Code in the Status Register is met.

##### 2. Encoding

* **I-Type**: `[Op: 0x33] [Cond: 4] [Offset: 22]` *(Note: Hybrid Encoding)*

##### 3. Condition Codes Table

| Mnemonic | Suffix | Flag State | Logic |
| :--- | :--- | :--- | :--- |
| **EQ** | Equal | `Z == 1` | Equal |
| **NE** | Not Equal | `Z == 0` | Not Equal |
| **CS / HS** | Carry Set | `C == 1` | Unsigned Higher or Same |
| **CC / LO** | Carry Clear | `C == 0` | Unsigned Lower |
| **MI** | Minus | `N == 1` | Negative |
| **PL** | Plus | `N == 0` | Positive or Zero |
| **VS** | Overflow | `V == 1` | Signed Overflow |
| **VC** | No Overflow | `V == 0` | No Signed Overflow |
| **HI** | Unsigned Higher | `C == 1 && Z == 0` | `A > B` (Unsigned) |
| **LS** | Unsigned Lower/Same | `C == 0 | | Z == 1` | `A <= B` (Unsigned) |
| **GE** | Signed >= | `N == V` | `A >= B` (Signed) |
| **LT** | Signed < | `N != V` | `A < B` (Signed) |
| **GT** | Signed > | `Z == 0 && N == V` | `A > B` (Signed) |
| **LE** | Signed <= | `Z == 1 | | N != V` | `A <= B` (Signed) |
| **AL** | Always | `True` | Always (Default) |

##### 4. Operation

```verilog
if (ConditionMet(Cond)) {
    PC = PC + (SignExtend(Offset) << 2);
    FlushPipeline();
}
```

* **Format**: I-Type.
* **Operation**:
    $$ Address = Rn + \text{SignExt}(Imm) $$
    $$ Rd \leftarrow Mem[Address] $$
* **Unaligned Access**: Triggers `Data Abort` exception.
* **Latency**: 3 Cycles (Address Calc $\to$ Bus Req $\to$ Data Arrive).

#### `STR` - Store Register

* **Format**: I-Type.
* **Operation**:
    $$ Address = Rn + \text{SignExt}(Imm) $$
    $$ Mem[Address] \leftarrow Rd $$
* **Latency**: 1 Cycle (Posted Write). The CPU continues execution while the Store Buffer drains to the Bus.

### 6.3.3 Control Flow Unit (BRU)

#### `B` - Unconditional Branch

* **Format**: B-Type.
* **Operation**: $PC \leftarrow PC + \text{SignExt}(Offset \ll 2)$
* **Penalty**: 2 Cycles (Pipeline Flush).

#### `BEQ` - Branch Equal

* **Condition**: `if (Z == 1)`
* **Operation**: $PC \leftarrow PC + \text{SignExt}(Offset)$ (Note: Implementation uses I-Type offset 11-bits).

#### `BL` - Branch with Link (Call)

* **Operation**:
    $$ LR \leftarrow PC + 4 $$
    $$ PC \leftarrow PC + \text{SignExt}(Offset) $$
* **Usage**: Function calls. Returns via `MOV PC, LR`.

## 6.4 Exception Encoding

| Code | Exception | Trigger |
| :---: | :--- | :--- |
| `0x00` | **Reset** | Power-on or WDT. |
| `0x04` | **Undefined** | Opcode not in decode table. |
| `0x08` | **SVC** | `SWI` instruction execution. |
| `0x0C` | **Prefetch** | Bus Error during fetch. |
| `0x10` | **Data Abort** | Bus Error during Load/Store. |
| `0x18` | **IRQ** | External Interrupt (PIC). |

---

# Volume 6: NAND Physics & Storage Theory

**(Revision 2.1 - Quantum Mechanical Reference)**

## 6.1 The Floating Gate Transistor (FGT)

The fundamental unit of storage is the FGT. It resembles a standard NMOS transistor but includes an electrically isolated **Floating Gate (FG)** sandwiched between the Control Gate (CG) and the Channel.

* **Logic 1 (Erased State)**: The FG has a neutral charge. The Threshold Voltage ($V_{th}$) is low. When $V_{read}$ is applied to the CG, the transistor conducts ($I_{ds} > 0$).
* **Logic 0 (Programmed State)**: Electrons are trapped in the FG. The negative charge shields the channel from the CG field, raising the $V_{th}$. When $V_{read}$ is applied, the transistor remains non-conductive ($I_{ds} \approx 0$).

---

## 6.2 Quantum Mechanical Operations

### 6.2.1 Fowler-Nordheim (FN) Tunneling

Programming and Erasing rely on quantum tunneling through the oxide barrier. The current density ($J_{FN}$) follows the Fowler-Nordheim equation:

$$ J_{FN} = A_{FN} E_{ox}^2 \exp\left(-\frac{B_{FN}}{E_{ox}}\right) $$

Where:

* $E_{ox}$: Electric Field across the tunnel oxide ($\text{V/cm}$).
* $A_{FN} \approx 1.54 \times 10^{-6} \frac{m}{m^*}$ ($\text{A/V}^2$)
* $B_{FN} \approx 6.83 \times 10^7 (\frac{m^*}{m})^{1/2} \Phi_B^{3/2}$ ($\text{V/cm}$)

### 6.2.2 Threshold Voltage Distributions

In a real array, process variations lead to a distribution of $V_{th}$ for "1" and "0" states. We model these as Gaussian distributions:

$$ P(V_{th} | \text{State}) = \frac{1}{\sqrt{2\pi\sigma^2}} \exp\left(-\frac{(V_{th} - \mu_{state})^2}{2\sigma^2}\right) $$

* **Read Margin**: The gap between $\mu_1$ and $\mu_0$.
* **Bit Error Rate (BER)**: The overlap integral of the two distributions. As the device wears, $\sigma$ increases, causing higher BER.

---

## 6.3 Reliability & Error Mechanics

### 6.3.1 Endurance & Window Closure

Repeated tunneling creates charge traps in the oxide layer, steadily increasing $V_{th}$ of the erased state and decreasing $V_{th}$ of the programmed state.

**Wear Model**:
$$ V_{th}^{erased}(N) = V_{th}^{init} + \delta \cdot \log(N) $$

### 6.3.2 Data Retention (Arrhenius Model)

Trapped electrons slowly leak out of the floating gate over time. The Mean Time To Failure (MTTF) scales exponentially with temperature:

$$ \text{MTTF} \propto \exp\left(\frac{E_a}{k_B T}\right) $$

* $E_a$: Activation Energy (~1.1 eV for Tunnel Oxide).
* $T$: Junction Temperature (Kelvin).

---

## 6.4 Error Correction Theory

Raw NAND has a native Bit Error Rate (BER) between $10^{-5}$ and $10^{-3}$. We mandate ECC to achieve $10^{-15}$ UBER.

### 6.4.1 Hard vs Soft Decision

* **Hard Decision (BCH)**: The controller reads the page once at reference voltage $V_{ref}$. The output is binary.
* **Soft Decision (LDPC)**: The controller reads the page multiple times at $V_{ref} \pm \Delta$. This provides "Likelihood" ratios (LLR) to the decoder, improving correction capability near the Shannon Limit.

---

# Volume 7: Assembly Language Guide

**(Revision 3.0 - Programmer's Reference)**

## 7.1 Syntax & Directives

```ebnf
instruction  ::= MNEMONIC operand_list
directive    ::= "." IDENTIFIER [ value ]
operand      ::= REGISTER | IMMEDIATE | MEMORY_REF
```

## 7.2 Application Binary Interface (ABI)

To ensure interoperability between assembly modules and C++ code, strictly adhere to the Aurelia ABI.

### 7.2.1 Register Usage

| Regs | Usage | Saver |
| :--- | :--- | :--- |
| **R0** | Return Value / Arg 0 | Caller |
| **R1-R7** | Arguments 1-7 | Caller |
| **R8-R15** | Temporary (Scratch) | Caller |
| **R16-R29** | Saved Variables | **Callee** |
| **R30 (SP)** | Stack Pointer | **Callee** |
| **R31 (LR)** | Return Address | Caller |

### 7.2.2 Stack Frame Layout

The Stack grows **downwards** (from High Logic Address to Low). `SP` must remain 16-byte aligned at function boundaries.

```text
      +------------------+  <- High Address
      |  Caller's Frame  |
      +------------------+
      |  Return Addr (LR)|  <- SP + FrameSize - 8
      +------------------+
      |  Saved R29       |
      +------------------+
      |  ...             |
      +------------------+
      |  Saved R16       |
      +------------------+
      |  Local Var 0     |
      +------------------+
      |  Local Var N     |  <- Current SP
      +------------------+  <- Low Address
```

**Prologue Example**:

```asm
SUB SP, SP, #32      ; Alloc 32 bytes (16-byte aligned)
STR LR, [SP, #24]    ; Save Return Address at top
STR R16, [SP, #16]   ; Save Callee-Saved reg
```

**Epilogue Example**:

```asm
LDR R16, [SP, #16]   ; Restore R16
LDR LR, [SP, #24]    ; Restore LR
ADD SP, SP, #32      ; Dealloc
MOV PC, LR           ; Return
```

## 7.3 Optimization Guide

### 7.3.1 Pipeline filling

Avoid dependent instructions.

```asm
; Poor
LDR R1, [R0]
ADD R1, R1, #1  ; Stall!

; Good
LDR R1, [R0]
ADD R2, R2, #1  ; Useful work during load
ADD R1, R1, #1  ; Data now ready
```

### 7.3.2 Branch Probability

The static branch predictor assumes:

* Backward Branches (Loops) are **TAKEN**.
* Forward Branches (If/Else) are **NOT TAKEN**.
Structure your code to align with this (e.g., error handling blocks should be forward branches).

---
**End of Guide.**

**(Revision 2.0 - Extended Technical Reference)**

## 1. Introduction to Floating Gate Physics

Aurelia performs a Cycle-Accurate simulation of NAND Flash memory, modeling the stochastic behavior of electron trapping and release in Floating Gate Transistors (FGT).

### 1.1 The Floating Gate Transistor (FGT)

The fundamental unit of storage is the FGT. It resembles a standard NMOS transistor but includes an electrically isolated **Floating Gate (FG)** sandwiched between the Control Gate (CG) and the Channel.

* **Logic 1 (Erased State)**: The FG has a neutral charge. The Threshold Voltage ($V_{th}$) is low. When $V_{read}$ is applied to the CG, the transistor conducts.
* **Logic 0 (Programmed State)**: Electrons are trapped in the FG. The negative charge shields the channel from the CG field, raising the $V_{th}$. When $V_{read}$ is applied, the transistor remains non-conductive.

---

## 2. Quantum Mechanical Operations

1

### 2.1 Fowler-Nordheim (FN) Tunneling

Programming and Erasing rely on quantum tunneling through the oxide barrier. The current density ($J_{FN}$) follows the Fowler-Nordheim equation:

$$ J_{FN} = A_{FN} E_{ox}^2 \exp\left(-\frac{B_{FN}}{E_{ox}}\right) $$

Where:

* $E_{ox}$: Electric Field across the tunnel oxide ($\text{V/cm}$).
* $A_{FN}, B_{FN}$: Physical constants dependent on the oxide material (typically $\text{SiO}_2$).
* $A_{FN} \approx 1.54 \times 10^{-6} \frac{m}{m^*}$ ($\text{A/V}^2$)
* $B_{FN} \approx 6.83 \times 10^7 (\frac{m^*}{m})^{1/2} \Phi_B^{3/2}$ ($\text{V/cm}$)

**Simulation Logic**:
Since we cannot simulate femtosecond-scale tunneling, we approximate the **Tunneling Probability** $P_{tunnel}$ per simulation tick as a function of the programming voltage $V_{pp}$:
$$ P_{tunnel}(V_{pp}) = \alpha \cdot \exp(\beta \cdot V_{pp}) $$
If $P_{tunnel} > \text{Rand}(0,1)$, the bit flips from 1 to 0.

### 2.2 Threshold Voltage Distributions

In a real array, process variations lead to a distribution of $V_{th}$ for "1" and "0" states. We model these as Gaussian distributions:

$$ P(V_{th} | \text{State}) = \frac{1}{\sqrt{2\pi\sigma^2}} \exp\left(-\frac{(V_{th} - \mu_{state})^2}{2\sigma^2}\right) $$

* **Read Margin**: The gap between $\mu_1$ and $\mu_0$.
* **Bit Error Rate (BER)**: The overlap integral of the two distributions. As the device wears, $\sigma$ increases, causing higher BER.

---

## 3. Reliability & Error Mechanics

### 3.1 Endurance & Oxide Degradation

Repeated tunneling creates charge traps in the oxide layer, steadily increasing $V_{th}$ of the erased state and decreasing $V_{th}$ of the programmed state (Window Closure).

**Wear Model**:
$$ V_{th}^{erased}(N) = V_{th}^{init} + \delta \cdot \log(N) $$
Where $N$ is the number of Program/Erase cycles.

### 3.2 Data Retention (Arrhenius Model)

Trapped electrons slowly leak out of the floating gate over time, especially at high temperatures.
The Time-To-Failure ($t_{fail}$) follows:
$$ t_{fail} \propto \exp\left(\frac{E_a}{k_B T}\right) $$

* $E_a$: Activation Energy (~1.1 eV).
* $k_B$: Boltzmann Constant.
* $T$: Temperature (Kelvin).

Aurelia simulates retention loss by incrementing a "Leakage Counter" for every block. When the counter exceeds a threshold, bits randomly flip back to 1.

### 3.3 Disturb Mechanisms

* **Read Disturb**: Applying $V_{pass}$ to unselected rows repeatedly can induce weak tunneling, unintentionally programming "1" cells to "0".
* **Program Disturb**: High voltage on a selected bitline can tunnel electrons into neighbor cells.

---

## 4. Error Correction Codes (ECC)

Raw NAND has a native Bit Error Rate (BER) between $10^{-5}$ and $10^{-3}$. We mandate ECC to achieve $10^{-15}$ UBER (Uncorrectable BER).

### 4.1 Hamming Code (Single Error Connection)

For simple pages, we use a Hamming(7,4) derivative.
**Parity Check Matrix** $H$:
$$ H = \begin{bmatrix}
1 & 0 & 1 & 0 & 1 & 0 & 1 \\
0 & 1 & 1 & 0 & 0 & 1 & 1 \\
0 & 0 & 0 & 1 & 1 & 1 & 1
\end{bmatrix} $$

**Syndrome Calculation**: $S = H \cdot r^T$
If $S \ne 0$, it points to the error bit location.

### 4.2 BCH / LDPC (Advanced)
For modern NAND simulations, Low-Density Parity-Check (LDPC) codes are used. They use **Soft-Decision** decoding, utilizing the $V_{th}$ probability information rather than just binary 0/1.
$$ L(c_i) = \log \frac{P(c_i=0 | y)}{P(c_i=1 | y)} $$
Aurelia currently implements a simplified Bose-Chaudhuri-Hocquenghem (BCH) correction model capable of fixing $t=4$ bits per 512 bytes.

---

## 5. Flash Translation Layer (FTL) Algorithms

The FTL is the firmware layer managing the imperfections of the physical media.

### 5.1 Address Mapping
We employ a **Page-Associative Mapping Table**.
*   **LBA**: Logical Block Address (from OS).
*   **PBA**: Physical Block Address (Device-internal).

Table Size $= (\text{DeviceCapacity} / \text{PageSize}) \times \text{EntrySize}$.
For 256MB SSD: $(256 \times 10^6 / 4096) \approx 65,536$ Entries. This fits in SRAM.

### 5.2 Garbage Collection (GC) Cost Analysis
GC is triggered when Free Blocks $< T_{threshold}$.
The Cost Function $C$ determines victim selection efficiency:
$$ C(b) = \frac{u}{1-u} $$
Where $u$ is the utilization (fraction of valid pages).
*   **Greedy Policy**: Select block with **minimum** $u$ (Most Invalid Pages). Cost is minimized because fewer valid pages need copying.

### 5.3 Wear Leveling
*   **Dynamic**: Writes data to blocks with the lowest Erase Count.
*   **Static**: Periodically moves static data (OS Kernels) from blocks with low Erase Counts to high-wear blocks, preventing "Cold Data" from protecting blocks indefinitely.
    *   **Threshold**: Triggered when $\Delta(\text{MaxErase} - \text{MinErase}) > 100$.

---

## 6. NVMe Interface Protocol

### 6.1 Admin & I/O Queues
Aurelia implements a simplified NVMe 1.4 Command Set.
*   **SQ (Submission Queue)**: Tail Pointer (T) updated by Host.
*   **CQ (Completion Queue)**: Head Pointer (H) updated by Host.

**Doorbell Mechanism**:
$$ \text{Doorbell}_{SQ} \leftarrow \text{NewTail} $$
This write triggers the Controller's "Fetch" Engine.

### 6.2 Command Execution Flow
1.  **Fetch**: Controller reads 64-byte Command from `SQ_Base + (Head * 64)`.
2.  **Decode**: Parse bitfields (Opcode, NSID, PRP1, PRP2).
    *   `PRP` (Physical Region Page): Describes scattered physical memory pages for DMA.
3.  **Transfer**:
    *   **Write**: DMA Read from Host PRP $\rightarrow$ Controller Buffer $\rightarrow$ NAND Program.
    *   **Read**: NAND Read $\rightarrow$ ECC Correction $\rightarrow$ Controller Buffer $\rightarrow$ DMA Write to Host PRP.
4.  **Completion**: Write 16-byte CQ Entry to `CQ_Base + (Tail * 16)`.
5.  **Interrupt**: Assert MSI-X / Pin-based Interrupt.

---
**End of Specification.**

# Volume 7: Assembly Language Guide

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

# Volume 8: Advanced NAND Flash Management (FTL Specifications)

**(Revision 3.0 - Deep Engineering Reference)**

This section details the firmware algorithms required to manage the stochastic nature of the NAND medium.

## 5.1 Flash Translation Layer (FTL) Architecture

The FTL maps Logical Block Addresses (LBA) from the host NVMe interface to Physical Block Addresses (PBA) on the NAND media.

### 5.1.1 Hybrid Log-Block Mapping

To balance RAM usage and Write Amplification Factor (WAF), we employ a Hybrid Mapping scheme.
1.  **Data Blocks**: Mapped at the coarse **Block Level** (Large granularity). Low RAM overhead.
2.  **Log Blocks**: Mapped at the fine **Page Level** (4KB granularity). Used for random write buffering.

**Associativity Logic**:
Let $L$ be the set of Log Blocks and $D$ be the set of Data Blocks.
When a Host Page write arrives for LBA $x$:
1.  Identify target Logical Data Block $B_{data} = x / \text{PagesPerBlock}$.
2.  Allocate a Log Block $B_{log}$ from the Free List if not present.
3.  Write data to the next free page offset in $B_{log}$.
4.  Update **Page Mapping Table (PMT)**: `PMT[LBA] = {Channel, Way, Block, Page}`.

### 5.1.2 Merge Operations (Switch / Partial / Full)

When a Log Block $B_{log}$ becomes full, a **Merge** is triggered to flush data to a Data Block.

*   **Switch Merge** (Best Case):
    *   Condition: All pages in $B_{log}$ are sequentially written and align perfectly with a Data Block.
    *   Action: $B_{log}$ is simply promoted to become a Data Block. Old Data Block is erased.
    *   Cost: $O(1)$. Zero copy.

*   **Partial Merge**:
    *   Condition: $B_{log}$ contains only valid updates for the *first k* pages.
    *   Action: Copy remaining valid pages $(k..N)$ from the old Data Block to $B_{log}$.
    *   Action: $B_{log}$ becomes the new Data Block.
    *   Cost: $O(N-k)$ page copies.

*   **Full Merge** (Worst Case):
    *   Condition: $B_{log}$ contains random updates scattered across the address space.
    *   Action: Allocate a NEW Block $B_{new}$.
    *   Action: For $i$ in $0..N$:
        *   Source = `PMT[i]` points to $B_{log}$ ? $B_{log}[offset]$ : $OldDataBlock[i]$.
        *   Copy Source $\to B_{new}[i]$.
    *   Action: Erase $B_{log}$ and $OldDataBlock$.
    *   Cost: $O(N)$ copies + 2 Erases.

**Algorithm 5.1: Merge Decision Tree**
```cpp
MergeType DecideMerge(LogBlock* log, DataBlock* data) {
    if (log->IsSequential() && log->StartLBA == data->StartLBA) {
        return SWITCH_MERGE;
    }
    if (log->ValidPageCount() + data->ValidPageCount() < PAGES_PER_BLOCK) {
        // Optimization: Garbage Collect both into a new block
        return FULL_MERGE_COMPACT;
    }
    return PARTIAL_MERGE;
}
```

## 5.2 Garbage Collection (GC) & Victim Selection

GC reclaims Invalid Pages caused by overwrites. It selects a Victim Block, copies its Valid Pages to a new block, and Erases the Victim.

### 5.2.1 Greedy vs. Cost-Benefit

*   **Greedy Policy**: Always select the block with the **Maximum Invalid Page Count**.
    *   Pros: Yields the most free space immediately.
    *   Cons: Ignores "Hot" data. If a block is hot (frequently updated), cleaning it now is wasteful as it will become invalid again soon.

*   **Cost-Benefit (CB) Policy**:
    Considers the age of the data (Time since last modification $T_{age}$).
    $$ \text{Value} = \frac{\text{FreeSpace} \times T_{age}}{\text{ValidSpace} + \text{ReadTime} + \text{ProgTime}} $$
    We prefer blocks with **High Free Space** (Low copy cost) and **High Age** (Cold data, unlikely to be updated soon).

### 5.2.2 Wear Leveling (Static & Dynamic)

NAND blocks have limited Program/Erase (P/E) cycles (e.g., 3,000 for TLC). Uneven wear leads to premature device failure.

*   **Dynamic Wear Leveling**:
    *   Mechanism: On every write allocation, pick a free block with the **Low Erase Count**.
    *   Effect: Spreads wear across active pool.

*   **Static Wear Leveling** (The Cold Data Problem):
    *   Issue: Blocks containing static data (e.g., OS files) never get erased/rewritten. They stay at Low Erase Counts while free blocks act as hot buffers and burn out.
    *   Solution:
        1.  Check Threshold: If $\Delta = \text{MaxErase} - \text{MinErase} > T_{wear}$ (e.g., 50).
        2.  Force Move: Identify the block with **MinErase** (Cold). Copy its data to a high-wear block.
        3.  Result: The low-wear block enters the allocation pool and absorbs write traffic.

---

## 5.3 Error Correction Engine (BCH/LDPC)

### 5.3.1 Bose-Chaudhuri-Hocquenghem (BCH) Codes

Aurelia implements a specialized BCH encoder over the Galois Field $GF(2^{13})$.
*   **Codeword Size**: $N = 2^m - 1 = 8191$ bits (approx 1KB).
*   **Correction Capability**: $t$ errors.
*   **Generator Polynomial** $g(x)$: The Least Common Multiple of minimal polynomials for $\alpha, \alpha^2, ..., \alpha^{2t}$.

**Encoding Logic (LFSR)**:
Data $d(x)$ is divided by $g(x)$. The remainder $r(x)$ is the Parity.
$$ c(x) = x^{n-k} d(x) + r(x) $$

**Decoding Logic (Syndrome Decoding)**:
1.  **Syndrome Calculation**: Calculate $2t$ syndromes $S_j = R(\alpha^j)$. If all $S_j == 0$, no error.
2.  **Error Locator Polynomial (ELP)** $\Lambda(x)$: Use the **Berlekamp-Massey Algorithm**.
    *   Iteratively compute $\Lambda(x) = 1 + \sigma_1 x + ... + \sigma_t x^t$.
3.  **Chien Search**: Find roots of $\Lambda(x)$.
    *   Evaluate $\Lambda(\alpha^{-i})$ for $i = 0..n-1$.
    *   If result is 0, then position $i$ is an error.
4.  **Bit Flip**: Invert the bit at position $i$.

**Mathematica Implementation of Berlekamp-Massey in C++**:
```cpp
// Logic for Step k:
// discrepancy d = S[k] + sigma[1]*S[k-1] + ...
if (d != 0) {
    T(x) = Lambda(x) - d * b^{-1} * x * B(x);
    if (2*L <= k) {
        L = k + 1 - L;
        B(x) = Lambda(x);
        b = d;
    }
    Lambda(x) = T(x);
}
```

---

## 5.4 Electrical Interface Specifications

This section defines the physical layer (PHY) timing requirements for the ONFI (Open NAND Flash Interface) bus.

### 5.4.1 AC Timing Characteristics

| Parameter | Symbol | Min (ns) | Max (ns) | Description |
| :--- | :---: | :---: | :---: | :--- |
| Clock Period | tCK | 10 | - | 100MHz DDR Mode |
| Command Latch Setup | tCLS | 2.5 | - | CLE setup to WE# rising |
| Command Latch Hold | tCLH | 1.5 | - | CLE hold from WE# rising |
| Address Latch Setup | tALS | 2.5 | - | ALE setup to WE# rising |
| Data Setup | tDS | 1.0 | - | DQ setup to DQS rising |
| Data Hold | tDH | 1.0 | - | DQ hold from DQS rising |
| Write Recovery | tWR | 10 | - | WE# high to Busy |

### 5.4.2 Signal Integrity & Termination

*   **ODT (On-Die Termination)**:
    *   To prevent signal reflection on the high-speed DQ bus, the Controller enables internal resistors ($R_{tt}$) pulling to $V_{ccq}/2$.
    *   Supported Values: 50$\Omega$, 75$\Omega$, 150$\Omega$.
*   **Slew Rate Control**:
    *   Output drivers are configurable to adjust rise/fall times ($t_{R}, t_{F}$) to minimize EMI and Cross-talk.
    *   Typical setting: 1.5 V/ns.

### 5.4.3 Power Consumption Models

The simulation models instantaneous current draw $I_{cc}$ based on state:

| State | Algorithm | Typical Current | Max Current |
| :--- | :--- | :---: | :---: |
| **Idle** | $I_{leak} + I_{standby}$ | 5 mA | 10 mA |
| **Read** | $I_{base} + (F_{clk} \times C_{load} \times V_{cc})$ | 25 mA | 40 mA |
| **Program** | Charge Pump Logic ($V_{pp}=20V$ gen) | 35 mA | 60 mA |
| **Erase** | Bulk tunneling current | 40 mA | 70 mA |

**Thermal Throttling Logic**:
If $T_{die} > 85^\circ C$:
1.  Assert `TEMP_WARN` interrupt.
2.  Throttle $F_{clk}$ from 800MT/s to 200MT/s.
3.  Increase $t_{PROG}$ delay to reduce charge pump duty cycle.

---

## 5.5 Data Integrity Validation (End-to-End)

Aurelia ensures data integrity not just on the media, but through the entire datapath.

### 5.5.1 Data Path Protection (DIF/DIX)

We implement T10-DIF (Data Integrity Field) standards. An 8-byte Footer is appended to every 512-byte sector.

*   `Guard Tag` (16-bit): CRC-16 of the data payload. Checked at every buffer transfer.
*   `Reference Tag` (32-bit): The LBA itself. Prevents **Mis-directed Writes** (Ghost writes where data lands in the wrong sector).
*   `App Tag` (16-bit): Reserved for Application-level checksums (e.g., Oracle DB).

**Validation Logic**:
When data moves from `Host -> Controller SRAM`:
1.  Compute CRC-16 of Payload.
2.  Compare with Footer.Guard.
3.  Compare Buffer Address with Footer.Ref.
4.  **Panic** if mismatch.

When data moves from `Controller SRAM -> NAND Page`:
1.  Re-verify CRC.
2.  Compute BCH Parity for Payload + DIF.
3.  Write {Payload, DIF, Parity} to Flash.

---
**End of Extended Technical Specification.**

# Volume 9: Glossary & Version History

## 9.1 Glossary of Terms

| Term | Definition | Context |
| :--- | :--- | :--- |
| **ACLK** | Aurelia System Clock. The master clock signal for the ASB. | Hardware |
| **ASB** | Aurelia System Bus. The interconnect protocol defined in Vol 1. | Bus |
| **BCH** | Bose-Chaudhuri-Hocquenghem. An error correction code used for NAND. | ECC |
| **BER** | Bit Error Rate. The probability of a bit flip in raw media. | NAND |
| **CLE** | Command Latch Enable. Signal to latch command bytes into NAND. | ONFI |
| **CPI** | Cycles Per Instruction. A metric of pipeline efficiency. Target = 1.0. | CPU |
| **DIF** | Data Integrity Field. An 8-byte footer for end-to-end protection. | SSD |
| **FGT** | Floating Gate Transistor. The storage cell of Flash memory. | Physics |
| **FN Tunneling** | Fowler-Nordheim Tunneling. Quantum mechanism for Charge Trap. | Physics |
| **FTL** | Flash Translation Layer. Firmware converting LBAs to PBAs. | Firmware |
| **GC** | Garbage Collection. The process of reclaiming invalid pages. | Firmware |
| **LBA** | Logical Block Address. The host view of the storage device. | Storage |
| **LDPC** | Low-Density Parity-Check. Advanced Soft-Decision ECC. | ECC |
| **LLR** | Log-Likelihood Ratio. Soft information for LDPC decoding. | ECC |
| **LSU** | Load/Store Unit. CPU pipeline stage for memory access. | CPU |
| **MMIO** | Memory-Mapped I/O. Accessing peripherals via physical address space. | Arch |
| **MSI-X** | Message Signaled Interrupts (Extended). PCIe interrupt mechanism. | Intr |
| **NVMe** | Non-Volatile Memory Express. First protocol designed for SSDs. | Protocol |
| **PBA** | Physical Block Address. The internal NAND location (Ch/Way/Blk/Pg).| Storage |
| **PRP** | Physical Region Page. Scatter-gather list format for NVMe. | DMA |
| **UBER** | Uncorrectable Bit Error Rate. Target reliability ($10^{-15}$). | NAND |
| **WAF** | Write Amplification Factor. Ratio of Media Writes to Host Writes. | FTL |

## 9.2 Version History

| Version | Date | Editor | Changes |
| :---: | :---: | :---: | :--- |
| **1.0** | 2025-01-01 | Yuriko | Initial Draft (Sparse). |
| **2.0** | 2025-01-08 | Klea | Added detailed Logic Specification. |
| **3.0** | 2025-01-15 | Klea | Expanded FTL and Physics (Vol 6, 8). |
| **4.0** | 2025-01-20 | Klea | Microarchitecture & Hazard Logic (Vol 2). |
| **4.1** | 2025-01-21 | Klea | Bus Electrical Specs & FSM (Vol 1). |
| **5.0** | 2025-01-21 | Klea | Encyclopedic ISA Reference (Vol 5). |

---
**End of Document.**
