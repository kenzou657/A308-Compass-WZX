/*
 * 电机驱动使用示例
 * 
 * 本文件展示如何在应用中使用电机驱动模块
 */

#include "drivers/drv_motor.h"
#include "config.h"

/* ==================== 初始化示例 ==================== */

void App_MotorInit(void)
{
    /* 初始化电机驱动模块 */
    Motor_Init();
}

/* ==================== 基本运动控制示例 ==================== */

void App_MotorDemo(void)
{
    /* 前进：速度200mm/s */
    Motor_MoveForward(200);
    
    /* 延迟2秒 */
    /* delay_ms(2000); */
    
    /* 停止 */
    Motor_Stop();
    
    /* 延迟1秒 */
    /* delay_ms(1000); */
    
    /* 后退：速度150mm/s */
    Motor_MoveBackward(150);
    
    /* 延迟2秒 */
    /* delay_ms(2000); */
    
    /* 停止 */
    Motor_Stop();
    
    /* 延迟1秒 */
    /* delay_ms(1000); */
    
    /* 转弯：左轮200mm/s，右轮100mm/s */
    Motor_Turn(200, 100);
    
    /* 延迟2秒 */
    /* delay_ms(2000); */
    
    /* 停止 */
    Motor_Stop();
}

/* ==================== 直接PWM控制示例 ==================== */

void App_MotorPWMControl(void)
{
    /* 禁用PID控制 */
    Motor_DisablePID(MOTOR_A);
    Motor_DisablePID(MOTOR_B);
    
    /* 设置电机方向 */
    Motor_SetDirection(MOTOR_A, MOTOR_DIR_FORWARD);
    Motor_SetDirection(MOTOR_B, MOTOR_DIR_FORWARD);
    
    /* 设置PWM占空比（0-2000） */
    Motor_SetDuty(MOTOR_A, 1000);  /* 50% */
    Motor_SetDuty(MOTOR_B, 1000);  /* 50% */
}

/* ==================== 速度反馈示例 ==================== */

void App_MotorSpeedFeedback(void)
{
    /* 设置目标速度 */
    Motor_SetTargetSpeed(MOTOR_A, 200);
    Motor_SetTargetSpeed(MOTOR_B, 200);
    
    /* 获取实时速度 */
    int16_t speed_a = Motor_GetSpeed(MOTOR_A);
    int16_t speed_b = Motor_GetSpeed(MOTOR_B);
    
    /* 获取脉冲计数 */
    int32_t pulse_a = Motor_GetEncoderCount(MOTOR_A);
    int32_t pulse_b = Motor_GetEncoderCount(MOTOR_B);
    
    /* 可以通过UART输出调试信息 */
    /* printf("Motor A: speed=%d mm/s, pulse=%d\r\n", speed_a, pulse_a); */
    /* printf("Motor B: speed=%d mm/s, pulse=%d\r\n", speed_b, pulse_b); */
}

/* ==================== 差速转向示例 ==================== */

void App_MotorTurnDemo(void)
{
    /* 原地左转：左轮后退，右轮前进 */
    Motor_Turn(-150, 150);
    /* delay_ms(1000); */
    
    /* 原地右转：左轮前进，右轮后退 */
    Motor_Turn(150, -150);
    /* delay_ms(1000); */
    
    /* 向左转弯：左轮速度小，右轮速度大 */
    Motor_Turn(100, 200);
    /* delay_ms(1000); */
    
    /* 向右转弯：左轮速度大，右轮速度小 */
    Motor_Turn(200, 100);
    /* delay_ms(1000); */
    
    /* 停止 */
    Motor_Stop();
}

/* ==================== 集成到main函数的示例 ==================== */

/*
void main(void)
{
    SYSCFG_DL_init();
    
    // 初始化电机驱动
    App_MotorInit();
    
    while (1) {
        // 前进
        Motor_MoveForward(200);
        delay_ms(2000);
        
        // 停止
        Motor_Stop();
        delay_ms(1000);
        
        // 后退
        Motor_MoveBackward(150);
        delay_ms(2000);
        
        // 停止
        Motor_Stop();
        delay_ms(1000);
        
        // 转弯
        Motor_Turn(200, 100);
        delay_ms(2000);
        
        // 停止
        Motor_Stop();
        delay_ms(1000);
    }
}
*/
