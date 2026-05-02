/**
 * @file app_task_4_digit_recognition.h
 * @brief Task 4: 数字识别 - 头文件
 * 
 * 功能描述：
 * - 识别数字标签（1-5）
 * - 3s 内识别并提示
 * - 蜂鸣器或语音提示识别结果
 * 
 * 赛题要求：
 * - 选手 1 次抽签获取目的地
 * - 将对应自制数字标签放于停车启动区
 * - 一键启动小车
 * - 小车需 3s 内识别任务数字
 * - 用蜂鸣器或语音示意识别结果
 */

#ifndef APP_TASK_4_DIGIT_RECOGNITION_H
#define APP_TASK_4_DIGIT_RECOGNITION_H

#include <stdint.h>
#include <stdbool.h>
#include "app_task_manager.h"

void Task4_Init(void);
void Task4_Run(void);
void Task4_Stop(void);
void Task4_Reset(void);
TaskState_t Task4_GetState(void);
bool Task4_IsSuccess(void);

/**
 * @brief 获取识别到的数字
 * @return 识别到的数字（1-5），0 表示未识别
 */
uint8_t Task4_GetRecognizedDigit(void);

#endif /* APP_TASK_4_DIGIT_RECOGNITION_H */
