# Aurelia V2 Architecture - NAND Physics & Storage Theory

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
