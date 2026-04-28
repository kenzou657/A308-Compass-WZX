# 真空泵吸取系统集成使用指南

## 1. 模块文件清单

### 驱动层
- [`src/drivers/drv_vacuum_pump.h`](src/drivers/drv_vacuum_pump.h) - 驱动头文件
- [`src/drivers/drv_vacuum_pump.c`](src/drivers/drv_vacuum_pump.c) - 驱动实现

### 应用层
- [`src/app/app_vacuum_pump.h`](src/app/app_vacuum_pump.h) - 应用头文件
- [`src/app/app_vacuum_pump.c`](src/app/app_vacuum_pump.c) - 应用实现

### 主程序
- [`empty.c`](empty.c) - 已集成真空泵初始化和更新

## 2. 快速集成步骤

### Step 1: 验证 SysConfig 配置
确保 TIMG8 已正确配置：
- TIMG8-CH0 (PA26)：真空泵 PWM
- TIMG8-CH1 (PA30)：电磁阀 PWM
- 频率：50Hz，周期：20ms，计数值：400

### Step 2: 编译验证
```bash
# 在 Keil 中编译项目
# 确保没有编译错误
```

### Step 3: 在任务中使用

在任务文件（如 `app_task_5_recognize_transport.c`）中添加真空泵控制：

```c
#include "app_vacuum_pump.h"

// 在任务执行函数中
void Task_Execute(void)
{
    // ... 其他任务逻辑 ...
    
    // 吸取物品
    if (需要吸物) {
        VacuumPump_App_SetCommand(VACUUM_CMD_SUCTION);
    }
    
    // 查询吸取状态
    if (VacuumPump_App_GetState() == VACUUM_STATE_SUCKING) {
        // 物品已吸取，可以移动小车
        // ... 移动逻辑 ...
    }
    
    // 释放物品
    if (需要释放) {
        VacuumPump_App_SetCommand(VACUUM_CMD_RELEASE);
    }
    
    // 查询释放状态
    if (VacuumPump_App_GetState() == VACUUM_STATE_IDLE) {
        // 物品已释放，可以继续下一步
        // ... 下一步逻辑 ...
    }
}
```

## 3. API 参考

### 驱动层 API

#### VacuumPump_Init()
初始化真空泵驱动模块。
```c
void VacuumPump_Init(void);
```

#### VacuumPump_SetPulse(uint16_t pulse)
设置真空泵 PWM 脉宽。
```c
void VacuumPump_SetPulse(uint16_t pulse);
// pulse: 10 (关闭) ~ 50 (打开)
```

#### VacuumPump_GetPulse()
获取真空泵当前脉宽。
```c
uint16_t VacuumPump_GetPulse(void);
```

#### SolenoidValve_SetPulse(uint16_t pulse)
设置电磁阀 PWM 脉宽。
```c
void SolenoidValve_SetPulse(uint16_t pulse);
// pulse: 10 (关闭) ~ 50 (打开)
```

#### SolenoidValve_GetPulse()
获取电磁阀当前脉宽。
```c
uint16_t SolenoidValve_GetPulse(void);
```

### 应用层 API

#### VacuumPump_App_Init()
初始化真空泵应用模块。
```c
void VacuumPump_App_Init(void);
```

#### VacuumPump_App_Update()
周期更新真空泵状态机（在主循环中调用）。
```c
void VacuumPump_App_Update(void);
```

#### VacuumPump_App_SetCommand(VacuumPumpCmd_t cmd)
设置真空泵控制指令。
```c
void VacuumPump_App_SetCommand(VacuumPumpCmd_t cmd);
// cmd: VACUUM_CMD_IDLE (待命)
//      VACUUM_CMD_SUCTION (吸物)
//      VACUUM_CMD_RELEASE (释放)
```

#### VacuumPump_App_GetState()
获取真空泵当前状态。
```c
VacuumPumpState_t VacuumPump_App_GetState(void);
// 返回值：
//   VACUUM_STATE_IDLE (待命)
//   VACUUM_STATE_SUCTION_CLOSE_VALVE (关闭电磁阀)
//   VACUUM_STATE_SUCTION_START_PUMP (启动真空泵)
//   VACUUM_STATE_SUCKING (吸物中)
//   VACUUM_STATE_RELEASE_STOP_PUMP (停止真空泵)
//   VACUUM_STATE_RELEASE_OPEN_VALVE (打开电磁阀)
//   VACUUM_STATE_RELEASING (卸物中)
```

## 4. 工作流程示例

### 完整的吸取-移动-释放流程

```c
#include "app_vacuum_pump.h"

typedef enum {
    TASK_STATE_IDLE,
    TASK_STATE_SUCTION,
    TASK_STATE_MOVING,
    TASK_STATE_RELEASE,
    TASK_STATE_DONE
} TaskState_t;

static TaskState_t g_task_state = TASK_STATE_IDLE;

void Task_Execute(void)
{
    switch (g_task_state) {
        case TASK_STATE_IDLE:
            // 初始状态，准备吸取
            VacuumPump_App_SetCommand(VACUUM_CMD_SUCTION);
            g_task_state = TASK_STATE_SUCTION;
            break;
            
        case TASK_STATE_SUCTION:
            // 等待吸取完成
            if (VacuumPump_App_GetState() == VACUUM_STATE_SUCKING) {
                // 物品已吸取，开始移动
                g_task_state = TASK_STATE_MOVING;
            }
            break;
            
        case TASK_STATE_MOVING:
            // 移动小车到目标位置
            if (小车到达目标位置) {
                // 触发释放
                VacuumPump_App_SetCommand(VACUUM_CMD_RELEASE);
                g_task_state = TASK_STATE_RELEASE;
            }
            break;
            
        case TASK_STATE_RELEASE:
            // 等待释放完成
            if (VacuumPump_App_GetState() == VACUUM_STATE_IDLE) {
                // 物品已释放，任务完成
                g_task_state = TASK_STATE_DONE;
            }
            break;
            
        case TASK_STATE_DONE:
            // 任务完成
            break;
    }
}
```

## 5. 时序参数

| 参数 | 值 | 说明 |
|------|-----|------|
| SUCTION_CLOSE_VALVE_TIME | 100ms | 关闭电磁阀时间 |
| SUCTION_START_PUMP_TIME | 100ms | 启动真空泵时间 |
| SUCTION_MAX_TIME | 60秒 | 吸物最长时间 |
| RELEASE_STOP_PUMP_TIME | 100ms | 停止真空泵时间 |
| RELEASE_OPEN_VALVE_TIME | 2秒 | 打开电磁阀时间 |
| RELEASE_MAX_TIME | 10秒 | 卸物最长时间 |

## 6. PWM 脉宽值

| 脉宽值 | 时间 | 占空比 | 状态 |
|--------|------|--------|------|
| 10 | 0.5ms | 2.5% | 关闭 |
| 50 | 2.5ms | 12.5% | 打开 |

## 7. 调试建议

### UART 日志输出

在 `app_vacuum_pump.c` 中添加日志输出：

```c
#include "drv_uart.h"

void VacuumPump_App_Update(void)
{
    // ... 状态机代码 ...
    
    // 在状态转移时输出日志
    static VacuumPumpState_t last_state = VACUUM_STATE_IDLE;
    if (g_vacuum_state != last_state) {
        printf("真空泵状态转移: %d -> %d\n", last_state, g_vacuum_state);
        last_state = g_vacuum_state;
    }
}
```

### OLED 显示状态

在 OLED 显示函数中添加真空泵状态显示：

```c
void OLED_ShowVacuumState(void)
{
    VacuumPumpState_t state = VacuumPump_App_GetState();
    const char *state_str[] = {
        "IDLE",
        "CLOSE_VALVE",
        "START_PUMP",
        "SUCKING",
        "STOP_PUMP",
        "OPEN_VALVE",
        "RELEASING"
    };
    
    OLED_ShowString(0, 48, (uint8_t *)state_str[state], 12, 1);
    OLED_Refresh();
}
```

### 逻辑分析仪观察

使用逻辑分析仪观察 PA26 和 PA30 的 PWM 波形：
- PA26 (真空泵)：应在吸物时输出 2.5ms 脉宽
- PA30 (电磁阀)：应在释放时输出 2.5ms 脉宽

## 8. 常见问题

### Q: 真空泵不工作
A: 检查以下几点：
1. 确认 TIMG8 已在 SysConfig 中正确配置
2. 验证 PA26 和 PA30 的 GPIO 映射
3. 使用逻辑分析仪检查 PWM 波形
4. 检查脉宽值是否正确（应为 50）

### Q: 状态转移不正常
A: 检查以下几点：
1. 确认 `VacuumPump_App_Update()` 在主循环中被调用
2. 检查时间戳函数 `get_tick()` 是否正常工作
3. 添加 UART 日志输出调试状态转移

### Q: 物品吸不住
A: 检查以下几点：
1. 真空泵是否正常工作
2. 电磁阀是否正确关闭
3. 吸取时间是否足够（SUCTION_MAX_TIME）
4. 检查真空泵和电磁阀的硬件连接

## 9. 性能优化

### 减少浮点运算
所有时序计算都使用整数，无浮点运算。

### 最小化中断
真空泵控制不使用中断，完全在主循环中处理。

### 内存占用
全局变量占用约 20 字节内存。

## 10. 扩展功能

### 添加压力传感器反馈
可在 `VACUUM_STATE_SUCKING` 状态中添加压力检测：

```c
case VACUUM_STATE_SUCKING:
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_OPEN);
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
    
    // 检查压力传感器
    if (Get_Pressure() < PRESSURE_THRESHOLD) {
        // 压力不足，物品可能未吸住
        // 可选：自动停止或报警
    }
    break;
```

### 添加故障检测
可在状态转移时添加故障检测逻辑。

### 添加手动控制
可在按键逻辑中添加手动吸取/释放功能。
