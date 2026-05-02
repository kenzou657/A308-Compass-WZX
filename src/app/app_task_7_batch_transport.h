/**
 * @file app_task_7_batch_transport.h
 * @brief Task 7: 全自动搬运 - 头文件
 * 
 * 功能描述：
 * - 物品随机放置于存放区
 * - 选手一键启动小车
 * - 小车自主将物品按顺序搬运至对应存放区
 * - 停车启动区可作为临时存放区
 * - 按顺序成功还原一个物品即可获得对应分值
 */

#ifndef APP_TASK_7_BATCH_TRANSPORT_H
#define APP_TASK_7_BATCH_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

void Task7_Init(void);
void Task7_Run(void);
void Task7_Stop(void);
void Task7_Reset(void);
TaskState_t Task7_GetState(void);
bool Task7_IsSuccess(void);

#endif /* APP_TASK_7_BATCH_TRANSPORT_H */
