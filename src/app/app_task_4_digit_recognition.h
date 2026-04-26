/*
 * Task 4: 数字识别
 * 
 * 功能：识别停车区的数字标签(1~5)，3s内识别并提示
 */

#ifndef APP_TASK_4_DIGIT_RECOGNITION_H
#define APP_TASK_4_DIGIT_RECOGNITION_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TASK_4_STATE_IDLE,
    TASK_4_STATE_INIT,
    TASK_4_STATE_RECOGNIZING,
    TASK_4_STATE_RECOGNIZED,
    TASK_4_STATE_SUCCESS,
    TASK_4_STATE_FAILED,
} Task4_State_t;

void Task4_Init(void);
void Task4_Run(void);
void Task4_Stop(void);
void Task4_Reset(void);
Task4_State_t Task4_GetState(void);
bool Task4_IsSuccess(void);

#endif /* APP_TASK_4_DIGIT_RECOGNITION_H */
