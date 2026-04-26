# 赛题任务设计方案（修正版）

## 赛题分析

根据赛题要求，共有**4个基本任务**和**4个发挥任务**，总计8个任务。

### 基本任务（4个）
1. **Task 1**：单点循迹 - 从停车区循迹到指定存放区(1~5)并停止
2. **Task 2**：双点循迹 - 循迹到目的地1停靠2秒，再循迹到目的地2并停止
3. **Task 3**：双点往返 - 在Task 2基础上返回停车区，总时间30±2s
4. **Task 4**：数字识别 - 识别停车区的数字标签(1~5)，3s内识别并提示

### 发挥任务（4个）
5. **Task 5**：识别+搬运 - 识别数字后搬运对应物品回停车区
6. **Task 6**：自主搬运 - 识别数字后自主找到物品并搬运回停车区
7. **Task 7**：全自动搬运 - 自主将物品按顺序搬运至对应存放区
8. **Task 8**：其他创意任务

## 硬件约束

- **按键数量**：3个（KEY1、KEY2、KEY3）
- **显示方式**：OLED屏幕
- **状态提示**：LED指示灯（无蜂鸣器）

## 按键交互设计（3按键方案）

### 按键功能

| 按键 | 功能 | 说明 |
|------|------|------|
| KEY1 | 上一个任务 | 在OLED上显示任务列表，向上选择 |
| KEY2 | 下一个任务 | 在OLED上显示任务列表，向下选择 |
| KEY3 | 启动/停止 | 启动选中的任务，再按停止任务 |

### 交互流程

```
初始化
  ↓
显示任务列表（OLED）
  ↓
KEY1/KEY2选择任务
  ↓
KEY3启动任务
  ↓
任务运行中（OLED显示进度）
  ↓
任务完成或KEY3停止
  ↓
显示结果（OLED）
  ↓
返回任务列表
```

## 任务架构设计

```
┌─────────────────────────────────────────────────────┐
│              主应用层 (main.c)                       │
│  - 按键处理：选择/启动任务                           │
│  - 任务切换：启动/停止任务                           │
│  - OLED显示：任务列表、进度、结果                    │
└────────────┬────────────────────────────────────────┘
             │
┌────────────▼────────────────────────────────────────┐
│         任务管理器 (app_task_manager.c)              │
│  - 任务枚举和状态管理                                │
│  - 任务切换逻辑                                      │
│  - 任务执行调度                                      │
└────────────┬────────────────────────────────────────┘
             │
    ┌────────┴────────┬────────────┬────────────┐
    │                 │            │            │
┌───▼──┐  ┌──────┐  ┌─▼──┐  ┌────▼─┐  ┌─────▼──┐
│Task1 │  │Task2 │  │Task3│  │Task4 │  │Task5~8 │
│循迹  │  │双点  │  │往返 │  │识别  │  │发挥    │
└──────┘  └──────┘  └─────┘  └──────┘  └────────┘
```

## 任务文件结构

```
src/app/
├── app_task_manager.h          # 任务管理器头文件
├── app_task_manager.c          # 任务管理器实现
├── app_ui_display.h            # OLED显示管理
├── app_ui_display.c
├── app_key_handler.h           # 按键处理
├── app_key_handler.c
├── app_task_1_line_tracking.h  # Task 1：单点循迹
├── app_task_1_line_tracking.c
├── app_task_2_dual_point.h     # Task 2：双点循迹
├── app_task_2_dual_point.c
├── app_task_3_round_trip.h     # Task 3：双点往返
├── app_task_3_round_trip.c
├── app_task_4_digit_recognition.h  # Task 4：数字识别
├── app_task_4_digit_recognition.c
├── app_task_5_recognize_transport.h # Task 5：识别+搬运
├── app_task_5_recognize_transport.c
├── app_task_6_auto_transport.h  # Task 6：自主搬运
├── app_task_6_auto_transport.c
├── app_task_7_batch_transport.h # Task 7：全自动搬运
├── app_task_7_batch_transport.c
├── app_task_8_creative.h        # Task 8：创意任务
└── app_task_8_creative.c
```

## 任务状态机设计

每个任务内部使用状态机完成状态切换。

### 通用状态定义

```c
typedef enum {
    TASK_STATE_IDLE,           // 空闲
    TASK_STATE_INIT,           // 初始化
    TASK_STATE_RUNNING,        // 运行中
    TASK_STATE_PAUSED,         // 暂停
    TASK_STATE_SUCCESS,        // 成功完成
    TASK_STATE_FAILED,         // 失败
    TASK_STATE_TIMEOUT,        // 超时
} TaskState_t;
```

### Task 1 状态机（单点循迹）

```
IDLE → INIT → RUNNING → SUCCESS/FAILED
       ↑                    ↓
       └────────────────────┘
```

**状态转移：**
- IDLE → INIT：KEY3启动
- INIT → RUNNING：初始化完成，开始循迹
- RUNNING → SUCCESS：到达目标区域
- RUNNING → FAILED：循迹失败或超时
- SUCCESS/FAILED → IDLE：KEY3停止或自动返回

### Task 2 状态机（双点循迹）

```
IDLE → INIT → RUNNING_P1 → WAIT_2S → RUNNING_P2 → SUCCESS/FAILED
       ↑                                              ↓
       └──────────────────────────────────────────────┘
```

### Task 3 状态机（双点往返）

```
IDLE → INIT → RUNNING_P1 → WAIT_2S → RUNNING_P2 → RUNNING_HOME → SUCCESS/FAILED
       ↑                                                            ↓
       └────────────────────────────────────────────────────────────┘
```

### Task 4 状态机（数字识别）

```
IDLE → INIT → RECOGNIZING → RECOGNIZED → SUCCESS/FAILED
       ↑                                      ↓
       └──────────────────────────────────────┘
```

## OLED显示设计

### 显示界面1：任务列表

```
┌─────────────────────────┐
│   任务选择 (3/8)        │
├─────────────────────────┤
│ > Task 3: 双点往返      │  ← 当前选中
│   Task 4: 数字识别      │
│   Task 5: 识别+搬运     │
├─────────────────────────┤
│ KEY1/KEY2选择 KEY3启动  │
└─────────────────────────┘
```

### 显示界面2：任务运行中

```
┌─────────────────────────┐
│ Task 1: 单点循迹        │
├─────────────────────────┤
│ 状态: 运行中            │
│ 进度: 循迹中...         │
│ 时间: 5.2s              │
│ 速度: 200mm/s           │
├─────────────────────────┤
│ KEY3停止                │
└─────────────────────────┘
```

### 显示界面3：任务完成

```
┌─────────────────────────┐
│ Task 1: 单点循迹        │
├─────────────────────────┤
│ 状态: ✓ 成功            │
│ 用时: 8.5s              │
│ 得分: 10分              │
├─────────────────────────┤
│ KEY3返回列表            │
└─────────────────────────┘
```

### 显示界面4：任务失败

```
┌─────────────────────────┐
│ Task 1: 单点循迹        │
├─────────────────────────┤
│ 状态: ✗ 失败            │
│ 原因: 循迹超时          │
│ 用时: 30.0s             │
│ 得分: 0分               │
├─────────────────────────┤
│ KEY3返回列表            │
└─────────────────────────┘
```

## 按键处理设计

### 按键处理器接口

```c
typedef enum {
    KEY_EVENT_NONE,
    KEY_EVENT_KEY1_PRESSED,
    KEY_EVENT_KEY2_PRESSED,
    KEY_EVENT_KEY3_PRESSED,
} KeyEvent_t;

// 初始化按键处理
void KeyHandler_Init(void);

// 扫描按键
KeyEvent_t KeyHandler_Scan(void);

// 获取按键事件
KeyEvent_t KeyHandler_GetEvent(void);
```

### 按键事件处理流程

```c
KeyEvent_t event = KeyHandler_Scan();

switch (event) {
    case KEY_EVENT_KEY1_PRESSED:
        // 上一个任务
        TaskManager_PrevTask();
        UIDisplay_UpdateTaskList();
        break;
        
    case KEY_EVENT_KEY2_PRESSED:
        // 下一个任务
        TaskManager_NextTask();
        UIDisplay_UpdateTaskList();
        break;
        
    case KEY_EVENT_KEY3_PRESSED:
        // 启动/停止任务
        if (TaskManager_IsRunning()) {
            TaskManager_StopTask();
        } else {
            TaskManager_StartTask();
        }
        break;
        
    default:
        break;
}
```

## OLED显示管理器

### 显示管理器接口

```c
// 初始化显示
void UIDisplay_Init(void);

// 显示任务列表
void UIDisplay_ShowTaskList(void);

// 显示任务运行界面
void UIDisplay_ShowTaskRunning(void);

// 显示任务结果
void UIDisplay_ShowTaskResult(bool success, const char *reason);

// 更新进度信息
void UIDisplay_UpdateProgress(const char *status, uint32_t elapsed_time);

// 清屏
void UIDisplay_Clear(void);
```

## 任务管理器设计

### 任务枚举

```c
typedef enum {
    TASK_ID_1_LINE_TRACKING = 1,
    TASK_ID_2_DUAL_POINT = 2,
    TASK_ID_3_ROUND_TRIP = 3,
    TASK_ID_4_DIGIT_RECOGNITION = 4,
    TASK_ID_5_RECOGNIZE_TRANSPORT = 5,
    TASK_ID_6_AUTO_TRANSPORT = 6,
    TASK_ID_7_BATCH_TRANSPORT = 7,
    TASK_ID_8_CREATIVE = 8,
    TASK_ID_COUNT = 8,
} TaskID_t;
```

### 任务结构

```c
typedef struct {
    TaskID_t task_id;
    const char *task_name;
    TaskState_t state;
    uint32_t start_time;
    uint32_t timeout;
    
    // 任务函数指针
    void (*init)(void);
    void (*run)(void);
    void (*stop)(void);
    void (*reset)(void);
    bool (*is_success)(void);
} Task_t;
```

### 任务管理器接口

```c
// 初始化任务管理器
void TaskManager_Init(void);

// 启动当前选中的任务
void TaskManager_StartTask(void);

// 停止当前任务
void TaskManager_StopTask(void);

// 选择上一个任务
void TaskManager_PrevTask(void);

// 选择下一个任务
void TaskManager_NextTask(void);

// 获取当前任务状态
TaskState_t TaskManager_GetTaskState(void);

// 获取当前任务ID
TaskID_t TaskManager_GetCurrentTaskID(void);

// 判断任务是否运行中
bool TaskManager_IsRunning(void);

// 任务主循环（在main中调用）
void TaskManager_Update(void);
```

## 任务模板

### 头文件模板 (app_task_X.h)

```c
#ifndef APP_TASK_X_H
#define APP_TASK_X_H

#include <stdint.h>
#include <stdbool.h>

/* 任务状态定义 */
typedef enum {
    TASK_X_STATE_IDLE,
    TASK_X_STATE_INIT,
    TASK_X_STATE_RUNNING,
    TASK_X_STATE_SUCCESS,
    TASK_X_STATE_FAILED,
} TaskX_State_t;

/* 任务初始化 */
void TaskX_Init(void);

/* 任务运行 */
void TaskX_Run(void);

/* 任务停止 */
void TaskX_Stop(void);

/* 任务重置 */
void TaskX_Reset(void);

/* 获取任务状态 */
TaskX_State_t TaskX_GetState(void);

/* 获取任务结果 */
bool TaskX_IsSuccess(void);

#endif /* APP_TASK_X_H */
```

### 源文件模板 (app_task_X.c)

```c
#include "app_task_X.h"
#include "drivers/drv_motor.h"
#include "drivers/drv_led.h"
#include "app_ui_display.h"

/* 任务状态 */
static TaskX_State_t g_task_state = TASK_X_STATE_IDLE;
static uint32_t g_task_start_time = 0;
static uint32_t g_task_timeout = 30000;  // 30s超时

/* 任务初始化 */
void TaskX_Init(void)
{
    g_task_state = TASK_X_STATE_INIT;
    g_task_start_time = 0;
    
    /* 初始化电机 */
    Motor_Stop();
    
    /* 初始化传感器 */
    // TODO: 初始化传感器
    
    /* 更新显示 */
    UIDisplay_ShowTaskRunning();
    
    g_task_state = TASK_X_STATE_RUNNING;
    g_task_start_time = SysTick_GetTick();  // 记录开始时间
}

/* 任务运行 */
void TaskX_Run(void)
{
    if (g_task_state != TASK_X_STATE_RUNNING) {
        return;
    }
    
    /* 检查超时 */
    uint32_t elapsed_time = SysTick_GetTick() - g_task_start_time;
    if (elapsed_time > g_task_timeout) {
        g_task_state = TASK_X_STATE_FAILED;
        Motor_Stop();
        UIDisplay_ShowTaskResult(false, "超时");
        return;
    }
    
    /* 更新进度显示 */
    UIDisplay_UpdateProgress("运行中...", elapsed_time);
    
    /* 任务逻辑 */
    // TODO: 实现具体的任务逻辑
    
    /* 状态转移 */
    // if (任务完成条件) {
    //     g_task_state = TASK_X_STATE_SUCCESS;
    //     Motor_Stop();
    //     UIDisplay_ShowTaskResult(true, "成功");
    // }
}

/* 任务停止 */
void TaskX_Stop(void)
{
    Motor_Stop();
    g_task_state = TASK_X_STATE_IDLE;
}

/* 任务重置 */
void TaskX_Reset(void)
{
    g_task_state = TASK_X_STATE_IDLE;
    g_task_start_time = 0;
}

/* 获取任务状态 */
TaskX_State_t TaskX_GetState(void)
{
    return g_task_state;
}

/* 获取任务结果 */
bool TaskX_IsSuccess(void)
{
    return g_task_state == TASK_X_STATE_SUCCESS;
}
```

## 主应用框架

### main.c 框架

```c
#include "app_task_manager.h"
#include "app_ui_display.h"
#include "app_key_handler.h"
#include "drivers/drv_motor.h"
#include "drivers/drv_led.h"

int main(void)
{
    SYSCFG_DL_init();
    
    /* 初始化驱动 */
    Motor_Init();
    // LED_Init();
    
    /* 初始化UI和按键 */
    UIDisplay_Init();
    KeyHandler_Init();
    
    /* 初始化任务管理器 */
    TaskManager_Init();
    
    /* 显示任务列表 */
    UIDisplay_ShowTaskList();
    
    /* 主循环 */
    while (1) {
        /* 按键处理 */
        KeyEvent_t event = KeyHandler_Scan();
        
        switch (event) {
            case KEY_EVENT_KEY1_PRESSED:
                TaskManager_PrevTask();
                UIDisplay_ShowTaskList();
                break;
                
            case KEY_EVENT_KEY2_PRESSED:
                TaskManager_NextTask();
                UIDisplay_ShowTaskList();
                break;
                
            case KEY_EVENT_KEY3_PRESSED:
                if (TaskManager_IsRunning()) {
                    TaskManager_StopTask();
                    UIDisplay_ShowTaskList();
                } else {
                    TaskManager_StartTask();
                }
                break;
                
            default:
                break;
        }
        
        /* 任务管理器更新 */
        TaskManager_Update();
        
        /* 延迟 */
        delay_ms(10);
    }
}
```

## LED状态指示

| 状态 | LED | 说明 |
|------|-----|------|
| 空闲 | 绿灯常亮 | 等待任务选择 |
| 任务选中 | 绿灯闪烁 | 已选中任务，等待启动 |
| 任务运行 | 蓝灯常亮 | 任务执行中 |
| 任务成功 | 绿灯常亮 | 任务成功完成 |
| 任务失败 | 红灯常亮 | 任务执行失败 |
| 任务超时 | 红灯闪烁 | 任务执行超时 |

## 集成步骤

1. 创建8个任务文件（头文件+源文件）
2. 创建任务管理器（头文件+源文件）
3. 创建OLED显示管理器（头文件+源文件）
4. 创建按键处理器（头文件+源文件）
5. 在Keil项目中添加所有源文件
6. 在main.c中初始化所有模块
7. 逐个实现每个任务的具体逻辑

## 优势

✅ **3按键方案** - 充分利用有限的按键资源
✅ **OLED显示** - 清晰显示任务列表和进度
✅ **模块化设计** - 每个任务独立，易于开发和测试
✅ **状态机清晰** - 每个任务内部状态转移明确
✅ **易于扩展** - 新增任务只需添加新文件
✅ **LED指示** - 直观的状态提示
✅ **超时保护** - 每个任务都有超时机制

