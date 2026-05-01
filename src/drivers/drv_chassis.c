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
#include <stdlib.h>

/* ============ 全局变量定义 ============ */
Chassis_t g_chassis;

/* ============ 外部变量声明 ============ */
extern volatile uint32_t uwTick;  // 系统滴答计数（在 ti_msp_dl_config.c 或 timer.c 中定义）

/* ============ 内部辅助函数声明 ============ */
static int16_t Chassis_LimitPWM(int32_t pwm);
static int16_t Chassis_CalculatePID(void);
static void Chassis_UpdateMotors(void);

/* ============ 函数实现 ============ */

/**
 * @brief 初始化小车底盘控制模块
 */
void ChassisInit(void)
{
    // 初始化陀螺仪 Yaw 角位置式 PID
    PID_Position_Init(
        &g_chassis.yaw_pid,
        GYRO_YAW_PID_KP,
        GYRO_YAW_PID_KI,
        GYRO_YAW_PID_KD,
        GYRO_PID_INTEGRAL_MAX,
        GYRO_PID_INTEGRAL_MIN,
        GYRO_PID_OUTPUT_MAX,
        GYRO_PID_OUTPUT_MIN
    );
    
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
    
    // 重置 PID 控制器
    PID_Position_Reset(&g_chassis.yaw_pid);
    
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
    
    // ===== 4. 计算 PID 输出（带死区判断） =====
    g_chassis.pid_output = Chassis_CalculatePID();
    
    // ===== 5. 更新电机 PWM（差速控制） =====
    Chassis_UpdateMotors();
}

/**
 * @brief 停止小车运动
 */
void ChassisStop(void)
{
    // 停止所有电机
    MotorStopAll();
    
    // 重置 PID 控制器
    PID_Position_Reset(&g_chassis.yaw_pid);
    
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

/**
 * @brief 重置陀螺仪 PID 控制器
 */
void ChassisResetPID(void)
{
    PID_Position_Reset(&g_chassis.yaw_pid);
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
 * @brief 计算 PID 输出（带死区判断）
 * @return PID 输出值（差速值）
 */
static int16_t Chassis_CalculatePID(void)
{
    int16_t pid_output;
    
    // 计算误差
    g_chassis.yaw_error = g_chassis.target_yaw - g_chassis.current_yaw;
    
    // 死区判断：误差小于 0.5° 时不动作
    if (abs(g_chassis.yaw_error) < GYRO_DEADZONE) {
        pid_output = 0;
    } else {
        // 执行位置式 PID 计算
        pid_output = PID_Position_Calculate(
            &g_chassis.yaw_pid,
            g_chassis.target_yaw,
            g_chassis.current_yaw
        );
    }
    
    return pid_output;
}

/**
 * @brief 更新电机 PWM（差速控制）
 * 
 * 差速控制策略：
 * - 当 PID 输出为正（需要向右转）：
 *   左电机减速（base_pwm - pid_output）
 *   右电机加速（base_pwm + pid_output）
 * - 当 PID 输出为负（需要向左转）：
 *   左电机加速（base_pwm - pid_output）
 *   右电机减速（base_pwm + pid_output）
 */
static void Chassis_UpdateMotors(void)
{
    int32_t left_pwm_raw, right_pwm_raw;
    
    // 计算差速 PWM（原始值）
    left_pwm_raw = g_chassis.base_pwm - g_chassis.pid_output;
    right_pwm_raw = g_chassis.base_pwm + g_chassis.pid_output;
    
    // 限幅处理
    g_chassis.motor_left_pwm = Chassis_LimitPWM(left_pwm_raw);
    g_chassis.motor_right_pwm = Chassis_LimitPWM(right_pwm_raw);
    
    // 根据运动方向设置电机
    if (g_chassis.direction == CHASSIS_DIR_FORWARD) {
        // 前进
        MotorASet(MOTOR_DIR_FORWARD, g_chassis.motor_left_pwm);
        MotorBSet(MOTOR_DIR_FORWARD, g_chassis.motor_right_pwm);
    } else {
        // 后退
        MotorASet(MOTOR_DIR_REVERSE, g_chassis.motor_left_pwm);
        MotorBSet(MOTOR_DIR_REVERSE, g_chassis.motor_right_pwm);
    }
}
