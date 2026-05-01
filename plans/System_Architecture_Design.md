# 仓储搬运小车系统架构设计

## 一、系统整体架构

### 1.1 分层架构图

```
┌─────────────────────────────────────────────────────────┐
│              应用层（App Layer）                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  任务管理器（Task Manager）                      │  │
│  │  ├─ Task 1: 单点循迹                            │  │
│  │  ├─ Task 2: 双点循迹                            │  │
│  │  ├─ Task 3: 双点往返                            │  │
│  │  ├─ Task 4: 数字识别                            │  │
│  │  ├─ Task 5-8: 发挥部分                          │  │
│  │  └─ 路径规划模块                                │  │
│  └──────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────┤
│              驱动层（Driver Layer）                      │
│  ┌──────────────────────────────────────────────────┐  │
│  │  底盘控制（Chassis）                             │  │
│  │  ├─ 陀螺仪反馈（JY61P）                         │  │
│  │  ├─ 电机控制（Motor PWM）                       │  │
│  │  ├─ 编码器反馈（Encoder）                       │  │
│  │  └─ PID 控制器                                  │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  通信模块                                        │  │
│  │  ├─ UART（OpenMV 摄像头）                       │  │
│  │  ├─ 蜂鸣器提示                                  │  │
│  │  └─ OLED 显示                                   │  │
│  └──────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────┤
│              硬件层（Hardware Layer）                    │
│  ┌──────────────────────────────────────────────────┐  │
│  │  TI MSPM0G3507 MCU                               │  │
│  │  ├─ Timer（PWM 生成）                           │  │
│  │  ├─ UART（通信）                                │  │
│  │  ├─ GPIO（传感器/执行器）                       │  │
│  │  └─ SysConfig（外设初始化）                     │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### 1.2 任务执行流程

```
┌─────────────┐
│  启动系统   │
└──────┬──────┘
       │
       ▼
┌─────────────────────────┐
│  初始化所有模块         │
│  ├─ SysConfig_DL_init() │
│  ├─ ChassisInit()       │
│  ├─ TaskManager_Init()  │
│  └─ ...                 │
└──────┬──────────────────┘
       │
       ▼
┌─────────────────────────┐
│  主循环（Main Loop）    │
│  周期：10ms             │
└──────┬──────────────────┘
       │
       ├─→ 读取传感器数据
       │   ├─ 陀螺仪 Yaw 角
       │   ├─ 编码器脉冲
       │   └─ 摄像头数据
       │
       ├─→ 更新底盘控制
       │   ├─ ChassisUpdate()
       │   └─ 计算 PWM 输出
       │
       ├─→ 执行当前任务
       │   ├─ TaskManager_Update()
       │   └─ 状态机转移
       │
       └─→ 更新显示/提示
           ├─ OLED 显示
           └─ 蜂鸣器提示
```

---

## 二、任务状态机设计

### 2.1 通用任务状态转移图

```
                    ┌─────────────┐
                    │    IDLE     │
                    └──────┬──────┘
                           │ TaskManager_StartTask()
                           ▼
                    ┌─────────────┐
                    │    INIT     │
                    └──────┬──────┘
                           │ 初始化完成
                           ▼
                    ┌─────────────┐
                    │   RUNNING   │◄─────┐
                    └──────┬──────┘      │
                           │            │ 恢复
                           │ 暂停       │
                           ▼            │
                    ┌─────────────┐     │
                    │   PAUSED    ├─────┘
                    └─────────────┘
                    
                    ┌─────────────┐
                    │   RUNNING   │
                    └──────┬──────┘
                           │
                    ┌──────┴──────┐
                    │             │
                    ▼             ▼
            ┌─────────────┐ ┌─────────────┐
            │   SUCCESS   │ │   FAILED    │
            └─────────────┘ └─────────────┘
            
            ┌─────────────┐
            │   TIMEOUT   │
            └─────────────┘
```

### 2.2 Task 1 详细状态机

```
IDLE
  │
  ├─ TaskManager_StartTask()
  │
  ▼
INIT
  │ 初始化：
  │ ├─ 获取目标区编号
  │ ├─ 查询路径规划表
  │ └─ 启动底盘运动
  │
  ▼
RUNNING
  │ 运行中：
  │ ├─ 检查运动时间
  │ ├─ 更新陀螺仪反馈
  │ └─ 调整 PWM 输出
  │
  ├─ 时间到达？
  │  ├─ 是 → 停止电机
  │  └─ 否 → 继续运行
  │
  ├─ 超时？（时间 > 预期 × 1.2）
  │  ├─ 是 → FAILED
  │  └─ 否 → 继续
  │
  ▼
SUCCESS
  │ 完成：
  │ ├─ 停止电机
  │ ├─ 蜂鸣器提示
  │ └─ 等待下一个任务
  │
  ▼
IDLE
```

---

## 三、路径规划详细设计

### 3.1 路径点数据结构

```c
// 路径点定义
typedef struct {
    int16_t target_yaw;      // 目标偏航角（°×100）
                             // 范围：-36000 ~ +36000
                             // 0°：直行，+45°：左转45°，-45°：右转45°
    
    uint32_t duration_ms;    // 运动时间（ms）
                             // 范围：100 ~ 30000
    
    uint16_t base_pwm;       // 基础 PWM 占空比（0-700）
                             // 0：原地转弯，200-400：正常速度
    
    uint8_t direction;       // 运动方向
                             // CHASSIS_DIR_FORWARD：前进
                             // CHASSIS_DIR_BACKWARD：后退
    
    uint32_t pause_ms;       // 到达后停顿时间（ms）
                             // 0：不停顿，500：停0.5s
} PathPoint_t;

// 完整路径定义
typedef struct {
    const PathPoint_t *points;  // 路径点数组
    uint16_t point_count;       // 路径点数量
    uint16_t current_point;     // 当前执行的路径点索引
    uint32_t point_start_time;  // 当前路径点开始时间
    TaskState_t state;          // 路径执行状态
} Path_t;
```

### 3.2 预定义路径表

```c
// 停车区到各目标区的路径
static const PathPoint_t g_path_to_target[5][2] = {
    // 目标区 1：直行（0°）
    {
        {0,     5000, 200, CHASSIS_DIR_FORWARD, 500},  // 直行5s
        {0,     0,    0,   CHASSIS_DIR_FORWARD, 0},    // 结束标记
    },
    
    // 目标区 2：左转45°
    {
        {0,     3000, 200, CHASSIS_DIR_FORWARD, 500},  // 直行3s
        {4500,  2000, 0,   CHASSIS_DIR_FORWARD, 500},  // 原地转弯2s
        {4500,  2000, 200, CHASSIS_DIR_FORWARD, 500},  // 直行2s
        {0,     0,    0,   CHASSIS_DIR_FORWARD, 0},    // 结束标记
    },
    
    // 目标区 3：左转90°
    {
        {0,     3000, 200, CHASSIS_DIR_FORWARD, 500},
        {9000,  2500, 0,   CHASSIS_DIR_FORWARD, 500},
        {9000,  2000, 200, CHASSIS_DIR_FORWARD, 500},
        {0,     0,    0,   CHASSIS_DIR_FORWARD, 0},
    },
    
    // 目标区 4：右转45°
    {
        {0,     3000, 200, CHASSIS_DIR_FORWARD, 500},
        {-4500, 2000, 0,   CHASSIS_DIR_FORWARD, 500},
        {-4500, 2000, 200, CHASSIS_DIR_FORWARD, 500},
        {0,     0,    0,   CHASSIS_DIR_FORWARD, 0},
    },
    
    // 目标区 5：右转90°
    {
        {0,     3000, 200, CHASSIS_DIR_FORWARD, 500},
        {-9000, 2500, 0,   CHASSIS_DIR_FORWARD, 500},
        {-9000, 2000, 200, CHASSIS_DIR_FORWARD, 500},
        {0,     0,    0,   CHASSIS_DIR_FORWARD, 0},
    },
};

// 返回停车区的路径
static const PathPoint_t g_path_return_home[1] = {
    {0, 5000, 200, CHASSIS_DIR_FORWARD, 500},  // 直行返回
};
```

### 3.3 路径规划执行算法

```c
// 路径规划执行函数
void PathExecute(Path_t *path) {
    if (path->current_point >= path->point_count) {
        path->state = TASK_STATE_SUCCESS;
        return;
    }
    
    const PathPoint_t *point = &path->points[path->current_point];
    uint32_t elapsed = uwTick - path->point_start_time;
    
    // 检查当前路径点是否完成
    if (elapsed >= point->duration_ms) {
        // 停顿处理
        if (elapsed < point->duration_ms + point->pause_ms) {
            ChassisStop();
            return;
        }
        
        // 移动到下一个路径点
        path->current_point++;
        path->point_start_time = uwTick;
        
        if (path->current_point < path->point_count) {
            const PathPoint_t *next_point = &path->points[path->current_point];
            ChassisSetMotion(
                next_point->target_yaw,
                next_point->duration_ms,
                next_point->base_pwm,
                next_point->direction
            );
        }
    }
}
```

---

## 四、任务实现框架

### 4.1 Task 1 实现框架

```c
// app_task_1_line_tracking.h
#ifndef APP_TASK_1_LINE_TRACKING_H
#define APP_TASK_1_LINE_TRACKING_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

void Task1_Init(void);
void Task1_Run(void);
void Task1_Stop(void);
void Task1_Reset(void);
TaskState_t Task1_GetState(void);
bool Task1_IsSuccess(void);

#endif
```

```c
// app_task_1_line_tracking.c
#include "app_task_1_line_tracking.h"
#include "../drivers/drv_chassis.h"
#include "../utils/timer.h"

typedef struct {
    TaskState_t state;
    uint8_t target_zone;        // 目标区编号（1-5）
    uint32_t start_time;
    uint32_t timeout_ms;        // 超时时间
    Path_t path;
} Task1_Context_t;

static Task1_Context_t g_task1_ctx;

// 初始化
void Task1_Init(void) {
    g_task1_ctx.state = TASK_STATE_INIT;
    g_task1_ctx.target_zone = 1;  // 默认目标区1
    g_task1_ctx.start_time = uwTick;
    g_task1_ctx.timeout_ms = 15000;  // 15s 超时
    
    // 初始化路径
    g_task1_ctx.path.points = g_path_to_target[g_task1_ctx.target_zone - 1];
    g_task1_ctx.path.point_count = 2;  // 简化版本
    g_task1_ctx.path.current_point = 0;
    g_task1_ctx.path.point_start_time = uwTick;
    g_task1_ctx.path.state = TASK_STATE_RUNNING;
    
    // 启动第一个路径点
    const PathPoint_t *point = &g_task1_ctx.path.points[0];
    ChassisSetMotion(point->target_yaw, point->duration_ms, 
                     point->base_pwm, point->direction);
    
    g_task1_ctx.state = TASK_STATE_RUNNING;
}

// 主循环
void Task1_Run(void) {
    if (g_task1_ctx.state != TASK_STATE_RUNNING) {
        return;
    }
    
    uint32_t elapsed = uwTick - g_task1_ctx.start_time;
    
    // 超时检查
    if (elapsed > g_task1_ctx.timeout_ms) {
        g_task1_ctx.state = TASK_STATE_TIMEOUT;
        ChassisStop();
        return;
    }
    
    // 执行路径规划
    PathExecute(&g_task1_ctx.path);
    
    // 检查是否完成
    if (g_task1_ctx.path.state == TASK_STATE_SUCCESS) {
        g_task1_ctx.state = TASK_STATE_SUCCESS;
        ChassisStop();
    }
}

// 停止
void Task1_Stop(void) {
    ChassisStop();
    g_task1_ctx.state = TASK_STATE_IDLE;
}

// 重置
void Task1_Reset(void) {
    g_task1_ctx.state = TASK_STATE_IDLE;
    g_task1_ctx.start_time = 0;
}

// 获取状态
TaskState_t Task1_GetState(void) {
    return g_task1_ctx.state;
}

// 判断成功
bool Task1_IsSuccess(void) {
    return g_task1_ctx.state == TASK_STATE_SUCCESS;
}

// 设置目标区
void Task1_SetTargetZone(uint8_t zone) {
    if (zone >= 1 && zone <= 5) {
        g_task1_ctx.target_zone = zone;
    }
}
```

### 4.2 Task 2 实现框架

```c
// Task 2 需要处理两个目标区
typedef struct {
    TaskState_t state;
    uint8_t target_zone_1;
    uint8_t target_zone_2;
    uint32_t start_time;
    uint32_t pause_time;        // 停顿时间
    uint8_t phase;              // 0: 前往目标1, 1: 停顿, 2: 前往目标2
    Path_t path;
} Task2_Context_t;

// 状态转移：
// INIT → RUNNING(前往目标1) → RUNNING(停顿2s) → RUNNING(前往目标2) → SUCCESS
```

### 4.3 Task 3 实现框架

```c
// Task 3 需要精确的时间控制
typedef struct {
    TaskState_t state;
    uint8_t target_zone_1;
    uint8_t target_zone_2;
    uint32_t start_time;
    uint32_t total_time_limit;  // 30±2s
    uint8_t phase;              // 0: 前往目标1, 1: 停顿, 2: 前往目标2, 3: 返回
    Path_t path;
} Task3_Context_t;

// 关键：需要动态调整运动速度以满足时间要求
// 如果运行时间超过预期，需要加速
// 如果运行时间不足，需要减速
```

---

## 五、数据流设计

### 5.1 传感器数据流

```
陀螺仪（JY61P）
    │
    ├─ ISR: isr_jy61p.c
    │  └─ 更新 g_jy61p.yaw
    │
    ▼
底盘控制（drv_chassis.c）
    │
    ├─ ChassisUpdate()
    │  ├─ 读取当前 Yaw 角
    │  ├─ 计算误差：error = target_yaw - current_yaw
    │  ├─ PID 计算：pwm_delta = P × error
    │  ├─ 差速控制：
    │  │  left_pwm = base_pwm - pwm_delta
    │  │  right_pwm = base_pwm + pwm_delta
    │  └─ 输出 PWM
    │
    ▼
电机驱动（drv_motor.c）
    │
    └─ 控制左右电机转速
```

### 5.2 任务执行数据流

```
任务管理器（app_task_manager.c）
    │
    ├─ TaskManager_Update()
    │  └─ 调用当前任务的 Run() 函数
    │
    ▼
当前任务（app_task_X_xxx.c）
    │
    ├─ 读取任务状态
    ├─ 执行路径规划
    ├─ 调用 ChassisSetMotion()
    └─ 更新任务状态
    
    ▼
底盘控制（drv_chassis.c）
    │
    └─ 执行运动控制
```

---

## 六、时间精度分析

### 6.1 时间源

| 时间源 | 精度 | 用途 |
|--------|------|------|
| `uwTick` | 1ms | 任务计时、路径规划 |
| 系统时钟 | 80MHz | 底层硬件计时 |
| Timer ISR | 1ms | 系统滴答更新 |

### 6.2 时间误差来源

1. **ISR 延迟**：中断处理延迟 < 1ms
2. **任务切换**：主循环周期 10ms
3. **陀螺仪漂移**：长时间运行可能累积误差
4. **电机响应延迟**：PWM 更新到实际转速变化 < 100ms

### 6.3 时间精度要求

| 任务 | 精度要求 | 实现方案 |
|------|---------|--------|
| Task 1 | ±500ms | 基于 uwTick 计时 |
| Task 2 | ±500ms | 基于 uwTick 计时 |
| Task 3 | ±2s（30±2s） | 动态速度调整 |
| Task 4 | ±3s | 摄像头识别超时 |

---

## 七、错误处理策略

### 7.1 超时处理

```c
// 每个任务都需要超时检查
if (elapsed_time > timeout_limit) {
    task_state = TASK_STATE_TIMEOUT;
    ChassisStop();
    // 记录日志
    // 蜂鸣器提示
}
```

### 7.2 传感器故障处理

```c
// 陀螺仪数据无效
if (!g_jy61p.data_valid) {
    // 使用编码器反馈替代
    // 或者停止运动
}

// 电机故障
if (motor_current > threshold) {
    // 停止电机
    // 报警
}
```

### 7.3 异常恢复

```c
// 任务失败后的恢复
if (task_state == TASK_STATE_FAILED) {
    // 停止所有运动
    ChassisStop();
    
    // 重置任务状态
    Task_Reset();
    
    // 等待用户重新启动
}
```

---

## 八、集成检查清单

### 8.1 编译检查
- [ ] 所有头文件无循环依赖
- [ ] 所有函数声明与实现一致
- [ ] 编译无警告
- [ ] 链接无错误

### 8.2 功能检查
- [ ] 任务管理器能正确切换任务
- [ ] 单个任务能独立运行
- [ ] 状态转移正确
- [ ] 超时检查有效

### 8.3 性能检查
- [ ] 主循环周期 < 10ms
- [ ] 中断延迟 < 1ms
- [ ] 内存使用 < 50% SRAM
- [ ] 无内存泄漏

### 8.4 鲁棒性检查
- [ ] 异常输入处理
- [ ] 边界条件测试
- [ ] 长时间运行测试
- [ ] 极端环境测试

---

## 九、调试工具与方法

### 9.1 UART 日志输出

```c
// 在关键位置添加日志
#define TASK_LOG(fmt, ...) \
    printf("[Task %d] " fmt "\n", current_task_id, ##__VA_ARGS__)

// 使用示例
TASK_LOG("State: %d, Yaw: %d, Time: %dms", 
         state, current_yaw, elapsed_time);
```

### 9.2 OLED 实时显示

```c
// 显示当前任务信息
void DisplayTaskInfo(void) {
    OLED_ShowString(0, 0, "Task: ", 16);
    OLED_ShowNum(48, 0, current_task_id, 1, 16);
    
    OLED_ShowString(0, 16, "State: ", 16);
    OLED_ShowNum(56, 16, task_state, 1, 16);
    
    OLED_ShowString(0, 32, "Yaw: ", 16);
    OLED_ShowNum(40, 32, current_yaw / 100, 3, 16);
    
    OLED_ShowString(0, 48, "Time: ", 16);
    OLED_ShowNum(40, 48, elapsed_time / 1000, 2, 16);
}
```

### 9.3 XDS-110 调试

```c
// 设置断点
// 1. 在 TaskManager_Update() 处设置断点
// 2. 在状态转移处设置条件断点
// 3. 观察变量变化

// 观察窗口
// - g_current_task_id
// - g_task_state
// - g_chassis.current_yaw
// - g_chassis.elapsed_ms
```

---

## 十、性能优化建议

### 10.1 计算优化

```c
// 使用整型缩放避免浮点运算
// 角度：°×100（0-36000 表示 0-360°）
// 速度：mm/s（整型）
// 加速度：mm/s²（整型）

// PID 计算优化
int32_t pid_output = (error * kp) >> 8;  // 使用移位代替除法
```

### 10.2 内存优化

```c
// 使用静态分配
static Task1_Context_t g_task1_ctx;  // 栈上分配

// 共享缓冲区
static uint8_t g_shared_buffer[256];

// 常量数据放在 Flash
static const PathPoint_t g_path_table[] = { ... };
```

### 10.3 功耗优化

```c
// 关闭未使用的外设
DL_GPIO_setPins(GPIO_PORT, GPIO_PIN_UNUSED);  // 设置为输出低

// 调整中断优先级
DL_Interrupt_setPriority(TIMER_INTERRUPT, DL_INTERRUPT_PRIORITY_LEVEL_1);

// 主循环频率
// 当前：10ms（100Hz）
// 可优化：20ms（50Hz）如果不影响控制精度
```

---

## 十一、下一步行动

### 第一阶段：框架搭建
1. 创建 8 个任务文件框架
2. 实现任务管理器集成
3. 编译验证

### 第二阶段：基础任务实现
1. 实现 Task 1：单点循迹
2. 实现 Task 2：双点循迹
3. 实现 Task 3：双点往返
4. 实现 Task 4：数字识别

### 第三阶段：发挥部分
1. 实现 Task 5-7：搬运相关
2. 实现 Task 8：创意任务

### 第四阶段：优化与测试
1. 性能优化
2. 鲁棒性测试
3. 场景测试

