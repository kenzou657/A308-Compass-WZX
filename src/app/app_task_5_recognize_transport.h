/**
 * @file app_task_5_recognize_transport.h
 * @brief Task 5: 识别+搬运 - 头文件
 * 
 * 功能描述：
 * - 在 Task 4 基础上搬运物品回停车启动区
 * - 识别数字标签
 * - 循迹到对应物品存放区
 * - 搬运物品回停车区
 * - 返回后提示且投影完全落于区内
 * 
 * 赛题要求（发挥部分）：
 * - 贴有 1~5 数字的物品随机放置于各物品存放区（每区 1 个）
 * - 在基础任务 (4) 基础上，小车将目的地物品搬运回停车启动区
 * - 返回后提示且投影完全落于区内，否则任务失败
 */

#ifndef APP_TASK_5_RECOGNIZE_TRANSPORT_H
#define APP_TASK_5_RECOGNIZE_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

void Task5_Init(void);
void Task5_Run(void);
void Task5_Stop(void);
void Task5_Reset(void);
TaskState_t Task5_GetState(void);
bool Task5_IsSuccess(void);

#endif /* APP_TASK_5_RECOGNIZE_TRANSPORT_H */
