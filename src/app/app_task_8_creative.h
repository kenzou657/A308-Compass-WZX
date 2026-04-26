/*
 * Task 8: 创意任务
 * 
 * 功能：其他创意任务（待定）
 */

#ifndef APP_TASK_8_CREATIVE_H
#define APP_TASK_8_CREATIVE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TASK_8_STATE_IDLE,
    TASK_8_STATE_INIT,
    TASK_8_STATE_RUNNING,
    TASK_8_STATE_SUCCESS,
    TASK_8_STATE_FAILED,
} Task8_State_t;

void Task8_Init(void);
void Task8_Run(void);
void Task8_Stop(void);
void Task8_Reset(void);
Task8_State_t Task8_GetState(void);
bool Task8_IsSuccess(void);

#endif /* APP_TASK_8_CREATIVE_H */
