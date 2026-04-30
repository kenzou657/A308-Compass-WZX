/*
 * 电机驱动模块实现
 * 
 * 功能：
 * - 编码器脉冲采集和速度计算
 * - PWM占空比控制
 * - PID闭环速度控制
 * - 电机运动控制
 */

#include "drv_motor.h"
#include "../config.h"
#include "ti_msp_dl_config.h"
#include <math.h>

/* ==================== 全局变量 ==================== */

/* 电机控制结构数组 */
static Motor_t g_motors[MOTOR_COUNT];

/* ==================== 初始化函数实现 ==================== */

void Motor_Init(void)
{
    /* 初始化编码器和定时器 */
    Motor_EncoderInit();
    
    /* 初始化PID参数 */
    Motor_PIDInit();
    
    /* 初始化电机状态 */
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        g_motors[i].pwm_duty = 0;
        g_motors[i].direction = MOTOR_DIR_STOP;
        g_motors[i].encoder.pulse_count = 0;
        g_motors[i].encoder.last_pulse_count = 0;
        g_motors[i].encoder.speed = 0;
    }
}

void Motor_EncoderInit(void)
{
    /* 启动编码器捕获定时器 */
    DL_TimerG_startCounter(ENCODER1A_INST);
    DL_TimerG_startCounter(ENCODER2A_INST);
    
    /* 启动CLOCK定时器（100Hz） */
    DL_TimerA_startCounter(CLOCK_INST);
}

void Motor_PIDInit(void)
{
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        g_motors[i].pid.kp = PID_KP;
        g_motors[i].pid.ki = PID_KI;
        g_motors[i].pid.kd = PID_KD;
        g_motors[i].pid.error = 0.0f;
        g_motors[i].pid.last_error = 0.0f;
        g_motors[i].pid.integral = 0.0f;
        g_motors[i].pid.target_speed = 0;
        g_motors[i].pid.output = 0;
        g_motors[i].pid.enabled = false;
    }
}

/* ==================== 编码器函数实现 ==================== */

int32_t Motor_GetEncoderCount(uint8_t motor_id)
{
    if (motor_id >= MOTOR_COUNT) {
        return 0;
    }
    return g_motors[motor_id].encoder.pulse_count;
}

void Motor_ResetEncoder(uint8_t motor_id)
{
    if (motor_id >= MOTOR_COUNT) {
        return;
    }
    g_motors[motor_id].encoder.pulse_count = 0;
    g_motors[motor_id].encoder.last_pulse_count = 0;
}

void Motor_CalcSpeed(void)
{
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        /* 计算采样周期内的脉冲数 */
        int32_t pulse_delta = g_motors[i].encoder.pulse_count - 
                              g_motors[i].encoder.last_pulse_count;
        
        /* 更新上次脉冲计数 */
        g_motors[i].encoder.last_pulse_count = g_motors[i].encoder.pulse_count;
        
        /* 计算速度：speed = (pulse_delta × SPEED_SCALE) / 1000
         * SPEED_SCALE = (2π × R) / (N × T)
         * 其中 R=轮半径, N=编码器PPR, T=采样周期
         */
        g_motors[i].encoder.speed = (int16_t)((pulse_delta * SPEED_SCALE) / 1000);
    }
}

int16_t Motor_GetSpeed(uint8_t motor_id)
{
    if (motor_id >= MOTOR_COUNT) {
        return 0;
    }
    return g_motors[motor_id].encoder.speed;
}

/* ==================== PWM控制函数实现 ==================== */

void Motor_SetDuty(uint8_t motor_id, uint16_t duty)
{
    if (motor_id >= MOTOR_COUNT) {
        return;
    }
    
    /* 限制占空比范围 */
    if (duty > PWM_MAX_DUTY) {
        duty = PWM_MAX_DUTY;
    }
    
    g_motors[motor_id].pwm_duty = duty;
    
    /* 更新PWM占空比 */
    if (motor_id == MOTOR_A) {
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, duty, DL_TIMER_CC_0_INDEX);
    } else {
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, duty, DL_TIMER_CC_1_INDEX);
    }
}

uint16_t Motor_GetDuty(uint8_t motor_id)
{
    if (motor_id >= MOTOR_COUNT) {
        return 0;
    }
    return g_motors[motor_id].pwm_duty;
}

/* ==================== 方向控制函数实现 ==================== */

void Motor_SetDirection(uint8_t motor_id, int8_t dir)
{
    if (motor_id >= MOTOR_COUNT) {
        return;
    }
    
    g_motors[motor_id].direction = dir;
    
    if (motor_id == MOTOR_A) {
        /* 电机A方向控制：AIN1(PA0), AIN2(PA1) */
        if (dir == MOTOR_DIR_FORWARD) {
            /* 前进：AIN1=1, AIN2=0 */
            DL_GPIO_setPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_AIN1_PIN);
            DL_GPIO_clearPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_AIN2_PIN);
        } else if (dir == MOTOR_DIR_BACKWARD) {
            /* 后退：AIN1=0, AIN2=1 */
            DL_GPIO_clearPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_AIN1_PIN);
            DL_GPIO_setPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_AIN2_PIN);
        } else {
            /* 停止：AIN1=0, AIN2=0 */
            DL_GPIO_clearPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_AIN1_PIN | 
                                     GPIO_MOTOR_DIR_MOTOR_AIN2_PIN);
        }
    } else {
        /* 电机B方向控制：BIN1(PA2), BIN2(PA3) */
        if (dir == MOTOR_DIR_FORWARD) {
            /* 前进：BIN1=1, BIN2=0 */
            DL_GPIO_setPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_BIN1_PIN);
            DL_GPIO_clearPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_BIN2_PIN);
        } else if (dir == MOTOR_DIR_BACKWARD) {
            /* 后退：BIN1=0, BIN2=1 */
            DL_GPIO_clearPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_BIN1_PIN);
            DL_GPIO_setPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_BIN2_PIN);
        } else {
            /* 停止：BIN1=0, BIN2=0 */
            DL_GPIO_clearPins(GPIOA, GPIO_MOTOR_DIR_MOTOR_BIN1_PIN | 
                                     GPIO_MOTOR_DIR_MOTOR_BIN2_PIN);
        }
    }
}

int8_t Motor_GetDirection(uint8_t motor_id)
{
    if (motor_id >= MOTOR_COUNT) {
        return MOTOR_DIR_STOP;
    }
    return g_motors[motor_id].direction;
}

/* ==================== PID控制函数实现 ==================== */

void Motor_SetTargetSpeed(uint8_t motor_id, int16_t speed)
{
    if (motor_id >= MOTOR_COUNT) {
        return;
    }
    
    /* 限制速度范围 */
    if (speed > 0) {
        if (speed > MAX_SPEED) {
            speed = MAX_SPEED;
        }
        if (speed < MIN_SPEED && speed > 0) {
            speed = MIN_SPEED;
        }
        Motor_SetDirection(motor_id, MOTOR_DIR_FORWARD);
    } else if (speed < 0) {
        if (speed < -MAX_SPEED) {
            speed = -MAX_SPEED;
        }
        if (speed > -MIN_SPEED && speed < 0) {
            speed = -MIN_SPEED;
        }
        Motor_SetDirection(motor_id, MOTOR_DIR_BACKWARD);
    } else {
        Motor_SetDirection(motor_id, MOTOR_DIR_STOP);
    }
    
    g_motors[motor_id].pid.target_speed = speed;
    g_motors[motor_id].pid.enabled = true;
    
    /* 重置PID状态 */
    g_motors[motor_id].pid.error = 0.0f;
    g_motors[motor_id].pid.last_error = 0.0f;
    g_motors[motor_id].pid.integral = 0.0f;
}

int16_t Motor_GetTargetSpeed(uint8_t motor_id)
{
    if (motor_id >= MOTOR_COUNT) {
        return 0;
    }
    return g_motors[motor_id].pid.target_speed;
}

void Motor_PIDUpdate(void)
{
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
        if (!g_motors[i].pid.enabled) {
            continue;
        }
        
        PID_t *pid = &g_motors[i].pid;
        int16_t actual_speed = g_motors[i].encoder.speed;
        
        /* 计算速度误差(mm/s) */
        pid->error = (float)(pid->target_speed - actual_speed);
        
        /* 比例项(mm/s) */
        float p_term = pid->kp * pid->error;
        
        /* 积分项(mm/s)，带限幅 */
        pid->integral += pid->error;
        if (pid->integral > PID_I_MAX) {
            pid->integral = PID_I_MAX;
        } else if (pid->integral < -PID_I_MAX) {
            pid->integral = -PID_I_MAX;
        }
        float i_term = pid->ki * pid->integral;
        
        /* 微分项(mm/s) */
        float d_term = pid->kd * (pid->error - pid->last_error);
        
        /* 计算PID输出(mm/s)：速度值，不是占空比 */
        float output_speed = p_term + i_term + d_term;
        
        /* 输出限幅(mm/s) */
        if (output_speed > PID_OUT_MAX) {
            output_speed = PID_OUT_MAX;
        } else if (output_speed < -PID_OUT_MAX) {
            output_speed = -PID_OUT_MAX;
        }
        
        pid->output = (int16_t)output_speed;
        pid->last_error = pid->error;
        
        /* 将速度转换为PWM占空比
         * duty = (speed × SPEED_TO_DUTY_SCALE) / 1000
         */
        uint16_t duty = (uint16_t)((pid->output > 0 ? pid->output : -pid->output) * SPEED_TO_DUTY_SCALE / 1000);
        if (duty > PWM_MAX_DUTY) {
            duty = PWM_MAX_DUTY;
        }
        Motor_SetDuty(i, duty);
    }
}

void Motor_DisablePID(uint8_t motor_id)
{
    if (motor_id >= MOTOR_COUNT) {
        return;
    }
    g_motors[motor_id].pid.enabled = false;
}

/* ==================== 运动控制函数实现 ==================== */

void Motor_MoveForward(int16_t speed)
{
    Motor_SetTargetSpeed(MOTOR_A, speed);
    Motor_SetTargetSpeed(MOTOR_B, speed);
}

void Motor_MoveBackward(int16_t speed)
{
    Motor_SetTargetSpeed(MOTOR_A, -speed);
    Motor_SetTargetSpeed(MOTOR_B, -speed);
}

void Motor_Turn(int16_t left_speed, int16_t right_speed)
{
    Motor_SetTargetSpeed(MOTOR_A, left_speed);
    Motor_SetTargetSpeed(MOTOR_B, right_speed);
}

void Motor_Stop(void)
{
    Motor_SetTargetSpeed(MOTOR_A, 0);
    Motor_SetTargetSpeed(MOTOR_B, 0);
    Motor_SetDuty(MOTOR_A, 0);
    Motor_SetDuty(MOTOR_B, 0);
    Motor_DisablePID(MOTOR_A);
    Motor_DisablePID(MOTOR_B);
}

/* ==================== 中断处理函数 ==================== */

void Motor_Encoder1A_ISR(void)
{
    /* 读取E1B(PA24)电平判断方向 */
    uint32_t b_level = DL_GPIO_readPins(GPIOA, GPIO_ENCODER_ENCODER1B_PIN);
    
    if (b_level) {
        /* B相为高，A相上升沿时为正向 */
        g_motors[MOTOR_A].encoder.pulse_count++;
    } else {
        /* B相为低，A相上升沿时为反向 */
        g_motors[MOTOR_A].encoder.pulse_count--;
    }
}

void Motor_Encoder2A_ISR(void)
{
    /* 读取E2B(PA22)电平判断方向 */
    uint32_t b_level = DL_GPIO_readPins(GPIOA, GPIO_ENCODER_ENCODER2B_PIN);
    
    if (b_level) {
        /* B相为高，A相上升沿时为正向 */
        g_motors[MOTOR_B].encoder.pulse_count++;
    } else {
        /* B相为低，A相上升沿时为反向 */
        g_motors[MOTOR_B].encoder.pulse_count--;
    }
}

void Motor_Clock_ISR(void)
{
    /* 计算电机实时速度 */
    Motor_CalcSpeed();
    
    /* 执行PID控制 */
    Motor_PIDUpdate();
}
