/**
 * @file app_task_8_creative.h
 * @brief Task 8: 创意任务 - 头文件
 * 
 * 功能描述：
 * - 创意任务，可根据实际情况自定义
 * - 例如：多物品同时搬运、优化搬运路径、增加难度等
 */

#ifndef APP_TASK_8_CREATIVE_H
#define APP_TASK_8_CREATIVE_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

void Task8_Init(void);
void Task8_Run(void);
void Task8_Stop(void);
void Task8_Reset(void);
TaskState_t Task8_GetState(void);
bool Task8_IsSuccess(void);

#endif /* APP_TASK_8_CREATIVE_H */
