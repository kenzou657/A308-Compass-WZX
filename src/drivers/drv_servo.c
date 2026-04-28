/*
 * drv_servo.c
 * 180°舵机驱动实现
 * 
 * 功能：
 * - 初始化TIMA1-CH0的PWM输出
 * - 提供角度控制接口
 * - 实现角度与PWM脉宽的转换
 */

#include "drv_servo.h"

/* ============ 内部变量 ============ */

/** 舵机当前角度缓存 */
static uint16_t s_servo_current_angle = SERVO_ANGLE_MID;

/* ============ 内部函数 ============ */

/**
 * @brief 限制角度在有效范围内
 * 
 * @param angle 输入角度
 * @return 限制后的角度（0~180°）
 */
static uint16_t servo_clamp_angle(uint16_t angle)
{
    if (angle > SERVO_ANGLE_MAX) {
        return SERVO_ANGLE_MAX;
    }
    return angle;
}

/**
 * @brief 限制CC值在有效范围内
 * 
 * @param cc_value 输入CC值
 * @return 限制后的CC值（20~100）
 */
static uint16_t servo_clamp_cc(uint16_t cc_value)
{
    if (cc_value < SERVO_CC_MIN) {
        return SERVO_CC_MIN;
    }
    if (cc_value > SERVO_CC_MAX) {
        return SERVO_CC_MAX;
    }
    return cc_value;
}

/* ============ 公开函数实现 ============ */

void drv_servo_init(void)
{
    /* 调用SysConfig生成的初始化函数 */
    SYSCFG_DL_PWM_SERVO_init();
    
    /* 设置舵机到中位（90°） */
    drv_servo_set_angle(SERVO_ANGLE_MID);
}

void drv_servo_set_angle(uint16_t angle)
{
    uint16_t cc_value;
    
    /* 限制角度在有效范围内 */
    angle = servo_clamp_angle(angle);
    
    /* 将角度转换为CC值 */
    cc_value = drv_servo_angle_to_cc(angle);
    
    /* 更新PWM脉宽 */
    DL_TimerA_setCaptureCompareValue(PWM_SERVO_INST, cc_value, DL_TIMER_CC_0_INDEX);
    
    /* 缓存当前角度 */
    s_servo_current_angle = angle;
}

uint16_t drv_servo_get_angle(void)
{
    return s_servo_current_angle;
}

void drv_servo_set_mid(void)
{
    drv_servo_set_angle(SERVO_ANGLE_MID);
}

void drv_servo_set_min(void)
{
    drv_servo_set_angle(SERVO_ANGLE_MIN);
}

void drv_servo_set_max(void)
{
    drv_servo_set_angle(SERVO_ANGLE_MAX);
}

uint16_t drv_servo_angle_to_cc(uint16_t angle)
{
    uint16_t cc_value;
    
    /* 限制角度在有效范围内 */
    angle = servo_clamp_angle(angle);
    
    /*
     * 转换公式：cc_value = angle × 80 / 180 + 20
     * 
     * 推导过程：
     * - 角度范围：0° ~ 180°
     * - CC值范围：20 ~ 100
     * - 线性映射：cc_value = (angle - 0) / (180 - 0) × (100 - 20) + 20
     *                      = angle / 180 × 80 + 20
     * 
     * 为避免浮点运算，使用整数计算：
     * cc_value = (angle × 80) / 180 + 20
     *          = (angle × 4) / 9 + 20
     */
    cc_value = (angle * 80) / 180 + SERVO_CC_MIN;
    
    /* 确保CC值在有效范围内 */
    cc_value = servo_clamp_cc(cc_value);
    
    return cc_value;
}

uint16_t drv_servo_cc_to_angle(uint16_t cc_value)
{
    uint16_t angle;
    
    /* 限制CC值在有效范围内 */
    cc_value = servo_clamp_cc(cc_value);
    
    /*
     * 反向转换公式：angle = (cc_value - 20) × 180 / 80
     * 
     * 推导过程：
     * - 从 cc_value = angle × 80 / 180 + 20 反推
     * - cc_value - 20 = angle × 80 / 180
     * - angle = (cc_value - 20) × 180 / 80
     * 
     * 为避免浮点运算，使用整数计算：
     * angle = ((cc_value - 20) × 180) / 80
     *       = ((cc_value - 20) × 9) / 4
     */
    angle = ((cc_value - SERVO_CC_MIN) * 180) / SERVO_CC_RANGE;
    
    /* 确保角度在有效范围内 */
    angle = servo_clamp_angle(angle);
    
    return angle;
}
