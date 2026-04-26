/*
 * Task 1: 单点循迹
 * 
 * 功能：从停车区循迹到指定存放区(1~5)并停止
 * 
 * 状态机：
 * IDLE → INIT → RUNNING → SUCCESS/FAILED → IDLE
 */

#ifndef APP_TASK_1_LINE_TRACKING_H
#define APP_TASK_1_LINE_TRACKING_H

#include <stdint.h>
#include <stdbool.h>

/* 任务状态定义 */
typedef enum {
    TASK_1_STATE_IDLE,
    TASK_1_STATE_INIT,
    TASK_1_STATE_RUNNING,
    TASK_1_STATE_SUCCESS,
    TASK_1_STATE_FAILED,
} Task1_State_t;

/* 任务初始化 */
void Task1_Init(void);

/* 任务运行 */
void Task1_Run(void);

/* 任务停止 */
void Task1_Stop(void);

/* 任务重置 */
void Task1_Reset(void);

/* 获取任务状态 */
Task1_State_t Task1_GetState(void);

/* 获取任务结果 */
bool Task1_IsSuccess(void);

#endif /* APP_TASK_1_LINE_TRACKING_H */
