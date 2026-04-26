/*
 * Task 7: 全自动搬运 - 实现
 */

#include "app_task_7_batch_transport.h"
#include "drivers/drv_motor.h"

static Task7_State_t g_task_state = TASK_7_STATE_IDLE;
static uint32_t g_task_start_time = 0;
static const uint32_t g_task_timeout = 120000;  /* 2分钟超时 */

void Task7_Init(void)
{
    g_task_state = TASK_7_STATE_INIT;
    Motor_Stop();
    /* TODO: 初始化摄像头和搬运机构 */
    g_task_state = TASK_7_STATE_SCANNING;
    g_task_start_time = 0;
}

void Task7_Run(void)
{
    if (g_task_state == TASK_7_STATE_IDLE) {
        return;
    }
    
    uint32_t elapsed_time = 0;  /* TODO: 计算运行时间 */
    if (elapsed_time > g_task_timeout) {
        g_task_state = TASK_7_STATE_FAILED;
        Motor_Stop();
        return;
    }
    
    switch (g_task_state) {
        case TASK_7_STATE_SCANNING:
            /* TODO: 扫描所有物品 */
            break;
        case TASK_7_STATE_MOVING_TO_ITEM:
            /* TODO: 循迹到物品位置 */
            break;
        case TASK_7_STATE_GRASPING:
            /* TODO: 搬运物品 */
            break;
        case TASK_7_STATE_MOVING_TO_STORAGE:
            /* TODO: 循迹到存放区 */
            break;
        case TASK_7_STATE_PLACING:
            /* TODO: 放置物品 */
            break;
        default:
            break;
    }
}

void Task7_Stop(void)
{
    Motor_Stop();
    g_task_state = TASK_7_STATE_IDLE;
}

void Task7_Reset(void)
{
    g_task_state = TASK_7_STATE_IDLE;
    g_task_start_time = 0;
}

Task7_State_t Task7_GetState(void)
{
    return g_task_state;
}

bool Task7_IsSuccess(void)
{
    return g_task_state == TASK_7_STATE_SUCCESS;
}
