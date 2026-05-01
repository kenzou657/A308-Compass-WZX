/**
 * @file drv_chassis.h
 * @brief 小车底盘运动控制模块 - 头文件
 *
 * 功能：
 * - 基于陀螺仪 Yaw 角的闭环方向控制（P 环）
 * - 基于运行时间的位移控制（uwTick 计时）
 * - 差速控制实现转向
 * - 死区机制防止抖动
 *
 * 控制策略：
 * - 方向控制：left_pwm = base_pwm - pwm_delta
 *             right_pwm = base_pwm + pwm_delta
 * - 位移控制：通过 uwTick 计算运行时间
 * - 死区判断：|error| < 0.5° 时不动作
 */

#ifndef _DRV_CHASSIS_H_
#define _DRV_CHASSIS_H_

#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "../config.h"

/* ============ 小车运动状态定义 ============ */
#define CHASSIS_STATE_IDLE      0   // 停止状态
#define CHASSIS_STATE_MOVING    1   // 运动中

/* ============ 小车运动方向定义 ============ */
#define CHASSIS_DIR_FORWARD     0   // 前进
#define CHASSIS_DIR_BACKWARD    1   // 后退

/* ============ 小车状态结构体 ============ */

/**
 * @brief 小车底盘控制结构体
 *
 * 包含运动目标、当前状态、反馈数据等
 */
typedef struct {
    /* ===== 运动目标 ===== */
    int32_t target_yaw;             // 目标偏航角（°×100）
    uint32_t motion_duration_ms;    // 运动持续时间（ms）
    uint16_t base_pwm;              // 基础 PWM 占空比（0-700）
    uint8_t direction;              // 运动方向（前进/后退）
    
    /* ===== 运动状态 ===== */
    uint8_t state;                  // 当前状态（IDLE/MOVING）
    uint32_t motion_start_tick;     // 运动开始时刻（uwTick）
    uint32_t elapsed_ms;            // 已运行时间（ms）
    
    /* ===== 反馈数据 ===== */
    int32_t current_yaw;            // 当前偏航角（°×100）
    int32_t yaw_error;              // 偏航角误差（target - current）
    int32_t pid_output;             // PID 输出（差速值）
    
    /* ===== 电机控制 ===== */
    int16_t motor_left_pwm;        // 左电机 PWM 占空比
    int16_t motor_right_pwm;       // 右电机 PWM 占空比
    uint8_t is_turning;             // 原地转弯标志（base_pwm == 0 时为 1）
    
    /* ===== 调试信息 ===== */
    uint32_t update_count;          // 更新次数计数
    uint8_t gyro_data_valid;        // 陀螺仪数据有效标志
} Chassis_t;

/* ============ 全局变量声明 ============ */
extern Chassis_t g_chassis;

/* ============ 函数声明 ============ */

/**
 * @brief 初始化小车底盘控制模块
 *
 * 功能：
 * - 初始化状态变量
 * - 停止所有电机
 *
 * @note 必须在 SYSCFG_DL_init() 和 MotorInit() 之后调用
 */
void ChassisInit(void);

/**
 * @brief 设置小车运动参数
 * 
 * @param target_yaw: 目标偏航角（°×100，例如 0 表示 0°，9000 表示 90°）
 * @param duration_ms: 运动持续时间（ms）
 * @param base_pwm: 基础 PWM 占空比（0-700）
 * @param direction: 运动方向（CHASSIS_DIR_FORWARD 或 CHASSIS_DIR_BACKWARD）
 * 
 * @note 调用此函数后，小车将开始运动，直到时间到达或手动停止
 * 
 * @example
 * // 设置小车以 PWM 500 前进 2000ms，保持偏航角 0°
 * ChassisSetMotion(0, 2000, 500, CHASSIS_DIR_FORWARD);
 */
void ChassisSetMotion(int16_t target_yaw, uint32_t duration_ms, uint16_t base_pwm, uint8_t direction);

/**
 * @brief 小车底盘控制主循环
 * 
 * 功能：
 * - 读取陀螺仪 Yaw 角
 * - 计算运行时间
 * - 执行 PID 计算（带死区判断）
 * - 更新电机 PWM（差速控制）
 * - 检查运动时间是否到达
 * 
 * @note 必须在主循环中周期性调用（建议 10-20ms 调用一次）
 */
void ChassisUpdate(void);

/**
 * @brief 停止小车运动
 * 
 * 功能：
 * - 停止所有电机
 * - 重置 PID 控制器
 * - 清除运动状态
 */
void ChassisStop(void);

/**
 * @brief 获取小车当前状态
 * 
 * @return 小车状态结构体指针
 * 
 * @note 用于调试和监测，可以读取当前 Yaw 角、误差、PID 输出等信息
 * 
 * @example
 * Chassis_t *state = ChassisGetState();
 * printf("Yaw: %d, Error: %d, PID: %d\n", 
 *        state->current_yaw, state->yaw_error, state->pid_output);
 */
Chassis_t* ChassisGetState(void);

/**
 * @brief 检查小车是否正在运动
 * 
 * @return 1: 正在运动，0: 停止状态
 */
uint8_t ChassisIsMoving(void);

#endif /* _DRV_CHASSIS_H_ */
