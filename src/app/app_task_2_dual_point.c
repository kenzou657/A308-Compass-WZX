/*
 * Task 2: 双点循迹 - 实现
 */

#include "app_task_2_dual_point.h"
#include "drivers/drv_motor.h"

static Task2_State_t g_task_state = TASK_2_STATE_IDLE;
static uint32_t g_task_start_time = 0;
static uint32_t g_wait_start_time = 0;
static const uint32_t g_task_timeout = 30000;

void Task2_Init(void)
{
    g_task_state = TASK_2_STATE_INIT;
    Motor_Stop();
    /* TODO: 初始化循迹传感器 */
    g_task_state = TASK_2_STATE_RUNNING_P1;
    g_task_start_time = 0;
}

void Task2_Run(void)
{
    if (g_task_state == TASK_2_STATE_IDLE) {
        return;
    }
    
    uint32_t elapsed_time = 0;  /* TODO: 计算运行时间 */
    if (elapsed_time > g_task_timeout) {
        g_task_state = TASK_2_STATE_FAILED;
        Motor_Stop();
        return;
    }
    
    switch (g_task_state) {
        case TASK_2_STATE_RUNNING_P1:
            /* TODO: 循迹到目的地1 */
            /* if (到达目的地1) {
                g_task_state = TASK_2_STATE_WAIT_2S;
                g_wait_start_time = 0;
                Motor_Stop();
            } */
            break;
            
        case TASK_2_STATE_WAIT_2S:
            /* TODO: 等待2秒 */
            /* if (等待时间 >= 2000ms) {
                g_task_state = TASK_2_STATE_RUNNING_P2;
            } */
            break;
            
        case TASK_2_STATE_RUNNING_P2:
            /* TODO: 循迹到目的地2 */
            /* if (到达目的地2) {
                g_task_state = TASK_2_STATE_SUCCESS;
                Motor_Stop();
            } */
            break;
            
        default:
            break;
    }
}

void Task2_Stop(void)
{
    Motor_Stop();
    g_task_state = TASK_2_STATE_IDLE;
}

void Task2_Reset(void)
{
    g_task_state = TASK_2_STATE_IDLE;
    g_task_start_time = 0;
}

Task2_State_t Task2_GetState(void)
{
    return g_task_state;
}

bool Task2_IsSuccess(void)
{
    return g_task_state == TASK_2_STATE_SUCCESS;
}
