/**
 * @file app_task_1_line_tracking.h
 * @brief Task 1: 单点循迹 - 头文件
 * 
 * 功能描述：
 * - 从停车启动区循迹到指定目标区
 * - 到达后停止并提示
 * - 支持目标区 1-5 的快速切换
 * 
 * 赛题要求：
 * - 选手 1 次抽签获取随机目的地（物品存放区 1~5）
 * - 人机交互启动小车
 * - 小车自主循迹至对应区域并停止
 * - 启动前与到达后用蜂鸣器或语音提示
 * - 小车投影需完全落在区域黑色边框内
 */

#ifndef APP_TASK_1_LINE_TRACKING_H
#define APP_TASK_1_LINE_TRACKING_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

/* ==================== 任务初始化 ==================== */

/**
 * @brief 初始化 Task 1
 * 
 * 功能：
 * - 初始化任务状态
 * - 启动底盘运动
 * - 蜂鸣器提示
 */
void Task1_Init(void);

/* ==================== 任务主循环 ==================== */

/**
 * @brief Task 1 主循环
 * 
 * 功能：
 * - 执行路径规划
 * - 检查运动时间
 * - 判断任务完成
 * - 超时处理
 * 
 * @note 应在主循环中定期调用（建议 10ms 周期）
 */
void Task1_Run(void);

/* ==================== 任务停止 ==================== */

/**
 * @brief 停止 Task 1
 * 
 * 功能：
 * - 停止底盘运动
 * - 设置任务状态为 IDLE
 */
void Task1_Stop(void);

/* ==================== 任务重置 ==================== */

/**
 * @brief 重置 Task 1
 * 
 * 功能：
 * - 重置任务状态
 * - 清除运行时间
 * - 重置路径规划
 */
void Task1_Reset(void);

/* ==================== 获取任务状态 ==================== */

/**
 * @brief 获取 Task 1 的当前状态
 * 
 * @return 任务状态（IDLE/INIT/RUNNING/SUCCESS/FAILED/TIMEOUT）
 */
TaskState_t Task1_GetState(void);

/* ==================== 判断任务是否成功 ==================== */

/**
 * @brief 判断 Task 1 是否成功完成
 * 
 * @return true 表示成功，false 表示未成功
 */
bool Task1_IsSuccess(void);

/* ==================== 任务参数设置 ==================== */

/**
 * @brief 设置目标区编号
 * 
 * @param zone 目标区编号（1-5）
 * 
 * @note 必须在 Task1_Init() 之前调用
 */
void Task1_SetTargetZone(uint8_t zone);

#endif /* APP_TASK_1_LINE_TRACKING_H */
