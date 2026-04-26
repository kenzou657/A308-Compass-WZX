/*
 * Task 7: 全自动搬运
 * 
 * 功能：自主将物品按顺序搬运至对应存放区
 */

#ifndef APP_TASK_7_BATCH_TRANSPORT_H
#define APP_TASK_7_BATCH_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TASK_7_STATE_IDLE,
    TASK_7_STATE_INIT,
    TASK_7_STATE_SCANNING,
    TASK_7_STATE_MOVING_TO_ITEM,
    TASK_7_STATE_GRASPING,
    TASK_7_STATE_MOVING_TO_STORAGE,
    TASK_7_STATE_PLACING,
    TASK_7_STATE_SUCCESS,
    TASK_7_STATE_FAILED,
} Task7_State_t;

void Task7_Init(void);
void Task7_Run(void);
void Task7_Stop(void);
void Task7_Reset(void);
Task7_State_t Task7_GetState(void);
bool Task7_IsSuccess(void);

#endif /* APP_TASK_7_BATCH_TRANSPORT_H */
