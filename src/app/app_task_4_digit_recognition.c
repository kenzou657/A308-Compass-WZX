/**
 * @file app_task_4_digit_recognition.c
 * @brief Task 4: 数字识别 - 实现文件
 * 
 * 功能描述：
 * - 启动摄像头识别
 * - 3s 内识别数字标签
 * - 蜂鸣器提示识别结果
 * 
 * 状态机流程：
 * IDLE → INIT → RUNNING(识别中) → SUCCESS/FAILED/TIMEOUT
 */

#include "app_task_4_digit_recognition.h"
#include "../drivers/drv_buzzer.h"
#include "../app/app_camera_uart.h"
#include "../utils/timer.h"
#include <stdlib.h>

typedef struct {
    TaskState_t state;
    uint32_t start_time;
    uint32_t elapsed_time;
    uint32_t timeout_ms;            /* 3s 超时 */
    
    uint8_t recognized_digit;       /* 识别到的数字（1-5） */
    uint8_t recognition_complete;   /* 识别是否完成 */
    
    uint32_t update_count;
} Task4_Context_t;

static Task4_Context_t g_task4_ctx;
extern volatile uint32_t uwTick;

void Task4_Init(void)
{
    /* TODO: 实现初始化逻辑
     * 
     * 步骤：
     * 1. 初始化任务状态为 TASK_STATE_INIT
     * 2. 记录任务开始时间
     * 3. 设置超时时间为 3000ms
     * 4. 初始化识别结果为 0
     * 5. 启动摄像头识别：
     *    - 调用 CameraUART_StartRecognition()
     * 6. 蜂鸣器提示：BuzzerBeep(100)
     * 7. 设置任务状态为 TASK_STATE_RUNNING
     */
}

void Task4_Run(void)
{
    /* TODO: 实现主循环逻辑
     * 
     * 步骤：
     * 1. 检查任务状态
     * 2. 计算已运行时间
     * 3. 超时检查：
     *    if (elapsed_time > timeout_ms) {
     *        state = TASK_STATE_TIMEOUT
     *        BuzzerBeep(500)
     *        return
     *    }
     * 4. 检查识别结果：
     *    - 调用 CameraUART_GetRecognitionResult()
     *    - 如果识别成功，保存结果并设置 recognition_complete = 1
     * 5. 如果识别完成：
     *    - 设置任务状态为 TASK_STATE_SUCCESS
     *    - 蜂鸣器提示识别结果（例如：识别到数字3，蜂鸣3次）
     * 6. 更新计数器
     */
}

void Task4_Stop(void)
{
    /* TODO: 实现停止逻辑
     * 
     * 步骤：
     * 1. 停止摄像头识别：CameraUART_StopRecognition()
     * 2. 设置任务状态为 TASK_STATE_IDLE
     */
}

void Task4_Reset(void)
{
    /* TODO: 实现重置逻辑
     * 
     * 步骤：
     * 1. 设置任务状态为 TASK_STATE_IDLE
     * 2. 清除运行时间
     * 3. 清除识别结果
     * 4. 重置计数器
     */
}

TaskState_t Task4_GetState(void)
{
    return TASK_STATE_IDLE;
}

bool Task4_IsSuccess(void)
{
    return false;
}

uint8_t Task4_GetRecognizedDigit(void)
{
    /* TODO: 实现获取识别结果逻辑
     * 
     * 步骤：
     * 1. 返回 g_task4_ctx.recognized_digit
     */
    return 0;
}
