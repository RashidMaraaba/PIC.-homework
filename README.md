# 🌡️ PIC18F4620 Industrial Climate Controller

A robust embedded system designed for real-time thermal regulation using the XC8 compiler.

## ⚙️ Engineering & Architecture
### 🔌 Signal Processing (ADC & PWM)
* **ADC Integration:** The system continuously samples analog signals from temperature sensors (LM35 simulation) and set-point potentiometers, converting them to 10-bit digital values for processing.
* **PWM Modulation:** Unlike simple ON/OFF controls, the Cooling mode utilizes **Pulse Width Modulation (PWM)** to dynamically adjust the DC fan's duty cycle based on the temperature error magnitude ($T_{current} - T_{target}$).

### 🛡️ System Stability (Hysteresis)
To prevent "hunting" (rapid switching between ON/OFF states) near the set-point, a **Hysteresis ($H_s$) Algorithm** is implemented.
* **Logic:** The system only switches modes when the temperature deviates beyond a defined safety buffer (e.g., $SP \pm H_s$), ensuring hardware longevity and energy efficiency.

### ⏱️ Interrupt-Driven Design
Critical tasks (like emergency stops or mode switching) are handled via **ISRs (Interrupt Service Routines)**, ensuring immediate CPU response regardless of the main loop's state.
