# 仓储搬运小车任务实现指南

## 一、快速开始

### 1.1 文件创建清单

需要创建以下 16 个文件（8 个任务 × 2 个文件）：

```
src/app/
├── app_task_1_line_tracking.h
├── app_task_1_line_tracking.c
├── app_task_2_dual_point.h
├── app_task_2_dual_point.c
├── app_task_3_round_trip.h
├── app_task_3_round_trip.c
├── app_task_4_digit_recognition.h
├── app_task_4_digit_recognition.c
├── app_task_5_recognize_transport.h
├── app_task_5_recognize_transport.c
├── app_task_6_auto_transport.h
├── app_task_6_auto_transport.c
├── app_task_7_batch_transport.h
├── app_task_7_batch_transport.c
├── app_task_8_creative.h
└── app_task_8_creative.c
```

### 1.2 路径规划模块（可选但推荐）

```
src/app/
├── app_path_planner.h
└── app_path_planner.c
```

---

## 二、任务函数接口规范

### 2.1 头文件模板

```c
/**
 * @file app_task_X_xxx.h
 * @brief Task X: 任务名称
 * 
 * 功能描述：
 * - 详细说明任务功能
 * - 列出关键步骤
 */

#ifndef APP_TASK_X_XXX_H
#define APP_TASK_X_XXX_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

/* ==================== 任务初始化 ==================== */
void TaskX_Init(void);

/* ==================== 任务主循环 ==================== */
void TaskX_Run(void);

/* ==================== 任务停止 ==================== */
void TaskX_Stop(void);

/* ==================== 任务重置 ==================== */
void TaskX_Reset(void);

/* ==================== 获取任务状态 ==================== */
TaskState_t TaskX_GetState(void);

/* ==================== 判断任务是否成功 ==================== */
bool TaskX_IsSuccess(void);

#endif /* APP_TASK_X_XXX_H */
```

### 2.2 实现文件模板

```c
/**
 * @file app_task_X_xxx.c
 * @brief Task X: 任务名称 - 实现文件
 */

#include "app_task_X_xxx.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../utils/timer.h"
#include <stdlib.h>

/* ==================== 任务上下文结构 ==================== */

typedef struct {
    TaskState_t state;              // 任务状态
    uint32_t start_time;            // 任务开始时间
    uint32_t elapsed_time;          // 已运行时间
    uint32_t timeout_ms;            // 超时时间
    
    // 任务特定的状态变量
    uint8_t target_zone;            // 目标区编号
    uint8_t phase;                  // 当前阶段
    uint32_t phase_start_time;      // 阶段开始时间
} TaskX_Context_t;

/* ==================== 全局变量 ==================== */

static TaskX_Context_t g_task_ctx;

/* ==================== 外部变量声明 ==================== */

extern volatile uint32_t uwTick;    // 系统滴答计数

/* ==================== 函数实现 ==================== */

/**
 * @brief 初始化任务
 */
void TaskX_Init(void)
{
    g_task_ctx.state = TASK_STATE_INIT;
    g_task_ctx.start_time = uwTick;
    g_task_ctx.elapsed_time = 0;
    g_task_ctx.timeout_ms = 15000;  // 15s 超时
    g_task_ctx.target_zone = 1;     // 默认目标区1
    g_task_ctx.phase = 0;
    g_task_ctx.phase_start_time = uwTick;
    
    // 蜂鸣器提示：任务开始
    BuzzerBeep(100);  // 100ms 蜂鸣
    
    // 启动底盘运动
    ChassisSetMotion(0, 5000, 200, CHASSIS_DIR_FORWARD);
    
    g_task_ctx.state = TASK_STATE_RUNNING;
}

/**
 * @brief 任务主循环
 */
void TaskX_Run(void)
{
    if (g_task_ctx.state != TASK_STATE_RUNNING) {
        return;
    }
    
    // 计算已运行时间
    g_task_ctx.elapsed_time = uwTick - g_task_ctx.start_time;
    
    // 超时检查
    if (g_task_ctx.elapsed_time > g_task_ctx.timeout_ms) {
        g_task_ctx.state = TASK_STATE_TIMEOUT;
        ChassisStop();
        BuzzerBeep(500);  // 500ms 蜂鸣（失败提示）
        return;
    }
    
    // 状态机处理
    switch (g_task_ctx.phase) {
        case 0:
            // 阶段0：前进到目标区
            if (g_task_ctx.elapsed_time >= 5000) {
                g_task_ctx.phase = 1;
                g_task_ctx.phase_start_time = uwTick;
                ChassisStop();
            }
            break;
        
        case 1:
            // 阶段1：停顿
            if (uwTick - g_task_ctx.phase_start_time >= 500) {
                g_task_ctx.state = TASK_STATE_SUCCESS;
                BuzzerBeep(200);  // 200ms 蜂鸣（成功提示）
            }
            break;
        
        default:
            break;
    }
}

/**
 * @brief 停止任务
 */
void TaskX_Stop(void)
{
    ChassisStop();
    g_task_ctx.state = TASK_STATE_IDLE;
}

/**
 * @brief 重置任务
 */
void TaskX_Reset(void)
{
    g_task_ctx.state = TASK_STATE_IDLE;
    g_task_ctx.start_time = 0;
    g_task_ctx.elapsed_time = 0;
    g_task_ctx.phase = 0;
}

/**
 * @brief 获取任务状态
 */
TaskState_t TaskX_GetState(void)
{
    return g_task_ctx.state;
}

/**
 * @brief 判断任务是否成功
 */
bool TaskX_IsSuccess(void)
{
    return g_task_ctx.state == TASK_STATE_SUCCESS;
}

/**
 * @brief 设置目标区编号
 */
void TaskX_SetTargetZone(uint8_t zone)
{
    if (zone >= 1 && zone <= 5) {
        g_task_ctx.target_zone = zone;
    }
}
```

---

## 三、路径规划模块实现

### 3.1 路径规划头文件

```c
// app_path_planner.h
#ifndef APP_PATH_PLANNER_H
#define APP_PATH_PLANNER_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== 路径点定义 ==================== */

typedef struct {
    int16_t target_yaw;      // 目标偏航角（°×100）
    uint32_t duration_ms;    // 运动时间（ms）
    uint16_t base_pwm;       // 基础 PWM 占空比
    uint8_t direction;       // 运动方向
    uint32_t pause_ms;       // 停顿时间（ms）
} PathPoint_t;

/* ==================== 路径定义 ==================== */

typedef struct {
    const PathPoint_t *points;
    uint16_t point_count;
    uint16_t current_point;
    uint32_t point_start_time;
    uint8_t state;           // 0: 运动中, 1: 停顿中, 2: 完成
} Path_t;

/* ==================== 路径规划函数 ==================== */

/**
 * @brief 初始化路径
 */
void PathInit(Path_t *path, const PathPoint_t *points, uint16_t count);

/**
 * @brief 执行路径规划
 */
void PathExecute(Path_t *path);

/**
 * @brief 检查路径是否完成
 */
bool PathIsComplete(const Path_t *path);

/**
 * @brief 重置路径
 */
void PathReset(Path_t *path);

/* ==================== 预定义路径表 ==================== */

// 停车区到各目标区的路径
extern const PathPoint_t g_path_to_target_1[];
extern const PathPoint_t g_path_to_target_2[];
extern const PathPoint_t g_path_to_target_3[];
extern const PathPoint_t g_path_to_target_4[];
extern const PathPoint_t g_path_to_target_5[];

// 返回停车区的路径
extern const PathPoint_t g_path_return_home[];

#endif /* APP_PATH_PLANNER_H */
```

### 3.2 路径规划实现文件

```c
// app_path_planner.c
#include "app_path_planner.h"
#include "../drivers/drv_chassis.h"

extern volatile uint32_t uwTick;

/* ==================== 预定义路径表 ==================== */

// 目标区1：直行（0°）
const PathPoint_t g_path_to_target_1[] = {
    {0,     5000, 200, 0, 500},  // 直行5s，停0.5s
    {0,     0,    0,   0, 0},    // 结束标记
};

// 目标区2：左转45°
const PathPoint_t g_path_to_target_2[] = {
    {0,     3000, 200, 0, 500},  // 直行3s
    {4500,  2000, 0,   0, 500},  // 原地转弯2s
    {4500,  2000, 200, 0, 500},  // 直行2s
    {0,     0,    0,   0, 0},    // 结束标记
};

// 目标区3：左转90°
const PathPoint_t g_path_to_target_3[] = {
    {0,     3000, 200, 0, 500},
    {9000,  2500, 0,   0, 500},
    {9000,  2000, 200, 0, 500},
    {0,     0,    0,   0, 0},
};

// 目标区4：右转45°
const PathPoint_t g_path_to_target_4[] = {
    {0,     3000, 200, 0, 500},
    {-4500, 2000, 0,   0, 500},
    {-4500, 2000, 200, 0, 500},
    {0,     0,    0,   0, 0},
};

// 目标区5：右转90°
const PathPoint_t g_path_to_target_5[] = {
    {0,     3000, 200, 0, 500},
    {-9000, 2500, 0,   0, 500},
    {-9000, 2000, 200, 0, 500},
    {0,     0,    0,   0, 0},
};

// 返回停车区
const PathPoint_t g_path_return_home[] = {
    {0, 5000, 200, 0, 500},
    {0, 0,    0,   0, 0},
};

/* ==================== 函数实现 ==================== */

void PathInit(Path_t *path, const PathPoint_t *points, uint16_t count)
{
    path->points = points;
    path->point_count = count;
    path->current_point = 0;
    path->point_start_time = uwTick;
    path->state = 0;  // 运动中
}

void PathExecute(Path_t *path)
{
    if (path->current_point >= path->point_count) {
        path->state = 2;  // 完成
        return;
    }
    
    const PathPoint_t *point = &path->points[path->current_point];
    uint32_t elapsed = uwTick - path->point_start_time;
    
    if (path->state == 0) {
        // 运动中
        if (elapsed >= point->duration_ms) {
            // 运动完成，进入停顿
            path->state = 1;
            path->point_start_time = uwTick;
            ChassisStop();
        }
    } else if (path->state == 1) {
        // 停顿中
        if (elapsed >= point->pause_ms) {
            // 停顿完成，移动到下一个路径点
            path->current_point++;
            path->point_start_time = uwTick;
            path->state = 0;
            
            if (path->current_point < path->point_count) {
                const PathPoint_t *next = &path->points[path->current_point];
                ChassisSetMotion(next->target_yaw, next->duration_ms,
                                next->base_pwm, next->direction);
            }
        }
    }
}

bool PathIsComplete(const Path_t *path)
{
    return path->state == 2;
}

void PathReset(Path_t *path)
{
    path->current_point = 0;
    path->point_start_time = uwTick;
    path->state = 0;
}
```

---

## 四、任务管理器集成

### 4.1 修改 app_task_manager.c

在文件顶部添加所有任务头文件的包含：

```c
#include "app_task_1_line_tracking.h"
#include "app_task_2_dual_point.h"
#include "app_task_3_round_trip.h"
#include "app_task_4_digit_recognition.h"
#include "app_task_5_recognize_transport.h"
#include "app_task_6_auto_transport.h"
#include "app_task_7_batch_transport.h"
#include "app_task_8_creative.h"
```

任务表已经在原文件中定义，只需确保函数指针正确指向各任务的函数。

### 4.2 编译验证

```bash
# 在 Keil 中编译
# 检查是否有链接错误
# 确保所有任务函数都被正确链接
```

---

## 五、具体任务实现示例

### 5.1 Task 1: 单点循迹（完整实现）

```c
// app_task_1_line_tracking.c
#include "app_task_1_line_tracking.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../utils/timer.h"

typedef struct {
    TaskState_t state;
    uint8_t target_zone;
    uint32_t start_time;
    uint32_t timeout_ms;
    Path_t path;
} Task1_Context_t;

static Task1_Context_t g_task1_ctx;
extern volatile uint32_t uwTick;

// 路径表指针数组
static const PathPoint_t *g_path_table[5] = {
    g_path_to_target_1,
    g_path_to_target_2,
    g_path_to_target_3,
    g_path_to_target_4,
    g_path_to_target_5,
};

void Task1_Init(void)
{
    g_task1_ctx.state = TASK_STATE_INIT;
    g_task1_ctx.target_zone = 1;
    g_task1_ctx.start_time = uwTick;
    g_task1_ctx.timeout_ms = 15000;
    
    // 初始化路径
    PathInit(&g_task1_ctx.path, 
             g_path_table[g_task1_ctx.target_zone - 1], 
             10);  // 最多10个路径点
    
    // 启动第一个路径点
    const PathPoint_t *point = &g_task1_ctx.path.points[0];
    ChassisSetMotion(point->target_yaw, point->duration_ms,
                    point->base_pwm, point->direction);
    
    BuzzerBeep(100);
    g_task1_ctx.state = TASK_STATE_RUNNING;
}

void Task1_Run(void)
{
    if (g_task1_ctx.state != TASK_STATE_RUNNING) {
        return;
    }
    
    uint32_t elapsed = uwTick - g_task1_ctx.start_time;
    
    // 超时检查
    if (elapsed > g_task1_ctx.timeout_ms) {
        g_task1_ctx.state = TASK_STATE_TIMEOUT;
        ChassisStop();
        BuzzerBeep(500);
        return;
    }
    
    // 执行路径规划
    PathExecute(&g_task1_ctx.path);
    
    // 检查是否完成
    if (PathIsComplete(&g_task1_ctx.path)) {
        g_task1_ctx.state = TASK_STATE_SUCCESS;
        ChassisStop();
        BuzzerBeep(200);
    }
}

void Task1_Stop(void)
{
    ChassisStop();
    g_task1_ctx.state = TASK_STATE_IDLE;
}

void Task1_Reset(void)
{
    g_task1_ctx.state = TASK_STATE_IDLE;
    g_task1_ctx.start_time = 0;
}

TaskState_t Task1_GetState(void)
{
    return g_task1_ctx.state;
}

bool Task1_IsSuccess(void)
{
    return g_task1_ctx.state == TASK_STATE_SUCCESS;
}

void Task1_SetTargetZone(uint8_t zone)
{
    if (zone >= 1 && zone <= 5) {
        g_task1_ctx.target_zone = zone;
    }
}
```

### 5.2 Task 2: 双点循迹

```c
// app_task_2_dual_point.c
typedef struct {
    TaskState_t state;
    uint8_t target_zone_1;
    uint8_t target_zone_2;
    uint32_t start_time;
    uint32_t timeout_ms;
    uint8_t phase;  // 0: 前往目标1, 1: 停顿2s, 2: 前往目标2
    uint32_t phase_start_time;
    Path_t path;
} Task2_Context_t;

static Task2_Context_t g_task2_ctx;

void Task2_Init(void)
{
    g_task2_ctx.state = TASK_STATE_INIT;
    g_task2_ctx.target_zone_1 = 1;
    g_task2_ctx.target_zone_2 = 2;
    g_task2_ctx.start_time = uwTick;
    g_task2_ctx.timeout_ms = 20000;
    g_task2_ctx.phase = 0;
    g_task2_ctx.phase_start_time = uwTick;
    
    // 初始化路径到目标1
    PathInit(&g_task2_ctx.path,
             g_path_table[g_task2_ctx.target_zone_1 - 1],
             10);
    
    const PathPoint_t *point = &g_task2_ctx.path.points[0];
    ChassisSetMotion(point->target_yaw, point->duration_ms,
                    point->base_pwm, point->direction);
    
    BuzzerBeep(100);
    g_task2_ctx.state = TASK_STATE_RUNNING;
}

void Task2_Run(void)
{
    if (g_task2_ctx.state != TASK_STATE_RUNNING) {
        return;
    }
    
    uint32_t elapsed = uwTick - g_task2_ctx.start_time;
    
    if (elapsed > g_task2_ctx.timeout_ms) {
        g_task2_ctx.state = TASK_STATE_TIMEOUT;
        ChassisStop();
        BuzzerBeep(500);
        return;
    }
    
    switch (g_task2_ctx.phase) {
        case 0:
            // 前往目标1
            PathExecute(&g_task2_ctx.path);
            if (PathIsComplete(&g_task2_ctx.path)) {
                g_task2_ctx.phase = 1;
                g_task2_ctx.phase_start_time = uwTick;
                ChassisStop();
                BuzzerBeep(200);  // 到达目标1提示
            }
            break;
        
        case 1:
            // 停顿2s
            if (uwTick - g_task2_ctx.phase_start_time >= 2000) {
                g_task2_ctx.phase = 2;
                g_task2_ctx.phase_start_time = uwTick;
                
                // 初始化路径到目标2
                PathReset(&g_task2_ctx.path);
                PathInit(&g_task2_ctx.path,
                        g_path_table[g_task2_ctx.target_zone_2 - 1],
                        10);
                
                const PathPoint_t *point = &g_task2_ctx.path.points[0];
                ChassisSetMotion(point->target_yaw, point->duration_ms,
                                point->base_pwm, point->direction);
            }
            break;
        
        case 2:
            // 前往目标2
            PathExecute(&g_task2_ctx.path);
            if (PathIsComplete(&g_task2_ctx.path)) {
                g_task2_ctx.state = TASK_STATE_SUCCESS;
                ChassisStop();
                BuzzerBeep(200);  // 到达目标2提示
            }
            break;
    }
}

void Task2_Stop(void)
{
    ChassisStop();
    g_task2_ctx.state = TASK_STATE_IDLE;
}

void Task2_Reset(void)
{
    g_task2_ctx.state = TASK_STATE_IDLE;
    g_task2_ctx.start_time = 0;
    g_task2_ctx.phase = 0;
}

TaskState_t Task2_GetState(void)
{
    return g_task2_ctx.state;
}

bool Task2_IsSuccess(void)
{
    return g_task2_ctx.state == TASK_STATE_SUCCESS;
}

void Task2_SetTargetZones(uint8_t zone1, uint8_t zone2)
{
    if (zone1 >= 1 && zone1 <= 5 && zone2 >= 1 && zone2 <= 5) {
        g_task2_ctx.target_zone_1 = zone1;
        g_task2_ctx.target_zone_2 = zone2;
    }
}
```

---

## 六、集成步骤

### 第一步：创建文件框架
1. 创建 8 个任务的 `.h` 和 `.c` 文件
2. 使用上述模板填充基本结构
3. 确保编译无错误

### 第二步：实现路径规划模块
1. 创建 `app_path_planner.h` 和 `app_path_planner.c`
2. 定义预定义路径表
3. 实现路径执行算法

### 第三步：实现基础任务
1. 实现 Task 1：单点循迹
2. 实现 Task 2：双点循迹
3. 实现 Task 3：双点往返
4. 实现 Task 4：数字识别

### 第四步：验证集成
1. 编译整个项目
2. 在 Keil 中调试
3. 验证任务切换功能
4. 验证单个任务执行

### 第五步：优化与测试
1. 性能优化
2. 鲁棒性测试
3. 场景测试

---

## 七、常见问题解决

### Q1: 编译错误 "undefined reference to TaskX_Init"
**A**: 检查是否在 `app_task_manager.c` 中包含了任务头文件

### Q2: 任务无法启动
**A**: 检查 `TaskManager_StartTask()` 是否正确调用了 `task->init()`

### Q3: 时间精度不足
**A**: 确保 `uwTick` 的更新频率为 1ms，检查中断优先级配置

### Q4: 陀螺仪数据不稳定
**A**: 检查 `drv_jy61p.c` 中的数据滤波算法，可能需要增加滤波系数

### Q5: 电机响应延迟
**A**: 检查 PWM 更新频率，可能需要增加 Timer 中断频率

---

## 八、调试技巧

### 8.1 UART 日志输出

```c
#define TASK_DEBUG 1

#if TASK_DEBUG
#define TASK_LOG(fmt, ...) \
    printf("[Task] " fmt "\n", ##__VA_ARGS__)
#else
#define TASK_LOG(fmt, ...)
#endif

// 使用示例
TASK_LOG("Task %d started, target_zone=%d", task_id, target_zone);
```

### 8.2 OLED 实时显示

```c
void DisplayTaskStatus(void)
{
    char buf[32];
    
    sprintf(buf, "Task: %d", current_task_id);
    OLED_ShowString(0, 0, buf, 16);
    
    sprintf(buf, "State: %d", task_state);
    OLED_ShowString(0, 16, buf, 16);
    
    sprintf(buf, "Yaw: %d", current_yaw / 100);
    OLED_ShowString(0, 32, buf, 16);
}
```

### 8.3 XDS-110 调试

```c
// 在关键位置设置断点
// 1. TaskManager_StartTask() - 任务启动
// 2. TaskX_Run() - 任务主循环
// 3. ChassisUpdate() - 底盘控制

// 观察变量
// - g_current_task_id
// - g_task_state
// - g_chassis.current_yaw
// - g_chassis.elapsed_ms
```

---

## 九、性能指标

| 指标 | 目标 | 实现方案 |
|------|------|--------|
| 主循环周期 | < 10ms | 使用 Timer ISR 驱动 |
| 中断延迟 | < 1ms | 优化 ISR 代码 |
| 内存使用 | < 50% SRAM | 使用静态分配 |
| 时间精度 | ±500ms | 基于 uwTick 计时 |
| 角度精度 | ±1° | 陀螺仪反馈 |

---

## 十、下一步行动

1. **确认方案** ✓
2. **创建文件框架** ← 下一步
3. **实现路径规划模块**
4. **实现基础任务**
5. **集成测试**
6. **性能优化**

