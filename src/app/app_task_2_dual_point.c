/**
 * @file app_task_2_dual_point.c
 * @brief Task 2: 双点循迹 - 实现文件
 * 
 * 功能描述：
 * - 循迹到目标区1，停止
 * - 停顿 2 秒
 * - 循迹到目标区2，停止
 * 
 * 状态机流程：
 * IDLE → INIT → RUNNING(前往目标1) → RUNNING(停顿2s) → RUNNING(前往目标2) → SUCCESS/FAILED/TIMEOUT
 */

#include "app_task_2_dual_point.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../utils/timer.h"
#include <stdlib.h>

/* ==================== 任务上下文结构 ==================== */

typedef struct {
    TaskState_t state;
    uint8_t target_zone_1;
    uint8_t target_zone_2;
    uint32_t start_time;
    uint32_t elapsed_time;
    uint32_t timeout_ms;
    
    /* 阶段控制 */
    uint8_t phase;                  /* 0: 前往目标1, 1: 停顿2s, 2: 前往目标2 */
    uint32_t phase_start_time;
    
    /* 路径规划相关 */
    uint16_t current_path_point;
    uint32_t path_point_start_time;
    uint8_t path_state;
    
    uint32_t update_count;
} Task2_Context_t;

static Task2_Context_t g_task2_ctx;
extern volatile uint32_t uwTick;

/* ==================== 函数实现 ==================== */

void Task2_Init(void)
{
    /* TODO: 实现初始化逻辑
     * 
     * 步骤：
     * 1. 初始化任务状态为 TASK_STATE_INIT
     * 2. 记录任务开始时间
     * 3. 设置超时时间：g_task2_ctx.timeout_ms = 20000（20s）
     * 4. 初始化阶段为 0（前往目标1）
     * 5. 初始化路径规划
     * 6. 启动底盘运动到目标1
     * 7. 蜂鸣器提示
     * 8. 设置任务状态为 TASK_STATE_RUNNING
     */
}

void Task2_Run(void)
{
    /* TODO: 实现主循环逻辑
     * 
     * 步骤：
     * 1. 检查任务状态
     * 2. 计算已运行时间
     * 3. 超时检查
     * 4. 根据阶段执行不同的逻辑：
     *    - 阶段0：前往目标1
     *      执行路径规划，检查是否到达
     *      到达后进入阶段1，蜂鸣器提示
     *    - 阶段1：停顿2s
     *      检查停顿时间是否到达
     *      到达后初始化路径到目标2，进入阶段2
     *    - 阶段2：前往目标2
     *      执行路径规划，检查是否到达
     *      到达后设置任务状态为 SUCCESS，蜂鸣器提示
     * 5. 更新计数器
     */
}

void Task2_Stop(void)
{
    /* TODO: 实现停止逻辑 */
}

void Task2_Reset(void)
{
    /* TODO: 实现重置逻辑 */
}

TaskState_t Task2_GetState(void)
{
    /* TODO: 实现获取状态逻辑 */
    return TASK_STATE_IDLE;
}

bool Task2_IsSuccess(void)
{
    /* TODO: 实现判断成功逻辑 */
    return false;
}

void Task2_SetTargetZones(uint8_t zone1, uint8_t zone2)
{
    /* TODO: 实现设置目标区逻辑
     * 
     * 步骤：
     * 1. 检查输入有效性
     * 2. 设置两个目标区
     */
}
