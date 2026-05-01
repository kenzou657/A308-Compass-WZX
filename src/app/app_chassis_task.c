/**
 * @file app_chassis_task.c
 * @brief 小车底盘任务 - 实现文件
 * 
 * 状态机流程：
 * IDLE -> STAGE1 (目标角度 0°，5000ms) -> STAGE2 (目标角度 -45°，5000ms) -> COMPLETE
 */

#include "app_chassis_task.h"
#include "../drivers/drv_chassis.h"
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
}

/**
 * @brief 启动小车底盘任务
 */
void ChassisTaskStart(void)
{
    // 重置任务状态为第一阶段
    g_chassis_task.state = CHASSIS_TASK_STATE_STAGE1;
    g_chassis_task.stage_start_tick = uwTick;
    g_chassis_task.elapsed_ms = 0;
    
    // 启动第一阶段：目标角度 0°，直行 5000ms
    ChassisSetMotion(
        CHASSIS_TASK_STAGE1_YAW,
        CHASSIS_TASK_STAGE1_DURATION,
        CHASSIS_TASK_BASE_PWM,
        CHASSIS_DIR_FORWARD
    );
}

/**
 * @brief 小车底盘任务主循环（状态机）
 * 
 * 状态转移逻辑：
 * - STAGE1：运行 5000ms 后切换到 STAGE2
 * - STAGE2：运行 5000ms 后切换到 COMPLETE
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
    g_chassis_task.elapsed_ms = (uwTick - g_chassis_task.stage_start_tick) * 20;  // 假设 uwTick 周期为 10ms
    
    switch (g_chassis_task.state) {
        case CHASSIS_TASK_STATE_STAGE1:
            // 第一阶段：检查是否运行时间到达
            if (g_chassis_task.elapsed_ms >= CHASSIS_TASK_STAGE1_DURATION) {
                // 切换到第二阶段
                g_chassis_task.state = CHASSIS_TASK_STATE_STAGE2;
                g_chassis_task.stage_start_tick = uwTick;
                g_chassis_task.elapsed_ms = 0;
                
                // 启动第二阶段：目标角度 -45°，直行 5000ms
                ChassisSetMotion(
                    CHASSIS_TASK_STAGE2_YAW,
                    CHASSIS_TASK_STAGE2_DURATION,
                    CHASSIS_TASK_BASE_PWM,
                    CHASSIS_DIR_FORWARD
                );
            }
            break;
        
        case CHASSIS_TASK_STATE_STAGE2:
            // 第二阶段：检查是否运行时间到达
            if (g_chassis_task.elapsed_ms >= CHASSIS_TASK_STAGE2_DURATION) {
                // 任务完成
                g_chassis_task.state = CHASSIS_TASK_STATE_COMPLETE;
                
                // 停止小车
                ChassisStop();
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
 * @return 任务状态（IDLE/STAGE1/STAGE2/COMPLETE）
 */
uint8_t ChassisTaskGetState(void)
{
    return g_chassis_task.state;
}
