# 陀螺仪闭环控制方案设计文档

## 📋 需求概述

基于 **陀螺仪 Yaw 角 + 运行时间** 实现小车闭环控制，放弃编码器反馈。

### 核心指标
- **方向控制**：位置式 PID（Kp=150, Ki=20, Kd=50）
- **死区**：|error| < 50（0.5°）时不动作
- **位移控制**：通过 `uwTick` 计算运行时间
- **差速控制**：左电机 PWM - PID输出，右电机 PWM + PID输出

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────┐
│         Main Loop (empty.c)                         │
├─────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────┐   │
│  │  Chassis_Control_Proc()                      │   │
│  │  ├─ 读取陀螺仪 Yaw 角                        │   │
│  │  ├─ 计算运行时间 (uwTick)                    │   │
│  │  ├─ 执行 PID 计算                            │   │
│  │  └─ 更新电机 PWM（差速）                     │   │
│  └──────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────┤
│  Driver Layer (drv_chassis.c)                       │
│  ├─ ChassisInit()                                   │
│  ├─ ChassisSetMotion()  [设置目标偏航角、运行时间]  │
│  ├─ ChassisUpdate()     [主控制循环]                │
│  ├─ ChassisStop()       [停止运动]                  │
│  └─ ChassisGetState()   [获取当前状态]              │
├─────────────────────────────────────────────────────┤
│  Hardware Layer                                     │
│  ├─ Motor Driver (drv_motor.c)                      │
│  ├─ Gyro Driver (drv_jy61p.c)                       │
│  └─ PID Controller (utils/pid.c)                    │
└─────────────────────────────────────────────────────┘
```

---

## 📝 核心模块设计

### 1. 小车状态结构体 (`drv_chassis.h`)

```c
typedef struct {
    // 运动目标
    int16_t target_yaw;           // 目标偏航角（°×100）
    uint32_t motion_duration_ms;  // 运动持续时间（ms）
    uint16_t base_pwm;            // 基础 PWM 占空比
    
    // 运动状态
    uint8_t is_moving;            // 是否正在运动
    uint32_t motion_start_tick;   // 运动开始时刻（uwTick）
    
    // 反馈数据
    int16_t current_yaw;          // 当前偏航角（°×100）
    int16_t yaw_error;            // 偏航角误差
    int16_t pid_output;           // PID 输出（差速值）
    
    // 电机控制
    uint16_t motor_left_pwm;      // 左电机 PWM
    uint16_t motor_right_pwm;     // 右电机 PWM
    
    // PID 控制器
    PID_Position_t yaw_pid;       // 陀螺仪 Yaw 角 PID
} Chassis_t;
```

### 2. PID 计算流程

```
输入：当前 Yaw 角、目标 Yaw 角
  ↓
计算误差：error = target_yaw - current_yaw
  ↓
死区判断：if (|error| < 50) → output = 0，跳过 PID
  ↓
位置式 PID：
  u(k) = Kp*e(k) + Ki*∑e(i) + Kd[e(k) - e(k-1)]
  ↓
输出限幅：output = clamp(output, -PWM_MAX, PWM_MAX)
  ↓
差速控制：
  left_pwm  = base_pwm - output
  right_pwm = base_pwm + output
  ↓
输出：更新电机 PWM
```

### 3. 运动时间控制

```c
// 在 main 循环中
uint32_t elapsed_ms = (uwTick - motion_start_tick) * TICK_PERIOD_MS;

if (elapsed_ms >= motion_duration_ms) {
    ChassisStop();  // 停止运动
}
```

---

## 🔧 实现步骤

### Step 1: 扩展 `config.h`

添加陀螺仪 PID 参数和运动控制常量：

```c
/* 陀螺仪 Yaw 角 PID 参数 */
#define GYRO_YAW_PID_KP         150
#define GYRO_YAW_PID_KI         20
#define GYRO_YAW_PID_KD         50

/* 陀螺仪 PID 输出限幅 */
#define GYRO_PID_OUTPUT_MAX     300
#define GYRO_PID_OUTPUT_MIN     -300

/* 死区设置 */
#define GYRO_DEADZONE           50    // 0.5°

/* 运动控制参数 */
#define CHASSIS_TICK_PERIOD_MS  1     // 系统滴答周期（ms）
```

### Step 2: 创建 `drv_chassis.h`

定义小车控制接口：

```c
void ChassisInit(void);
void ChassisSetMotion(int16_t target_yaw, uint32_t duration_ms, uint16_t base_pwm);
void ChassisUpdate(void);
void ChassisStop(void);
Chassis_t* ChassisGetState(void);
```

### Step 3: 实现 `drv_chassis.c`

核心控制逻辑：

- `ChassisInit()`：初始化 PID、状态变量
- `ChassisSetMotion()`：设置运动目标
- `ChassisUpdate()`：主控制循环（PID 计算、差速控制）
- `ChassisStop()`：停止运动

### Step 4: 集成到 `empty.c`

在主循环中调用 `ChassisUpdate()`：

```c
while (1) {
    ChassisUpdate();  // 执行闭环控制
    // 其他处理...
}
```

---

## 🎯 关键设计点

### 1. 死区机制

```c
if (abs(yaw_error) < GYRO_DEADZONE) {
    pid_output = 0;  // 不动作
} else {
    pid_output = PID_Position_Calculate(&yaw_pid, target_yaw, current_yaw);
}
```

### 2. 差速控制

```c
// 当需要向右转时，PID 输出为正
// 左电机减速，右电机加速
left_pwm  = base_pwm - pid_output;
right_pwm = base_pwm + pid_output;

// 限幅处理
left_pwm  = clamp(left_pwm, 0, PWM_MAX);
right_pwm = clamp(right_pwm, 0, PWM_MAX);
```

### 3. 运动时间控制

```c
uint32_t elapsed_ms = (uwTick - motion_start_tick) * CHASSIS_TICK_PERIOD_MS;

if (is_moving && elapsed_ms >= motion_duration_ms) {
    ChassisStop();
}
```

### 4. 陀螺仪数据更新

```c
// 在 ChassisUpdate() 中
jy61p_angle_t angle;
if (jy61p_get_angle(&angle) == 0) {
    current_yaw = angle.yaw_deg_x100;  // 更新当前 Yaw 角
}
```

---

## 📊 状态转移图

```
    ┌─────────────┐
    │   IDLE      │
    │ (停止状态)   │
    └──────┬──────┘
           │ ChassisSetMotion()
           ↓
    ┌─────────────────────┐
    │   MOVING            │
    │ (运动中)             │
    │ - PID 闭环控制       │
    │ - 差速调整方向       │
    │ - 监测运行时间       │
    └──────┬──────────────┘
           │ elapsed_ms >= duration_ms
           ↓
    ┌─────────────┐
    │   IDLE      │
    │ (停止状态)   │
    └─────────────┘
```

---

## ✅ 验证清单

- [ ] `config.h` 添加陀螺仪 PID 参数
- [ ] `drv_chassis.h` 定义接口和数据结构
- [ ] `drv_chassis.c` 实现核心控制逻辑
- [ ] `empty.c` 集成 `ChassisUpdate()` 到主循环
- [ ] 编译通过，无错误和警告
- [ ] 陀螺仪数据正确读取
- [ ] PID 计算逻辑正确
- [ ] 差速控制有效
- [ ] 运动时间控制准确

---

## 🚀 使用示例

```c
// 初始化
ChassisInit();

// 设置运动：目标偏航角 0°，运动 2000ms，基础 PWM 500
ChassisSetMotion(0, 2000, 500);

// 主循环中持续调用
while (1) {
    ChassisUpdate();
}

// 获取当前状态
Chassis_t *state = ChassisGetState();
printf("Yaw: %d, Error: %d, PID Output: %d\n", 
       state->current_yaw, state->yaw_error, state->pid_output);
```

---

## 📌 注意事项

1. **陀螺仪校准**：确保在运动前完成 Yaw 角零点校准
2. **PID 参数调试**：根据实际小车响应调整 Kp/Ki/Kd
3. **死区设置**：防止小车在目标附近抖动
4. **电机限幅**：确保 PWM 不超过最大值
5. **时间精度**：`uwTick` 的周期决定了运动时间的精度
