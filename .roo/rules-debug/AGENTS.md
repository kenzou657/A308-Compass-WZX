# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Debug Mode - Project-Specific Rules

### Debugging Setup
- **Debugger**: XDS-110 (built-in on LaunchPad)
- **SWD Pins**: PA19 (SWDIO), PA20 (SWCLK)
- **Jumper Config**: J101 13:14 (SWDIO), J101 15:16 (SWCLK) must be ON for debugging
- **IDE**: Keil µVision v5.39 with integrated debugger

### Common Silent Failures
- **SysConfig Not Regenerated**: Code compiles but peripherals don't initialize - always rebuild after `.syscfg` changes
- **Unused Pins Not Configured**: Can cause unexpected behavior - use SysConfig "Configure Unused Pins" feature
- **ISR Priority Conflicts**: Set in SysConfig, not in code - verify in generated `ti_msp_dl_config.c`
- **UART Frame Loss**: Missing frame validation causes silent data corruption - always check 0x55/0xAA headers

### Breakpoint Strategy
- **Initialization**: Set breakpoint in `SYSCFG_DL_init()` to verify peripheral setup
- **ISR Debugging**: Avoid breakpoints in ISRs (can cause timing issues) - use flag-based logging instead
- **State Machine**: Breakpoint at state transitions to verify control flow

### Logging Without Debugger
- **UART Output**: Use UART to print debug messages (requires separate UART config in SysConfig)
- **LED Indicators**: Blink patterns for state indication (GPIO toggle in main loop)
- **Timing Verification**: Use Timer overflow flags to measure execution time

### Memory Constraints
- **SRAM**: 32KB total (0x20000000-0x20007FFF + 0x20100000-0x20107FFF)
- **Flash**: 128KB (0x00000000-0x0001FFFF + 0x00400000-0x0041FFFF)
- **Stack Overflow**: Check if ISR flags not being cleared (infinite loop symptom)

### Performance Profiling
- **Timer Measurement**: Use TIMA/TIMG to measure code execution time
- **Floating Point Cost**: Profile before/after integer scaling optimization
- **ISR Duration**: Keep ISRs under 100µs to avoid missing interrupts
