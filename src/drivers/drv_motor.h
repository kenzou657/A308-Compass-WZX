#ifndef _DRV_MOTOR_H_
#define _DRV_MOTOR_H_

#include "ti_msp_dl_config.h"

/* ============ 电机方向定义 ============ */
#define MOTOR_DIR_FORWARD   1   // 正转（高电平）
#define MOTOR_DIR_REVERSE   0   // 反转（低电平）

/* ============ 电机编号定义 ============ */
#define MOTOR_A             0
#define MOTOR_B             1

/* ============ PWM占空比范围 ============ */
#define PWM_MIN             0       // 最小占空比（停止）
#define PWM_MAX             1000    // 最大占空比（全速）

/* ============ 底层GPIO控制宏 ============ */

/* 电机A方向控制 */
#define MOTOR_A_DIR_FORWARD()   DL_GPIO_setPins(GPIO_MOTOR_DIR_MOTOR_A_DIR_PORT, GPIO_MOTOR_DIR_MOTOR_A_DIR_PIN)
#define MOTOR_A_DIR_REVERSE()   DL_GPIO_clearPins(GPIO_MOTOR_DIR_MOTOR_A_DIR_PORT, GPIO_MOTOR_DIR_MOTOR_A_DIR_PIN)

/* 电机B方向控制 */
#define MOTOR_B_DIR_FORWARD()   DL_GPIO_setPins(GPIO_MOTOR_DIR_MOTOR_B_DIR_PORT, GPIO_MOTOR_DIR_MOTOR_B_DIR_PIN)
#define MOTOR_B_DIR_REVERSE()   DL_GPIO_clearPins(GPIO_MOTOR_DIR_MOTOR_B_DIR_PORT, GPIO_MOTOR_DIR_MOTOR_B_DIR_PIN)

/* ============ 电机驱动函数声明 ============ */

/**
 * @brief 初始化电机驱动模块
 * @note 在main函数中调用SYSCFG_DL_init()后调用此函数
 */
void MotorInit(void);

/**
 * @brief 设置单个电机的速度和方向
 * @param motor_id: 电机编号 (MOTOR_A 或 MOTOR_B)
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorSet(uint8_t motor_id, uint8_t direction, uint16_t pwm_value);

/**
 * @brief 设置电机A的速度和方向
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorASet(uint8_t direction, uint16_t pwm_value);

/**
 * @brief 设置电机B的速度和方向
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorBSet(uint8_t direction, uint16_t pwm_value);

/**
 * @brief 停止指定电机
 * @param motor_id: 电机编号 (MOTOR_A 或 MOTOR_B)
 */
void MotorStop(uint8_t motor_id);

/**
 * @brief 停止所有电机
 */
void MotorStopAll(void);

/* ============ 高级控制函数 ============ */

/**
 * @brief 小车前进
 * @param speed: 速度等级 (0-1000)
 */
void Forward(uint16_t speed);

/**
 * @brief 小车后退
 * @param speed: 速度等级 (0-1000)
 */
void Backward(uint16_t speed);

/**
 * @brief 小车左转（差速转弯）
 * @param speed: 基础速度等级 (0-1000)
 * @note 左电机速度降低，右电机保持，实现左转
 */
void TurnLeft(uint16_t speed);

/**
 * @brief 小车右转（差速转弯）
 * @param speed: 基础速度等级 (0-1000)
 * @note 右电机速度降低，左电机保持，实现右转
 */
void TurnRight(uint16_t speed);

/**
 * @brief 小车停止
 */
void Halt(void);

#endif /* _DRV_MOTOR_H_ */
