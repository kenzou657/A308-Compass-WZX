# 电机驱动设计方案

## 1. 硬件资源总结

### PWM电机控制（TIMG0）
- **时钟频率**：40MHz
- **PWM周期**：2000（对应50kHz）
- **CH0（PA12）**：电机A PWM输出
- **CH1（PA13）**：电机B PWM输出
- **方向控制引脚**：
  - PA0: AIN1（电机A方向1）
  - PA1: AIN2（电机A方向2）
  - PA2: BIN1（电机B方向1）
  - PA3: BIN2（电机B方向2）

### 编码器采集
- **电机A编码器**：
  - E1A（PA23）：TIMG7捕获上升沿，时钟10MHz
  - E1B（PA24）：GPIO读取（判断方向）
  
- **电机B编码器**：
  - E2A（PA21）：TIMA捕获上升沿，时钟10MHz
  - E2B（PA22）：GPIO读取（判断方向）

- **计时器CLOCK**：
  - 频率：100Hz（10ms周期）
  - 用途：编码器计数周期、PID控制周期

## 2. 运动学模型

### 脉冲转速度
```
v = (脉冲数 × 2π × R) / (N × T)

其中：
- 脉冲数：采样周期内的编码器脉冲数
- R：轮半径（mm）
- N：编码器分辨率（每转脉冲数）
- T：采样周期（ms）
```

### 转向角速度
```
ω = (v_left - v_right) / L

其中：
- v_left, v_right：左右轮速度（mm/s）
- L：轮距（mm）
```

## 3. 驱动架构

```
┌─────────────────────────────────────────┐
│         应用层 (App)                     │
│  - Motor_MoveForward(speed)             │
│  - Motor_MoveBackward(speed)            │
│  - Motor_Turn(left_speed, right_speed)  │
└────────────┬────────────────────────────┘
             │
┌────────────▼────────────────────────────┐
│         驱动层 (Driver)                  │
│  ┌──────────────────────────────────┐   │
│  │ 编码器模块                       │   │
│  │ - ISR：脉冲计数、方向判断        │   │
│  │ - Main：速度计算                 │   │
│  └──────────────────────────────────┘   │
│  ┌──────────────────────────────────┐   │
│  │ PWM控制模块                      │   │
│  │ - 占空比更新（0-2000）           │   │
│  │ - 方向控制（GPIO）               │   │
│  └──────────────────────────────────┘   │
│  ┌──────────────────────────────────┐   │
│  │ PID控制模块                      │   │
│  │ - 位置式PID                      │   │
│  │ - 积分限幅、微分滤波              │   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

## 4. 核心函数设计

### 初始化函数
```c
void Motor_Init(void);                    // 初始化所有电机模块
void Motor_EncoderInit(void);             // 初始化编码器和CLOCK定时器
void Motor_PIDInit(void);                 // 初始化PID参数
```

### 编码器函数
```c
int32_t Motor_GetEncoderCount(uint8_t motor_id);  // 获取脉冲计数
void Motor_CalcSpeed(void);               // 计算实时速度（CLOCK中断调用）
void Motor_ResetEncoder(uint8_t motor_id); // 重置脉冲计数
```

### PWM控制函数
```c
void Motor_SetDuty(uint8_t motor_id, uint16_t duty);  // 设置占空比(0-2000)
void Motor_SetDirection(uint8_t motor_id, int8_t dir); // 设置方向(-1/0/1)
```

### PID控制函数
```c
void Motor_SetTargetSpeed(uint8_t motor_id, int16_t speed); // 设置目标速度
void Motor_PIDUpdate(void);               // 更新PID控制（CLOCK中断调用）
```

### 运动控制函数
```c
void Motor_MoveForward(int16_t speed);    // 前进
void Motor_MoveBackward(int16_t speed);   // 后退
void Motor_Turn(int16_t left_speed, int16_t right_speed); // 转弯
void Motor_Stop(void);                    // 停止
```

## 5. 中断处理流程

### ENCODER1A/2A中断（捕获上升沿）
```
1. 读取对应B相GPIO电平
2. 判断方向（A相上升沿时B相电平）
3. 更新脉冲计数（+1或-1）
4. 清除中断标志
```

### CLOCK中断（100Hz）
```
1. 计算两个电机的实时速度
   - 脉冲数 = 当前计数 - 上次计数
   - 速度 = (脉冲数 × 2π × R) / (N × T)
2. 执行PID控制算法
   - error = target_speed - actual_speed
   - 计算P、I、D项
   - 输出 = P + I + D
3. 更新PWM占空比
4. 重置脉冲计数器
```

## 6. 配置参数（config.h）

### 编码器参数
```c
#define ENCODER_PPR              (20)        // 每转脉冲数
#define WHEEL_RADIUS             (25)        // 轮半径(mm)
#define WHEEL_DISTANCE           (120)       // 轮距(mm)
#define ENCODER_SAMPLE_PERIOD    (10)        // 采样周期(ms)
```

### PWM参数
```c
#define PWM_PERIOD               (2000)      // PWM周期
#define PWM_FREQ                 (40000000)  // PWM时钟频率
#define PWM_MAX_DUTY             (2000)      // 最大占空比
```

### PID参数
```c
#define PID_KP                   (1.5f)      // 比例系数
#define PID_KI                   (0.3f)      // 积分系数
#define PID_KD                   (0.1f)      // 微分系数
#define PID_I_MAX                (500)       // 积分限幅
#define PID_OUT_MAX              (2000)      // 输出限幅
```

### 速度限制
```c
#define MAX_SPEED                (500)       // 最大速度(mm/s)
#define MIN_SPEED                (50)        // 最小速度(mm/s)
```

### 电机ID定义
```c
#define MOTOR_A                  (0)
#define MOTOR_B                  (1)
#define MOTOR_COUNT              (2)
```

## 7. 数据结构设计

### 编码器数据
```c
typedef struct {
    int32_t pulse_count;        // 脉冲计数
    int32_t last_pulse_count;   // 上次脉冲计数
    int16_t speed;              // 实时速度(mm/s)
} Encoder_t;
```

### PID控制器
```c
typedef struct {
    float kp, ki, kd;           // PID系数
    float error;                // 当前误差
    float last_error;           // 上次误差
    float integral;             // 积分项
    int16_t target_speed;       // 目标速度
    int16_t output;             // 控制输出
} PID_t;
```

### 电机控制结构
```c
typedef struct {
    Encoder_t encoder;          // 编码器数据
    PID_t pid;                  // PID控制器
    uint16_t pwm_duty;          // PWM占空比
    int8_t direction;           // 方向(-1/0/1)
} Motor_t;
```

## 8. 设计特点

✅ **ISR最小化**：中断仅做计数和标志设置，复杂计算在主循环
✅ **驱动层解耦**：驱动层无业务逻辑，仅提供硬件抽象
✅ **闭环控制**：支持速度反馈和PID调节
✅ **转向支持**：支持差速转向控制
✅ **参数集中**：所有配置参数在config.h中，便于调试
✅ **方向判断**：通过B相电平判断编码器方向，支持正反转

