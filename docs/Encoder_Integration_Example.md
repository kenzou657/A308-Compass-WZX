# 编码器模块集成示例

## 📋 文件清单

已创建的文件：
- `src/config.h` - 添加了编码器配置宏
- `src/drivers/drv_encoder.h` - 编码器驱动接口
- `src/drivers/drv_encoder.c` - 编码器驱动实现
- `src/isr/isr_encoder.c` - 编码器中断服务函数

## 🔧 在 empty.c 中集成

### 1. 添加头文件引用

```c
#include "ti_msp_dl_config.h"
#include "src/drivers/drv_motor.h"
#include "src/drivers/drv_encoder.h"  // 添加编码器驱动
```

### 2. 初始化流程

```c
int main(void)
{
    // 1. SysConfig初始化（已包含E1A、E2A、CLOCK定时器配置）
    SYSCFG_DL_init();
    
    // 2. 使能全局中断
    NVIC_EnableIRQ(E1A_INST_INT_IRQN);   // 使能E1A中断（TIMG0）
    NVIC_EnableIRQ(E2A_INST_INT_IRQN);   // 使能E2A中断（TIMA1）
    NVIC_EnableIRQ(CLOCK_INST_INT_IRQN); // 使能CLOCK中断（TIMA0）
    
    // 3. 初始化电机驱动
    MotorInit();
    
    // 4. 初始化编码器驱动
    EncoderInit();
    
    // 5. 启动编码器采样
    EncoderStart();
    
    // 6. 使能全局中断
    __enable_irq();
    
    // 主循环
    while (1) {
        // 读取编码器速度
        int16_t speed_a = EncoderGetSpeed(ENCODER_A);  // mm/s
        int16_t speed_b = EncoderGetSpeed(ENCODER_B);  // mm/s
        
        // 使用速度进行PID控制或其他逻辑
        // ...
        
        delay_cycles(16000000);  // 延时约200ms
    }
}
```

### 3. 完整示例代码

```c
#include "ti_msp_dl_config.h"
#include "src/drivers/drv_motor.h"
#include "src/drivers/drv_encoder.h"

int main(void)
{
    // 系统初始化
    SYSCFG_DL_init();
    
    // 使能编码器相关中断
    NVIC_EnableIRQ(E1A_INST_INT_IRQN);   // TIMG0
    NVIC_EnableIRQ(E2A_INST_INT_IRQN);   // TIMA1
    NVIC_EnableIRQ(CLOCK_INST_INT_IRQN); // TIMA0
    
    // 初始化驱动模块
    MotorInit();
    EncoderInit();
    
    // 启动编码器采样
    EncoderStart();
    
    // 使能全局中断
    __enable_irq();
    
    // 测试：电机A以50%速度正转
    MotorASet(MOTOR_DIR_FORWARD, 500);
    
    // 主循环
    while (1) {
        // 读取编码器速度
        int16_t speed_a = EncoderGetSpeed(ENCODER_A);  // mm/s
        int16_t speed_b = EncoderGetSpeed(ENCODER_B);  // mm/s
        
        // 读取脉冲计数
        int16_t count_a = EncoderGetCount(ENCODER_A);
        int16_t count_b = EncoderGetCount(ENCODER_B);
        
        // 读取方向
        uint8_t dir_a = EncoderGetDirection(ENCODER_A);
        uint8_t dir_b = EncoderGetDirection(ENCODER_B);
        
        // 这里可以添加PID控制逻辑
        // 例如：根据目标速度和实际速度计算PWM值
        
        delay_cycles(16000000);  // 延时约200ms
    }
}
```

## 📊 使用示例

### 示例1：读取速度用于PID控制

```c
// 目标速度（mm/s）
int16_t target_speed_a = 500;  // 500 mm/s
int16_t target_speed_b = 500;

// 读取实际速度
int16_t actual_speed_a = EncoderGetSpeed(ENCODER_A);
int16_t actual_speed_b = EncoderGetSpeed(ENCODER_B);

// 计算误差
int16_t error_a = target_speed_a - actual_speed_a;
int16_t error_b = target_speed_b - actual_speed_b;

// PID控制（简化版）
int16_t pwm_a = 500 + error_a / 10;  // 简单比例控制
int16_t pwm_b = 500 + error_b / 10;

// 限幅
if (pwm_a > PWM_MAX) pwm_a = PWM_MAX;
if (pwm_a < PWM_MIN) pwm_a = PWM_MIN;
if (pwm_b > PWM_MAX) pwm_b = PWM_MAX;
if (pwm_b < PWM_MIN) pwm_b = PWM_MIN;

// 设置电机PWM
MotorASet(MOTOR_DIR_FORWARD, pwm_a);
MotorBSet(MOTOR_DIR_FORWARD, pwm_b);
```

### 示例2：监控编码器状态

```c
// 读取所有编码器数据
int16_t count_a = EncoderGetCount(ENCODER_A);
int16_t raw_speed_a = EncoderGetRawSpeed(ENCODER_A);
int16_t filtered_speed_a = EncoderGetFilteredSpeed(ENCODER_A);
int16_t speed_mmps_a = EncoderGetSpeed(ENCODER_A);
uint8_t direction_a = EncoderGetDirection(ENCODER_A);

// 通过UART输出调试信息（需要实现UART发送函数）
// printf("Encoder A: count=%d, raw=%d, filtered=%d, speed=%d mm/s, dir=%d\n",
//        count_a, raw_speed_a, filtered_speed_a, speed_mmps_a, direction_a);
```

### 示例3：重置编码器计数

```c
// 重置编码器A的脉冲计数
EncoderResetCount(ENCODER_A);

// 重置编码器B的脉冲计数
EncoderResetCount(ENCODER_B);
```

## ⚙️ 配置参数调整

在 `src/config.h` 中可以调整以下参数：

```c
/* 编码器配置 */
#define ENCODER_PPR                 20      // 编码器分辨率（根据实际编码器修改）
#define WHEEL_CIRCUMFERENCE_MM      100     // 轮子周长（根据实际轮子修改）
#define ENCODER_SAMPLE_PERIOD_MS    10      // 采样周期（由CLOCK定时器决定）
#define SPEED_FILTER_ALPHA          400     // 滤波系数（300-500推荐）
```

### 滤波系数调整建议

- `SPEED_FILTER_ALPHA = 300`：滤波强度大，响应慢，适合低速平稳运行
- `SPEED_FILTER_ALPHA = 400`：平衡滤波和响应，推荐值
- `SPEED_FILTER_ALPHA = 500`：滤波强度小，响应快，适合快速变速

## 🧪 测试验证

### 测试1：脉冲计数测试

```c
// 电机以固定速度运行
MotorASet(MOTOR_DIR_FORWARD, 300);

// 延时1秒
delay_cycles(80000000);

// 读取脉冲计数
int16_t count = EncoderGetCount(ENCODER_A);
// 预期：count应该接近编码器实际脉冲数

// 停止电机
MotorStop(MOTOR_A);
```

### 测试2：速度计算测试

```c
// 电机以固定速度运行
MotorASet(MOTOR_DIR_FORWARD, 500);

// 延时100ms，等待速度稳定
delay_cycles(8000000);

// 读取速度
int16_t speed = EncoderGetSpeed(ENCODER_A);
// 预期：speed应该稳定在某个值

// 停止电机
MotorStop(MOTOR_A);
```

### 测试3：方向切换测试

```c
// 正转
MotorASet(MOTOR_DIR_FORWARD, 300);
delay_cycles(8000000);
int16_t speed_forward = EncoderGetSpeed(ENCODER_A);
// 预期：speed_forward > 0

// 反转
MotorASet(MOTOR_DIR_REVERSE, 300);
delay_cycles(8000000);
int16_t speed_reverse = EncoderGetSpeed(ENCODER_A);
// 预期：speed_reverse < 0

// 停止
MotorStop(MOTOR_A);
```

## 📌 注意事项

1. **中断优先级**：确保E1A、E2A、CLOCK中断优先级设置正确（在SysConfig中配置）

2. **数据一致性**：在主循环中读取速度时，可能被CLOCK中断打断，建议在关键代码段使用临界区保护：
   ```c
   __disable_irq();
   int16_t speed = EncoderGetSpeed(ENCODER_A);
   __enable_irq();
   ```

3. **整数溢出**：脉冲计数使用 `int16_t`，范围 -32768~+32767，10ms内最多计数约32脉冲，对应速度约1.6m/s，不会溢出

4. **编码器分辨率**：根据实际使用的编码器修改 `ENCODER_PPR` 宏定义

5. **轮子周长**：根据实际轮子直径计算周长：`周长 = π × 直径`

## 🔍 调试技巧

1. **查看原始脉冲计数**：使用 `EncoderGetCount()` 查看脉冲计数是否正常递增/递减

2. **查看原始速度**：使用 `EncoderGetRawSpeed()` 查看未滤波的速度值

3. **查看滤波后速度**：使用 `EncoderGetFilteredSpeed()` 查看滤波效果

4. **调整滤波系数**：如果速度波动大，减小 `SPEED_FILTER_ALPHA`；如果响应慢，增大 `SPEED_FILTER_ALPHA`

## ✅ 编译配置

确保在Keil项目中添加以下文件：
- `src/drivers/drv_encoder.c`
- `src/isr/isr_encoder.c`

并确保包含路径正确：
- `src/`
- `src/drivers/`
- `src/isr/`
