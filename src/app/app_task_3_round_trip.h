/**
 * @file app_task_3_round_trip.h
 * @brief Task 3: 双点往返 - 头文件
 * 
 * 功能描述：
 * - 在 Task 2 基础上返回停车启动区
 * - 总时间控制在 30±2s 内
 * - 未到达目的地或停车区时中途不可停车
 * 
 * 赛题要求：
 * - 在任务 2 基础上返回停车启动区
 * - 任务执行时间控制在 30±2s 内
 * - 未到达目的地或停车区时中途不可停车
 */

#ifndef APP_TASK_3_ROUND_TRIP_H
#define APP_TASK_3_ROUND_TRIP_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

void Task3_Init(void);
void Task3_Run(void);
void Task3_Stop(void);
void Task3_Reset(void);
TaskState_t Task3_GetState(void);
bool Task3_IsSuccess(void);

void Task3_SetTargetZones(uint8_t zone1, uint8_t zone2);

#endif /* APP_TASK_3_ROUND_TRIP_H */
