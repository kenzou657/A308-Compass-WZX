# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Architect Mode - Project-Specific Architecture

### System Architecture Overview
```
┌─────────────────────────────────────────────────────┐
│         TI MSPM0G3507 (ARM Cortex-M0+)              │
├─────────────────────────────────────────────────────┤
│  SysConfig Layer (Peripheral Initialization)        │
│  ├─ TIMA/TIMG (PWM: Motor, Servo @ 50Hz)           │
│  ├─ UART (Communication: OpenMV, IMU)              │
│  ├─ GPIO (Line Sensors, Motor Control)             │
│  └─ Clock Tree (80MHz MCLK)                        │
├─────────────────────────────────────────────────────┤
│  DriverLib Layer (Hardware Abstraction)             │
│  ├─ DL_TimerG_setCaptureCompareValue() [PWM]       │
│  ├─ DL_UART_transmitData() [Communication]         │
│  └─ DL_GPIO_setPins() [Digital I/O]                │
├─────────────────────────────────────────────────────┤
│  Driver Layer (Decoupled from App)                  │
│  ├─ Motor Driver (PWM control, speed feedback)     │
│  ├─ Servo Driver (50Hz pulse width control)        │
│  ├─ UART Driver (frame validation, buffering)      │
│  └─ Sensor Driver (line tracking, IMU parsing)     │
├─────────────────────────────────────────────────────┤
│  App Layer (Business Logic)                         │
│  ├─ Line Tracking (PID control)                    │
│  ├─ Digit Recognition (OpenMV integration)         │
│  ├─ Object Transport (state machine)               │
│  └─ Navigation (path planning)                     │
├─────────────────────────────────────────────────────┤
│  Main Loop + ISR Coordination                       │
│  ├─ ISRs: Set flags only (minimal processing)      │
│  └─ Main: Process flags, run control algorithms    │
└─────────────────────────────────────────────────────┘
```

### Critical Architectural Constraints
1. **SysConfig Dependency**: All peripheral initialization must be defined in SysConfig first
   - Modifying `.syscfg` requires regeneration in Keil
   - Generated `ti_msp_dl_config.c/h` is the source of truth for peripheral setup
   
2. **Driver-App Decoupling**: Mandatory separation
   - Drivers: Hardware abstraction only (no business logic)
   - App: Control algorithms only (no direct hardware access)
   - Communication: Via callbacks, flags, or queues (never direct function calls from ISR to App)

3. **ISR Minimalism**: Keep interrupt handlers lightweight
   - ISRs: Set flags, update counters, queue data
   - Main Loop: Process flags, run PID, make decisions
   - Rationale: Prevent ISR latency from missing high-frequency signals

4. **No Floating Point in ISRs**: Use integer arithmetic
   - ISRs: Integer counters, bit operations
   - Main Loop: Can use float for PID calculations (if needed)
   - Optimization: Pre-scale integers (e.g., angle as 0-36000 for 0-360°)

### Contest-Driven Requirements
- **Line Tracking**: Requires fast sensor sampling (ISR-based) + PID control (main loop)
- **Digit Recognition**: OpenMV sends data via UART - frame validation critical
- **Object Transport**: State machine (idle → navigate → pickup → return → drop)
- **Size Constraint**: ≤25cm×15cm×15cm - impacts sensor/motor placement

### Build & Deployment Pipeline
1. **SysConfig Edit**: Modify `empty.syscfg` in Keil SysConfig editor
2. **Regenerate**: Keil BeforeMake hook runs `syscfg.bat` automatically
3. **Compile**: Keil compiles C code + generated config
4. **Output**: 
   - ELF: `keil/Objects/empty_LP_MSPM0G3507_nortos_keil.axf`
   - HEX: `keil/Objects/empty_LP_MSPM0G3507_nortos_keil.hex` (for flashing)
5. **Debug**: XDS-110 debugger via SWD (PA19/PA20)

### Performance Optimization Strategy
- **Servo Control**: 50Hz fixed (20ms period) - no optimization needed
- **Motor PWM**: Frequency configurable in SysConfig - balance between smoothness and ISR load
- **Line Tracking**: Sensor sampling rate determines control responsiveness
- **Integer Scaling**: Replace float math with scaled integers in critical loops
  - Example: Store angle as `int16_t` (0-36000) instead of `float` (0.0-360.0)
  - Benefit: 10-100x faster on Cortex-M0+

### Testing & Validation
- **Unit Testing**: Not applicable (embedded hardware)
- **Integration Testing**: Verify each subsystem (motor, servo, UART, sensors) independently
- **System Testing**: Full contest scenario (line tracking → digit recognition → object transport)
- **Debugging**: Use XDS-110 breakpoints or UART logging (no printf available)
