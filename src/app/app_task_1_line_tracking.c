/*
 * Task 1: 单点循迹 - 实现
 */

#include "app_task_1_line_tracking.h"
#include "drivers/drv_motor.h"

/* 任务状态 */
static Task1_State_t g_task_state = TASK_1_STATE_IDLE;
static uint32_t g_task_start_time = 0;
static const uint32_t g_task_timeout = 30000;  /* 30s超时 */

/* 任务初始化 */
void Task1_Init(void)
{
    g_task_state = TASK_1_STATE_INIT;
    g_task_start_time = 0;
    
    /* 初始化电机 */
    Motor_Stop();
    
    /* 初始化传感器 */
    /* TODO: 初始化循迹传感器 */
    
    g_task_state = TASK_1_STATE_RUNNING;
    g_task_start_time = 0;  /* TODO: 使用系统时间 */
}

/* 任务运行 */
void Task1_Run(void)
{
    if (g_task_state != TASK_1_STATE_RUNNING) {
        return;
    }
    
    /* 检查超时 */
    uint32_t elapsed_time = 0;  /* TODO: 计算运行时间 */
    if (elapsed_time > g_task_timeout) {
        g_task_state = TASK_1_STATE_FAILED;
        Motor_Stop();
        return;
    }
    
    /* 任务逻辑 */
    /* TODO: 实现循迹逻辑 */
    
    /* 状态转移 */
    /* TODO: 检查是否到达目标区域 */
    /* if (到达目标) {
        g_task_state = TASK_1_STATE_SUCCESS;
        Motor_Stop();
    } */
}

/* 任务停止 */
void Task1_Stop(void)
{
    Motor_Stop();
    g_task_state = TASK_1_STATE_IDLE;
}

/* 任务重置 */
void Task1_Reset(void)
{
    g_task_state = TASK_1_STATE_IDLE;
    g_task_start_time = 0;
}

/* 获取任务状态 */
Task1_State_t Task1_GetState(void)
{
    return g_task_state;
}

/* 获取任务结果 */
bool Task1_IsSuccess(void)
{
    return g_task_state == TASK_1_STATE_SUCCESS;
}
