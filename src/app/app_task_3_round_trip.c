/**
 * @file app_task_3_round_trip.c
 * @brief Task 3: 双点往返 - 实现文件
 * 
 * 功能描述：
 * - 循迹到目标区1，停止
 * - 停顿 2 秒
 * - 转弯到返回方向（目标1角度+180°）
 * - 沿返回方向直行5s
 * - 转弯到-180°方向
 * - 沿-180°直行5s到达起始点
 * - 转弯到0°
 * - 沿0°直行到交汇点
 * - 转弯到目的地2方向
 * - 沿目的地2方向直行到目标2，停止
 * - 蜂鸣器鸣响
 * 
 * 状态机流程：
 * IDLE → INIT → RUNNING(前往目标1) → RUNNING(停顿2s) → RUNNING(转弯到返回方向)
 * → RUNNING(沿返回方向直行) → RUNNING(转弯到-180°) → RUNNING(沿-180°直行)
 * → RUNNING(转弯到0°) → RUNNING(沿0°直行到交汇点) → RUNNING(转弯到目标2) → RUNNING(前往目标2) → SUCCESS/FAILED/TIMEOUT
 * 
 * 关键实现：
 * - 处理-180度方向的角度跳变（-179和179相近）
 * - 使用角度归一化避免偏差计算的跳变
 * - 基于 uwTick 进行时间控制
 * - 每次进入任务都进行陀螺仪零点校准
 */

#include "app_task_3_round_trip.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../drivers/drv_jy61p.h"
#include "../utils/timer.h"
#include "../config.h"
#include "app_chassis_task.h"
#include <stdlib.h>

/* ==================== 任务上下文结构 ==================== */

/**
 * @brief Task 3 上下文结构
 */
typedef struct {
    TaskState_t state;              /* 任务状态 */
    uint8_t target_zone_1;          /* 目标区1编号（1-5） */
    uint8_t target_zone_2;          /* 目标区2编号（1-5） */
    uint32_t start_time;            /* 任务开始时间 */
    uint32_t elapsed_time;          /* 已运行时间 */
    uint32_t timeout_ms;            /* 超时时间 */
    
    /* 陀螺仪校准相关 */
    uint8_t gyro_calibration_done;  /* 陀螺仪校准完成标志 */
    uint32_t calibration_start_time;/* 校准开始时间 */
    
    /* 阶段控制 */
    uint8_t phase;                  /* 运行阶段：
                                       0: 前往目标1
                                       1: 停顿2s
                                       2: 转弯到返回方向
                                       3: 沿返回方向直行5s
                                       4: 转弯到-180°
                                       5: 沿-180°直行5s
                                       6: 转弯到0°
                                       7: 沿0°直行到交汇点
                                       8: 转弯到目标2
                                       9: 前往目标2 */
    uint32_t phase_start_time;      /* 当前阶段开始时间 */
    
    /* 底盘任务相关 */
    uint8_t chassis_task_running;   /* 底盘任务运行标志 */
    
    /* 调试信息 */
    uint32_t update_count;          /* 更新次数计数 */
} Task3_Context_t;

/* ==================== 全局变量 ==================== */

/**
 * @brief Task 3 上下文
 */
static Task3_Context_t g_task3_ctx;

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
static int16_t Task3_GetTargetYaw(uint8_t destination)
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

/**
 * @brief 计算旋转180度后的目标角度
 * 
 * @param original_yaw 原始角度（°×100）
 * @return 旋转180度后的角度（°×100）
 */
static int16_t Task3_RotateYaw180(int16_t original_yaw)
{
    int32_t rotated = (int32_t)original_yaw + 18000;  // 加180°
    
    /* 角度归一化到 [-18000, 18000) 范围 */
    while (rotated >= 18000) {
        rotated -= 36000;
    }
    while (rotated < -18000) {
        rotated += 36000;
    }
    
    return (int16_t)rotated;
}

/* ==================== 函数实现 ==================== */

/**
 * @brief 初始化 Task 3
 * 
 * 功能：
 * - 初始化任务状态为 INIT
 * - 启动陀螺仪零点校准
 * - 记录任务开始时间
 * - 设置超时时间
 * 
 * @note 必须在 TaskManager_StartTask() 中调用
 */
void Task3_Init(void)
{
    /* 初始化任务状态为 INIT */
    g_task3_ctx.state = TASK_STATE_INIT;
    
    /* 初始化时间相关变量 */
    g_task3_ctx.start_time = 0;  /* 校准完成后再记录 */
    g_task3_ctx.elapsed_time = 0;
    
    /* 设置超时时间：120s（足够完成往返循迹） */
    g_task3_ctx.timeout_ms = 120000;
    
    /* 初始化陀螺仪校准标志 */
    g_task3_ctx.gyro_calibration_done = 0;
    g_task3_ctx.calibration_start_time = uwTick;
    
    /* 启动陀螺仪零点校准（采集20个样本） */
    jy61p_start_calibration(GYRO_CALIBRATION_SAMPLES);
    
    /* 初始化底盘任务 */
    ChassisTaskInit();
    g_task3_ctx.chassis_task_running = 0;
    
    /* 初始化阶段为 0（前往目标1） */
    g_task3_ctx.phase = 0;
    g_task3_ctx.phase_start_time = 0;
    
    /* 初始化调试计数 */
    g_task3_ctx.update_count = 0;
}

/**
 * @brief Task 3 主循环
 * 
 * 功能：
 * - 等待陀螺仪校准完成
 * - 校准完成后启动底盘任务
 * - 监控底盘任务执行状态
 * - 根据阶段执行不同的逻辑
 * - 任务完成后蜂鸣器响1s
 * 
 * @note 应在主循环中定期调用（建议 10ms 周期）
 */
void Task3_Run(void)
{
    /* 检查任务状态：如果不是 INIT 或 RUNNING，直接返回 */
    if (g_task3_ctx.state != TASK_STATE_INIT && 
        g_task3_ctx.state != TASK_STATE_RUNNING) {
        return;
    }
    
    /* 计算已运行时间 */
    g_task3_ctx.elapsed_time = uwTick - g_task3_ctx.start_time;
    
    /* 超时检查 */
    if (g_task3_ctx.elapsed_time > g_task3_ctx.timeout_ms) {
        g_task3_ctx.state = TASK_STATE_TIMEOUT;
        ChassisTaskStop();
        return;
    }
    
    /* ========== INIT 状态：等待陀螺仪校准完成 ========== */
    if (g_task3_ctx.state == TASK_STATE_INIT) {
        /* 检查陀螺仪校准是否完成 */
        if (jy61p_is_calibration_done()) {
            g_task3_ctx.gyro_calibration_done = 1;
            
            /* 校准完成后，重新记录任务开始时间（避免校准时间影响） */
            g_task3_ctx.start_time = uwTick;
            g_task3_ctx.elapsed_time = 0;
            g_task3_ctx.phase_start_time = uwTick;
            
            /* 获取目标1的角度 */
            int16_t target_yaw_1 = Task3_GetTargetYaw(g_task3_ctx.target_zone_1);
            
            /* 启动底盘任务（参数化版本）
             * 第一阶段：沿0°直行到交汇点（5s）
             * 转弯阶段：从0°转弯到目标1角度（2s）
             * 第二阶段：沿目标1角度直行（5s）
             */
            ChassisTaskStartWithParams(
                0,                          // stage1_yaw: 0°
                TASK1_STAGE1_DURATION,      // stage1_duration: 5000ms
                target_yaw_1,               // turn1_yaw: 目标1角度
                TASK1_TURN_DURATION,        // turn1_duration: 2000ms
                target_yaw_1,               // stage2_yaw: 目标1角度
                TASK1_STAGE2_DURATION,      // stage2_duration: 5000ms
                TASK1_BASE_PWM,             // base_pwm: 200
                TASK1_TURN_PWM              // turn_pwm: 0（原地转弯）
            );
            
            g_task3_ctx.chassis_task_running = 1;
            g_task3_ctx.state = TASK_STATE_RUNNING;
            g_task3_ctx.phase = 0;  /* 前往目标1 */
        }
    }
    
    /* ========== RUNNING 状态：监控底盘任务执行 ========== */
    if (g_task3_ctx.state == TASK_STATE_RUNNING) {
        uint8_t chassis_state = ChassisTaskGetState();
        uint32_t phase_elapsed = uwTick - g_task3_ctx.phase_start_time;
        
        /* ===== 阶段0：前往目标1 ===== */
        if (g_task3_ctx.phase == 0) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 到达目标1，进入停顿阶段 */
                g_task3_ctx.phase = 1;
                g_task3_ctx.phase_start_time = uwTick;
                ChassisTaskStop();
                g_task3_ctx.chassis_task_running = 0;
                
                /* 蜂鸣器提示到达目标1 */
                beep_1s_start();
            }
        }
        
        /* ===== 阶段1：停顿2s ===== */
        else if (g_task3_ctx.phase == 1) {
            if (phase_elapsed >= 2000) {
                /* 停顿完成，进入转弯到返回方向阶段 */
                g_task3_ctx.phase = 2;
                g_task3_ctx.phase_start_time = uwTick;
                
                /* 获取目标1的角度 */
                int16_t target_yaw_1 = Task3_GetTargetYaw(g_task3_ctx.target_zone_1);
                
                /* 计算返回方向（目标1角度+180°） */
                int16_t return_yaw = Task3_RotateYaw180(target_yaw_1);
                
                /* 启动转弯到返回方向（2s） */
                ChassisTaskStartWithParams(
                    target_yaw_1,               // stage1_yaw: 目标1角度
                    0,                          // stage1_duration: 0（不直行）
                    return_yaw,                 // turn1_yaw: 返回方向
                    TASK1_TURN_DURATION,        // turn1_duration: 2000ms
                    return_yaw,                 // stage2_yaw: 返回方向
                    0,                          // stage2_duration: 0（不直行）
                    TASK1_BASE_PWM,             // base_pwm: 200
                    TASK1_TURN_PWM              // turn_pwm: 0（原地转弯）
                );
                
                g_task3_ctx.chassis_task_running = 1;
            }
        }
        
        /* ===== 阶段2：转弯到返回方向 ===== */
        else if (g_task3_ctx.phase == 2) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 转弯完成，进入沿返回方向直行阶段 */
                g_task3_ctx.phase = 3;
                g_task3_ctx.phase_start_time = uwTick;
                
                /* 获取目标1的角度 */
                int16_t target_yaw_1 = Task3_GetTargetYaw(g_task3_ctx.target_zone_1);
                
                /* 计算返回方向（目标1角度+180°） */
                int16_t return_yaw = Task3_RotateYaw180(target_yaw_1);
                
                /* 启动沿返回方向直行5s */
                ChassisTaskStartWithParams(
                    return_yaw,                 // stage1_yaw: 返回方向
                    TASK1_STAGE1_DURATION,      // stage1_duration: 5000ms
                    return_yaw,                 // turn1_yaw: 返回方向
                    0,                          // turn1_duration: 0（不转弯）
                    return_yaw,                 // stage2_yaw: 返回方向
                    0,                          // stage2_duration: 0（不直行）
                    TASK1_BASE_PWM,             // base_pwm: 200
                    TASK1_TURN_PWM              // turn_pwm: 0
                );
                
                g_task3_ctx.chassis_task_running = 1;
            }
        }
        
        /* ===== 阶段3：沿返回方向直行5s ===== */
        else if (g_task3_ctx.phase == 3) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 直行完成，进入转弯到-180°阶段 */
                g_task3_ctx.phase = 4;
                g_task3_ctx.phase_start_time = uwTick;
                
                /* 获取目标1的角度 */
                int16_t target_yaw_1 = Task3_GetTargetYaw(g_task3_ctx.target_zone_1);
                
                /* 计算返回方向（目标1角度+180°） */
                int16_t return_yaw = Task3_RotateYaw180(target_yaw_1);
                
                /* 启动转弯到-180°（2s） */
                ChassisTaskStartWithParams(
                    return_yaw,                 // stage1_yaw: 返回方向
                    0,                          // stage1_duration: 0（不直行）
                    -18000,                     // turn1_yaw: -180°
                    TASK1_TURN_DURATION,        // turn1_duration: 2000ms
                    -18000,                     // stage2_yaw: -180°
                    0,                          // stage2_duration: 0（不直行）
                    TASK1_BASE_PWM,             // base_pwm: 200
                    TASK1_TURN_PWM              // turn_pwm: 0（原地转弯）
                );
                
                g_task3_ctx.chassis_task_running = 1;
            }
        }
        
        /* ===== 阶段4：转弯到-180° ===== */
        else if (g_task3_ctx.phase == 4) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 转弯完成，进入沿-180°直行阶段 */
                g_task3_ctx.phase = 5;
                g_task3_ctx.phase_start_time = uwTick;
                
                /* 启动沿-180°直行5s到达起始点 */
                ChassisTaskStartWithParams(
                    -18000,                     // stage1_yaw: -180°
                    TASK1_STAGE1_DURATION,      // stage1_duration: 5000ms
                    -18000,                     // turn1_yaw: -180°
                    0,                          // turn1_duration: 0（不转弯）
                    -18000,                     // stage2_yaw: -180°
                    0,                          // stage2_duration: 0（不直行）
                    TASK1_BASE_PWM,             // base_pwm: 200
                    TASK1_TURN_PWM              // turn_pwm: 0
                );
                
                g_task3_ctx.chassis_task_running = 1;
            }
        }
        
        /* ===== 阶段5：沿-180°直行5s ===== */
        else if (g_task3_ctx.phase == 5) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 直行完成，进入转弯到0°阶段 */
                g_task3_ctx.phase = 6;
                g_task3_ctx.phase_start_time = uwTick;
                
                /* 启动转弯到0°（2s） */
                ChassisTaskStartWithParams(
                    -18000,                     // stage1_yaw: -180°
                    0,                          // stage1_duration: 0（不直行）
                    0,                          // turn1_yaw: 0°
                    TASK1_TURN_DURATION,        // turn1_duration: 2000ms
                    0,                          // stage2_yaw: 0°
                    0,                          // stage2_duration: 0（不直行）
                    TASK1_BASE_PWM,             // base_pwm: 200
                    TASK1_TURN_PWM              // turn_pwm: 0（原地转弯）
                );
                
                g_task3_ctx.chassis_task_running = 1;
            }
        }
        
        /* ===== 阶段6：转弯到0° ===== */
        else if (g_task3_ctx.phase == 6) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 转弯完成，进入沿0°直行到交汇点阶段 */
                g_task3_ctx.phase = 7;
                g_task3_ctx.phase_start_time = uwTick;
                
                /* 启动沿0°直行到交汇点（5s） */
                ChassisTaskStartWithParams(
                    0,                          // stage1_yaw: 0°
                    TASK1_STAGE1_DURATION,      // stage1_duration: 5000ms
                    0,                          // turn1_yaw: 0°
                    0,                          // turn1_duration: 0（不转弯）
                    0,                          // stage2_yaw: 0°
                    0,                          // stage2_duration: 0（不直行）
                    TASK1_BASE_PWM,             // base_pwm: 200
                    TASK1_TURN_PWM              // turn_pwm: 0
                );
                
                g_task3_ctx.chassis_task_running = 1;
            }
        }
        
        /* ===== 阶段7：沿0°直行到交汇点 ===== */
        else if (g_task3_ctx.phase == 7) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 直行完成，进入转弯到目标2阶段 */
                g_task3_ctx.phase = 8;
                g_task3_ctx.phase_start_time = uwTick;
                
                /* 获取目标2的角度 */
                int16_t target_yaw_2 = Task3_GetTargetYaw(g_task3_ctx.target_zone_2);
                
                /* 启动转弯到目标2方向（2s） */
                ChassisTaskStartWithParams(
                    0,                          // stage1_yaw: 0°
                    0,                          // stage1_duration: 0（不直行）
                    target_yaw_2,               // turn1_yaw: 目标2角度
                    TASK1_TURN_DURATION,        // turn1_duration: 2000ms
                    target_yaw_2,               // stage2_yaw: 目标2角度
                    0,                          // stage2_duration: 0（不直行）
                    TASK1_BASE_PWM,             // base_pwm: 200
                    TASK1_TURN_PWM              // turn_pwm: 0（原地转弯）
                );
                
                g_task3_ctx.chassis_task_running = 1;
            }
        }
        
        /* ===== 阶段8：转弯到目标2 ===== */
        else if (g_task3_ctx.phase == 8) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 转弯完成，进入前往目标2阶段 */
                g_task3_ctx.phase = 9;
                g_task3_ctx.phase_start_time = uwTick;
                
                /* 获取目标2的角度 */
                int16_t target_yaw_2 = Task3_GetTargetYaw(g_task3_ctx.target_zone_2);
                
                /* 启动直行到目标2（沿目标2角度直行5s） */
                ChassisTaskStartWithParams(
                    target_yaw_2,               // stage1_yaw: 目标2角度
                    TASK1_STAGE2_DURATION,      // stage1_duration: 5000ms
                    target_yaw_2,               // turn1_yaw: 目标2角度
                    0,                          // turn1_duration: 0（不转弯）
                    target_yaw_2,               // stage2_yaw: 目标2角度
                    0,                          // stage2_duration: 0（不直行）
                    TASK1_BASE_PWM,             // base_pwm: 200
                    TASK1_TURN_PWM              // turn_pwm: 0
                );
                
                g_task3_ctx.chassis_task_running = 1;
            }
        }
        
        /* ===== 阶段9：前往目标2 ===== */
        else if (g_task3_ctx.phase == 9) {
            ChassisTaskUpdate();
            
            if (chassis_state == CHASSIS_TASK_STATE_COMPLETE) {
                /* 任务完成 */
                g_task3_ctx.state = TASK_STATE_SUCCESS;
                g_task3_ctx.chassis_task_running = 0;
                
                /* 启动蜂鸣器响1s */
                beep_1s_start();
            }
        }
    }
    
    /* 更新计数器 */
    g_task3_ctx.update_count++;
}

/**
 * @brief 停止 Task 3
 * 
 * 功能：
 * - 停止底盘运动
 * - 设置任务状态为 IDLE
 */
void Task3_Stop(void)
{
    /* 停止底盘任务 */
    ChassisTaskStop();
    g_task3_ctx.chassis_task_running = 0;
    
    /* 设置任务状态为 IDLE */
    g_task3_ctx.state = TASK_STATE_IDLE;
}

/**
 * @brief 重置 Task 3
 * 
 * 功能：
 * - 重置任务状态
 * - 清除运行时间
 */
void Task3_Reset(void)
{
    g_task3_ctx.state = TASK_STATE_IDLE;
    g_task3_ctx.start_time = 0;
    g_task3_ctx.elapsed_time = 0;
    g_task3_ctx.gyro_calibration_done = 0;
    g_task3_ctx.chassis_task_running = 0;
    g_task3_ctx.phase = 0;
    g_task3_ctx.phase_start_time = 0;
    g_task3_ctx.update_count = 0;
}

/**
 * @brief 获取 Task 3 的当前状态
 * 
 * @return 任务状态（IDLE/INIT/RUNNING/SUCCESS/FAILED/TIMEOUT）
 */
TaskState_t Task3_GetState(void)
{
    return g_task3_ctx.state;
}

/**
 * @brief 判断 Task 3 是否成功完成
 * 
 * @return true 表示成功，false 表示未成功
 */
bool Task3_IsSuccess(void)
{
    return (g_task3_ctx.state == TASK_STATE_SUCCESS);
}

/**
 * @brief 设置两个目标区编号
 * 
 * @param zone1 第一个目标区（1-5）
 * @param zone2 第二个目标区（1-5）
 * 
 * @note 必须在 Task3_Init() 之前调用
 */
void Task3_SetTargetZones(uint8_t zone1, uint8_t zone2)
{
    if (zone1 >= 1 && zone1 <= 5) {
        g_task3_ctx.target_zone_1 = zone1;
    }
    if (zone2 >= 1 && zone2 <= 5) {
        g_task3_ctx.target_zone_2 = zone2;
    }
}
