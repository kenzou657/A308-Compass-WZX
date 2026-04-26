# 电机驱动模块集成指南

## 1. 文件结构

```
src/
├── config.h                    # 全局配置（编码器参数、PID参数等）
├── drivers/
│   ├── drv_motor.h            # 电机驱动头文件
│   ├── drv_motor.c            # 电机驱动实现
│   ├── drv_led.h
│   ├── drv_led.c
│   ├── drv_buzzer.h
│   └── drv_buzzer.c
├── isr/
│   └── isr_motor.c            # 电机中断处理
├── app/
│   └── app_motor_example.c    # 使用示例
└── utils/
    ├── timer.h
    └── timer.c
```

## 2. 集成步骤

### 步骤1：在empty.c中包含头文件

```c
#include "drivers/drv_motor.h"
```

### 步骤2：在SYSCFG_DL_init()后初始化电机驱动

```c
int main(void)
{
    SYSCFG_DL_init();
    
    // 初始化电机驱动
    Motor_Init();
    
    while (1) {
        // 应用代码
    }
}
```

### 步骤3：在Keil项目中添加源文件

在Keil µVision中：
1. 右键点击Project → Manage Project Items
2. 添加以下文件到项目：
   - `src/drivers/drv_motor.c`
   - `src/isr/isr_motor.c`

### 步骤4：验证SysConfig配置

确保SysConfig中已配置以下外设：
- **PWM_MOTOR (TIMG0)**：
  - CH0 (PA12)、CH1 (PA13)
  - 周期：2000
  - 启动定时器

- **ENCODER1A (TIMG7)**：
  - 捕获模式，上升沿
  - 输入通道0 (PA23)
  - 启用中断 (CC0_DN_EVENT)

- **ENCODER2A (TIMA)**：
  - 捕获模式，上升沿
  - 输入通道0 (PA21)
  - 启用中断 (CC0_DN_EVENT)

- **CLOCK (TIMA)**：
  - 定时器模式，周期模式
  - 频率：100Hz（周期10ms）
  - 启用中断 (ZERO_EVENT)

- **GPIO**：
  - 方向控制：PA0, PA1, PA2, PA3（输出）
  - 编码器B相：PA24, PA22（输入）

## 3. 使用示例

### 基本运动控制

```c
// 前进：200mm/s
Motor_MoveForward(200);

// 后退：150mm/s
Motor_MoveBackward(150);

// 转弯：左轮200mm/s，右轮100mm/s
Motor_Turn(200, 100);

// 停止
Motor_Stop();
```

### 获取速度反馈

```c
// 获取实时速度
int16_t speed_a = Motor_GetSpeed(MOTOR_A);
int16_t speed_b = Motor_GetSpeed(MOTOR_B);

// 获取脉冲计数
int32_t pulse_a = Motor_GetEncoderCount(MOTOR_A);
int32_t pulse_b = Motor_GetEncoderCount(MOTOR_B);
```

### 直接PWM控制

```c
// 禁用PID
Motor_DisablePID(MOTOR_A);
Motor_DisablePID(MOTOR_B);

// 设置方向
Motor_SetDirection(MOTOR_A, MOTOR_DIR_FORWARD);
Motor_SetDirection(MOTOR_B, MOTOR_DIR_FORWARD);

// 设置占空比（0-2000）
Motor_SetDuty(MOTOR_A, 1000);  // 50%
Motor_SetDuty(MOTOR_B, 1000);  // 50%
```

## 4. 配置参数调整

所有参数在 [`src/config.h`](src/config.h) 中定义，可根据实际硬件调整：

### 编码器参数
```c
#define ENCODER_PPR              (20)        // 编码器分辨率
#define WHEEL_RADIUS             (25)        // 轮半径(mm)
#define WHEEL_DISTANCE           (120)       // 轮距(mm)
```

### PID参数
```c
#define PID_KP                   (1.5f)      // 比例系数
#define PID_KI                   (0.3f)      // 积分系数
#define PID_KD                   (0.1f)      // 微分系数
```

### 速度限制
```c
#define MAX_SPEED                (500)       // 最大速度(mm/s)
#define MIN_SPEED                (50)        // 最小速度(mm/s)
```

## 5. 中断处理流程

### ENCODER1A/2A中断（捕获上升沿）
```
1. 捕获编码器A相上升沿
2. 读取B相电平判断方向
3. 更新脉冲计数（+1或-1）
4. 清除中断标志
```

### CLOCK中断（100Hz）
```
1. 计算电机实时速度
   speed = (pulse_delta × SPEED_SCALE) / 1000
2. 执行PID控制
   error = target_speed - actual_speed
   output = Kp×error + Ki×∫error + Kd×d(error)/dt
3. 更新PWM占空比
4. 重置脉冲计数器
```

## 6. 调试建议

### 编码器调试
```c
// 打印脉冲计数和速度
int32_t pulse = Motor_GetEncoderCount(MOTOR_A);
int16_t speed = Motor_GetSpeed(MOTOR_A);
printf("Pulse: %d, Speed: %d mm/s\r\n", pulse, speed);
```

### PID调试
```c
// 设置目标速度并观察实际速度
Motor_SetTargetSpeed(MOTOR_A, 200);
int16_t target = Motor_GetTargetSpeed(MOTOR_A);
int16_t actual = Motor_GetSpeed(MOTOR_A);
printf("Target: %d, Actual: %d\r\n", target, actual);
```

### PWM调试
```c
// 直接设置占空比测试电机
Motor_SetDirection(MOTOR_A, MOTOR_DIR_FORWARD);
Motor_SetDuty(MOTOR_A, 1000);  // 50%
```

## 7. 常见问题

### Q1: 电机不转
- 检查方向控制GPIO是否正确设置
- 检查PWM占空比是否大于0
- 检查编码器中断是否正常工作

### Q2: 速度反馈不准确
- 检查编码器PPR参数是否正确
- 检查轮半径参数是否正确
- 检查CLOCK定时器频率是否为100Hz

### Q3: PID控制不稳定
- 调整PID系数（Kp、Ki、Kd）
- 检查积分限幅是否合理
- 检查输出限幅是否合理

### Q4: 转弯不直
- 检查两个电机的编码器分辨率是否一致
- 调整轮距参数
- 使用差速控制补偿电机差异

## 8. 性能指标

- **编码器采样频率**：100Hz（10ms周期）
- **PID更新频率**：100Hz
- **PWM频率**：50kHz（TIMG0时钟40MHz，周期2000）
- **最大速度**：500mm/s
- **最小速度**：50mm/s
- **速度精度**：±1mm/s（取决于编码器分辨率）

## 9. 扩展功能

### 添加加速度控制
```c
// 可在Motor_SetTargetSpeed()中添加加速度限制
// 避免电机突然加速导致打滑
```

### 添加转向角度控制
```c
// 可基于轮距和速度差计算转向角度
// ω = (v_left - v_right) / L
```

### 添加里程计
```c
// 可累积脉冲计数计算行驶距离
// distance = (pulse_count × 2π × R) / N
```

