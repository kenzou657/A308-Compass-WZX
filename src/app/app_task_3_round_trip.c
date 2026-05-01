/**
 * @file app_task_3_round_trip.c
 * @brief Task 3: 双点往返 - 实现文件
 * 
 * 功能描述：
 * - 在 Task 2 基础上返回停车启动区
 * - 总时间控制在 30±2s 内
 * - 需要动态调整运动速度以满足时间要求
 * 
 * 状态机流程：
 * IDLE → INIT → RUNNING(前往目标1) → RUNNING(停顿2s) → RUNNING(前往目标2) → RUNNING(返回停车区) → SUCCESS/FAILED/TIMEOUT
 */

#include "app_task_3_round_trip.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../utils/timer.h"
#include <stdlib.h>

typedef struct {
    TaskState_t state;
    uint8_t target_zone_1;
    uint8_t target_zone_2;
    uint32_t start_time;
    uint32_t elapsed_time;
    uint32_t total_time_limit;      /* 30±2s */
    
    uint8_t phase;                  /* 0: 前往目标1, 1: 停顿2s, 2: 前往目标2, 3: 返回停车区 */
    uint32_t phase_start_time;
    
    uint16_t current_path_point;
    uint32_t path_point_start_time;
    uint8_t path_state;
    
    uint32_t update_count;
} Task3_Context_t;

static Task3_Context_t g_task3_ctx;
extern volatile uint32_t uwTick;

void Task3_Init(void)
{
    /* TODO: 实现初始化逻辑
     * 
     * 关键点：
     * - 设置总时间限制为 30s
     * - 初始化 4 个阶段
     * - 需要预留时间用于动态速度调整
     */
}

void Task3_Run(void)
{
    /* TODO: 实现主循环逻辑
     * 
     * 关键点：
     * - 实时计算已用时间
     * - 根据剩余时间动态调整 PWM
     * - 确保在 30±2s 内完成
     * - 4 个阶段的状态转移
     */
}

void Task3_Stop(void)
{
    /* TODO: 实现停止逻辑 */
}

void Task3_Reset(void)
{
    /* TODO: 实现重置逻辑 */
}

TaskState_t Task3_GetState(void)
{
    return TASK_STATE_IDLE;
}

bool Task3_IsSuccess(void)
{
    return false;
}

void Task3_SetTargetZones(uint8_t zone1, uint8_t zone2)
{
    /* TODO: 实现设置目标区逻辑 */
}
