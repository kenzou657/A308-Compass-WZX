# TI MSPM0G3507 电赛小车项目

## 项目要求

### 竞赛任务
2026 年四川省大学生电子设计竞赛 - 仓储搬运小车（B 题）

**基本要求**:
1. 循迹导航 - 小车自主循迹到达指定物品存放区
2. 多点导航 - 依次到达两个目标区域
3. 返回停靠 - 完成任务后返回停车启动区（30±2s 内）
4. 数字识别 - 3s 内识别任务数字（1-5）

**发挥部分**:
1. 自动物品搬运 - 识别并搬运指定物品
2. 智能寻物 - 自主查找并搬运目标物品
3. 顺序搬运 - 按顺序搬运多个物品

### 硬件约束
- **MCU**: TI MSPM0G3507 (ARM Cortex-M0+ @ 80MHz)
- **尺寸**: ≤25cm(长)×15cm(宽)×15cm(高)
- **调试器**: DAP-LINK

---

## 项目功能

### 核心功能
- ✅ **循迹控制** - 基于 PID 算法的自适应循迹
- ✅ **舵机控制** - 50Hz 固定频率的精确角度控制
- ✅ **电机驱动** - PWM 调速、速度反馈
- ✅ **UART 通信** - 与摄像头的帧级通信
- ✅ **数字识别** - 图像识别结果解析
- ✅ **物品搬运** - 状态机驱动的自动搬运流程
- ✅ **路径规划** - 多点导航和返回停靠

### 传感器支持
- 循迹传感器
- IMU 传感器（加速度、陀螺仪）
- 摄像头（数字识别）

---

## 项目架构

### 分层设计
```
┌─────────────────────────────────────────┐
│         Main Loop (主循环)              │
│  处理标志位、调用应用层、更新状态机    │
└─────────────────────────────────────────┘
                  ▲
                  │
┌─────────────────────────────────────────┐
│      应用层 (src/app/)                  │
│  PID、循迹、数字识别、物品搬运、导航   │
└─────────────────────────────────────────┘
                  ▲
                  │
┌─────────────────────────────────────────┐
│      驱动层 (src/drivers/)              │
│  电机、舵机、UART、传感器、GPIO        │
└─────────────────────────────────────────┘
                  ▲
                  │
┌─────────────────────────────────────────┐
│    DriverLib 层 (TI SDK)                │
│    DL_TimerG_*、DL_UART_*、等          │
└─────────────────────────────────────────┘
```

### 目录结构
```
project-root/
├── src/                          # 源代码
│   ├── main.c                    # 主程序入口
│   ├── config.h                  # 全局配置宏
│   ├── config/                   # SysConfig 生成文件
│   ├── drivers/                  # 驱动层（硬件抽象）
│   ├── app/                      # 应用层（业务逻辑）
│   ├── utils/                    # 工具函数
│   └── isr/                      # 中断服务函数
├── keil/                         # Keil 项目文件
├── config/                       # 配置文件
│   └── empty.syscfg              # SysConfig 配置
├── doc/                          # 文档
├── tools/                        # 工具脚本
├── .roo/                         # AI 助手规则
├── AGENTS.md                     # AI 助手指南
├── .gitignore                    # Git 忽略规则
└── README.md                     # 本文件
```

### 模块划分

**驱动层** (`src/drivers/`)
- `motor_driver` - PWM 电机控制
- `servo_driver` - 50Hz 舵机控制
- `uart_driver` - 中断接收、帧验证
- `line_sensor` - 循迹传感器采样
- `imu_sensor` - IMU 数据解析
- `gpio_driver` - GPIO 初始化、控制

**应用层** (`src/app/`)
- `pid_controller` - PID 算法实现
- `line_tracker` - 循迹控制逻辑
- `digit_recognizer` - 数字识别解析
- `transport_fsm` - 物品搬运状态机
- `navigator` - 路径规划、导航

**工具层** (`src/utils/`)
- `queue` - 环形队列（UART 缓冲）
- `timer_utils` - 定时器工具
- `math_utils` - 数学函数（整型缩放）

---

## 关键设计原则

### 1. 驱动-应用解耦
- 驱动层只负责硬件操作
- 应用层只负责算法逻辑
- 通过函数调用或标志位通信

### 2. ISR 最小化
- ISR 仅设置标志位或更新计数器
- 所有处理逻辑在主循环中执行
- 避免 ISR 中的浮点运算

### 3. 模块独立性
- 每个模块有明确的接口 (`.h` 文件)
- 模块间通过接口通信
- 便于单元测试和代码复用

### 4. 配置集中化
- 所有宏定义在 `src/config.h`
- SysConfig 配置在 `config/empty.syscfg`
- 便于快速调整参数

---

## 构建与编译

### 环境要求
- Keil µVision v5.39
- TI MSPM0 SDK
- TI SysConfig

---

## 文档

- [AGENTS.md](AGENTS.md) - AI 助手指南
- [doc/ARCHITECTURE_REFACTOR.md](doc/ARCHITECTURE_REFACTOR.md) - 架构重构方案
- [doc/ARCHITECTURE_IMPLEMENTATION.md](doc/ARCHITECTURE_IMPLEMENTATION.md) - 实施指南
- [doc/ARCHITECTURE_SUMMARY.md](doc/ARCHITECTURE_SUMMARY.md) - 架构总结
- [doc/2026 年四川省大学生电子设计竞赛 西南石油大学成都校区选拔赛.md](doc/2026%20年四川省大学生电子设计竞赛%20西南石油大学成都校区选拔赛.md) - 竞赛要求

---

## 快速开始

1. **克隆项目**
   ```bash
   git clone <repository>
   cd empty
   ```

2. **打开 Keil 项目**
   ```
   keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx
   ```

3. **编译**
   ```
   Keil → Build Target (Ctrl+F7)
   ```

4. **烧写**
   ```
   Keil → Download (Ctrl+F8)
   ```

5. **调试**
   ```
   Keil → Start/Stop Debug Session (Ctrl+F5)
   ```

---

## 性能指标

- **MCLK**: 80MHz
- **SRAM**: 32KB
- **Flash**: 128KB
- **循迹频率**: 10ms (100Hz)
- **舵机频率**: 50Hz (20ms)
- **UART 波特率**: 可配置（推荐 115200)

---

## 常见问题

**Q: 编译失败，找不到头文件**  
A: 检查 Keil 项目的 Include Paths，确保包含 `src`、`src/config`、`src/drivers` 等目录

**Q: SysConfig 配置不生效**  
A: 在 Keil 中重新运行 SysConfig，确保生成的文件输出到 `src/config/`

**Q: 烧写后无反应**  
A: 检查 `src/main.c` 中的 `main()` 函数是否调用了 `SYSCFG_DL_init()`

**Q: 中断不工作**  
A: 在 SysConfig 中检查中断优先级配置，确保没有优先级冲突
