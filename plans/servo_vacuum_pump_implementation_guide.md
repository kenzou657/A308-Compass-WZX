# 舵机+真空泵夹爪系统实现指南

## 概述

本文档提供舵机控制真空泵吸嘴系统的详细实现步骤。该系统用于电赛小车的物体搬运任务，通过舵机旋转吸嘴、真空泵吸取、电磁阀释放来完成吸取和放下物体的操作。

---

## 第一部分：驱动层实现

### 1.1 夹爪驱动头文件 (drv_gripper.h)

**文件位置**: `src/drivers/drv_gripper.h`

**核心功能**:
- 定义夹爪硬件操作的原子接口
- 不涉及状态机逻辑，只负责硬件协调
- 提供延迟管理和时序控制

**关键设计点**:
1. 舵机、真空泵、电磁阀的独立控制
2. 时序延迟的灵活配置
3. 原子操作保证（关键段保护）

### 1.2 夹爪驱动实现 (drv_gripper.c)

**文件位置**: `src/drivers/drv_gripper.c`

**核心函数**:

```c
/* 初始化 */
void Gripper_Init(void)
{
    /* 调用底层驱动初始化 */
    drv_servo_init();
    VacuumPump_Init();
    
    /* 设置初始位置 */
    drv_servo_set_angle(GRIPPER_IDLE_ANGLE);
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
}

/* 舵机移动 */
void Gripper_MoveServo(uint16_t angle, uint16_t delay_ms)
{
    drv_servo_set_angle(angle);
    delay_ms(delay_ms);  /* 等待舵机到位 */
}

/* 启动真空泵 */
void Gripper_StartPump(uint16_t delay_ms)
{
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_OPEN);
    delay_ms(delay_ms);  /* 等待真空形成 */
}

/* 停止真空泵 */
void Gripper_StopPump(uint16_t delay_ms)
{
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
    delay_ms(delay_ms);
}

/* 打开电磁阀 */
void Gripper_OpenValve(uint16_t delay_ms)
{
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_OPEN);
    delay_ms(delay_ms);
}

/* 关闭电磁阀 */
void Gripper_CloseValve(uint16_t delay_ms)
{
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
    delay_ms(delay_ms);
}
```

**实现要点**:
- 每个操作后都有延迟，确保硬件有足够时间响应
- 使用 `delay_ms()` 函数（来自 `src/utils/timer.c`）
- 所有操作都是阻塞式的，便于上层状态机控制

---

## 第二部分：应用层实现

### 2.1 夹爪应用头文件 (app_gripper.h)

**文件位置**: `src/app/app_gripper.h`

**核心定义**:

```c
/* 夹爪状态枚举 */
typedef enum {
    GRIPPER_STATE_IDLE = 0,           /* 待命 */
    GRIPPER_STATE_SUCTION_MOVE_DOWN,  /* 吸取-舵机下降 */
    GRIPPER_STATE_SUCTION_PUMP,       /* 吸取-启动泵 */
    GRIPPER_STATE_SUCTION_MOVE_UP,    /* 吸取-舵机上升 */
    GRIPPER_STATE_HOLDING,            /* 物体被吸取 */
    GRIPPER_STATE_RELEASE_MOVE_DOWN,  /* 放下-舵机下降 */
    GRIPPER_STATE_RELEASE_OPEN_VALVE, /* 放下-打开阀 */
    GRIPPER_STATE_RELEASE_STOP_PUMP,  /* 放下-停止泵 */
    GRIPPER_STATE_RELEASE_CLOSE_VALVE,/* 放下-关闭阀 */
    GRIPPER_STATE_RELEASE_MOVE_MID,   /* 放下-回到中位 */
    GRIPPER_STATE_ERROR               /* 错误 */
} GripperState_t;

/* 夹爪命令枚举 */
typedef enum {
    GRIPPER_CMD_IDLE = 0,
    GRIPPER_CMD_SUCTION,
    GRIPPER_CMD_RELEASE
} GripperCmd_t;

/* 公开接口 */
void Gripper_App_Init(void);
void Gripper_App_Update(void);
void Gripper_App_SetCommand(GripperCmd_t cmd);
GripperState_t Gripper_App_GetState(void);
bool Gripper_App_IsBusy(void);
```

### 2.2 夹爪应用实现 (app_gripper.c)

**文件位置**: `src/app/app_gripper.c`

**核心实现**:

```c
/* 全局变量 */
static GripperState_t g_gripper_state = GRIPPER_STATE_IDLE;
static GripperCmd_t g_gripper_cmd = GRIPPER_CMD_IDLE;
static uint32_t g_state_timer = 0;

/* 时序参数 */
static uint16_t g_move_delay = GRIPPER_MOVE_DELAY;
static uint16_t g_pump_delay = GRIPPER_PUMP_DELAY;
static uint16_t g_release_delay = GRIPPER_RELEASE_DELAY;

/* 初始化 */
void Gripper_App_Init(void)
{
    Gripper_Init();  /* 调用驱动层初始化 */
    g_gripper_state = GRIPPER_STATE_IDLE;
    g_gripper_cmd = GRIPPER_CMD_IDLE;
    g_state_timer = 0;
}

/* 周期更新（应在主循环中以10ms周期调用） */
void Gripper_App_Update(void)
{
    g_state_timer += 10;  /* 假设每次调用间隔10ms */
    
    switch (g_gripper_state) {
        case GRIPPER_STATE_IDLE:
            if (g_gripper_cmd == GRIPPER_CMD_SUCTION) {
                /* 开始吸取流程 */
                g_gripper_state = GRIPPER_STATE_SUCTION_MOVE_DOWN;
                g_state_timer = 0;
            }
            break;
            
        case GRIPPER_STATE_SUCTION_MOVE_DOWN:
            if (g_state_timer >= g_move_delay) {
                /* 舵机已到位，启动泵 */
                Gripper_StartPump(0);  /* 不阻塞，立即返回 */
                g_gripper_state = GRIPPER_STATE_SUCTION_PUMP;
                g_state_timer = 0;
            }
            break;
            
        case GRIPPER_STATE_SUCTION_PUMP:
            if (g_state_timer >= g_pump_delay) {
                /* 真空已形成，抬起吸嘴 */
                Gripper_MoveServo(GRIPPER_HOLD_ANGLE, 0);
                g_gripper_state = GRIPPER_STATE_SUCTION_MOVE_UP;
                g_state_timer = 0;
            }
            break;
            
        case GRIPPER_STATE_SUCTION_MOVE_UP:
            if (g_state_timer >= g_move_delay) {
                /* 物体已吸取 */
                g_gripper_state = GRIPPER_STATE_HOLDING;
                g_gripper_cmd = GRIPPER_CMD_IDLE;
            }
            break;
            
        case GRIPPER_STATE_HOLDING:
            if (g_gripper_cmd == GRIPPER_CMD_RELEASE) {
                /* 开始放下流程 */
                Gripper_MoveServo(GRIPPER_RELEASE_ANGLE, 0);
                g_gripper_state = GRIPPER_STATE_RELEASE_MOVE_DOWN;
                g_state_timer = 0;
            }
            break;
            
        case GRIPPER_STATE_RELEASE_MOVE_DOWN:
            if (g_state_timer >= g_move_delay) {
                /* 吸嘴已降低，打开电磁阀 */
                Gripper_OpenValve(0);
                g_gripper_state = GRIPPER_STATE_RELEASE_OPEN_VALVE;
                g_state_timer = 0;
            }
            break;
            
        case GRIPPER_STATE_RELEASE_OPEN_VALVE:
            if (g_state_timer >= g_release_delay) {
                /* 真空已释放，停止泵 */
                Gripper_StopPump(0);
                g_gripper_state = GRIPPER_STATE_RELEASE_STOP_PUMP;
                g_state_timer = 0;
            }
            break;
            
        case GRIPPER_STATE_RELEASE_STOP_PUMP:
            if (g_state_timer >= 100) {  /* 100ms延迟 */
                /* 泵已停止，关闭阀 */
                Gripper_CloseValve(0);
                g_gripper_state = GRIPPER_STATE_RELEASE_CLOSE_VALVE;
                g_state_timer = 0;
            }
            break;
            
        case GRIPPER_STATE_RELEASE_CLOSE_VALVE:
            if (g_state_timer >= 100) {  /* 100ms延迟 */
                /* 阀已关闭，回到中位 */
                Gripper_MoveServo(GRIPPER_IDLE_ANGLE, 0);
                g_gripper_state = GRIPPER_STATE_RELEASE_MOVE_MID;
                g_state_timer = 0;
            }
            break;
            
        case GRIPPER_STATE_RELEASE_MOVE_MID:
            if (g_state_timer >= g_move_delay) {
                /* 放下完成 */
                g_gripper_state = GRIPPER_STATE_IDLE;
                g_gripper_cmd = GRIPPER_CMD_IDLE;
            }
            break;
            
        case GRIPPER_STATE_ERROR:
            /* 错误状态，停止所有操作 */
            Gripper_StopPump(0);
            Gripper_CloseValve(0);
            Gripper_MoveServo(GRIPPER_IDLE_ANGLE, 0);
            break;
    }
}

/* 设置命令 */
void Gripper_App_SetCommand(GripperCmd_t cmd)
{
    g_gripper_cmd = cmd;
}

/* 获取状态 */
GripperState_t Gripper_App_GetState(void)
{
    return g_gripper_state;
}

/* 判断是否忙碌 */
bool Gripper_App_IsBusy(void)
{
    return (g_gripper_state != GRIPPER_STATE_IDLE && 
            g_gripper_state != GRIPPER_STATE_HOLDING &&
            g_gripper_state != GRIPPER_STATE_ERROR);
}
```

**实现要点**:
- 使用非阻塞式状态机，每次 `Update()` 调用只处理一个状态转移
- 使用计时器 `g_state_timer` 管理状态停留时间
- 驱动层函数调用时传入 `0` 延迟，避免阻塞主循环
- 状态转移清晰，便于调试和扩展

---

## 第三部分：任务层集成

### 3.1 在任务中使用夹爪

```c
/* 示例：吸取物体 */
void Task_PickObject(void)
{
    /* 初始化 */
    Gripper_App_Init();
    
    /* 发送吸取命令 */
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    
    /* 等待吸取完成 */
    while (Gripper_App_IsBusy()) {
        Gripper_App_Update();
        delay_ms(10);  /* 10ms周期调用 */
    }
    
    /* 检查是否成功 */
    if (Gripper_App_GetState() == GRIPPER_STATE_HOLDING) {
        /* 吸取成功，可以进行下一步 */
        // ... 导航到目标位置 ...
    } else {
        /* 吸取失败，处理错误 */
        // ... 错误处理 ...
    }
}

/* 示例：放下物体 */
void Task_ReleaseObject(void)
{
    /* 发送放下命令 */
    Gripper_App_SetCommand(GRIPPER_CMD_RELEASE);
    
    /* 等待放下完成 */
    while (Gripper_App_IsBusy()) {
        Gripper_App_Update();
        delay_ms(10);
    }
    
    /* 放下完成 */
    if (Gripper_App_GetState() == GRIPPER_STATE_IDLE) {
        // ... 继续下一个任务 ...
    }
}
```

### 3.2 在主循环中集成

```c
/* 主循环示例 */
int main(void)
{
    SYSCFG_DL_init();
    
    /* 初始化所有模块 */
    Gripper_App_Init();
    // ... 其他模块初始化 ...
    
    while (1) {
        /* 周期调用夹爪更新 */
        Gripper_App_Update();
        
        /* 其他任务处理 */
        // ... 导航、传感器处理等 ...
        
        delay_ms(10);  /* 10ms主循环周期 */
    }
}
```

---

## 第四部分：参数配置

### 4.1 时序参数调优

**吸取流程时序**:
```
初始状态 (0ms)
    ↓
舵机下降 (0-500ms)
    ↓
启动泵 (500-1500ms)
    ↓
舵机上升 (1500-2000ms)
    ↓
完成 (2000ms)
```

**调优建议**:
- 如果舵机转速快，可减少 `GRIPPER_MOVE_DELAY`
- 如果真空形成慢，可增加 `GRIPPER_PUMP_DELAY`
- 根据实际物体重量调整参数

### 4.2 角度配置

```c
/* 根据实际机械结构调整 */
#define GRIPPER_SUCTION_ANGLE       0       /* 吸取位置 */
#define GRIPPER_RELEASE_ANGLE       0       /* 放下位置 */
#define GRIPPER_HOLD_ANGLE          180     /* 抬起位置 */
#define GRIPPER_IDLE_ANGLE          90      /* 待命位置 */
```

**调优方法**:
1. 手动旋转舵机找到最佳吸取角度
2. 记录对应的角度值
3. 更新配置常量

---

## 第五部分：调试与测试

### 5.1 单元测试

```c
/* 测试舵机 */
void Test_Servo(void)
{
    drv_servo_init();
    drv_servo_set_angle(0);
    delay_ms(500);
    drv_servo_set_angle(90);
    delay_ms(500);
    drv_servo_set_angle(180);
    delay_ms(500);
}

/* 测试真空泵 */
void Test_VacuumPump(void)
{
    VacuumPump_Init();
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_OPEN);
    delay_ms(1000);
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
}

/* 测试完整流程 */
void Test_GripperSuction(void)
{
    Gripper_App_Init();
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    
    while (Gripper_App_IsBusy()) {
        Gripper_App_Update();
        delay_ms(10);
    }
    
    if (Gripper_App_GetState() == GRIPPER_STATE_HOLDING) {
        // 成功
    }
}
```

### 5.2 调试输出

```c
/* 在 Gripper_App_Update() 中添加调试信息 */
void Gripper_App_Update(void)
{
    // ... 状态机逻辑 ...
    
    #ifdef DEBUG_GRIPPER
    printf("State: %d, Timer: %d\r\n", g_gripper_state, g_state_timer);
    #endif
}
```

---

## 第六部分：常见问题

### Q1: 吸取失败，物体掉落
**原因**: 真空泵启动延迟不足
**解决**: 增加 `GRIPPER_PUMP_DELAY` 值

### Q2: 舵机转动不到位
**原因**: 舵机移动延迟不足
**解决**: 增加 `GRIPPER_MOVE_DELAY` 值

### Q3: 放下时物体不释放
**原因**: 电磁阀打开延迟不足
**解决**: 增加 `GRIPPER_RELEASE_DELAY` 值

### Q4: 系统卡死
**原因**: 状态机陷入死循环
**解决**: 添加超时保护，自动回到 IDLE 状态

---

## 总结

该实现方案采用分层设计：
- **驱动层**: 提供硬件操作的原子接口
- **应用层**: 实现完整的状态机逻辑
- **任务层**: 调用应用层接口完成业务逻辑

通过这种设计，系统具有良好的可维护性、可扩展性和可测试性。
