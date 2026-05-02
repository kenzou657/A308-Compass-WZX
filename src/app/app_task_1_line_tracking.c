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
 * - 使用底盘任务模块执行多阶段运动
 * - 基于 uwTick 进行时间控制
 * - 每次进入任务都进行陀螺仪零点校准
 * - 先沿0角度运动到路径交汇点（5s）
 * - 再根据设定角度转弯面向目标角度（2s）
 * - 再沿目标角度直行
 * - 完成任务后蜂鸣器响1s
 */

#include "app_task_1_line_tracking.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../drivers/drv_jy61p.h"
#include "../utils/timer.h"
#include "../config.h"
#include "app_chassis_task.h"
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
    
    /* 陀螺仪校准相关 */
    uint8_t gyro_calibration_done;  /* 陀螺仪校准完成标志 */
    uint32_t calibration_start_time;/* 校准开始时间 */
    
    /* 底盘任务相关 */
    uint8_t chassis_task_running;   /* 底盘任务运行标志 */
    
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

/* ==================== 辅助函数 ==================== */

/**
 * @brief 根据目的地编号获取目标角度
 * 
 * @param destination 目的地编号（1-5）
 * @return 目标角度（°×100）
 */
static int16_t Task1_GetTargetYaw(uint8_t destination)
{
    switch (destination) {
        case 1:
            return DESTINATION_1_YAW;
        case 2:
            return DESTINATION_2_YAW;
        case 3:
            return DESTINATION_3_YAW;
        case 4:
            return DESTINATION_4_YAW;
        case 5:
            return DESTINATION_5_YAW;
        default:
            return 0;
    }
}

/* ==================== 函数实现 ==================== */

/**
 * @brief 初始化 Task 1
 * 
 * 功能：
 * - 初始化任务状态为 INIT
 * - 启动陀螺仪零点校准
 * - 记录任务开始时间
 * - 设置超时时间
 * 
 * @note 必须在 TaskManager_StartTask() 中调用
 */
void Task1_Init(void)
{
    /* 初始化任务状态为 INIT */
    g_task1_ctx.state = TASK_STATE_INIT;
    
    /* 初始化时间相关变量 */
    g_task1_ctx.start_time = 0;  /* 校准完成后再记录 */
    g_task1_ctx.elapsed_time = 0;
    
    /* 设置超时时间：30s */
    g_task1_ctx.timeout_ms = 30000;
    
    /* 初始化陀螺仪校准标志 */
    g_task1_ctx.gyro_calibration_done = 0;
    g_task1_ctx.calibration_start_time = uwTick;
    
    /* 启动陀螺仪零点校准（采集20个样本） */
    jy61p_start_calibration(GYRO_CALIBRATION_SAMPLES);
    
    /* 初始化底盘任务 */
    ChassisTaskInit();
    g_task1_ctx.chassis_task_running = 0;
    
    /* 初始化调试计数 */
    g_task1_ctx.update_count = 0;
}

/**
 * @brief Task 1 主循环
 * 
 * 功能：
 * - 等待陀螺仪校准完成
 * - 校准完成后启动底盘任务
 * - 监控底盘任务执行状态
 * - 任务完成后蜂鸣器响1s
 * 
 * @note 应在主循环中定期调用（建议 10ms 周期）
 */
void Task1_Run(void)
{
    /* 检查任务状态：如果不是 INIT 或 RUNNING，直接返回 */
    if (g_task1_ctx.state != TASK_STATE_INIT && 
        g_task1_ctx.state != TASK_STATE_RUNNING) {
        return;
    }
    
    /* 计算已运行时间 */
    g_task1_ctx.elapsed_time = uwTick - g_task1_ctx.start_time;
    
    /* 超时检查 */
    if (g_task1_ctx.elapsed_time > g_task1_ctx.timeout_ms) {
        g_task1_ctx.state = TASK_STATE_TIMEOUT;
        ChassisTaskStop();
        return;
    }
    
    /* ========== INIT 状态：等待陀螺仪校准完成 ========== */
    if (g_task1_ctx.state == TASK_STATE_INIT) {
        /* 检查陀螺仪校准是否完成 */
        if (jy61p_is_calibration_done()) {
            g_task1_ctx.gyro_calibration_done = 1;
            
            /* 校准完成后，重新记录任务开始时间（避免校准时间影响） */
            g_task1_ctx.start_time = uwTick;
            g_task1_ctx.elapsed_time = 0;
            
            /* 获取目标角度 */
            int16_t target_yaw = Task1_GetTargetYaw(g_task1_ctx.target_zone);
            
            /* 启动底盘任务（参数化版本）
             * 第一阶段：沿0°直行到交汇点（5s）
             * 转弯阶段：从0°转弯到目标角度（2s）
             * 第二阶段：沿目标角度直行（5s）
             */
            ChassisTaskStartWithParams(
                0,                          // stage1_yaw: 0°
                TASK1_STAGE1_DURATION,      // stage1_duration: 5000ms
                target_yaw,                 // turn1_yaw: 目标角度
                TASK1_TURN_DURATION,        // turn1_duration: 2000ms
                target_yaw,                 // stage2_yaw: 目标角度
                TASK1_STAGE2_DURATION,      // stage2_duration: 5000ms
                TASK1_BASE_PWM,             // base_pwm: 200
                TASK1_TURN_PWM              // turn_pwm: 0（原地转弯）
            );
            
            g_task1_ctx.chassis_task_running = 1;
            g_task1_ctx.state = TASK_STATE_RUNNING;
        }
    }
    
    /* ========== RUNNING 状态：监控底盘任务执行 ========== */
    if (g_task1_ctx.state == TASK_STATE_RUNNING && g_task1_ctx.chassis_task_running) {
        /* 更新底盘任务状态机 */
        ChassisTaskUpdate();
        
        /* 检查底盘任务是否完成 */
        uint8_t chassis_state = ChassisTaskGetState();
        if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
            /* 任务完成 */
            g_task1_ctx.state = TASK_STATE_SUCCESS;
            g_task1_ctx.chassis_task_running = 0;
            
            /* 启动蜂鸣器响1s */
            beep_1s_start();
        }
    }
    
    /* 更新计数器 */
    g_task1_ctx.update_count++;
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
    /* 停止底盘任务 */
    ChassisTaskStop();
    g_task1_ctx.chassis_task_running = 0;
    
    /* 设置任务状态为 IDLE */
    g_task1_ctx.state = TASK_STATE_IDLE;
}

/**
 * @brief 重置 Task 1
 * 
 * 功能：
 * - 重置任务状态
 * - 清除运行时间
 */
void Task1_Reset(void)
{
    g_task1_ctx.state = TASK_STATE_IDLE;
    g_task1_ctx.start_time = 0;
    g_task1_ctx.elapsed_time = 0;
    g_task1_ctx.gyro_calibration_done = 0;
    g_task1_ctx.chassis_task_running = 0;
    g_task1_ctx.update_count = 0;
}

/**
 * @brief 获取 Task 1 的当前状态
 * 
 * @return 任务状态（IDLE/INIT/RUNNING/SUCCESS/FAILED/TIMEOUT）
 */
TaskState_t Task1_GetState(void)
{
    return g_task1_ctx.state;
}

/**
 * @brief 判断 Task 1 是否成功完成
 * 
 * @return true 表示成功，false 表示未成功
 */
bool Task1_IsSuccess(void)
{
    return (g_task1_ctx.state == TASK_STATE_SUCCESS);
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
    if (zone >= 1 && zone <= 5) {
        g_task1_ctx.target_zone = zone;
    }
}

/**
 * @brief 获取目标区编号
 * 
 * @return 目标区编号（1-5）
 */
uint8_t Task1_GetTargetZone(void)
{
    return g_task1_ctx.target_zone;
}
