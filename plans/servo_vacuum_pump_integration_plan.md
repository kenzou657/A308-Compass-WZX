# 舵机控制真空泵吸嘴集成方案

## 1. 系统架构概述

### 硬件组成
```
┌─────────────────────────────────────────────────────┐
│         MSPM0G3507 MCU (80MHz)                      │
├─────────────────────────────────────────────────────┤
│  TIMA1-CH0 (PA10)                                   │
│  └─ 舵机PWM (50Hz, 0.5~2.5ms)                      │
│     └─ 控制吸嘴旋转角度 (0°~180°)                  │
│                                                     │
│  TIMG8-CH0 (PA26)                                   │
│  └─ 真空泵PWM (50Hz, 0.5~2.5ms)                    │
│     └─ 控制真空泵启停                              │
│                                                     │
│  TIMG8-CH1 (PA30)                                   │
│  └─ 电磁阀PWM (50Hz, 0.5~2.5ms)                    │
│     └─ 控制电磁阀打开/关闭                         │
└─────────────────────────────────────────────────────┘
```

### 软件分层
```
┌─────────────────────────────────────────────────────┐
│  应用层 (App Layer)                                 │
│  ├─ app_gripper.c (新增)                           │
│  │  └─ 夹爪状态机：吸取/放下流程                   │
│  └─ 任务层调用夹爪接口                             │
├─────────────────────────────────────────────────────┤
│  驱动层 (Driver Layer)                              │
│  ├─ drv_servo.c (已有)                             │
│  │  └─ 舵机角度控制                                │
│  ├─ drv_vacuum_pump.c (已有)                       │
│  │  └─ 真空泵/电磁阀脉宽控制                       │
│  └─ drv_gripper.c (新增)                           │
│     └─ 夹爪硬件协调层                              │
└─────────────────────────────────────────────────────┘
```

## 2. 工作流程设计

### 2.1 吸取物体流程

```
┌─────────────────────────────────────────────────────┐
│ 初始状态：舵机90°(中位)，真空泵关闭，电磁阀关闭   │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 步骤1：舵机旋转到吸取位置 (例如 0°)                │
│ - 调用 drv_servo_set_angle(0)                      │
│ - 等待舵机到位 (延迟 ~500ms)                       │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 步骤2：启动真空泵                                   │
│ - 调用 VacuumPump_SetPulse(VACUUM_PUMP_PULSE_OPEN) │
│ - 等待真空形成 (延迟 ~1000ms)                      │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 步骤3：舵机旋转到抬起位置 (例如 180°)              │
│ - 调用 drv_servo_set_angle(180)                    │
│ - 等待舵机到位 (延迟 ~500ms)                       │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 完成：物体已吸取并抬起                              │
└─────────────────────────────────────────────────────┘
```

### 2.2 放下物体流程

```
┌─────────────────────────────────────────────────────┐
│ 初始状态：舵机180°(抬起)，真空泵开启，物体被吸取  │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 步骤1：舵机旋转到放下位置 (例如 0°)                │
│ - 调用 drv_servo_set_angle(0)                      │
│ - 等待舵机到位 (延迟 ~500ms)                       │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 步骤2：打开电磁阀释放真空                           │
│ - 调用 SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_OPEN) │
│ - 等待物体释放 (延迟 ~500ms)                       │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 步骤3：停止真空泵                                   │
│ - 调用 VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE) │
│ - 延迟 ~100ms                                       │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 步骤4：关闭电磁阀                                   │
│ - 调用 SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE) │
│ - 延迟 ~100ms                                       │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 步骤5：舵机回到中位 (90°)                           │
│ - 调用 drv_servo_set_angle(90)                     │
│ - 等待舵机到位 (延迟 ~500ms)                       │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 完成：物体已放下，系统回到待命状态                  │
└─────────────────────────────────────────────────────┘
```

## 3. 新增模块设计

### 3.1 夹爪驱动层 (drv_gripper.h/c)

**职责**：协调舵机、真空泵、电磁阀的硬件操作

```c
/* 夹爪状态枚举 */
typedef enum {
    GRIPPER_STATE_IDLE,           /* 待命 */
    GRIPPER_STATE_MOVING_DOWN,    /* 舵机下降中 */
    GRIPPER_STATE_PUMPING,        /* 真空泵启动中 */
    GRIPPER_STATE_MOVING_UP,      /* 舵机上升中 */
    GRIPPER_STATE_HOLDING,        /* 物体被吸取 */
    GRIPPER_STATE_RELEASING,      /* 释放中 */
    GRIPPER_STATE_ERROR           /* 错误状态 */
} GripperState_t;

/* 核心函数 */
void Gripper_Init(void);                    /* 初始化 */
void Gripper_SetSuctionAngle(uint16_t angle);  /* 设置吸取角度 */
void Gripper_SetReleaseAngle(uint16_t angle);  /* 设置放下角度 */
void Gripper_SetHoldAngle(uint16_t angle);     /* 设置抬起角度 */
void Gripper_SetTimings(uint16_t move_delay, uint16_t pump_delay, uint16_t release_delay);
```

### 3.2 夹爪应用层 (app_gripper.h/c)

**职责**：实现吸取/放下的完整状态机

```c
/* 夹爪命令枚举 */
typedef enum {
    GRIPPER_CMD_IDLE,      /* 待命 */
    GRIPPER_CMD_SUCTION,   /* 吸取 */
    GRIPPER_CMD_RELEASE    /* 放下 */
} GripperCmd_t;

/* 核心函数 */
void Gripper_App_Init(void);
void Gripper_App_Update(void);              /* 周期调用，驱动状态机 */
void Gripper_App_SetCommand(GripperCmd_t cmd);
GripperState_t Gripper_App_GetState(void);
bool Gripper_App_IsBusy(void);              /* 判断是否忙碌 */
```

## 4. 时序参数配置

### 4.1 吸取流程时序

| 步骤 | 操作 | 延迟时间 | 说明 |
|------|------|---------|------|
| 1 | 舵机移动到吸取位置 | 500ms | 给舵机足够时间到位 |
| 2 | 启动真空泵 | 1000ms | 等待真空形成，确保吸力充足 |
| 3 | 舵机移动到抬起位置 | 500ms | 抬起物体 |
| **总耗时** | | **2000ms** | 约2秒完成吸取 |

### 4.2 放下流程时序

| 步骤 | 操作 | 延迟时间 | 说明 |
|------|------|---------|------|
| 1 | 舵机移动到放下位置 | 500ms | 降低吸嘴 |
| 2 | 打开电磁阀 | 500ms | 释放真空 |
| 3 | 停止真空泵 | 100ms | 关闭泵 |
| 4 | 关闭电磁阀 | 100ms | 关闭阀 |
| 5 | 舵机回到中位 | 500ms | 回到待命位置 |
| **总耗时** | | **1700ms** | 约1.7秒完成放下 |

### 4.3 可配置参数

```c
#define GRIPPER_SUCTION_ANGLE       0       /* 吸取位置角度 */
#define GRIPPER_RELEASE_ANGLE       0       /* 放下位置角度 */
#define GRIPPER_HOLD_ANGLE          180     /* 抬起位置角度 */
#define GRIPPER_IDLE_ANGLE          90      /* 待命位置角度 */

#define GRIPPER_MOVE_DELAY          500     /* 舵机移动延迟 (ms) */
#define GRIPPER_PUMP_DELAY          1000    /* 真空泵启动延迟 (ms) */
#define GRIPPER_RELEASE_DELAY       500     /* 释放延迟 (ms) */
#define GRIPPER_VALVE_DELAY         100     /* 电磁阀操作延迟 (ms) */
```

## 5. 集成步骤

### 5.1 驱动层集成

1. **创建 `src/drivers/drv_gripper.h/c`**
   - 定义夹爪硬件操作接口
   - 实现舵机+真空泵+电磁阀的协调控制
   - 提供原子操作（不可中断）

2. **依赖关系**
   ```
   drv_gripper.c
   ├─ #include "drv_servo.h"
   ├─ #include "drv_vacuum_pump.h"
   └─ #include "ti_msp_dl_config.h"
   ```

### 5.2 应用层集成

1. **创建 `src/app/app_gripper.h/c`**
   - 实现吸取/放下的完整状态机
   - 管理时序和超时
   - 提供任务层接口

2. **依赖关系**
   ```
   app_gripper.c
   ├─ #include "drv_gripper.h"
   ├─ #include "app_vacuum_pump.h"
   └─ #include "src/utils/timer.h"
   ```

### 5.3 任务层调用

```c
/* 在任务中调用 */
void Task_PickAndPlace(void)
{
    /* 初始化 */
    Gripper_App_Init();
    
    /* 吸取物体 */
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    while (Gripper_App_IsBusy()) {
        Gripper_App_Update();
        delay_ms(10);
    }
    
    /* 移动到目标位置 */
    // ... 导航逻辑 ...
    
    /* 放下物体 */
    Gripper_App_SetCommand(GRIPPER_CMD_RELEASE);
    while (Gripper_App_IsBusy()) {
        Gripper_App_Update();
        delay_ms(10);
    }
}
```

## 6. 错误处理与保护

### 6.1 超时保护

```c
#define GRIPPER_SUCTION_TIMEOUT     5000    /* 吸取超时 5秒 */
#define GRIPPER_RELEASE_TIMEOUT     3000    /* 放下超时 3秒 */

/* 超时时自动停止所有操作 */
if (elapsed_time > timeout) {
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
    drv_servo_set_angle(GRIPPER_IDLE_ANGLE);
    state = GRIPPER_STATE_ERROR;
}
```

### 6.2 状态验证

```c
/* 只允许在特定状态下执行操作 */
void Gripper_App_SetCommand(GripperCmd_t cmd)
{
    switch (cmd) {
        case GRIPPER_CMD_SUCTION:
            if (current_state == GRIPPER_STATE_IDLE || 
                current_state == GRIPPER_STATE_HOLDING) {
                /* 允许吸取 */
            }
            break;
        case GRIPPER_CMD_RELEASE:
            if (current_state == GRIPPER_STATE_HOLDING) {
                /* 允许放下 */
            }
            break;
    }
}
```

## 7. 测试验证清单

- [ ] 舵机单独控制测试（0°、90°、180°）
- [ ] 真空泵启停测试
- [ ] 电磁阀打开/关闭测试
- [ ] 吸取流程完整测试
- [ ] 放下流程完整测试
- [ ] 超时保护测试
- [ ] 状态转移正确性测试
- [ ] 时序参数优化测试

## 8. 性能指标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| 吸取周期 | 2.0s | 从待命到物体抬起 |
| 放下周期 | 1.7s | 从抬起到待命 |
| 舵机响应时间 | <100ms | 从命令到开始转动 |
| 真空泵启动时间 | <100ms | 从命令到开始工作 |
| 系统可靠性 | >99% | 连续操作100次成功率 |

## 9. 后续优化方向

1. **传感器反馈**
   - 添加压力传感器检测真空度
   - 添加位置传感器确认舵机到位
   - 实现闭环控制

2. **自适应控制**
   - 根据物体重量调整真空泵启动时间
   - 根据环境温度调整时序参数
   - 动态优化吸取/放下角度

3. **故障诊断**
   - 检测真空泄漏
   - 检测舵机卡顿
   - 记录故障日志用于调试
