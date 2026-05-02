/**
 * @file drv_chassis.c
 * @brief 小车底盘运动控制模块 - 实现文件
 * 
 * 核心功能：
 * 1. 陀螺仪 Yaw 角闭环控制（位置式 PID）
 * 2. 运行时间控制（基于 uwTick）
 * 3. 差速控制实现转向
 * 4. 死区机制防止抖动
 */

#include "drv_chassis.h"
#include "drv_motor.h"
#include "drv_jy61p.h"
#include "../config.h"
#include <stdlib.h>

/* ============ 全局变量定义 ============ */
Chassis_t g_chassis;

/* ============ 外部变量声明 ============ */
extern volatile uint32_t uwTick;  // 系统滴答计数（在 ti_msp_dl_config.c 或 timer.c 中定义）

/* ============ 内部辅助函数声明 ============ */
static int16_t Chassis_LimitPWM(int32_t pwm);
static int16_t Chassis_CalculatePID(void);
static void Chassis_UpdateMotors(void);
static int16_t Chassis_NormalizeYawError(int16_t target_yaw, int16_t current_yaw);

/* ============ 函数实现 ============ */

/**
 * @brief 初始化小车底盘控制模块
 */
void ChassisInit(void)
{
    // 初始化状态变量
    g_chassis.state = CHASSIS_STATE_IDLE;
    g_chassis.target_yaw = 0;
    g_chassis.motion_duration_ms = 0;
    g_chassis.base_pwm = CHASSIS_DEFAULT_BASE_PWM;
    g_chassis.direction = CHASSIS_DIR_FORWARD;
    
    g_chassis.motion_start_tick = 0;
    g_chassis.elapsed_ms = 0;
    
    g_chassis.current_yaw = 0;
    g_chassis.yaw_error = 0;
    g_chassis.pid_output = 0;
    
    g_chassis.motor_left_pwm = 0;
    g_chassis.motor_right_pwm = 0;
    
    g_chassis.update_count = 0;
    g_chassis.gyro_data_valid = 0;
    
    // 停止所有电机
    MotorStopAll();
}

/**
 * @brief 设置小车运动参数
 */
void ChassisSetMotion(int16_t target_yaw, uint32_t duration_ms, uint16_t base_pwm, uint8_t direction)
{
    // 限制 PWM 范围
    if (base_pwm > CHASSIS_MAX_BASE_PWM) {
        base_pwm = CHASSIS_MAX_BASE_PWM;
    }
    
    // 设置运动参数
    g_chassis.target_yaw = target_yaw;
    g_chassis.motion_duration_ms = duration_ms;
    g_chassis.base_pwm = base_pwm;
    g_chassis.direction = direction;
    
    // 记录运动开始时刻
    g_chassis.motion_start_tick = uwTick;
    g_chassis.elapsed_ms = 0;
    
    // 设置原地转弯标志（base_pwm == 0 时为原地转弯）
    g_chassis.is_turning = (base_pwm == 0) ? 1 : 0;
    
    // 设置为运动状态
    g_chassis.state = CHASSIS_STATE_MOVING;
}

/**
 * @brief 小车底盘控制主循环
 */
void ChassisUpdate(void)
{
    // 如果不在运动状态，直接返回
    if (g_chassis.state != CHASSIS_STATE_MOVING) {
        return;
    }
    
    // 更新计数
    g_chassis.update_count++;
    
    // ===== 1. 读取陀螺仪 Yaw 角 =====
    jy61p_angle_t angle;
    if (jy61p_get_angle(&angle) == 0) {
        g_chassis.current_yaw = angle.yaw_deg_x100;
        g_chassis.gyro_data_valid = 1;
    } else {
        g_chassis.gyro_data_valid = 0;
        // 如果陀螺仪数据无效，使用上一次的值继续控制
    }
    
    // ===== 2. 计算运行时间 =====
    g_chassis.elapsed_ms = (uwTick - g_chassis.motion_start_tick) * CHASSIS_TICK_PERIOD_MS;
    
    // ===== 3. 检查运动时间是否到达 =====
    if (g_chassis.elapsed_ms >= g_chassis.motion_duration_ms) {
        ChassisStop();
        return;
    }
    
    // ===== 4. 计算偏航角误差（处理-180°边界跳变） =====
    g_chassis.yaw_error = Chassis_NormalizeYawError(g_chassis.target_yaw, g_chassis.current_yaw);
    
    // ===== 5. 更新电机 PWM（P 环差速控制） =====
    Chassis_UpdateMotors();
}

/**
 * @brief 停止小车运动
 */
void ChassisStop(void)
{
    // 停止所有电机
    MotorStopAll();
    
    // 清除运动状态
    g_chassis.state = CHASSIS_STATE_IDLE;
    g_chassis.motor_left_pwm = 0;
    g_chassis.motor_right_pwm = 0;
    g_chassis.pid_output = 0;
}

/**
 * @brief 获取小车当前状态
 */
Chassis_t* ChassisGetState(void)
{
    return &g_chassis;
}

/**
 * @brief 检查小车是否正在运动
 */
uint8_t ChassisIsMoving(void)
{
    return (g_chassis.state == CHASSIS_STATE_MOVING);
}

/* ============ 内部辅助函数实现 ============ */

/**
 * @brief 限制 PWM 值在有效范围内
 * @param pwm: 输入 PWM 值
 * @return 限制后的 PWM 值（0-PWM_MAX）
 */
static int16_t Chassis_LimitPWM(int32_t pwm)
{
    if (pwm < PWM_MIN) {
        return PWM_MIN;
    } else if (pwm > PWM_MAX) {
        return PWM_MAX;
    } else {
        return (int16_t)pwm;
    }
}

/**
 * @brief 更新电机 PWM（差速控制）
 *
 * 运动模式区分：
 * 1. 直行模式（base_pwm > 0）：
 *    - 使用直行专用系数 GYRO_YAW_PID_KP
 *    - 差速控制：左电机 = base_pwm - pwm_delta，右电机 = base_pwm + pwm_delta
 *
 * 2. 转弯模式（base_pwm == 0）：
 *    - 使用转弯专用系数 GYRO_YAW_PID_KP_TURN
 *    - 转弯方向由 yaw_error 的正负决定：
 *      * yaw_error > 0：向右转（左电机前进，右电机后退）
 *      * yaw_error < 0：向左转（左电机后退，右电机前进）
 */
static void Chassis_UpdateMotors(void)
{
    volatile int32_t left_pwm_raw, right_pwm_raw;
    volatile int32_t pwm_delta;  // PWM 差速值
    int32_t kp_value;   // 当前使用的 Kp 系数
    
    // ===== 1. 根据运动模式选择 Kp 系数 =====
    if (g_chassis.base_pwm > 0) {
        // 直行模式：使用直行专用系数
        kp_value = GYRO_YAW_PID_KP;
    } else {
        // 转弯模式（base_pwm == 0）：使用转弯专用系数
        kp_value = GYRO_YAW_PID_KP_TURN;
    }
    
    // ===== 2. 计算 PWM 差速值 =====
    // 公式：PWM_delta = yaw_error × Kp / 1000 × GYRO_PID_TO_PWM_NUM / GYRO_PID_TO_PWM_DEN
    // 简化为：PWM_delta = yaw_error × (Kp × GYRO_PID_TO_PWM_NUM) / (1000 × GYRO_PID_TO_PWM_DEN)
    pwm_delta = ((int32_t)g_chassis.yaw_error * kp_value * GYRO_PID_TO_PWM_NUM) /
               (1000 * GYRO_PID_TO_PWM_DEN);
    
    // ===== 3. 限幅处理（防止差速值过大） =====
    if (pwm_delta > GYRO_PID_OUTPUT_MAX) {
        pwm_delta = GYRO_PID_OUTPUT_MAX;
    } else if (pwm_delta < GYRO_PID_OUTPUT_MIN) {
        pwm_delta = GYRO_PID_OUTPUT_MIN;
    }
    
    // ===== 4. 根据运动模式计算电机 PWM =====
    if (g_chassis.base_pwm > 0) {
        // 直行模式：差速控制
        left_pwm_raw = g_chassis.base_pwm - pwm_delta;
        right_pwm_raw = g_chassis.base_pwm + pwm_delta;
        
        // 直行模式限幅：当 PWM 小于 0 时，输出 0（防止反向）
        if (left_pwm_raw < 0) {
            left_pwm_raw = 0;
        }
        if (right_pwm_raw < 0) {
            right_pwm_raw = 0;
        }
    } else {
        // 转弯模式：原地转弯
        // yaw_error > 0 时向右转：左电机前进，右电机后退
        // yaw_error < 0 时向左转：左电机后退，右电机前进
        left_pwm_raw = -pwm_delta;      // 左电机 PWM（正值前进，负值后退）
        right_pwm_raw = pwm_delta;    // 右电机 PWM（与左电机相反）
    }
    
    // ===== 5. 限幅处理 =====
    g_chassis.motor_left_pwm = Chassis_LimitPWM(left_pwm_raw);
    g_chassis.motor_right_pwm = Chassis_LimitPWM(right_pwm_raw);
    
    // ===== 6. 根据运动方向和模式设置电机 =====
    if (g_chassis.base_pwm > 0) {
        // 直行模式：根据 direction 设置方向
        if (g_chassis.direction == CHASSIS_DIR_FORWARD) {
            MotorASet(MOTOR_DIR_FORWARD, g_chassis.motor_left_pwm);
            MotorBSet(MOTOR_DIR_FORWARD, g_chassis.motor_right_pwm);
        } else {
            MotorASet(MOTOR_DIR_REVERSE, g_chassis.motor_left_pwm);
            MotorBSet(MOTOR_DIR_REVERSE, g_chassis.motor_right_pwm);
        }
    } else {
        // 转弯模式：根据 pwm_delta 的正负决定方向
        // pwm_delta > 0：向右转（左前右后）
        // pwm_delta < 0：向左转（左后右前）
        if (g_chassis.motor_left_pwm >= 0) {
            MotorASet(MOTOR_DIR_FORWARD, g_chassis.motor_left_pwm);
        } else {
            MotorASet(MOTOR_DIR_REVERSE, -g_chassis.motor_left_pwm);
        }
        
        if (g_chassis.motor_right_pwm >= 0) {
            MotorBSet(MOTOR_DIR_FORWARD, g_chassis.motor_right_pwm);
        } else {
            MotorBSet(MOTOR_DIR_REVERSE, -g_chassis.motor_right_pwm);
        }
    }
}

/**
 * @brief 计算角度差值，处理-180°边界的跳变问题
 *
 * 功能：
 * - 计算 target_yaw - current_yaw 的最小差值
 * - 处理-180°和180°的等价性
 * - 避免-175到175之间的跳变
 *
 * 例如：
 * - target_yaw = -18000 (-180°), current_yaw = 17500 (175°)
 *   直接相减：-18000 - 17500 = -35500（错误）
 *   归一化后：-18000 - (-18500) = 500（正确，向右转）
 *
 * - target_yaw = -18000 (-180°), current_yaw = -17500 (-175°)
 *   直接相减：-18000 - (-17500) = -500（正确）
 *   归一化后：-18000 - (-17500) = -500（正确）
 *
 * @param target_yaw 目标角度（°×100，范围 [-18000, 18000)）
 * @param current_yaw 当前角度（°×100，范围 [-18000, 18000)）
 * @return 角度差值（°×100，范围 [-18000, 18000)）
 */
static int16_t Chassis_NormalizeYawError(int16_t target_yaw, int16_t current_yaw)
{
    int32_t error = (int32_t)target_yaw - (int32_t)current_yaw;
    
    /* 将误差归一化到 [-18000, 18000) 范围 */
    /* 如果误差 > 18000，说明应该向反方向转（更短的路径） */
    if (error > 18000) {
        error -= 36000;
    }
    /* 如果误差 < -18000，说明应该向反方向转（更短的路径） */
    else if (error < -18000) {
        error += 36000;
    }
    
    return (int16_t)error;
}
