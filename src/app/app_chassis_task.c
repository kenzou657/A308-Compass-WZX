/**
 * @file app_chassis_task.c
 * @brief 小车底盘任务 - 实现文件
 *
 * 状态机流程：
 * IDLE -> STAGE1 (0°，5000ms) -> STOP_PAUSE1 (停车0.5s) -> TURN1 (-45°，2000ms) ->
 * STOP_PAUSE2 (停车0.5s) -> STAGE2 (-45°，5000ms) -> STOP_PAUSE3 (停车0.5s) -> COMPLETE
 */

#include "app_chassis_task.h"
#include "../drivers/drv_chassis.h"
#include "../utils/timer.h"
#include <stdlib.h>

/* ============ 全局变量定义 ============ */
ChassisTask_t g_chassis_task;

/* ============ 外部变量声明 ============ */
extern volatile uint32_t uwTick;  // 系统滴答计数

/* ============ 函数实现 ============ */

/**
 * @brief 初始化小车底盘任务
 */
void ChassisTaskInit(void)
{
    g_chassis_task.state = CHASSIS_TASK_STATE_IDLE;
    g_chassis_task.stage_start_tick = 0;
    g_chassis_task.elapsed_ms = 0;
    
    // 初始化动态参数为默认值
    g_chassis_task.stage1_yaw = CHASSIS_TASK_STAGE1_YAW;
    g_chassis_task.stage1_duration = CHASSIS_TASK_STAGE1_DURATION;
    g_chassis_task.turn1_yaw = CHASSIS_TASK_TURN1_YAW;
    g_chassis_task.turn1_duration = CHASSIS_TASK_TURN1_DURATION;
    g_chassis_task.stage2_yaw = CHASSIS_TASK_STAGE2_YAW;
    g_chassis_task.stage2_duration = CHASSIS_TASK_STAGE2_DURATION;
    g_chassis_task.base_pwm = CHASSIS_TASK_BASE_PWM;
    g_chassis_task.turn_pwm = CHASSIS_TASK_TURN_PWM;
}

/**
 * @brief 启动小车底盘任务（使用默认参数）
 */
void ChassisTaskStart(void)
{
    // 重置任务状态为第一阶段
    g_chassis_task.state = CHASSIS_TASK_STATE_STAGE1;
    g_chassis_task.stage_start_tick = uwTick;
    g_chassis_task.elapsed_ms = 0;
    
    // 启动第一阶段：使用结构体中的参数
    ChassisSetMotion(
        g_chassis_task.stage1_yaw,
        g_chassis_task.stage1_duration,
        g_chassis_task.base_pwm,
        CHASSIS_DIR_FORWARD
    );
}

/**
 * @brief 启动小车底盘任务（参数化版本）
 */
void ChassisTaskStartWithParams(
    int16_t stage1_yaw,
    uint32_t stage1_duration,
    int16_t turn1_yaw,
    uint32_t turn1_duration,
    int16_t stage2_yaw,
    uint32_t stage2_duration,
    uint16_t base_pwm,
    uint16_t turn_pwm
)
{
    // 设置动态参数
    g_chassis_task.stage1_yaw = stage1_yaw;
    g_chassis_task.stage1_duration = stage1_duration;
    g_chassis_task.turn1_yaw = turn1_yaw;
    g_chassis_task.turn1_duration = turn1_duration;
    g_chassis_task.stage2_yaw = stage2_yaw;
    g_chassis_task.stage2_duration = stage2_duration;
    g_chassis_task.base_pwm = base_pwm;
    g_chassis_task.turn_pwm = turn_pwm;
    
    // 重置任务状态为第一阶段
    g_chassis_task.state = CHASSIS_TASK_STATE_STAGE1;
    g_chassis_task.stage_start_tick = uwTick;
    g_chassis_task.elapsed_ms = 0;
    
    // 启动第一阶段
    ChassisSetMotion(
        g_chassis_task.stage1_yaw,
        g_chassis_task.stage1_duration,
        g_chassis_task.base_pwm,
        CHASSIS_DIR_FORWARD
    );
}

/**
 * @brief 小车底盘任务主循环（状态机）
 *
 * 状态转移逻辑：
 * - STAGE1：运行 5000ms 后切换到 STOP_PAUSE1
 * - STOP_PAUSE1：停车 0.5s 后切换到 TURN1
 * - TURN1：原地转弯 2000ms 后切换到 STOP_PAUSE2
 * - STOP_PAUSE2：停车 0.5s 后切换到 STAGE2
 * - STAGE2：运行 5000ms 后切换到 STOP_PAUSE3
 * - STOP_PAUSE3：停车 0.5s 后切换到 COMPLETE
 * - COMPLETE：保持完成状态
 */
void ChassisTaskUpdate(void)
{
    // 如果任务未启动或已完成，直接返回
    if (g_chassis_task.state == CHASSIS_TASK_STATE_IDLE ||
        g_chassis_task.state == CHASSIS_TASK_STATE_COMPLETE) {
        return;
    }
    
    // 计算当前阶段已运行时间
    g_chassis_task.elapsed_ms = (uwTick - g_chassis_task.stage_start_tick);  // 假设 uwTick 周期为 1ms
    
    switch (g_chassis_task.state) {
        case CHASSIS_TASK_STATE_STAGE1:
            // 第一阶段：检查是否运行时间到达
            if (g_chassis_task.elapsed_ms >= g_chassis_task.stage1_duration) {
                // 切换到停车暂停1
                g_chassis_task.state = CHASSIS_TASK_STATE_STOP_PAUSE1;
                g_chassis_task.stage_start_tick = uwTick;
                g_chassis_task.elapsed_ms = 0;
                
                // 停止小车
                ChassisStop();
            }
            break;
        
        case CHASSIS_TASK_STATE_STOP_PAUSE1:
            // 停车暂停1：延时 0.5s
            if (g_chassis_task.elapsed_ms >= 500) {
                // 切换到转弯阶段1
                g_chassis_task.state = CHASSIS_TASK_STATE_TURN1;
                g_chassis_task.stage_start_tick = uwTick;
                g_chassis_task.elapsed_ms = 0;
                
                // 启动转弯阶段1：使用动态参数
                ChassisSetMotion(
                    g_chassis_task.turn1_yaw,
                    g_chassis_task.turn1_duration,
                    g_chassis_task.turn_pwm,
                    CHASSIS_DIR_FORWARD
                );
            }
            break;
        
        case CHASSIS_TASK_STATE_TURN1:
            // 转弯阶段1：检查是否运行时间到达
            if (g_chassis_task.elapsed_ms >= g_chassis_task.turn1_duration) {
                // 切换到停车暂停2
                g_chassis_task.state = CHASSIS_TASK_STATE_STOP_PAUSE2;
                g_chassis_task.stage_start_tick = uwTick;
                g_chassis_task.elapsed_ms = 0;
                
                // 停止小车
                ChassisStop();
            }
            break;
        
        case CHASSIS_TASK_STATE_STOP_PAUSE2:
            // 停车暂停2：延时 0.5s
            if (g_chassis_task.elapsed_ms >= 500) {
                // 切换到第二阶段
                g_chassis_task.state = CHASSIS_TASK_STATE_STAGE2;
                g_chassis_task.stage_start_tick = uwTick;
                g_chassis_task.elapsed_ms = 0;
                
                // 启动第二阶段：使用动态参数
                ChassisSetMotion(
                    g_chassis_task.stage2_yaw,
                    g_chassis_task.stage2_duration,
                    g_chassis_task.base_pwm,
                    CHASSIS_DIR_FORWARD
                );
            }
            break;
        
        case CHASSIS_TASK_STATE_STAGE2:
            // 第二阶段：检查是否运行时间到达
            if (g_chassis_task.elapsed_ms >= g_chassis_task.stage2_duration) {
                // 切换到停车暂停3
                g_chassis_task.state = CHASSIS_TASK_STATE_STOP_PAUSE3;
                g_chassis_task.stage_start_tick = uwTick;
                g_chassis_task.elapsed_ms = 0;
                
                // 停止小车
                ChassisStop();
            }
            break;
        
        case CHASSIS_TASK_STATE_STOP_PAUSE3:
            // 停车暂停3：延时 0.5s
            if (g_chassis_task.elapsed_ms >= 500) {
                // 任务完成
                g_chassis_task.state = CHASSIS_TASK_STATE_COMPLETE;
            }
            break;
        
        default:
            break;
    }
}

/**
 * @brief 停止小车底盘任务
 */
void ChassisTaskStop(void)
{
    g_chassis_task.state = CHASSIS_TASK_STATE_IDLE;
    g_chassis_task.stage_start_tick = 0;
    g_chassis_task.elapsed_ms = 0;
    
    // 停止小车运动
    ChassisStop();
}

/**
 * @brief 获取任务当前状态
 * 
 * @return 任务状态（IDLE/STAGE1/TURN1/STAGE2/COMPLETE）
 */
uint8_t ChassisTaskGetState(void)
{
    return g_chassis_task.state;
}
