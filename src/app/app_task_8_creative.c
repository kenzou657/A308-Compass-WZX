/*
 * Task 8: 创意任务 - 实现
 */

#include "app_task_8_creative.h"
#include "drivers/drv_motor.h"

static Task8_State_t g_task_state = TASK_8_STATE_IDLE;
static uint32_t g_task_start_time = 0;
static const uint32_t g_task_timeout = 60000;

void Task8_Init(void)
{
    g_task_state = TASK_8_STATE_INIT;
    Motor_Stop();
    /* TODO: 初始化创意任务所需的模块 */
    g_task_state = TASK_8_STATE_RUNNING;
    g_task_start_time = 0;
}

void Task8_Run(void)
{
    if (g_task_state == TASK_8_STATE_IDLE) {
        return;
    }
    
    uint32_t elapsed_time = 0;  /* TODO: 计算运行时间 */
    if (elapsed_time > g_task_timeout) {
        g_task_state = TASK_8_STATE_FAILED;
        Motor_Stop();
        return;
    }
    
    switch (g_task_state) {
        case TASK_8_STATE_RUNNING:
            /* TODO: 实现创意任务逻辑 */
            break;
        default:
            break;
    }
}

void Task8_Stop(void)
{
    Motor_Stop();
    g_task_state = TASK_8_STATE_IDLE;
}

void Task8_Reset(void)
{
    g_task_state = TASK_8_STATE_IDLE;
    g_task_start_time = 0;
}

Task8_State_t Task8_GetState(void)
{
    return g_task_state;
}

bool Task8_IsSuccess(void)
{
    return g_task_state == TASK_8_STATE_SUCCESS;
}
