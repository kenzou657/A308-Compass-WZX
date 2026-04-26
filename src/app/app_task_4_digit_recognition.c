/*
 * Task 4: 数字识别 - 实现
 */

#include "app_task_4_digit_recognition.h"
#include "drivers/drv_motor.h"

static Task4_State_t g_task_state = TASK_4_STATE_IDLE;
static uint32_t g_task_start_time = 0;
static const uint32_t g_task_timeout = 3000;  /* 3s超时 */

void Task4_Init(void)
{
    g_task_state = TASK_4_STATE_INIT;
    Motor_Stop();
    /* TODO: 初始化摄像头/识别模块 */
    g_task_state = TASK_4_STATE_RECOGNIZING;
    g_task_start_time = 0;
}

void Task4_Run(void)
{
    if (g_task_state == TASK_4_STATE_IDLE) {
        return;
    }
    
    uint32_t elapsed_time = 0;  /* TODO: 计算运行时间 */
    if (elapsed_time > g_task_timeout && g_task_state == TASK_4_STATE_RECOGNIZING) {
        g_task_state = TASK_4_STATE_FAILED;
        return;
    }
    
    switch (g_task_state) {
        case TASK_4_STATE_RECOGNIZING:
            /* TODO: 识别数字 */
            /* if (识别成功) {
                g_task_state = TASK_4_STATE_RECOGNIZED;
            } */
            break;
            
        case TASK_4_STATE_RECOGNIZED:
            /* TODO: 提示识别结果 */
            g_task_state = TASK_4_STATE_SUCCESS;
            break;
            
        default:
            break;
    }
}

void Task4_Stop(void)
{
    Motor_Stop();
    g_task_state = TASK_4_STATE_IDLE;
}

void Task4_Reset(void)
{
    g_task_state = TASK_4_STATE_IDLE;
    g_task_start_time = 0;
}

Task4_State_t Task4_GetState(void)
{
    return g_task_state;
}

bool Task4_IsSuccess(void)
{
    return g_task_state == TASK_4_STATE_SUCCESS;
}
