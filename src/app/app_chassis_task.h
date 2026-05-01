/**
 * @file app_chassis_task.h
 * @brief 小车底盘任务 - 头文件
 * 
 * 功能：
 * - 状态机实现多阶段运动控制
 * - 第一阶段：目标角度 0°，直行 5000ms
 * - 第二阶段：目标角度 -45°，直行 5000ms
 */

#ifndef _APP_CHASSIS_TASK_H_
#define _APP_CHASSIS_TASK_H_

#include <stdint.h>

/* ============ 任务状态定义 ============ */
#define CHASSIS_TASK_STATE_IDLE       0   // 空闲
#define CHASSIS_TASK_STATE_STAGE1     1   // 第一阶段：直行，目标角度 0°
#define CHASSIS_TASK_STATE_STAGE2     2   // 第二阶段：直行，目标角度 -45°
#define CHASSIS_TASK_STATE_COMPLETE   3   // 任务完成

/* ============ 任务参数定义 ============ */
#define CHASSIS_TASK_STAGE1_YAW       0      // 第一阶段目标角度：0°（°×100）
#define CHASSIS_TASK_STAGE1_DURATION  5000   // 第一阶段持续时间：5000ms
#define CHASSIS_TASK_STAGE2_YAW       -4500  // 第二阶段目标角度：-45°（°×100）
#define CHASSIS_TASK_STAGE2_DURATION  5000   // 第二阶段持续时间：5000ms
#define CHASSIS_TASK_BASE_PWM         500    // 基础 PWM 占空比

/* ============ 任务状态结构体 ============ */
typedef struct {
    uint8_t state;                    // 当前任务状态
    uint32_t stage_start_tick;        // 当前阶段开始时刻
    uint32_t elapsed_ms;              // 当前阶段已运行时间
} ChassisTask_t;

/* ============ 全局变量声明 ============ */
extern ChassisTask_t g_chassis_task;

/* ============ 函数声明 ============ */

/**
 * @brief 初始化小车底盘任务
 */
void ChassisTaskInit(void);

/**
 * @brief 启动小车底盘任务
 */
void ChassisTaskStart(void);

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
 * @return 任务状态（IDLE/STAGE1/STAGE2/COMPLETE）
 */
uint8_t ChassisTaskGetState(void);

#endif /* _APP_CHASSIS_TASK_H_ */
