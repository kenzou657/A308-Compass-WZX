/*
 * Task 3: 双点往返 - 实现
 */

#include "app_task_3_round_trip.h"
#include "drivers/drv_motor.h"

static Task3_State_t g_task_state = TASK_3_STATE_IDLE;
static uint32_t g_task_start_time = 0;
static uint32_t g_wait_start_time = 0;
static const uint32_t g_task_timeout = 32000;  /* 30±2s */

void Task3_Init(void)
{
    g_task_state = TASK_3_STATE_INIT;
    Motor_Stop();
    /* TODO: 初始化循迹传感器 */
    g_task_state = TASK_3_STATE_RUNNING_P1;
    g_task_start_time = 0;
}

void Task3_Run(void)
{
    if (g_task_state == TASK_3_STATE_IDLE) {
        return;
    }
    
    uint32_t elapsed_time = 0;  /* TODO: 计算运行时间 */
    if (elapsed_time > g_task_timeout) {
        g_task_state = TASK_3_STATE_FAILED;
        Motor_Stop();
        return;
    }
    
    switch (g_task_state) {
        case TASK_3_STATE_RUNNING_P1:
            /* TODO: 循迹到目的地1 */
            break;
        case TASK_3_STATE_WAIT_2S:
            /* TODO: 等待2秒 */
            break;
        case TASK_3_STATE_RUNNING_P2:
            /* TODO: 循迹到目的地2 */
            break;
        case TASK_3_STATE_RUNNING_HOME:
            /* TODO: 循迹返回停车区 */
            break;
        default:
            break;
    }
}

void Task3_Stop(void)
{
    Motor_Stop();
    g_task_state = TASK_3_STATE_IDLE;
}

void Task3_Reset(void)
{
    g_task_state = TASK_3_STATE_IDLE;
    g_task_start_time = 0;
}

Task3_State_t Task3_GetState(void)
{
    return g_task_state;
}

bool Task3_IsSuccess(void)
{
    return g_task_state == TASK_3_STATE_SUCCESS;
}
