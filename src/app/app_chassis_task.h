/**
 * @file app_chassis_task.h
 * @brief 小车底盘任务 - 头文件
 * 
 * 功能：
 * - 状态机实现多阶段运动控制，每个阶段完成后停车 0.5s
 * - 第一阶段：目标角度 0°，直行 5000ms
 * - 停车暂停1：停车 0.5s
 * - 转弯阶段1：原地转弯到 -45°，2000ms
 * - 停车暂停2：停车 0.5s
 * - 第二阶段：目标角度 -45°，直行 5000ms
 * - 停车暂停3：停车 0.5s
 */

#ifndef _APP_CHASSIS_TASK_H_
#define _APP_CHASSIS_TASK_H_

#include <stdint.h>

/* ============ 任务状态定义 ============ */
#define CHASSIS_TASK_STATE_IDLE           0   // 空闲
#define CHASSIS_TASK_STATE_STAGE1         1   // 第一阶段：直行，目标角度 0°
#define CHASSIS_TASK_STATE_STOP_PAUSE1    2   // 停车暂停1：STAGE1 完成后停车 0.5s
#define CHASSIS_TASK_STATE_TURN1          3   // 转弯阶段1：原地转弯到 -45°
#define CHASSIS_TASK_STATE_STOP_PAUSE2    4   // 停车暂停2：TURN1 完成后停车 0.5s
#define CHASSIS_TASK_STATE_STAGE2         5   // 第二阶段：直行，目标角度 -45°
#define CHASSIS_TASK_STATE_STOP_PAUSE3    6   // 停车暂停3：STAGE2 完成后停车 0.5s
#define CHASSIS_TASK_STATE_COMPLETE       7   // 任务完成

/* ============ 任务参数定义 ============ */
#define CHASSIS_TASK_STAGE1_YAW       0      // 第一阶段目标角度：0°（°×100）
#define CHASSIS_TASK_STAGE1_DURATION  5000   // 第一阶段持续时间：5000ms

#define CHASSIS_TASK_TURN1_YAW        -4500  // 转弯1目标角度：-45°（°×100）
#define CHASSIS_TASK_TURN1_DURATION   2000   // 转弯1持续时间：2000ms

#define CHASSIS_TASK_STAGE2_YAW       -4500  // 第二阶段目标角度：-45°（°×100）
#define CHASSIS_TASK_STAGE2_DURATION  5000   // 第二阶段持续时间：5000ms

#define CHASSIS_TASK_BASE_PWM         200    // 基础 PWM 占空比
#define CHASSIS_TASK_TURN_PWM         0      // 转弯时 PWM 占空比（原地转弯）

/* ============ 任务状态结构体 ============ */
typedef struct {
    uint8_t state;                    // 当前任务状态
    uint32_t stage_start_tick;        // 当前阶段开始时刻
    uint32_t elapsed_ms;              // 当前阶段已运行时间
    
    // 动态参数
    int16_t stage1_yaw;               // 第一阶段目标角度（°×100）
    uint32_t stage1_duration;         // 第一阶段持续时间（ms）
    
    int16_t turn1_yaw;                // 转弯1目标角度（°×100）
    uint32_t turn1_duration;          // 转弯1持续时间（ms）
    
    int16_t stage2_yaw;               // 第二阶段目标角度（°×100）
    uint32_t stage2_duration;         // 第二阶段持续时间（ms）
    
    uint16_t base_pwm;                // 基础 PWM 占空比
    uint16_t turn_pwm;                // 转弯时 PWM 占空比
} ChassisTask_t;

/* ============ 全局变量声明 ============ */
extern ChassisTask_t g_chassis_task;

/* ============ 函数声明 ============ */

/**
 * @brief 初始化小车底盘任务
 */
void ChassisTaskInit(void);

/**
 * @brief 启动小车底盘任务（使用默认参数）
 */
void ChassisTaskStart(void);

/**
 * @brief 启动小车底盘任务（参数化版本）
 *
 * @param stage1_yaw      第一阶段目标角度（°×100）
 * @param stage1_duration 第一阶段持续时间（ms）
 * @param turn1_yaw       转弯1目标角度（°×100）
 * @param turn1_duration  转弯1持续时间（ms）
 * @param stage2_yaw      第二阶段目标角度（°×100）
 * @param stage2_duration 第二阶段持续时间（ms）
 * @param base_pwm        基础 PWM 占空比
 * @param turn_pwm        转弯时 PWM 占空比
 */
void ChassisTaskStartWithParams(
    int16_t stage1_yaw,
    uint32_t stage1_duration,
    int16_t turn1_yaw,
    uint32_t turn1_duration,
    int16_t stage2_yaw,
    uint32_t stage2_duration,
    uint16_t base_pwm,
    uint16_t turn_pwm
);

/**
 * @brief 小车底盘任务主循环（状态机）
 * 
 * @note 应在主循环中定期调用（建议 10ms 周期）
 */
void ChassisTaskUpdate(void);

/**
 * @brief 停止小车底盘任务
 */
void ChassisTaskStop(void);

/**
 * @brief 获取任务当前状态
 * 
 * @return 任务状态（IDLE/STAGE1/TURN1/STAGE2/COMPLETE）
 */
uint8_t ChassisTaskGetState(void);

#endif /* _APP_CHASSIS_TASK_H_ */
