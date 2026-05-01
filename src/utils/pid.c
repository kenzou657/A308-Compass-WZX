/**
 * @file pid.c
 * @brief PID 控制器模块 - 实现文件
 *
 * 实现增量式和位置式 PID 控制算法
 * 使用整数运算，避免浮点数计算
 */

#include "pid.h"
#include <stddef.h>

/* ============ 增量式 PID 实现 ============ */

/**
 * @brief 初始化增量式 PID 控制器
 */
void PID_Increment_Init(PID_Increment_t *pid, int16_t kp, int16_t ki, int16_t kd,
                        int16_t output_max, int16_t output_min)
{
    if (pid == NULL) {
        return;
    }
    
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    
    pid->error_prev = 0;
    pid->error_prev2 = 0;
    pid->output_prev = 0;
    
    pid->output_max = output_max;
    pid->output_min = output_min;
}

/**
 * @brief 计算增量式 PID 输出
 * 
 * 增量式 PID 公式：
 * Δu(k) = Kp[e(k) - e(k-1)] + Ki*e(k) + Kd[e(k) - 2*e(k-1) + e(k-2)]
 * 
 * 其中：
 * - e(k) = setpoint - feedback（当前误差）
 * - e(k-1) = 上一次误差
 * - e(k-2) = 上上次误差
 * - Δu(k) = 增量输出
 */
int16_t PID_Increment_Calculate(PID_Increment_t *pid, int16_t setpoint, int16_t feedback)
{
    if (pid == NULL) {
        return 0;
    }
    
    int32_t temp;           // 临时变量，用于中间计算
    int16_t error;          // 当前误差 e(k)
    int16_t delta_output;   // 增量输出 Δu(k)
    int16_t output;         // 最终输出 u(k)
    
    // 1. 计算当前误差
    error = setpoint - feedback;
    
    // 2. 计算比例项：Kp[e(k) - e(k-1)]
    delta_output = (int32_t)pid->kp * (error - pid->error_prev);
    
    // 3. 计算积分项：Ki*e(k)
    delta_output += (int32_t)pid->ki * error;
    
    // 4. 计算微分项：Kd[e(k) - 2*e(k-1) + e(k-2)]
    delta_output += (int32_t)pid->kd * (error - 2 * pid->error_prev + pid->error_prev2);
    
    // 5. 计算最终输出：u(k) = u(k-1) + Δu(k)
    temp = (int32_t)pid->output_prev + delta_output;
    
    // 6. 输出限幅
    if (temp > pid->output_max) {
        output = pid->output_max;
    } else if (temp < pid->output_min) {
        output = pid->output_min;
    } else {
        output = (int16_t)temp;
    }
    
    // 7. 更新历史数据
    pid->error_prev2 = pid->error_prev;
    pid->error_prev = error;
    pid->output_prev = output;
    
    return output;
}

/**
 * @brief 重置增量式 PID 控制器
 */
void PID_Increment_Reset(PID_Increment_t *pid)
{
    if (pid == NULL) {
        return;
    }
    
    pid->error_prev = 0;
    pid->error_prev2 = 0;
    pid->output_prev = 0;
}

/* ============ 位置式 PID 实现 ============ */

/**
 * @brief 初始化位置式 PID 控制器
 */
void PID_Position_Init(PID_Position_t *pid, int16_t kp, int16_t ki, int16_t kd,
                       int16_t integral_max, int16_t integral_min,
                       int16_t output_max, int16_t output_min)
{
    if (pid == NULL) {
        return;
    }
    
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    
    pid->error_prev = 0;
    pid->integral_sum = 0;
    
    pid->integral_max = integral_max;
    pid->integral_min = integral_min;
    
    pid->output_max = output_max;
    pid->output_min = output_min;
}

/**
 * @brief 计算位置式 PID 输出
 * 
 * 位置式 PID 公式：
 * u(k) = Kp*e(k) + Ki*∑e(i) + Kd[e(k) - e(k-1)]
 * 
 * 其中：
 * - e(k) = setpoint - feedback（当前误差）
 * - e(k-1) = 上一次误差
 * - ∑e(i) = 误差积分累加值
 * - u(k) = 绝对输出
 */
int32_t PID_Position_Calculate(PID_Position_t *pid, int32_t setpoint, int32_t feedback)
{
    if (pid == NULL) {
        return 0;
    }
    
    int32_t temp;           // 临时变量，用于中间计算
    int32_t error;          // 当前误差 e(k)
    int32_t output;         // 最终输出 u(k)
    
    // 1. 计算当前误差
    error = setpoint - feedback;
    
    // 2. 计算比例项：Kp*e(k) / 1000
    output = ((int32_t)pid->kp * error) / 100;
    
    // 3. 计算积分项：Ki*∑e(i) / 1000
    // 先累加误差
    pid->integral_sum += error;
    
    // 积分限幅
    if (pid->integral_sum > pid->integral_max) {
        pid->integral_sum = pid->integral_max;
    } else if (pid->integral_sum < pid->integral_min) {
        pid->integral_sum = pid->integral_min;
    }
    
    output += ((int32_t)pid->ki * pid->integral_sum) / 100;
    
    // 4. 计算微分项：Kd[e(k) - e(k-1)] / 1000
    output += ((int32_t)pid->kd * (error - pid->error_prev)) / 100;
    
    // 5. 输出限幅
    if (output > pid->output_max) {
        output = pid->output_max;
    } else if (output < pid->output_min) {
        output = pid->output_min;
    }
    
    // 6. 更新历史数据
    pid->error_prev = error;
    
    return output;
}

/**
 * @brief 重置位置式 PID 控制器
 */
void PID_Position_Reset(PID_Position_t *pid)
{
    if (pid == NULL) {
        return;
    }
    
    pid->error_prev = 0;
    pid->integral_sum = 0;
}
