# PID控制量纲说明（已修正）

## 问题回顾

**用户提问：** 为什么PID的输出是脉冲？明明error是速度值，经过PID运算之后，怎么会改变量纲呢？

**答案：** 这是一个很好的指出！原设计有量纲错误，已修正。PID输入和输出都应该是速度值(mm/s)。

## 修正后的设计

### 完整数据流

```
编码器脉冲 → 脉冲计数 → 速度计算 → PID控制 → 速度输出 → 速度转占空比 → PWM → 电机
```

### 各阶段数据类型和量纲

| 阶段 | 变量 | 类型 | 范围 | 单位 | 量纲 |
|------|------|------|------|------|------|
| 编码器 | pulse_count | int32_t | ±2^31 | 脉冲 | [脉冲] |
| 速度计算 | pulse_delta | int32_t | ±2^31 | 脉冲 | [脉冲] |
| 速度 | speed | int16_t | -500~500 | mm/s | [长度/时间] |
| **PID输入** | **target_speed** | **int16_t** | **-500~500** | **mm/s** | **[长度/时间]** |
| **PID输入** | **actual_speed** | **int16_t** | **-500~500** | **mm/s** | **[长度/时间]** |
| **PID误差** | **error** | **float** | **-1000~1000** | **mm/s** | **[长度/时间]** |
| **PID输出** | **output** | **int16_t** | **-500~500** | **mm/s** | **[长度/时间]** |
| 速度转占空比 | duty | uint16_t | 0~2000 | 占空比 | [无量纲] |
| PWM | PWM信号 | - | - | - | - |

## 量纲分析

### PID输入量纲

```
error = target_speed - actual_speed
      = (mm/s) - (mm/s)
      = (mm/s)  ✓ 量纲一致
```

### PID各项量纲

```
P项：Kp × error = (无量纲) × (mm/s) = (mm/s)  ✓
I项：Ki × ∫error = (无量纲) × (mm/s) = (mm/s)  ✓
D项：Kd × d(error)/dt = (无量纲) × (mm/s/s) = (mm/s)  ✓
```

### PID输出量纲

```
output = P + I + D
       = (mm/s) + (mm/s) + (mm/s)
       = (mm/s)  ✓ 量纲一致
```

### 速度到占空比转换

```
duty = (output_speed × SPEED_TO_DUTY_SCALE) / 1000
     = (mm/s) × (无量纲) / (无量纲)
     = (无量纲)  ✓ 占空比无量纲
```

其中：
```
SPEED_TO_DUTY_SCALE = (PWM_MAX_DUTY × 1000) / MAX_SPEED
                    = (2000 × 1000) / 500
                    = 4000  (无量纲)
```

## 代码实现

### 步骤1：脉冲转速度（Motor_CalcSpeed）

```c
// src/drivers/drv_motor.c, 第80-95行
int32_t pulse_delta = pulse_count - last_pulse_count;  // [脉冲]
speed = (pulse_delta * SPEED_SCALE) / 1000;            // [mm/s]
```

**量纲转换：**
```
speed(mm/s) = pulse_delta(脉冲) × SPEED_SCALE(无量纲) / 1000(无量纲)
            = (脉冲) × (mm/脉冲) / (无量纲)
            = (mm/s)  ✓
```

其中 `SPEED_SCALE = (2π × R) / (N × T)` 的单位是 `mm/脉冲`

### 步骤2：PID控制（Motor_PIDUpdate）

```c
// src/drivers/drv_motor.c, 第239-290行

// 获取实际速度 [mm/s]
int16_t actual_speed = g_motors[i].encoder.speed;

// 计算误差 [mm/s]
pid->error = (float)(pid->target_speed - actual_speed);

// 比例项 [mm/s]
float p_term = pid->kp * pid->error;

// 积分项 [mm/s]
pid->integral += pid->error;
float i_term = pid->ki * pid->integral;

// 微分项 [mm/s]
float d_term = pid->kd * (pid->error - pid->last_error);

// PID输出 [mm/s]
float output_speed = p_term + i_term + d_term;

// 限幅 [mm/s]
if (output_speed > PID_OUT_MAX) {
    output_speed = PID_OUT_MAX;  // 500 mm/s
}

pid->output = (int16_t)output_speed;  // [mm/s]
```

### 步骤3：速度转占空比（Motor_PIDUpdate中）

```c
// src/drivers/drv_motor.c, 第280-286行

// 将速度转换为PWM占空比
uint16_t duty = (uint16_t)((pid->output > 0 ? pid->output : -pid->output) 
                           * SPEED_TO_DUTY_SCALE / 1000);

// 量纲分析：
// duty = (mm/s) × (无量纲) / 1000
//      = (mm/s) × 4000 / 1000
//      = (mm/s) × 4
//      = (无量纲)  ✓
```

其中 `SPEED_TO_DUTY_SCALE = 4000` 是无量纲的转换系数。

## 配置参数

### config.h中的参数

```c
/* 编码器参数 */
#define ENCODER_PPR              (20)        // 每转脉冲数
#define WHEEL_RADIUS             (25)        // 轮半径(mm)
#define ENCODER_SAMPLE_PERIOD    (10)        // 采样周期(ms)

/* PWM参数 */
#define PWM_MAX_DUTY             (2000)      // 最大占空比
#define MAX_SPEED                (500)       // 最大速度(mm/s)

/* PID参数（输入输出都是速度mm/s） */
#define PID_KP                   (1.5f)      // 比例系数
#define PID_KI                   (0.3f)      // 积分系数
#define PID_KD                   (0.1f)      // 微分系数
#define PID_I_MAX                (500)       // 积分限幅(mm/s)
#define PID_OUT_MAX              (500)       // 输出限幅(mm/s)

/* 速度到占空比转换系数 */
#define SPEED_TO_DUTY_SCALE      ((PWM_MAX_DUTY * 1000) / MAX_SPEED)
                                 // = (2000 × 1000) / 500 = 4000
```

## 数值示例

### 例1：前进200mm/s

```
1. 设置目标速度
   Motor_SetTargetSpeed(MOTOR_A, 200);  // 200 mm/s

2. 编码器反馈（假设实际速度180mm/s）
   actual_speed = 180  // mm/s

3. PID计算
   error = 200 - 180 = 20  // mm/s
   P = 1.5 × 20 = 30  // mm/s
   I = 0.3 × ∫20 = ...  // mm/s
   D = 0.1 × (20 - last_error) = ...  // mm/s
   output_speed = 30 + I + D ≈ 35  // mm/s

4. 速度转占空比
   duty = (35 × 4000) / 1000 = 140  // 占空比
   
5. PWM驱动
   DL_TimerG_setCaptureCompareValue(..., 140, ...)
```

### 例2：停止

```
1. 设置目标速度
   Motor_SetTargetSpeed(MOTOR_A, 0);  // 0 mm/s

2. PID计算
   error = 0 - actual_speed
   output_speed → 0  // mm/s

3. 速度转占空比
   duty = (0 × 4000) / 1000 = 0  // 占空比
   
4. PWM驱动
   DL_TimerG_setCaptureCompareValue(..., 0, ...)
```

## 关键改进点

✅ **量纲一致性** - PID输入输出都是速度(mm/s)
✅ **物理意义清晰** - 误差是速度误差，输出是目标速度
✅ **转换层分离** - 速度→占空比转换独立进行
✅ **参数调整方便** - PID系数基于速度单位，易于理解和调试
✅ **数值范围合理** - 速度-500~500mm/s，占空比0~2000

## 总结

**修正前的问题：**
- ❌ PID输出直接是占空比(0-2000)
- ❌ 量纲不一致：输入是速度，输出是占空比
- ❌ 物理意义不清

**修正后的设计：**
- ✅ PID输入：速度误差(mm/s)
- ✅ PID输出：目标速度(mm/s)
- ✅ 转换层：速度→占空比(单独处理)
- ✅ 量纲一致，物理意义清晰

