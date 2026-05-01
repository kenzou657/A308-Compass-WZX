# 仓储搬运小车任务实现方案

## 一、赛题分析

### 1.1 任务层级结构
```
基本要求（40分）
├─ Task 1: 单点循迹（10分）- 从停车区循迹到目标区
├─ Task 2: 双点循迹（10分）- 循迹到目标1，停2s，再循迹到目标2
├─ Task 3: 双点往返（10分）- Task 2 + 返回停车区（30±2s）
└─ Task 4: 数字识别（10分）- 识别数字标签，3s内识别并提示

发挥部分（60分）
├─ Task 5: 识别+搬运（15分）- Task 4 + 搬运物品回停车区
├─ Task 6: 自主搬运（15分）- 自主找到对应物品搬回
├─ Task 7: 全自动搬运（25分）- 按顺序搬运所有物品
└─ Task 8: 创意任务（5分）- 其他创意功能
```

### 1.2 场地布局
```
停车启动区 ← 数字识别区
    ↓
物品存放区1 ~ 物品存放区5
（黑色引导线连接）
```

### 1.3 核心控制需求
- **循迹控制**：沿黑色引导线自主运动
- **方向控制**：基于陀螺仪 Yaw 角的闭环控制（已有 [`drv_chassis.h`](src/drivers/drv_chassis.h)）
- **位移控制**：基于运行时间的精确控制
- **状态管理**：多阶段任务的状态机管理

---

## 二、现有代码架构分析

### 2.1 底层驱动层（Driver Layer）
| 模块 | 功能 | 关键接口 |
|------|------|--------|
| [`drv_chassis.h`](src/drivers/drv_chassis.h) | 底盘运动控制 | `ChassisSetMotion()`, `ChassisUpdate()`, `ChassisStop()` |
| [`drv_motor.h`](src/drivers/drv_motor.h) | 电机PWM控制 | `MotorSetPWM()` |
| [`drv_jy61p.h`](src/drivers/drv_jy61p.h) | 陀螺仪数据 | 获取Yaw角 |
| [`drv_uart.h`](src/drivers/drv_uart.h) | UART通信 | 与OpenMV通信 |

### 2.2 应用层（App Layer）
| 模块 | 功能 | 状态 |
|------|------|------|
| [`app_chassis_task.h`](src/app/app_chassis_task.h) | 原子任务示例 | ✓ 已实现 |
| [`app_task_manager.h`](src/app/app_task_manager.h) | 任务管理器 | ✓ 已实现 |
| `app_task_1_line_tracking.h` | Task 1 | ✗ 待实现 |
| `app_task_2_dual_point.h` | Task 2 | ✗ 待实现 |
| ... | ... | ✗ 待实现 |

### 2.3 任务管理器接口
```c
typedef struct {
    TaskID_t task_id;
    const char *task_name;
    void (*init)(void);      // 初始化
    void (*run)(void);       // 主循环
    void (*stop)(void);      // 停止
    void (*reset)(void);     // 重置
    void (*get_state)(void); // 获取状态
    bool (*is_success)(void);// 判断成功
} Task_t;
```

---

## 三、任务函数设计规范

### 3.1 任务状态定义
```c
typedef enum {
    TASK_STATE_IDLE,      // 空闲
    TASK_STATE_INIT,      // 初始化
    TASK_STATE_RUNNING,   // 运行中
    TASK_STATE_PAUSED,    // 暂停
    TASK_STATE_SUCCESS,   // 成功完成
    TASK_STATE_FAILED,    // 失败
    TASK_STATE_TIMEOUT,   // 超时
} TaskState_t;
```

### 3.2 任务函数模板
每个任务需要实现以下接口：

```c
// 头文件：app_task_X_xxx.h
#ifndef APP_TASK_X_XXX_H
#define APP_TASK_X_XXX_H

#include <stdint.h>
#include <stdbool.h>

void TaskX_Init(void);
void TaskX_Run(void);
void TaskX_Stop(void);
void TaskX_Reset(void);
TaskState_t TaskX_GetState(void);
bool TaskX_IsSuccess(void);

#endif
```

```c
// 实现文件：app_task_X_xxx.c
#include "app_task_X_xxx.h"
#include "../drivers/drv_chassis.h"
#include "../utils/timer.h"

// 任务状态结构
typedef struct {
    TaskState_t state;
    uint32_t start_time;
    uint32_t elapsed_time;
    // 任务特定的状态变量
} TaskX_Context_t;

static TaskX_Context_t g_task_ctx;

void TaskX_Init(void) {
    g_task_ctx.state = TASK_STATE_INIT;
    g_task_ctx.start_time = 0;
    // 初始化任务特定的资源
}

void TaskX_Run(void) {
    // 状态机实现
    switch (g_task_ctx.state) {
        case TASK_STATE_INIT:
            // 初始化逻辑
            break;
        case TASK_STATE_RUNNING:
            // 运行逻辑
            break;
        // ...
    }
}

void TaskX_Stop(void) {
    ChassisStop();
    g_task_ctx.state = TASK_STATE_IDLE;
}

void TaskX_Reset(void) {
    g_task_ctx.state = TASK_STATE_IDLE;
    g_task_ctx.start_time = 0;
    g_task_ctx.elapsed_time = 0;
}

TaskState_t TaskX_GetState(void) {
    return g_task_ctx.state;
}

bool TaskX_IsSuccess(void) {
    return g_task_ctx.state == TASK_STATE_SUCCESS;
}
```

---

## 四、路径规划设计

### 4.1 路径点定义
```c
// 路径点结构
typedef struct {
    int16_t target_yaw;      // 目标偏航角（°×100）
    uint32_t duration_ms;    // 运动时间（ms）
    uint16_t base_pwm;       // 基础PWM占空比
    uint8_t direction;       // 运动方向
    uint32_t pause_ms;       // 到达后停顿时间（ms）
} PathPoint_t;

// 路径定义
typedef struct {
    const PathPoint_t *points;
    uint16_t point_count;
    uint16_t current_point;
    uint32_t point_start_time;
    TaskState_t state;
} Path_t;
```

### 4.2 场地坐标系统
```
初始方向：0°（面向目标区方向）
左转：正角度（+45°, +90°）
右转：负角度（-45°, -90°）

停车区 → 目标区1: 0°, 5000ms
停车区 → 目标区2: +45°, 5000ms
停车区 → 目标区3: +90°, 5000ms
停车区 → 目标区4: -45°, 5000ms
停车区 → 目标区5: -90°, 5000ms
```

### 4.3 路径规划算法
```c
// 基础路径规划函数
typedef struct {
    int16_t target_yaw;      // 目标区域的偏航角
    uint32_t forward_time;   // 前进时间
    uint32_t return_time;    // 返回时间
} PathPlan_t;

// 预定义的路径规划表
static const PathPlan_t g_path_plans[5] = {
    {0,     5000, 5000},     // 目标区1：直行
    {4500,  5000, 5000},     // 目标区2：左转45°
    {9000,  5000, 5000},     // 目标区3：左转90°
    {-4500, 5000, 5000},     // 目标区4：右转45°
    {-9000, 5000, 5000},     // 目标区5：右转90°
};
```

---

## 五、任务实现详细设计

### 5.1 Task 1: 单点循迹（10分）
**需求**：从停车区循迹到指定目标区，停止

**流程**：
1. 初始化：获取目标区编号（1-5）
2. 运行：按路径规划前进到目标区
3. 停止：到达后停车并提示

**状态机**：
```
IDLE → INIT → RUNNING → SUCCESS
```

**实现要点**：
- 使用 [`app_chassis_task.h`](src/app/app_chassis_task.h) 中的原子任务模式
- 修改角度参数即可适配不同目标区
- 超时判断：运动时间超过预期时间 20% 则失败

---

### 5.2 Task 2: 双点循迹（10分）
**需求**：循迹到目标1 → 停2s → 循迹到目标2

**流程**：
1. 初始化：获取两个目标区编号
2. 运行：
   - 阶段1：前进到目标1
   - 阶段2：停顿2s
   - 阶段3：转向180度
   - 阶段4：前进到目标2
3. 停止：到达目标2后停车

**状态机**：
```
IDLE → INIT → FORWARD_TO_TARGET1 → PAUSE_2S → TURN_180 → FORWARD_TO_TARGET2 → SUCCESS
```

注意：转180度后，需要前进到路径交汇点，这一段的目标角度需使用目标1角度与180度进行计算。

---

### 5.3 Task 3: 双点往返（10分）
**需求**：Task 2 + 返回停车区，总时间 30±2s

**流程**：
1. 执行 Task 2 的阶段
2. 加上返回停车区（-180°方向） 注意-180度方向，179与-179相近，在计算偏差时需特殊处理。
3. 时间控制：30±2s 内完成

**关键**：
- 需要精确的时间管理
- 可能需要调整运动速度（PWM）以满足时间要求
- 返回路径需要反向运动

---

### 5.4 Task 4: 数字识别（10分）
**需求**：识别数字标签，3s 内识别并提示

**流程**：
1. 初始化：启动摄像头（OpenMV）
2. 运行：等待识别结果
3. 提示：蜂鸣器或语音提示识别结果

**实现要点**：
- 与 [`app_camera_uart.h`](src/app/app_camera_uart.h) 集成
- 3s 超时判断
- 识别结果验证

---

### 5.5 Task 5-8: 发挥部分
**Task 5**：Task 4 + 搬运物品回停车区
**Task 6**：自主找到对应物品搬回
**Task 7**：按顺序搬运所有物品
**Task 8**：创意任务

这些任务需要额外的硬件支持（抓取装置）和算法（物品检测、位置记忆）。

---

## 六、实现步骤

### 第一阶段：基础框架
1. 创建 8 个任务文件框架（`.h` 和 `.c`）
2. 实现任务状态机基础结构
3. 集成到任务管理器

### 第二阶段：基本任务
1. 实现 Task 1：单点循迹
2. 实现 Task 2：双点循迹
3. 实现 Task 3：双点往返（时间控制）
4. 实现 Task 4：数字识别

### 第三阶段：发挥部分
1. 实现 Task 5-7：搬运相关任务
2. 实现 Task 8：创意任务

### 第四阶段：优化与测试
1. 性能优化：减少浮点运算，使用整型缩放
2. 鲁棒性测试：异常情况处理
3. 时间精度验证

---

## 七、关键技术点

### 7.1 陀螺仪角度控制
- 使用 [`drv_jy61p.h`](src/drivers/drv_jy61p.h) 获取 Yaw 角
- 基于 P 环控制差速，实现方向控制
- 死区设置：|error| < 0.5° 时不动作

### 7.2 时间精确控制
- 使用 `uwTick` 系统滴答计数
- 每个阶段记录开始时间
- 计算已运行时间，判断是否到达目标时间

### 7.3 状态机设计
- 每个任务独立的状态机
- 状态转移条件清晰
- 支持暂停、恢复、停止操作

### 7.4 路径规划
- 预定义路径表，支持快速查询
- 支持动态路径修改（通过修改角度参数）
- 支持多段路径组合

---

## 八、文件结构规划

```
src/app/
├── app_task_manager.c/h          ✓ 已有
├── app_chassis_task.c/h          ✓ 已有
├── app_task_1_line_tracking.c/h  ← 待创建
├── app_task_2_dual_point.c/h     ← 待创建
├── app_task_3_round_trip.c/h     ← 待创建
├── app_task_4_digit_recognition.c/h ← 待创建
├── app_task_5_recognize_transport.c/h ← 待创建
├── app_task_6_auto_transport.c/h ← 待创建
├── app_task_7_batch_transport.c/h ← 待创建
├── app_task_8_creative.c/h       ← 待创建
└── app_path_planner.c/h          ← 待创建（路径规划模块）
```

---

## 九、集成检查清单

- [ ] 所有 8 个任务文件创建完成
- [ ] 任务管理器正确引入所有任务头文件
- [ ] 任务函数指针正确注册到任务表
- [ ] 编译无错误
- [ ] 任务切换功能正常
- [ ] 单个任务执行正常
- [ ] 时间控制精度验证
- [ ] 异常处理完善

---

## 十、性能优化建议

### 10.1 计算优化
- 角度使用整型缩放（°×100）避免浮点运算
- PID 计算使用整型算术
- 预计算常用值

### 10.2 内存优化
- 使用静态分配，避免动态内存
- 共享缓冲区，减少栈使用
- 常量数据放在 Flash 中

### 10.3 功耗优化
- 未使用的外设及时关闭
- 中断优先级合理配置
- 主循环频率适当降低

---

## 十一、调试建议

### 11.1 UART 日志
```c
// 在关键位置添加日志
printf("Task %d: State=%d, Yaw=%d, Time=%dms\n", 
       task_id, state, current_yaw, elapsed_time);
```

### 11.2 OLED 显示
- 实时显示当前任务状态
- 显示陀螺仪数据
- 显示运动时间

### 11.3 XDS-110 调试
- 设置断点在状态转移处
- 观察变量变化
- 验证时间精度

---

## 十二、下一步行动

1. **确认方案**：用户审核并确认此方案
2. **创建框架**：生成 8 个任务文件框架
3. **实现 Task 1**：作为参考实现
4. **逐步完善**：按优先级实现其他任务
5. **集成测试**：验证任务管理器集成
6. **性能优化**：根据实际运行情况优化

