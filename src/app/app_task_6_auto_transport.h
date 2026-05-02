/**
 * @file app_task_6_auto_transport.h
 * @brief Task 6: 自主搬运 - 头文件
 * 
 * 功能描述：
 * - 物品随机放置于存放区
 * - 选手抽签获取目标数字标签并放置于停车区
 * - 一键启动小车
 * - 小车 3s 内识别并提示数字
 * - 自主找到对应物品带回停车区
 * - 返回后提示且投影完全落于区内
 */

#ifndef APP_TASK_6_AUTO_TRANSPORT_H
#define APP_TASK_6_AUTO_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

void Task6_Init(void);
void Task6_Run(void);
void Task6_Stop(void);
void Task6_Reset(void);
TaskState_t Task6_GetState(void);
bool Task6_IsSuccess(void);

#endif /* APP_TASK_6_AUTO_TRANSPORT_H */
