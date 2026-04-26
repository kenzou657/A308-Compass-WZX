/*
 * Task 3: 双点往返
 * 
 * 功能：在Task 2基础上返回停车区，总时间30±2s
 */

#ifndef APP_TASK_3_ROUND_TRIP_H
#define APP_TASK_3_ROUND_TRIP_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TASK_3_STATE_IDLE,
    TASK_3_STATE_INIT,
    TASK_3_STATE_RUNNING_P1,
    TASK_3_STATE_WAIT_2S,
    TASK_3_STATE_RUNNING_P2,
    TASK_3_STATE_RUNNING_HOME,
    TASK_3_STATE_SUCCESS,
    TASK_3_STATE_FAILED,
} Task3_State_t;

void Task3_Init(void);
void Task3_Run(void);
void Task3_Stop(void);
void Task3_Reset(void);
Task3_State_t Task3_GetState(void);
bool Task3_IsSuccess(void);

#endif /* APP_TASK_3_ROUND_TRIP_H */
