/**
 * @file pid.h
 * @brief PID 控制器模块 - 头文件
 * 
 * 提供两种 PID 控制算法：
 * 1. 增量式 PID - 用于电机闭环速度控制
 * 2. 位置式 PID - 用于陀螺仪角度控制
 * 
 * 使用整数运算，避免浮点数计算
 */

#ifndef _PID_H_
#define _PID_H_

#include <stdint.h>

/* ============ PID 控制器类型定义 ============ */

/**
 * @brief 增量式 PID 控制器结构体
 * 
 * 增量式 PID 输出为增量值：
 * Δu(k) = Kp[e(k) - e(k-1)] + Ki*e(k) + Kd[e(k) - 2*e(k-1) + e(k-2)]
 * 
 * 适用于：电机速度闭环控制（PWM 占空比增量调整）
 */
typedef struct {
    int16_t kp;              // 比例系数 Kp（整数表示，实际值 = kp/1000）
    int16_t ki;              // 积分系数 Ki（整数表示，实际值 = ki/1000）
    int16_t kd;              // 微分系数 Kd（整数表示，实际值 = kd/1000）
    
    int16_t error_prev;      // 上一次误差 e(k-1)
    int16_t error_prev2;     // 上上次误差 e(k-2)
    int16_t output_prev;     // 上一次输出 u(k-1)
    
    int16_t output_max;      // 输出限幅上限
    int16_t output_min;      // 输出限幅下限
} PID_Increment_t;

/**
 * @brief 位置式 PID 控制器结构体
 * 
 * 位置式 PID 输出为绝对值：
 * u(k) = Kp*e(k) + Ki*∑e(i) + Kd[e(k) - e(k-1)]
 * 
 * 适用于：陀螺仪角度控制、舵机角度控制
 */
typedef struct {
    int16_t kp;              // 比例系数 Kp（整数表示，实际值 = kp/1000）
    int16_t ki;              // 积分系数 Ki（整数表示，实际值 = ki/1000）
    int16_t kd;              // 微分系数 Kd（整数表示，实际值 = kd/1000）
    
    int16_t error_prev;      // 上一次误差 e(k-1)
    int32_t integral_sum;    // 积分累加值 ∑e(i)
    
    int16_t integral_max;    // 积分限幅上限
    int16_t integral_min;    // 积分限幅下限
    
    int16_t output_max;      // 输出限幅上限
    int16_t output_min;      // 输出限幅下限
} PID_Position_t;

/* ============ 增量式 PID 函数声明 ============ */

/**
 * @brief 初始化增量式 PID 控制器
 * @param pid: PID 控制器指针
 * @param kp: 比例系数（整数表示，实际值 = kp/100）
 * @param ki: 积分系数（整数表示，实际值 = ki/100）
 * @param kd: 微分系数（整数表示，实际值 = kd/100）
 * @param output_max: 输出限幅上限
 * @param output_min: 输出限幅下限
 */
void PID_Increment_Init(PID_Increment_t *pid, int16_t kp, int16_t ki, int16_t kd,
                         int16_t output_max, int16_t output_min);

/** 注：增量式 PID 系数仍使用 /100 缩放 */

/**
 * @brief 计算增量式 PID 输出
 * @param pid: PID 控制器指针
 * @param setpoint: 目标值（设定值）
 * @param feedback: 反馈值（实际值）
 * @return 增量输出值 Δu(k)
 */
int16_t PID_Increment_Calculate(PID_Increment_t *pid, int16_t setpoint, int16_t feedback);

/**
 * @brief 重置增量式 PID 控制器
 * @param pid: PID 控制器指针
 */
void PID_Increment_Reset(PID_Increment_t *pid);

/* ============ 位置式 PID 函数声明 ============ */

/**
 * @brief 初始化位置式 PID 控制器
 * @param pid: PID 控制器指针
 * @param kp: 比例系数（整数表示，实际值 = kp/1000）
 * @param ki: 积分系数（整数表示，实际值 = ki/1000）
 * @param kd: 微分系数（整数表示，实际值 = kd/1000）
 * @param integral_max: 积分限幅上限
 * @param integral_min: 积分限幅下限
 * @param output_max: 输出限幅上限
 * @param output_min: 输出限幅下限
 */
void PID_Position_Init(PID_Position_t *pid, int16_t kp, int16_t ki, int16_t kd,
                       int16_t integral_max, int16_t integral_min,
                       int16_t output_max, int16_t output_min);

/**
 * @brief 计算位置式 PID 输出
 * @param pid: PID 控制器指针
 * @param setpoint: 目标值（设定值）
 * @param feedback: 反馈值（实际值）
 * @return 绝对输出值 u(k)
 */
int32_t PID_Position_Calculate(PID_Position_t *pid, int32_t setpoint, int32_t feedback);

/**
 * @brief 重置位置式 PID 控制器
 * @param pid: PID 控制器指针
 */
void PID_Position_Reset(PID_Position_t *pid);

#endif /* _PID_H_ */
