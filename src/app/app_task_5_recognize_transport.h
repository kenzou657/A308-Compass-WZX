/*
 * Task 5: 识别+搬运
 * 
 * 功能：识别数字后搬运对应物品回停车区
 */

#ifndef APP_TASK_5_RECOGNIZE_TRANSPORT_H
#define APP_TASK_5_RECOGNIZE_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TASK_5_STATE_IDLE,
    TASK_5_STATE_INIT,
    TASK_5_STATE_RECOGNIZING,
    TASK_5_STATE_MOVING_TO_ITEM,
    TASK_5_STATE_GRASPING,
    TASK_5_STATE_RETURNING,
    TASK_5_STATE_SUCCESS,
    TASK_5_STATE_FAILED,
} Task5_State_t;

void Task5_Init(void);
void Task5_Run(void);
void Task5_Stop(void);
void Task5_Reset(void);
Task5_State_t Task5_GetState(void);
bool Task5_IsSuccess(void);

#endif /* APP_TASK_5_RECOGNIZE_TRANSPORT_H */
