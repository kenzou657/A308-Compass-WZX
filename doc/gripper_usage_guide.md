# 任务函数中使用舵机控制真空泵夹爪指南

## 概述

本文档说明如何在任务函数中使用夹爪系统实现物体的吸取和放下操作。夹爪系统已集成到 [`empty.c`](empty.c) 的主循环中，任务函数只需调用应用层接口即可。

---

## 第一部分：基础概念

### 1.1 夹爪系统架构

```
任务函数 (Task Function)
    ↓
应用层 (app_gripper.c)
    ├─ Gripper_App_SetCommand()    // 发送命令
    ├─ Gripper_App_Update()        // 状态机更新（在主循环中调用）
    ├─ Gripper_App_GetState()      // 查询状态
    └─ Gripper_App_IsBusy()        // 判断忙碌
    ↓
驱动层 (drv_gripper.c)
    ├─ Gripper_MoveServo()         // 舵机控制
    ├─ Gripper_StartPump()         // 启动泵
    ├─ Gripper_StopPump()          // 停止泵
    ├─ Gripper_OpenValve()         // 打开阀
    └─ Gripper_CloseValve()        // 关闭阀
    ↓
硬件 (TIMA1, TIMG8)
```

### 1.2 状态机流程

**吸取流程**:
```
IDLE → SUCTION_MOVE_DOWN → SUCTION_PUMP → SUCTION_MOVE_UP → HOLDING
```

**放下流程**:
```
HOLDING → RELEASE_MOVE_DOWN → RELEASE_OPEN_VALVE → RELEASE_STOP_PUMP 
→ RELEASE_CLOSE_VALVE → RELEASE_MOVE_MID → IDLE
```

### 1.3 关键特性

- **非阻塞式**: 状态机在主循环中自动更新，不阻塞任务执行
- **异步操作**: 任务发送命令后可继续执行其他逻辑
- **状态查询**: 任务可随时查询夹爪状态
- **时序自动管理**: 所有延迟由状态机自动处理

---

## 第二部分：基础使用

### 2.1 最简单的吸取操作

```c
#include "app_gripper.h"

void Task_SimplePickup(void)
{
    /* 发送吸取命令 */
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    
    /* 等待吸取完成 */
    while (Gripper_App_IsBusy()) {
        delay_ms(10);  /* 让出CPU时间 */
    }
    
    /* 吸取完成，物体已被吸起 */
    if (Gripper_App_GetState() == GRIPPER_STATE_HOLDING) {
        // 可以进行下一步操作
    }
}
```

**说明**:
- `Gripper_App_SetCommand()` 发送命令，立即返回
- `Gripper_App_IsBusy()` 判断是否还在执行操作
- 主循环中的 `Gripper_App_Update()` 自动驱动状态转移
- 约2秒后吸取完成

### 2.2 最简单的放下操作

```c
void Task_SimpleRelease(void)
{
    /* 发送放下命令 */
    Gripper_App_SetCommand(GRIPPER_CMD_RELEASE);
    
    /* 等待放下完成 */
    while (Gripper_App_IsBusy()) {
        delay_ms(10);
    }
    
    /* 放下完成，系统回到待命状态 */
    if (Gripper_App_GetState() == GRIPPER_STATE_IDLE) {
        // 可以进行下一步操作
    }
}
```

---

## 第三部分：完整的吸取-放下流程

### 3.1 标准流程

```c
void Task_PickAndPlace(void)
{
    /* 步骤1：吸取物体 */
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    while (Gripper_App_IsBusy()) {
        delay_ms(10);
    }
    
    if (Gripper_App_GetState() != GRIPPER_STATE_HOLDING) {
        // 吸取失败，处理错误
        return;
    }
    
    /* 步骤2：移动到目标位置 */
    // ... 导航逻辑 ...
    // 例如：沿着寻线路径移动到目标位置
    
    /* 步骤3：放下物体 */
    Gripper_App_SetCommand(GRIPPER_CMD_RELEASE);
    while (Gripper_App_IsBusy()) {
        delay_ms(10);
    }
    
    if (Gripper_App_GetState() == GRIPPER_STATE_IDLE) {
        // 放下成功
    }
}
```

### 3.2 带超时保护的流程

```c
void Task_PickAndPlace_WithTimeout(void)
{
    uint32_t timeout_ms = 5000;  /* 5秒超时 */
    uint32_t start_time = uwTick;
    
    /* 吸取物体 */
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    
    while (Gripper_App_IsBusy()) {
        if ((uwTick - start_time) > timeout_ms) {
            /* 超时，停止操作 */
            Gripper_App_SetCommand(GRIPPER_CMD_IDLE);
            return;  /* 吸取失败 */
        }
        delay_ms(10);
    }
    
    /* 吸取成功，继续放下 */
    start_time = uwTick;
    Gripper_App_SetCommand(GRIPPER_CMD_RELEASE);
    
    while (Gripper_App_IsBusy()) {
        if ((uwTick - start_time) > timeout_ms) {
            /* 超时 */
            Gripper_App_SetCommand(GRIPPER_CMD_IDLE);
            return;
        }
        delay_ms(10);
    }
}
```

---

## 第四部分：高级用法

### 4.1 非阻塞式操作（推荐）

在任务中不阻塞等待，而是在主循环中检查状态：

```c
/* 任务状态机 */
typedef enum {
    TASK_STATE_IDLE,
    TASK_STATE_PICKING,
    TASK_STATE_MOVING,
    TASK_STATE_RELEASING,
    TASK_STATE_DONE
} TaskState_t;

static TaskState_t g_task_state = TASK_STATE_IDLE;

void Task_PickAndPlace_NonBlocking(void)
{
    switch (g_task_state) {
        case TASK_STATE_IDLE:
            /* 发送吸取命令 */
            Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
            g_task_state = TASK_STATE_PICKING;
            break;
        
        case TASK_STATE_PICKING:
            /* 等待吸取完成 */
            if (!Gripper_App_IsBusy()) {
                if (Gripper_App_GetState() == GRIPPER_STATE_HOLDING) {
                    /* 吸取成功，开始移动 */
                    g_task_state = TASK_STATE_MOVING;
                } else {
                    /* 吸取失败 */
                    g_task_state = TASK_STATE_DONE;
                }
            }
            break;
        
        case TASK_STATE_MOVING:
            /* 移动到目标位置 */
            // ... 导航逻辑 ...
            if (/* 到达目标 */) {
                /* 发送放下命令 */
                Gripper_App_SetCommand(GRIPPER_CMD_RELEASE);
                g_task_state = TASK_STATE_RELEASING;
            }
            break;
        
        case TASK_STATE_RELEASING:
            /* 等待放下完成 */
            if (!Gripper_App_IsBusy()) {
                g_task_state = TASK_STATE_DONE;
            }
            break;
        
        case TASK_STATE_DONE:
            /* 任务完成 */
            break;
    }
}
```

**优点**:
- 不阻塞主循环
- 可以同时处理其他任务
- 更好的系统响应性

### 4.2 多次吸取-放下操作

```c
void Task_MultiplePickAndPlace(void)
{
    uint8_t pick_count = 3;  /* 吸取3个物体 */
    
    for (uint8_t i = 0; i < pick_count; i++) {
        /* 吸取 */
        Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
        while (Gripper_App_IsBusy()) {
            delay_ms(10);
        }
        
        if (Gripper_App_GetState() != GRIPPER_STATE_HOLDING) {
            break;  /* 吸取失败，退出 */
        }
        
        /* 移动到目标位置 */
        // ... 导航逻辑 ...
        
        /* 放下 */
        Gripper_App_SetCommand(GRIPPER_CMD_RELEASE);
        while (Gripper_App_IsBusy()) {
            delay_ms(10);
        }
        
        /* 返回起点 */
        // ... 导航逻辑 ...
    }
}
```

### 4.3 时序参数调优

```c
void Task_PickAndPlace_Optimized(void)
{
    /* 根据实际情况调整时序参数 */
    Gripper_App_SetTimings(
        400,    /* 舵机移动延迟：400ms */
        800,    /* 真空泵启动延迟：800ms */
        400     /* 释放延迟：400ms */
    );
    
    /* 执行吸取-放下 */
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    while (Gripper_App_IsBusy()) {
        delay_ms(10);
    }
    
    // ... 其他逻辑 ...
}
```

---

## 第五部分：错误处理

### 5.1 检查吸取是否成功

```c
void Task_PickWithErrorCheck(void)
{
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    
    while (Gripper_App_IsBusy()) {
        delay_ms(10);
    }
    
    /* 检查最终状态 */
    GripperState_t state = Gripper_App_GetState();
    
    if (state == GRIPPER_STATE_HOLDING) {
        /* 吸取成功 */
        // 继续操作
    } else if (state == GRIPPER_STATE_ERROR) {
        /* 发生错误 */
        // 处理错误
    } else {
        /* 未知状态 */
        // 处理异常
    }
}
```

### 5.2 超时保护

```c
bool Task_PickWithTimeout(uint32_t timeout_ms)
{
    uint32_t start_time = uwTick;
    
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    
    while (Gripper_App_IsBusy()) {
        if ((uwTick - start_time) > timeout_ms) {
            /* 超时 */
            Gripper_App_SetCommand(GRIPPER_CMD_IDLE);
            return false;
        }
        delay_ms(10);
    }
    
    return (Gripper_App_GetState() == GRIPPER_STATE_HOLDING);
}
```

---

## 第六部分：集成到现有任务

### 6.1 在寻线任务中添加吸取

```c
void Task_LineTrackingWithPickup(void)
{
    /* 寻线移动到物体位置 */
    while (/* 未到达物体 */) {
        // 寻线逻辑
    }
    
    /* 到达物体，执行吸取 */
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    while (Gripper_App_IsBusy()) {
        delay_ms(10);
    }
    
    if (Gripper_App_GetState() != GRIPPER_STATE_HOLDING) {
        return;  /* 吸取失败 */
    }
    
    /* 寻线移动到目标位置 */
    while (/* 未到达目标 */) {
        // 寻线逻辑
    }
    
    /* 放下物体 */
    Gripper_App_SetCommand(GRIPPER_CMD_RELEASE);
    while (Gripper_App_IsBusy()) {
        delay_ms(10);
    }
}
```

### 6.2 在数字识别任务中添加吸取

```c
void Task_DigitRecognitionWithPickup(void)
{
    /* 识别数字 */
    uint8_t digit = camera_get_digit();
    
    /* 根据数字决定是否吸取 */
    if (digit >= 5) {
        /* 吸取 */
        Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
        while (Gripper_App_IsBusy()) {
            delay_ms(10);
        }
        
        if (Gripper_App_GetState() == GRIPPER_STATE_HOLDING) {
            /* 吸取成功，执行相应操作 */
        }
    }
}
```

---

## 第七部分：调试技巧

### 7.1 打印状态信息

```c
void Task_PickAndPlace_Debug(void)
{
    printf("开始吸取...\r\n");
    Gripper_App_SetCommand(GRIPPER_CMD_SUCTION);
    
    while (Gripper_App_IsBusy()) {
        GripperState_t state = Gripper_App_GetState();
        printf("当前状态: %d\r\n", state);
        delay_ms(100);
    }
    
    printf("吸取完成，状态: %d\r\n", Gripper_App_GetState());
}
```

### 7.2 OLED显示状态

```c
void Display_GripperStatus(void)
{
    GripperState_t state = Gripper_App_GetState();
    uint8_t buf[32];
    
    switch (state) {
        case GRIPPER_STATE_IDLE:
            sprintf((char *)buf, "Gripper: IDLE");
            break;
        case GRIPPER_STATE_HOLDING:
            sprintf((char *)buf, "Gripper: HOLDING");
            break;
        case GRIPPER_STATE_SUCTION_PUMP:
            sprintf((char *)buf, "Gripper: PUMPING");
            break;
        default:
            sprintf((char *)buf, "Gripper: STATE %d", state);
            break;
    }
    
    OLED_ShowString(4, 0, buf, 12, 1);
    OLED_Refresh();
}
```

---

## 第八部分：常见问题

### Q1: 如何在任务中等待夹爪完成？

**A**: 使用 `Gripper_App_IsBusy()` 判断：
```c
while (Gripper_App_IsBusy()) {
    delay_ms(10);
}
```

### Q2: 如何调整吸取速度？

**A**: 使用 `Gripper_App_SetTimings()` 调整时序参数：
```c
Gripper_App_SetTimings(400, 800, 400);  /* 加快速度 */
```

### Q3: 如何检查吸取是否成功？

**A**: 检查最终状态：
```c
if (Gripper_App_GetState() == GRIPPER_STATE_HOLDING) {
    /* 吸取成功 */
}
```

### Q4: 夹爪可以中途停止吗？

**A**: 可以，发送 `GRIPPER_CMD_IDLE` 命令：
```c
Gripper_App_SetCommand(GRIPPER_CMD_IDLE);
```

### Q5: 如何实现非阻塞式操作？

**A**: 在任务中使用状态机，而不是 `while` 循环等待。参考第四部分的示例。

---

## 总结

使用夹爪系统的三个关键步骤：

1. **发送命令**: `Gripper_App_SetCommand(GRIPPER_CMD_SUCTION/RELEASE)`
2. **等待完成**: `while (Gripper_App_IsBusy()) { delay_ms(10); }`
3. **检查状态**: `if (Gripper_App_GetState() == GRIPPER_STATE_HOLDING) { ... }`

主循环中的 `Gripper_App_Update()` 自动驱动状态机，任务只需调用应用层接口即可。
