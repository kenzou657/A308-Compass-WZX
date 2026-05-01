# 小车底盘闭环控制使用指南

## 📋 概述

本文档介绍如何使用基于**陀螺仪 Yaw 角 + 运行时间**的小车底盘闭环控制系统。

---

## 🎯 核心特性

- ✅ **方向控制**：位置式 PID 闭环控制，保持目标偏航角
- ✅ **位移控制**：基于 `uwTick` 的精确时间控制
- ✅ **差速转向**：自动调整左右电机 PWM 实现转向
- ✅ **死区机制**：误差 < 0.5° 时不动作，防止抖动
- ✅ **自动停止**：运行时间到达后自动停止

---

## 🚀 快速开始

### 1. 基本使用示例

```c
// 在 main 函数中已自动初始化
// ChassisInit();  // 已在 main() 中调用

// 等待陀螺仪校准完成
while (!jy61p_is_calibration_done()) {
    // 等待校准
}

// 设置小车运动：目标偏航角 0°，运动 2000ms，基础 PWM 500，前进
ChassisSetMotion(0, 2000, 500, CHASSIS_DIR_FORWARD);

// 主循环会自动执行闭环控制
// ChassisUpdate() 已在 Chassis_Control_Proc() 中周期性调用
```

### 2. 直线前进（保持 0° 方向）

```c
// 以 PWM 500 前进 3000ms，保持偏航角 0°
ChassisSetMotion(0, 3000, 500, CHASSIS_DIR_FORWARD);
```

### 3. 转向后前进（保持 90° 方向）

```c
// 先转向到 90°（向右转）
ChassisSetMotion(9000, 2000, 400, CHASSIS_DIR_FORWARD);

// 等待转向完成
while (ChassisIsMoving()) {
    // 等待
}

// 保持 90° 方向前进
ChassisSetMotion(9000, 3000, 500, CHASSIS_DIR_FORWARD);
```

### 4. 后退

```c
// 以 PWM 400 后退 2000ms，保持偏航角 0°
ChassisSetMotion(0, 2000, 400, CHASSIS_DIR_BACKWARD);
```

### 5. 手动停止

```c
// 在任何时候都可以手动停止
ChassisStop();
```

---

## 📊 API 参考

### ChassisInit()

**功能**：初始化小车底盘控制模块

**调用时机**：在 `main()` 函数中，`SYSCFG_DL_init()` 和 `MotorInit()` 之后

**示例**：
```c
ChassisInit();
```

---

### ChassisSetMotion()

**功能**：设置小车运动参数并开始运动

**原型**：
```c
void ChassisSetMotion(int16_t target_yaw, uint32_t duration_ms, 
                      uint16_t base_pwm, uint8_t direction);
```

**参数**：
- `target_yaw`：目标偏航角（°×100）
  - 例如：`0` = 0°，`9000` = 90°，`18000` = 180°，`-9000` = -90°
- `duration_ms`：运动持续时间（毫秒）
- `base_pwm`：基础 PWM 占空比（0-700）
  - 推荐范围：400-600
- `direction`：运动方向
  - `CHASSIS_DIR_FORWARD`：前进
  - `CHASSIS_DIR_BACKWARD`：后退

**示例**：
```c
// 以 PWM 500 前进 2000ms，保持偏航角 0°
ChassisSetMotion(0, 2000, 500, CHASSIS_DIR_FORWARD);

// 以 PWM 450 前进 3000ms，保持偏航角 90°（向右）
ChassisSetMotion(9000, 3000, 450, CHASSIS_DIR_FORWARD);

// 以 PWM 400 后退 1500ms，保持偏航角 0°
ChassisSetMotion(0, 1500, 400, CHASSIS_DIR_BACKWARD);
```

---

### ChassisUpdate()

**功能**：小车底盘控制主循环（自动调用）

**调用时机**：在 `Chassis_Control_Proc()` 中每 10ms 自动调用

**说明**：用户无需手动调用，系统会自动执行

---

### ChassisStop()

**功能**：停止小车运动

**示例**：
```c
ChassisStop();
```

---

### ChassisIsMoving()

**功能**：检查小车是否正在运动

**返回值**：
- `1`：正在运动
- `0`：停止状态

**示例**：
```c
if (ChassisIsMoving()) {
    // 小车正在运动
} else {
    // 小车已停止
}

// 等待运动完成
while (ChassisIsMoving()) {
    // 等待
}
```

---

### ChassisGetState()

**功能**：获取小车当前状态（用于调试）

**返回值**：`Chassis_t*` 指针

**示例**：
```c
Chassis_t *state = ChassisGetState();

printf("当前 Yaw 角: %d (0.01°)\n", state->current_yaw);
printf("目标 Yaw 角: %d (0.01°)\n", state->target_yaw);
printf("偏航角误差: %d (0.01°)\n", state->yaw_error);
printf("PID 输出: %d\n", state->pid_output);
printf("左电机 PWM: %d\n", state->motor_left_pwm);
printf("右电机 PWM: %d\n", state->motor_right_pwm);
printf("已运行时间: %lu ms\n", state->elapsed_ms);
```

---

### ChassisResetPID()

**功能**：重置陀螺仪 PID 控制器

**调用时机**：切换运动模式或出现异常时

**示例**：
```c
ChassisResetPID();
```

---

## 🎮 实战场景

### 场景 1：直线前进 3 米

假设小车速度约为 0.5 m/s，需要运行 6000ms

```c
// 等待陀螺仪校准完成
while (!jy61p_is_calibration_done()) {}

// 直线前进 6000ms
ChassisSetMotion(0, 6000, 500, CHASSIS_DIR_FORWARD);

// 等待运动完成
while (ChassisIsMoving()) {}
```

### 场景 2：90° 转向 + 前进

```c
// 1. 原地转向到 90°（向右转）
ChassisSetMotion(9000, 2000, 350, CHASSIS_DIR_FORWARD);
while (ChassisIsMoving()) {}

// 2. 保持 90° 方向前进
ChassisSetMotion(9000, 3000, 500, CHASSIS_DIR_FORWARD);
while (ChassisIsMoving()) {}
```

### 场景 3：S 型路径

```c
// 1. 直线前进
ChassisSetMotion(0, 2000, 500, CHASSIS_DIR_FORWARD);
while (ChassisIsMoving()) {}

// 2. 向右转 45°
ChassisSetMotion(4500, 1500, 400, CHASSIS_DIR_FORWARD);
while (ChassisIsMoving()) {}

// 3. 保持 45° 前进
ChassisSetMotion(4500, 2000, 500, CHASSIS_DIR_FORWARD);
while (ChassisIsMoving()) {}

// 4. 向左转回 0°
ChassisSetMotion(0, 1500, 400, CHASSIS_DIR_FORWARD);
while (ChassisIsMoving()) {}

// 5. 直线前进
ChassisSetMotion(0, 2000, 500, CHASSIS_DIR_FORWARD);
while (ChassisIsMoving()) {}
```

### 场景 4：紧急停止

```c
// 开始运动
ChassisSetMotion(0, 5000, 500, CHASSIS_DIR_FORWARD);

// 检测到障碍物，紧急停止
if (obstacle_detected) {
    ChassisStop();
}
```

---

## ⚙️ 参数调整

### PID 参数调整（在 `config.h` 中）

```c
// 陀螺仪 Yaw 角 PID 参数
#define GYRO_YAW_PID_KP             150   // 比例系数
#define GYRO_YAW_PID_KI             20    // 积分系数
#define GYRO_YAW_PID_KD             50    // 微分系数
```

**调试建议**：
1. **Kp 过大**：小车会震荡、抖动
2. **Kp 过小**：响应慢，转向不足
3. **Ki 过大**：容易超调、振荡
4. **Ki 过小**：稳态误差大
5. **Kd 过大**：对噪声敏感，抖动
6. **Kd 过小**：超调明显

### 死区调整

```c
#define GYRO_DEADZONE               50    // 0.5°
```

- 死区过大：响应迟钝
- 死区过小：容易抖动

### PWM 范围调整

```c
#define CHASSIS_DEFAULT_BASE_PWM    500   // 默认速度
#define CHASSIS_MAX_BASE_PWM        700   // 最大速度
```

---

## 🐛 调试技巧

### 1. 查看实时状态

在 `Chassis_Control_Proc()` 中已集成调试信息输出（每 1 秒打印一次）：

```
Chassis: Yaw=0, Err=5, PID=15, L=485, R=515
```

- `Yaw`：当前偏航角（°×100）
- `Err`：偏航角误差（°×100）
- `PID`：PID 输出（差速值）
- `L`：左电机 PWM
- `R`：右电机 PWM

### 2. 检查陀螺仪数据

```c
Chassis_t *state = ChassisGetState();
if (state->gyro_data_valid) {
    // 陀螺仪数据有效
} else {
    // 陀螺仪数据无效，检查连接
}
```

### 3. 监测运行时间

```c
Chassis_t *state = ChassisGetState();
printf("已运行: %lu ms / %lu ms\n", 
       state->elapsed_ms, state->motion_duration_ms);
```

---

## ⚠️ 注意事项

1. **陀螺仪校准**：必须等待校准完成后再开始运动
   ```c
   while (!jy61p_is_calibration_done()) {}
   ```

2. **PWM 限制**：基础 PWM 不要超过 700，否则会被自动限制

3. **时间精度**：运行时间精度取决于 `uwTick` 的周期（默认 1ms）

4. **死区设置**：误差 < 0.5° 时 PID 不动作，防止抖动

5. **差速限制**：PID 输出限制在 ±300，防止单侧电机过载

6. **方向定义**：
   - 0° = 初始方向（上电时的方向）
   - 正值 = 向右转
   - 负值 = 向左转

---

## 📈 性能指标

- **控制周期**：10ms
- **PID 计算频率**：100Hz
- **时间精度**：±1ms
- **角度精度**：±0.5°（死区）
- **PWM 范围**：0-700
- **差速范围**：±300

---

## 🔧 故障排查

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| 小车不动 | 陀螺仪未校准 | 等待校准完成 |
| 小车抖动 | PID 参数不合适 | 降低 Kp 或增大死区 |
| 转向不足 | Kp 过小 | 增大 Kp |
| 转向过度 | Kp 过大 | 降低 Kp |
| 稳态误差大 | Ki 过小 | 增大 Ki |
| 振荡 | Ki 过大 | 降低 Ki |
| 陀螺仪数据无效 | 连接问题 | 检查 UART1 连接 |

---

## 📚 相关文档

- [`plans/Gyro_Closed_Loop_Control_Plan.md`](../plans/Gyro_Closed_Loop_Control_Plan.md) - 详细设计方案
- [`src/drivers/drv_chassis.h`](../src/drivers/drv_chassis.h) - API 接口定义
- [`src/drivers/drv_chassis.c`](../src/drivers/drv_chassis.c) - 核心实现
- [`src/config.h`](../src/config.h) - 参数配置

---

## ✅ 总结

本闭环控制系统提供了简单易用的 API，支持：
- ✅ 精确的方向控制（基于陀螺仪 Yaw 角）
- ✅ 精确的时间控制（基于 uwTick）
- ✅ 自动差速转向
- ✅ 死区防抖
- ✅ 自动停止

只需调用 `ChassisSetMotion()` 即可实现复杂的运动控制。
