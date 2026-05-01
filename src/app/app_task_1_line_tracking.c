/**
 * @file app_task_1_line_tracking.c
 * @brief Task 1: 单点循迹 - 实现文件
 * 
 * 功能描述：
 * - 从停车启动区循迹到指定目标区
 * - 到达后停止并提示
 * - 支持目标区 1-5 的快速切换
 * 
 * 状态机流程：
 * IDLE → INIT → RUNNING → SUCCESS/FAILED/TIMEOUT
 * 
 * 实现要点：
 * - 使用路径规划模块执行多阶段运动
 * - 基于 uwTick 进行时间控制
 * - 超时检查：运动时间超过预期时间 20% 则失败
 */

#include "app_task_1_line_tracking.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../utils/timer.h"
#include <stdlib.h>

/* ==================== 任务上下文结构 ==================== */

/**
 * @brief Task 1 上下文结构
 */
typedef struct {
    TaskState_t state;              /* 任务状态 */
    uint8_t target_zone;            /* 目标区编号（1-5） */
    uint32_t start_time;            /* 任务开始时间 */
    uint32_t elapsed_time;          /* 已运行时间 */
    uint32_t timeout_ms;            /* 超时时间 */
    
    /* 路径规划相关 */
    uint16_t current_path_point;    /* 当前路径点索引 */
    uint32_t path_point_start_time; /* 路径点开始时间 */
    uint8_t path_state;             /* 路径执行状态（0: 运动中, 1: 停顿中, 2: 完成） */
    
    /* 调试信息 */
    uint32_t update_count;          /* 更新次数计数 */
} Task1_Context_t;

/* ==================== 全局变量 ==================== */

/**
 * @brief Task 1 上下文
 */
static Task1_Context_t g_task1_ctx;

/* ==================== 外部变量声明 ==================== */

/**
 * @brief 系统滴答计数（1ms 周期）
 */
extern volatile uint32_t uwTick;

/* ==================== 函数实现 ==================== */

/**
 * @brief 初始化 Task 1
 * 
 * 功能：
 * - 初始化任务状态为 INIT
 * - 设置目标区编号
 * - 初始化路径规划
 * - 启动底盘运动
 * - 蜂鸣器提示：任务开始
 * 
 * @note 必须在 TaskManager_StartTask() 中调用
 */
void Task1_Init(void)
{
    /* TODO: 实现初始化逻辑
     * 
     * 步骤：
     * 1. 初始化任务状态为 TASK_STATE_INIT
     * 2. 记录任务开始时间：g_task1_ctx.start_time = uwTick
     * 3. 设置超时时间：g_task1_ctx.timeout_ms = 15000（15s）
     * 4. 初始化路径规划：
     *    - 根据 target_zone 查询预定义路径表
     *    - 初始化路径点索引：current_path_point = 0
     *    - 初始化路径状态：path_state = 0（运动中）
     * 5. 启动底盘运动：
     *    - 调用 ChassisSetMotion() 启动第一个路径点
     *    - 参数：target_yaw, duration_ms, base_pwm, direction
     * 6. 蜂鸣器提示：BuzzerBeep(100)（100ms 蜂鸣）
     * 7. 设置任务状态为 TASK_STATE_RUNNING
     */
}

/**
 * @brief Task 1 主循环
 * 
 * 功能：
 * - 计算已运行时间
 * - 超时检查
 * - 执行路径规划
 * - 检查任务完成
 * 
 * @note 应在主循环中定期调用（建议 10ms 周期）
 */
void Task1_Run(void)
{
    /* TODO: 实现主循环逻辑
     * 
     * 步骤：
     * 1. 检查任务状态：如果不是 RUNNING，直接返回
     * 2. 计算已运行时间：
     *    g_task1_ctx.elapsed_time = uwTick - g_task1_ctx.start_time
     * 3. 超时检查：
     *    if (elapsed_time > timeout_ms) {
     *        state = TASK_STATE_TIMEOUT
     *        ChassisStop()
     *        BuzzerBeep(500)  // 500ms 蜂鸣（失败提示）
     *        return
     *    }
     * 4. 执行路径规划：
     *    - 调用 PathExecute() 执行当前路径点
     *    - 检查路径点是否完成
     *    - 如果完成，移动到下一个路径点
     * 5. 检查任务完成：
     *    if (所有路径点完成) {
     *        state = TASK_STATE_SUCCESS
     *        ChassisStop()
     *        BuzzerBeep(200)  // 200ms 蜂鸣（成功提示）
     *    }
     * 6. 更新计数器：g_task1_ctx.update_count++
     */
}

/**
 * @brief 停止 Task 1
 * 
 * 功能：
 * - 停止底盘运动
 * - 设置任务状态为 IDLE
 */
void Task1_Stop(void)
{
    /* TODO: 实现停止逻辑
     * 
     * 步骤：
     * 1. 停止底盘运动：ChassisStop()
     * 2. 设置任务状态为 TASK_STATE_IDLE
     */
}

/**
 * @brief 重置 Task 1
 * 
 * 功能：
 * - 重置任务状态为 IDLE
 * - 清除运行时间
 * - 重置路径规划
 */
void Task1_Reset(void)
{
    /* TODO: 实现重置逻辑
     * 
     * 步骤：
     * 1. 设置任务状态为 TASK_STATE_IDLE
     * 2. 清除运行时间：
     *    - g_task1_ctx.start_time = 0
     *    - g_task1_ctx.elapsed_time = 0
     * 3. 重置路径规划：
     *    - g_task1_ctx.current_path_point = 0
     *    - g_task1_ctx.path_state = 0
     * 4. 重置计数器：g_task1_ctx.update_count = 0
     */
}

/**
 * @brief 获取 Task 1 的当前状态
 * 
 * @return 任务状态（IDLE/INIT/RUNNING/SUCCESS/FAILED/TIMEOUT）
 */
TaskState_t Task1_GetState(void)
{
    /* TODO: 实现获取状态逻辑
     * 
     * 步骤：
     * 1. 返回 g_task1_ctx.state
     */
    return TASK_STATE_IDLE;  /* 临时返回 */
}

/**
 * @brief 判断 Task 1 是否成功完成
 * 
 * @return true 表示成功，false 表示未成功
 */
bool Task1_IsSuccess(void)
{
    /* TODO: 实现判断成功逻辑
     * 
     * 步骤：
     * 1. 返回 (g_task1_ctx.state == TASK_STATE_SUCCESS)
     */
    return false;  /* 临时返回 */
}

/**
 * @brief 设置目标区编号
 * 
 * @param zone 目标区编号（1-5）
 * 
 * @note 必须在 Task1_Init() 之前调用
 */
void Task1_SetTargetZone(uint8_t zone)
{
    /* TODO: 实现设置目标区逻辑
     * 
     * 步骤：
     * 1. 检查输入有效性：if (zone >= 1 && zone <= 5)
     * 2. 设置目标区：g_task1_ctx.target_zone = zone
     */
}

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 执行路径规划（内部函数）
 * 
 * 功能：
 * - 根据当前路径点执行运动
 * - 检查路径点是否完成
 * - 移动到下一个路径点
 * 
 * @note 这是内部函数，不对外暴露
 */
static void Task1_ExecutePath(void)
{
    /* TODO: 实现路径规划执行逻辑
     * 
     * 步骤：
     * 1. 检查是否所有路径点都已完成
     * 2. 获取当前路径点信息
     * 3. 根据路径状态处理：
     *    - 如果是运动中（path_state == 0）：
     *      检查运动时间是否到达，如果到达则停止电机并进入停顿状态
     *    - 如果是停顿中（path_state == 1）：
     *      检查停顿时间是否到达，如果到达则移动到下一个路径点
     * 4. 更新路径状态
     */
}

/**
 * @brief 获取目标区的路径规划（内部函数）
 * 
 * @param zone 目标区编号（1-5）
 * @return 指向路径规划表的指针
 * 
 * @note 这是内部函数，不对外暴露
 */
static const void* Task1_GetPathTable(uint8_t zone)
{
    /* TODO: 实现获取路径规划表逻辑
     * 
     * 步骤：
     * 1. 根据 zone 查询预定义的路径规划表
     * 2. 返回对应的路径规划表指针
     * 
     * 路径规划表应包含：
     * - 目标区1：直行（0°）
     * - 目标区2：左转45°
     * - 目标区3：左转90°
     * - 目标区4：右转45°
     * - 目标区5：右转90°
     */
    return NULL;  /* 临时返回 */
}
