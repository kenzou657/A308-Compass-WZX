/*
 * drv_servo.h
 * 180°舵机驱动头文件
 * 
 * 硬件配置：
 * - 使用TIMA1-CH0产生PWM信号
 * - PWM频率：50Hz（周期20ms）
 * - 脉宽范围：0.5ms ~ 2.5ms（对应转角0° ~ 180°）
 */

#ifndef DRV_SERVO_H_
#define DRV_SERVO_H_

#include <stdint.h>
#include "ti_msp_dl_config.h"

/* ============ 舵机参数定义 ============ */

/** 舵机转角范围 */
#define SERVO_ANGLE_MIN         0       // 最小转角（°）
#define SERVO_ANGLE_MAX         180     // 最大转角（°）
#define SERVO_ANGLE_MID         90      // 中位转角（°）

/** PWM计数值范围 */
#define SERVO_CC_MIN            20      // 0°对应的CC值（脉宽0.5ms）
#define SERVO_CC_MAX            100     // 180°对应的CC值（脉宽2.5ms）
#define SERVO_CC_RANGE          (SERVO_CC_MAX - SERVO_CC_MIN)  // 80

/** 舵机角度范围 */
#define SERVO_ANGLE_RANGE       (SERVO_ANGLE_MAX - SERVO_ANGLE_MIN)  // 180

/** PWM周期计数值 */
#define SERVO_PWM_PERIOD        800     // 20ms周期对应的计数值

/* ============ 函数声明 ============ */

/**
 * @brief 初始化舵机驱动
 * 
 * 调用SysConfig生成的初始化函数，设置舵机到中位（90°）
 * 
 * @return 无
 */
void drv_servo_init(void);

/**
 * @brief 设置舵机转角
 * 
 * 根据输入的角度值计算对应的PWM脉宽，更新TIMA1-CH0的CC值
 * 
 * @param angle 目标转角，范围0~180°
 *              - 0°：脉宽0.5ms，舵机转到最小角度
 *              - 90°：脉宽1.5ms，舵机转到中位
 *              - 180°：脉宽2.5ms，舵机转到最大角度
 * 
 * @return 无
 * 
 * @note 如果输入角度超出范围，将被限制在[0, 180]内
 */
void drv_servo_set_angle(uint16_t angle);

/**
 * @brief 获取舵机当前角度
 * 
 * 读取TIMA1-CH0的当前CC值，计算对应的转角
 * 
 * @return 当前舵机转角（0~180°）
 */
uint16_t drv_servo_get_angle(void);

/**
 * @brief 设置舵机到中位（90°）
 * 
 * 便捷函数，等同于 drv_servo_set_angle(90)
 * 
 * @return 无
 */
void drv_servo_set_mid(void);

/**
 * @brief 设置舵机到最小角度（0°）
 * 
 * 便捷函数，等同于 drv_servo_set_angle(0)
 * 
 * @return 无
 */
void drv_servo_set_min(void);

/**
 * @brief 设置舵机到最大角度（180°）
 * 
 * 便捷函数，等同于 drv_servo_set_angle(180)
 * 
 * @return 无
 */
void drv_servo_set_max(void);

/**
 * @brief 角度转换为PWM计数值
 * 
 * 内部函数，将角度值转换为对应的CC值
 * 
 * @param angle 转角（0~180°）
 * @return 对应的CC值（20~100）
 * 
 * @note 公式：cc_value = angle × 80 / 180 + 20
 */
uint16_t drv_servo_angle_to_cc(uint16_t angle);

/**
 * @brief PWM计数值转换为角度
 * 
 * 内部函数，将CC值转换为对应的角度
 * 
 * @param cc_value PWM计数值（20~100）
 * @return 对应的转角（0~180°）
 * 
 * @note 公式：angle = (cc_value - 20) × 180 / 80
 */
uint16_t drv_servo_cc_to_angle(uint16_t cc_value);

#endif /* DRV_SERVO_H_ */
