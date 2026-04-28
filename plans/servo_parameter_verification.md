# 舵机PWM参数验证与计算

## 1. 配置参数验证

### SysConfig配置（来自 ti_msp_dl_config.c）

```c
// 时钟配置
static const DL_TimerA_ClockConfig gPWM_SERVOClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,      // 80MHz BUSCLK
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8, // ÷8
    .prescale = 249U                         // ÷250
};

// PWM配置
static const DL_TimerA_PWMConfig gPWM_SERVOConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = 800,                           // Period Count = 800
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_START,
};
```

### 时钟频率计算

```
MCLK = 80MHz
分频系数 = divideRatio × (prescale + 1)
        = 8 × (249 + 1)
        = 8 × 250
        = 2000

定时器时钟频率 = 80MHz / 2000 = 40kHz
```

### PWM频率验证

```
PWM周期 = Period Count / 定时器时钟频率
        = 800 / 40kHz
        = 20ms
        = 50Hz ✓ 正确

PWM频率 = 1 / 20ms = 50Hz ✓
```

## 2. 舵机脉宽与转角映射

### 舵机规格
- 转角范围：0° ~ 180°
- 脉宽范围：0.5ms ~ 2.5ms
- PWM周期：20ms（50Hz）

### 脉宽与计数值的转换

```
PWM周期 = 20ms，对应计数值 = 800

脉宽 0.5ms 对应的计数值：
  CC_value_0° = (0.5ms / 20ms) × 800 = 0.025 × 800 = 20

脉宽 2.5ms 对应的计数值：
  CC_value_180° = (2.5ms / 20ms) × 800 = 0.125 × 800 = 100

脉宽范围对应的计数值范围：[20, 100]
```

### 角度与计数值的线性映射

```
转角范围：0° ~ 180°
计数值范围：20 ~ 100

线性映射公式：
  angle(°) = (cc_value - 20) / (100 - 20) × 180
           = (cc_value - 20) / 80 × 180

反向映射公式（角度转计数值）：
  cc_value = angle(°) / 180 × 80 + 20
           = angle(°) × 80 / 180 + 20
           = angle(°) × 4 / 9 + 20
```

### 验证示例

```
角度 0°：
  cc_value = 0 × 4/9 + 20 = 20 ✓
  脉宽 = 20/800 × 20ms = 0.5ms ✓

角度 90°：
  cc_value = 90 × 4/9 + 20 = 40 + 20 = 60
  脉宽 = 60/800 × 20ms = 1.5ms ✓

角度 180°：
  cc_value = 180 × 4/9 + 20 = 80 + 20 = 100 ✓
  脉宽 = 100/800 × 20ms = 2.5ms ✓
```

## 3. 驱动函数设计

### 初始化函数
- 调用 `SYSCFG_DL_PWM_SERVO_init()` 初始化PWM
- 设置舵机初始位置（通常为90°中位）

### 控制函数
- `drv_servo_set_angle(uint16_t angle)` - 设置舵机转角
  - 输入：角度值（0~180）
  - 内部计算对应的CC值
  - 调用 `DL_TimerA_setCaptureCompareValue()` 更新PWM脉宽

### 关键参数
```c
#define SERVO_ANGLE_MIN         0       // 最小角度
#define SERVO_ANGLE_MAX         180     // 最大角度
#define SERVO_CC_MIN            20      // 0°对应的CC值
#define SERVO_CC_MAX            100     // 180°对应的CC值
#define SERVO_CC_RANGE          (SERVO_CC_MAX - SERVO_CC_MIN)  // 80
#define SERVO_ANGLE_RANGE       (SERVO_ANGLE_MAX - SERVO_ANGLE_MIN)  // 180
```

## 4. 结论

✓ PWM频率配置正确：50Hz
✓ Period Count正确：800
✓ 时钟分频配置正确
✓ 脉宽范围正确：0.5ms ~ 2.5ms
✓ 参数可用于180°舵机控制
