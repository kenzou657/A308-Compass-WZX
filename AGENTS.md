# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Project Overview
TI MSPM0G3507 嵌入式电赛小车项目。基于 TI SDK DriverLib，使用 SysConfig 配置外设，Keil v5.39 编译调试。

## Build & Compilation
- **IDE**: Keil µVision v5.39 (ARM-ADS toolchain, ARMCLANG v6.21)
- **Build Process**: 
  - SysConfig 自动生成 `ti_msp_dl_config.c/h`（在 Keil BeforeMake 阶段执行 `syscfg.bat`）
  - 编译输出: `keil/Objects/empty_LP_MSPM0G3507_nortos_keil.axf` (ELF) 和 `.hex` (烧写文件)
  - 不支持命令行编译，必须通过 Keil IDE 或 UV4.exe 调用

## SysConfig Workflow (Critical)
- **配置文件**: `empty.syscfg`（必须在 Keil 中打开 SysConfig 编辑）
- **生成文件**: `ti_msp_dl_config.c/h`（自动生成，勿手动编辑）
- **必须步骤**: 修改外设配置后，在 Keil 中重新生成配置文件，否则编译会失败或配置不生效

## Code Structure & Patterns
- **入口**: `empty.c` 中 `main()` 调用 `SYSCFG_DL_init()` 初始化所有外设
- **DriverLib 使用**: 严禁直接寄存器操作，必须使用 DriverLib 函数（如 `DL_TimerG_setCaptureCompareValue`）
- **中文注释**: 所有代码注释、变量名建议使用中文
- **模块化要求**: 
  - 驱动层(Driver) 与算法层(App) 必须解耦
  - PID 逻辑禁止写在 Timer 中断服务函数内，使用标志位或回调机制

## PWM & Servo Control
- **舵机**: 必须锁定 50Hz，脉宽 0.5ms-2.5ms（使用 TIMA/TIMG 模块）
- **电机**: 使用 PWM 控制，占空比更新通过 `DL_TimerG_setCaptureCompareValue()`

## UART Communication
- **接收方式**: 必须支持中断或 DMA 接收
- **协议验证**: 帧头帧尾验证（如 0x55, 0xAA）
- **数据源**: OpenMV 图像识别、IMU 传感器数据需高效解析

## Performance Optimization
- 减少浮点运算，关键循环使用整型比例缩放
- 中断优先级配置在 SysConfig 中设置

## Contest Requirements
- 赛题详见 `doc/2026 年四川省大学生电子设计竞赛 西南石油大学成都校区选拔赛.md`
- 仓储搬运小车：循迹、数字识别、物品搬运
- 小车尺寸限制: ≤25cm(长)×15cm(宽)×15cm(高)

## Debugging
- **调试器**: XDS-110 (LaunchPad 内置)
- **SWD 引脚**: PA19 (SWDIO), PA20 (SWCLK)
- **低功耗**: 未使用的引脚在 SysConfig 中配置为 GPIO 输出低或带上拉/下拉输入
