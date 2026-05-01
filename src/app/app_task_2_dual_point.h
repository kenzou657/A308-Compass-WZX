/**
 * @file app_task_2_dual_point.h
 * @brief Task 2: 双点循迹 - 头文件
 * 
 * 功能描述：
 * - 循迹到目标区1，停止
 * - 停顿 2 秒
 * - 循迹到目标区2，停止
 * 
 * 赛题要求：
 * - 选手 2 次抽签获取不重复目的地
 * - 人机交互启动小车
 * - 先停靠目的地 1 对应区域
 * - 2 秒后前往目的地 2 对应区域并停止
 * - 启动与到达任意目的地均需提示
 * - 投影需完全落在区域内
 */

#ifndef APP_TASK_2_DUAL_POINT_H
#define APP_TASK_2_DUAL_POINT_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

void Task2_Init(void);
void Task2_Run(void);
void Task2_Stop(void);
void Task2_Reset(void);
TaskState_t Task2_GetState(void);
bool Task2_IsSuccess(void);

/**
 * @brief 设置两个目标区编号
 * @param zone1 第一个目标区（1-5）
 * @param zone2 第二个目标区（1-5）
 */
void Task2_SetTargetZones(uint8_t zone1, uint8_t zone2);

#endif /* APP_TASK_2_DUAL_POINT_H */
