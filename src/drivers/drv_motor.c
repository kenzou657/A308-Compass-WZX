#include "drv_motor.h"

/* ============ 全局变量 ============ */
// 记录当前电机状态
static struct {
    uint8_t direction_a;    // 电机A方向
    uint16_t pwm_a;         // 电机A PWM值
    uint8_t direction_b;    // 电机B方向
    uint16_t pwm_b;         // 电机B PWM值
} g_motor_state = {
    .direction_a = MOTOR_DIR_FORWARD,
    .pwm_a = 0,
    .direction_b = MOTOR_DIR_FORWARD,
    .pwm_b = 0
};

/* ============ 初始化函数 ============ */

/**
 * @brief 初始化电机驱动模块
 * @note PWM已在SysConfig中配置，此函数初始化电机状态
 */
void MotorInit(void)
{
    // 初始化电机状态为停止
    g_motor_state.direction_a = MOTOR_DIR_FORWARD;
    g_motor_state.pwm_a = PWM_MIN;
    g_motor_state.direction_b = MOTOR_DIR_FORWARD;
    g_motor_state.pwm_b = PWM_MIN;
    
    // 设置初始方向为正转
    MOTOR_A_DIR_FORWARD();
    MOTOR_B_DIR_FORWARD();
    
    // 设置初始PWM为0（停止）
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, PWM_MIN, DL_TIMER_CC_1_INDEX);  // 电机A (PA4)
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, PWM_MIN, DL_TIMER_CC_0_INDEX);  // 电机B (PA7)
}

/* ============ 底层驱动函数 ============ */

/**
 * @brief 设置单个电机的速度和方向
 * @param motor_id: 电机编号 (MOTOR_A 或 MOTOR_B)
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorSet(uint8_t motor_id, uint8_t direction, uint16_t pwm_value)
{
    // 限制PWM值在有效范围内
    if (pwm_value > PWM_MAX) {
        pwm_value = PWM_MAX;
    }
    
    if (motor_id == MOTOR_A) {
        // 设置电机A方向
        if (direction == MOTOR_DIR_FORWARD) {
            MOTOR_A_DIR_FORWARD();
        } else {
            MOTOR_A_DIR_REVERSE();
        }
        
        // 更新电机A PWM（通道1，PA4）
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, pwm_value, DL_TIMER_CC_1_INDEX);
        
        // 记录状态
        g_motor_state.direction_a = direction;
        g_motor_state.pwm_a = pwm_value;
        
    } else if (motor_id == MOTOR_B) {
        // 设置电机B方向
        if (direction == MOTOR_DIR_FORWARD) {
            MOTOR_B_DIR_FORWARD();
        } else {
            MOTOR_B_DIR_REVERSE();
        }
        
        // 更新电机B PWM（通道0，PA7）
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, pwm_value, DL_TIMER_CC_0_INDEX);
        
        // 记录状态
        g_motor_state.direction_b = direction;
        g_motor_state.pwm_b = pwm_value;
    }
}

/**
 * @brief 设置电机A的速度和方向
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorASet(uint8_t direction, uint16_t pwm_value)
{
    MotorSet(MOTOR_A, direction, pwm_value);
}

/**
 * @brief 设置电机B的速度和方向
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorBSet(uint8_t direction, uint16_t pwm_value)
{
    MotorSet(MOTOR_B, direction, pwm_value);
}

/**
 * @brief 停止指定电机
 * @param motor_id: 电机编号 (MOTOR_A 或 MOTOR_B)
 */
void MotorStop(uint8_t motor_id)
{
    if (motor_id == MOTOR_A) {
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, PWM_MIN, DL_TIMER_CC_1_INDEX);
        g_motor_state.pwm_a = PWM_MIN;
    } else if (motor_id == MOTOR_B) {
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, PWM_MIN, DL_TIMER_CC_0_INDEX);
        g_motor_state.pwm_b = PWM_MIN;
    }
}

/**
 * @brief 停止所有电机
 */
void MotorStopAll(void)
{
    MotorStop(MOTOR_A);
    MotorStop(MOTOR_B);
}

/* ============ 高级控制函数 ============ */

/**
 * @brief 小车前进
 * @param speed: 速度等级 (0-1000)
 */
void Forward(uint16_t speed)
{
    // 限制速度范围
    if (speed > PWM_MAX) {
        speed = PWM_MAX;
    }
    
    // 两个电机都正转，速度相同
    MotorASet(MOTOR_DIR_FORWARD, speed);
    MotorBSet(MOTOR_DIR_FORWARD, speed);
}

/**
 * @brief 小车后退
 * @param speed: 速度等级 (0-1000)
 */
void Backward(uint16_t speed)
{
    // 限制速度范围
    if (speed > PWM_MAX) {
        speed = PWM_MAX;
    }
    
    // 两个电机都反转，速度相同
    MotorASet(MOTOR_DIR_REVERSE, speed);
    MotorBSet(MOTOR_DIR_REVERSE, speed);
}

/**
 * @brief 小车左转（差速转弯）
 * @param speed: 基础速度等级 (0-1000)
 * @note 左电机（电机A）速度降低，右电机（电机B）保持，实现左转
 */
void TurnLeft(uint16_t speed)
{
    // 限制速度范围
    if (speed > PWM_MAX) {
        speed = PWM_MAX;
    }
    
    // 左电机（A）速度降低到50%，右电机（B）保持全速
    uint16_t left_speed = speed / 2;
    
    MotorASet(MOTOR_DIR_FORWARD, left_speed);
    MotorBSet(MOTOR_DIR_FORWARD, speed);
}

/**
 * @brief 小车右转（差速转弯）
 * @param speed: 基础速度等级 (0-1000)
 * @note 右电机（电机B）速度降低，左电机（电机A）保持，实现右转
 */
void TurnRight(uint16_t speed)
{
    // 限制速度范围
    if (speed > PWM_MAX) {
        speed = PWM_MAX;
    }
    
    // 右电机（B）速度降低到50%，左电机（A）保持全速
    uint16_t right_speed = speed / 2;
    
    MotorASet(MOTOR_DIR_FORWARD, speed);
    MotorBSet(MOTOR_DIR_FORWARD, right_speed);
}

/**
 * @brief 小车停止
 */
void Halt(void)
{
    MotorStopAll();
}
