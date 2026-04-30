# 主程序集成示例

## 一、在 empty.c 中添加头文件

```c
#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 添加 UART 和 IMU 驱动头文件
#include "src/drivers/drv_uart.h"
#include "src/drivers/drv_jy61p.h"
#include "src/app/app_camera_uart.h"

// 其他驱动
#include "src/utils/timer.h"
#include "src/drivers/drv_buzzer.h"
#include "src/drivers/drv_led.h"
#include "src/drivers/drv_motor.h"
```

## 二、在 main() 函数中初始化

```c
int main(void)
{
    // 系统初始化（SysConfig 自动生成）
    SYSCFG_DL_init();
    
    // 初始化电机
    MotorInit();
    
    // 初始化 UART0（摄像头）
    uart_init();
    camera_uart_init();
    
    // 初始化 UART3（IMU）
    jy61p_init();
    
    // 主循环
    while (1) {
        Motor_Proc();
        Camera_Proc();
        IMU_Proc();
    }
}
```

## 三、摄像头数据处理示例

```c
// 摄像头数据处理任务函数
void Camera_Proc(void)
{
    camera_frame_data_t frame;
    
    // 获取最新的摄像头数据
    if (camera_uart_get_rx_frame(&frame) == 0) {
        // 成功获取数据
        
        if (frame.mode == CAMERA_MODE_LINE_TRACKING) {
            // 寻线模式
            // data_x: 线偏移量（放大 1000 倍）
            // data_y: 角度偏移（放大 1000 倍）
            
            float line_offset = (float)frame.data_x / 1000.0f;
            float angle_offset = (float)frame.data_y / 1000.0f;
            
            // 根据偏移量控制电机
            // ... 你的控制逻辑 ...
            
        } else if (frame.mode == CAMERA_MODE_DIGIT_RECOG) {
            // 数字识别模式
            uint8_t digit_id = frame.id;
            
            // 处理识别到的数字
            // ... 你的逻辑 ...
        }
    }
}
```

## 四、IMU 数据处理示例

```c
// IMU 数据处理任务函数
void IMU_Proc(void)
{
    jy61p_acc_t acc;
    jy61p_gyro_t gyro;
    jy61p_angle_t angle;
    
    // 获取加速度数据
    if (jy61p_get_acc(&acc) == 0) {
        // 成功获取加速度数据
        // acc.ax_g, acc.ay_g, acc.az_g (单位: g)
        // acc.temp_c (单位: °C)
    }
    
    // 获取角速度数据
    if (jy61p_get_gyro(&gyro) == 0) {
        // 成功获取角速度数据
        // gyro.wx_dps, gyro.wy_dps, gyro.wz_dps (单位: °/s)
        // gyro.voltage_v (单位: V)
    }
    
    // 获取欧拉角数据
    if (jy61p_get_angle(&angle) == 0) {
        // 成功获取欧拉角数据
        // angle.roll_deg, angle.pitch_deg, angle.yaw_deg (单位: °, 范围: -180 ~ 180)
        
        // 使用偏航角控制小车方向
        float yaw = angle.yaw_deg;
        
        // ... 你的控制逻辑 ...
    }
}
```

## 五、发送数据示例

### 5.1 发送摄像头命令

```c
// 发送摄像头命令
void Send_Camera_Command(void)
{
    // 发送切换到寻线模式的命令
    camera_uart_send_frame(CAMERA_MODE_LINE_TRACKING, 0, 0, 0);
    
    // 或发送切换到数字识别模式的命令
    // camera_uart_send_frame(CAMERA_MODE_DIGIT_RECOG, 0, 0, 0);
}
```

### 5.2 发送 IMU 命令

```c
// 发送 IMU 命令示例（根据 JY61P 手册）
void Send_IMU_Command(void)
{
    // 示例：发送校准命令
    uint8_t cmd[] = {0xFF, 0xAA, 0x01, 0x00};
    jy61p_send_command(cmd, sizeof(cmd));
}
```

## 六、统计信息查询示例

```c
// 查询 UART 统计信息
void Print_UART_Stats(void)
{
    uint32_t frame_count, error_count, checksum_error;
    
    // 获取摄像头接收统计
    camera_uart_get_rx_stats(&frame_count, &error_count, &checksum_error);
    
    // 打印统计信息（需要实现 printf 或使用其他方式）
    // printf("Camera RX: %lu frames, %lu errors, %lu checksum errors\n", 
    //        frame_count, error_count, checksum_error);
}

// 查询 IMU 统计信息
void Print_IMU_Stats(void)
{
    uint32_t frame_count, error_count;
    
    // 获取 IMU 接收统计
    jy61p_get_stats(&frame_count, &error_count);
    
    // 打印统计信息
    // printf("IMU RX: %lu frames, %lu errors\n", frame_count, error_count);
}
```

## 七、完整的 main.c 示例

```c
#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// UART 和 IMU 驱动
#include "src/drivers/drv_uart.h"
#include "src/drivers/drv_jy61p.h"
#include "src/app/app_camera_uart.h"

// 其他驱动
#include "src/utils/timer.h"
#include "src/drivers/drv_buzzer.h"
#include "src/drivers/drv_led.h"
#include "src/drivers/drv_motor.h"

// 全局变量
volatile uint32_t uwTick_Motor_Set_Point = 0;
volatile uint32_t uwTick_Camera_Point = 0;
volatile uint32_t uwTick_IMU_Point = 0;

// 函数声明
void Motor_Proc(void);
void Camera_Proc(void);
void IMU_Proc(void);

int main(void)
{
    // 系统初始化
    SYSCFG_DL_init();
    
    // 初始化电机
    MotorInit();
    
    // 初始化 UART0（摄像头）
    uart_init();
    camera_uart_init();
    
    // 初始化 UART3（IMU）
    jy61p_init();
    
    // 启动指示
    LEDG_ON();
    delay_ms(500);
    LEDG_OFF();
    
    // 主循环
    while (1) {
        Motor_Proc();
        Camera_Proc();
        IMU_Proc();
    }
}

// 电机控制任务（每 20ms 执行一次）
void Motor_Proc(void)
{
    if ((uwTick - uwTick_Motor_Set_Point) < 20) {
        return;
    }
    uwTick_Motor_Set_Point = uwTick;
    
    // 电机控制逻辑
    LEDG_TOGGLE();
}

// 摄像头数据处理任务（每 50ms 执行一次）
void Camera_Proc(void)
{
    if ((uwTick - uwTick_Camera_Point) < 50) {
        return;
    }
    uwTick_Camera_Point = uwTick;
    
    camera_frame_data_t frame;
    
    // 获取最新的摄像头数据
    if (camera_uart_get_rx_frame(&frame) == 0) {
        if (frame.mode == CAMERA_MODE_LINE_TRACKING) {
            // 寻线模式处理
            float line_offset = (float)frame.data_x / 1000.0f;
            float angle_offset = (float)frame.data_y / 1000.0f;
            
            // 根据偏移量控制电机
            // ... 你的控制逻辑 ...
        }
    }
}

// IMU 数据处理任务（每 10ms 执行一次）
void IMU_Proc(void)
{
    if ((uwTick - uwTick_IMU_Point) < 10) {
        return;
    }
    uwTick_IMU_Point = uwTick;
    
    jy61p_angle_t angle;
    
    // 获取欧拉角数据
    if (jy61p_get_angle(&angle) == 0) {
        // 使用偏航角控制小车方向
        float yaw = angle.yaw_deg;
        
        // ... 你的控制逻辑 ...
    }
}
```

## 八、注意事项

### 8.1 中断优先级
- UART0 和 UART3 的中断优先级已在 SysConfig 中配置
- 确保中断优先级设置合理，避免冲突

### 8.2 数据同步
- 摄像头和 IMU 数据在中断中更新
- 主循环中读取数据时无需关中断（数据结构已设计为原子操作）

### 8.3 性能优化
- 避免在主循环中频繁调用浮点运算
- 可以在中断中完成数据解析，主循环只读取结果

### 8.4 调试建议
- 使用统计函数监控接收状态
- 检查 frame_count 和 error_count 判断通信质量
- 使用 LED 指示通信状态

## 九、Keil 项目配置

### 9.1 添加源文件到项目
在 Keil 项目中添加以下文件：

**驱动层**
- `src/drivers/drv_uart.c`
- `src/drivers/drv_jy61p.c`

**应用层**
- `src/app/app_camera_uart.c`

**中断层**
- `src/isr/isr_uart.c`
- `src/isr/isr_jy61p.c`

### 9.2 包含路径设置
在 Keil 项目设置中添加包含路径：
- `./src/drivers`
- `./src/app`
- `./src/isr`
- `./src/utils`

### 9.3 编译选项
- 确保启用 C99 标准
- 优化级别建议设置为 -O1 或 -O2

## 十、测试步骤

### 10.1 单元测试
1. 测试 UART0 接收：发送测试帧，检查 frame_count
2. 测试 UART3 接收：连接 IMU，检查数据更新
3. 测试发送功能：使用示波器或逻辑分析仪验证

### 10.2 集成测试
1. 同时启用摄像头和 IMU
2. 检查两个 UART 是否正常工作
3. 验证数据解析正确性

### 10.3 性能测试
1. 监控帧丢失率（error_count / frame_count）
2. 测试最大数据速率
3. 验证实时性要求
