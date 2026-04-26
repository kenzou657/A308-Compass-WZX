/*
 * Task 2: 双点循迹
 * 
 * 功能：循迹到目的地1停靠2秒，再循迹到目的地2并停止
 * 
 * 状态机：
 * IDLE → INIT → RUNNING_P1 → WAIT_2S → RUNNING_P2 → SUCCESS/FAILED → IDLE
 */

#ifndef APP_TASK_2_DUAL_POINT_H
#define APP_TASK_2_DUAL_POINT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TASK_2_STATE_IDLE,
    TASK_2_STATE_INIT,
    TASK_2_STATE_RUNNING_P1,
    TASK_2_STATE_WAIT_2S,
    TASK_2_STATE_RUNNING_P2,
    TASK_2_STATE_SUCCESS,
    TASK_2_STATE_FAILED,
} Task2_State_t;

void Task2_Init(void);
void Task2_Run(void);
void Task2_Stop(void);
void Task2_Reset(void);
Task2_State_t Task2_GetState(void);
bool Task2_IsSuccess(void);

#endif /* APP_TASK_2_DUAL_POINT_H */
