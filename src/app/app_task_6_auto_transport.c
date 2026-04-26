/*
 * Task 6: 自主搬运 - 实现
 */

#include "app_task_6_auto_transport.h"
#include "drivers/drv_motor.h"

static Task6_State_t g_task_state = TASK_6_STATE_IDLE;
static uint32_t g_task_start_time = 0;
static const uint32_t g_task_timeout = 60000;

void Task6_Init(void)
{
    g_task_state = TASK_6_STATE_INIT;
    Motor_Stop();
    /* TODO: 初始化摄像头和搬运机构 */
    g_task_state = TASK_6_STATE_RECOGNIZING;
    g_task_start_time = 0;
}

void Task6_Run(void)
{
    if (g_task_state == TASK_6_STATE_IDLE) {
        return;
    }
    
    uint32_t elapsed_time = 0;  /* TODO: 计算运行时间 */
    if (elapsed_time > g_task_timeout) {
        g_task_state = TASK_6_STATE_FAILED;
        Motor_Stop();
        return;
    }
    
    switch (g_task_state) {
        case TASK_6_STATE_RECOGNIZING:
            /* TODO: 识别数字 */
            break;
        case TASK_6_STATE_SEARCHING:
            /* TODO: 搜索物品 */
            break;
        case TASK_6_STATE_MOVING_TO_ITEM:
            /* TODO: 循迹到物品位置 */
            break;
        case TASK_6_STATE_GRASPING:
            /* TODO: 搬运物品 */
            break;
        case TASK_6_STATE_RETURNING:
            /* TODO: 返回停车区 */
            break;
        default:
            break;
    }
}

void Task6_Stop(void)
{
    Motor_Stop();
    g_task_state = TASK_6_STATE_IDLE;
}

void Task6_Reset(void)
{
    g_task_state = TASK_6_STATE_IDLE;
    g_task_start_time = 0;
}

Task6_State_t Task6_GetState(void)
{
    return g_task_state;
}

bool Task6_IsSuccess(void)
{
    return g_task_state == TASK_6_STATE_SUCCESS;
}
