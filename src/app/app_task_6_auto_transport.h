/*
 * Task 6: 自主搬运
 * 
 * 功能：识别数字后自主找到物品并搬运回停车区
 */

#ifndef APP_TASK_6_AUTO_TRANSPORT_H
#define APP_TASK_6_AUTO_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TASK_6_STATE_IDLE,
    TASK_6_STATE_INIT,
    TASK_6_STATE_RECOGNIZING,
    TASK_6_STATE_SEARCHING,
    TASK_6_STATE_MOVING_TO_ITEM,
    TASK_6_STATE_GRASPING,
    TASK_6_STATE_RETURNING,
    TASK_6_STATE_SUCCESS,
    TASK_6_STATE_FAILED,
} Task6_State_t;

void Task6_Init(void);
void Task6_Run(void);
void Task6_Stop(void);
void Task6_Reset(void);
Task6_State_t Task6_GetState(void);
bool Task6_IsSuccess(void);

#endif /* APP_TASK_6_AUTO_TRANSPORT_H */
