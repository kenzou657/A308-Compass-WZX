/*
 * 项目全局配置宏定义
 * 
 * 本文件包含所有项目级别的配置宏，便于快速调整参数。
 * 
 * 修改此文件后需要重新编译项目。
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ============================================================================
 * PID 控制器配置（电机速度闭环）
 * ============================================================================ */

/** 增量式 PID 比例系数 Kp（整数表示，实际值 = Kp/100） */
#define MOTOR_PID_KP            100

/** 增量式 PID 积分系数 Ki（整数表示，实际值 = Ki/100） */
#define MOTOR_PID_KI            10

/** 增量式 PID 微分系数 Kd（整数表示，实际值 = Kd/100） */
#define MOTOR_PID_KD            0

/** PID 输出限幅（PWM占空比，0-700） */
#define MOTOR_PID_OUTPUT_MAX    350
#define MOTOR_PID_OUTPUT_MIN    -350

/* ============================================================================
 * 速度映射配置（mm/s 转 PWM占空比）
 * ============================================================================ */

/** 速度转 PWM 占空比的比例系数 */
/** 已知：25659 mm/s 对应占空比 700 */
/** 映射公式：pwm = (speed_mmps * 700) / 25659 */
#define SPEED_TO_PWM_RATIO_NUM  700
#define SPEED_TO_PWM_RATIO_DEN  8530

/** 最大速度限制（mm/s），对应 PWM 占空比 700 */
#define MAX_SPEED_MMPS          25600

/* ============================================================================
 * 编码器配置
 * ============================================================================ */

/** 脉冲转速度的比例系数 (整数运算) */
/** 14个脉冲对应速度196.35 mm/s，比例系数 = 196.35 / 14 = 14.03 */
/** 使用整数表示：14030/1000，即 speed(mm/s) = (pulse_count * 14030) / 1000 */
#define ENCODER_SPEED_RATIO_NUM     1403

/** 一阶低通滤波系数 (整数表示，范围0-1000，实际值=SPEED_FILTER_ALPHA/1000) */
/** 推荐值：300-500，值越大响应越快，滤波越弱 */
#define SPEED_FILTER_ALPHA          400

/* ============================================================================
 * 陀螺仪 Yaw 角 PID 控制器配置（小车方向闭环）
 * ============================================================================ */

/** 位置式 PID 比例系数 Kp（整数表示，实际值 = Kp/1000） - 直行模式 */
#define GYRO_YAW_PID_KP             1000

/** 位置式 PID 比例系数 Kp（整数表示，实际值 = Kp/1000） - 转弯模式 */
#define GYRO_YAW_PID_KP_TURN        400

/** 位置式 PID 积分系数 Ki（整数表示，实际值 = Ki/1000） */
#define GYRO_YAW_PID_KI             2

/** 位置式 PID 微分系数 Kd（整数表示，实际值 = Kd/1000） */
#define GYRO_YAW_PID_KD             5

/** 陀螺仪 PID 输出限幅（PWM 差速值，±300） */
#define GYRO_PID_OUTPUT_MAX         1500
#define GYRO_PID_OUTPUT_MIN         -1500

/** 陀螺仪 PID 积分限幅 */
#define GYRO_PID_INTEGRAL_MAX       5000
#define GYRO_PID_INTEGRAL_MIN       -5000

/** 陀螺仪死区设置（°×100，即 50 = 0.5°） */
/** 当误差小于死区时，PID 输出为 0，防止抖动 */
#define GYRO_DEADZONE               50

/** 陀螺仪 PID 输出到 PWM 差速值的映射因子 */
/**
 * PID 输入/输出单位：°×100（例如 4500 表示 45°）
 * 映射关系：PID_output(°×100) × GYRO_PID_TO_PWM_NUM / GYRO_PID_TO_PWM_DEN = PWM_差速值
 *
 * 例如：PID_output = 300(°×100) 对应 PWM 差速值 50
 * 则：300 × GYRO_PID_TO_PWM_NUM / GYRO_PID_TO_PWM_DEN = 50
 * 计算：GYRO_PID_TO_PWM_NUM / GYRO_PID_TO_PWM_DEN = 50 / 300 = 1 / 6
 * 取值：GYRO_PID_TO_PWM_NUM = 1, GYRO_PID_TO_PWM_DEN = 6
 */
#define GYRO_PID_TO_PWM_NUM         1
#define GYRO_PID_TO_PWM_DEN         6

/* ============================================================================
 * 小车运动控制配置
 * ============================================================================ */

/** 系统滴答周期（ms），用于运行时间计算 */
/** 假设 uwTick 每 1ms 递增 1 */
#define CHASSIS_TICK_PERIOD_MS      1

/** 小车默认基础速度（PWM 占空比） */
#define CHASSIS_DEFAULT_BASE_PWM    500

/** 小车最大基础速度（PWM 占空比） */
#define CHASSIS_MAX_BASE_PWM        350

/* ============================================================================
 * 任务一 - 目的地到角度映射
 * ============================================================================ */

/** 目的地编号到目标角度的映射（°×100） */
#define DESTINATION_1_YAW           9000    // 目的地1：90°
#define DESTINATION_2_YAW           4500    // 目的地2：45°
#define DESTINATION_3_YAW           0       // 目的地3：0°
#define DESTINATION_4_YAW           -4500   // 目的地4：-45°
#define DESTINATION_5_YAW           -9000   // 目的地5：-90°

/** 任务一 - 运动参数 */
#define TASK1_STAGE1_DURATION       5000    // 第一阶段（直行到交汇点）：5000ms
#define TASK1_TURN_DURATION         2000    // 转弯阶段：2000ms
#define TASK1_STAGE2_DURATION       5000    // 第二阶段（沿目标角度直行）：5000ms
#define TASK1_BASE_PWM              200     // 基础 PWM 占空比
#define TASK1_TURN_PWM              0       // 转弯时 PWM 占空比（原地转弯）

/** 陀螺仪零点校准参数 */
#define GYRO_CALIBRATION_SAMPLES    20      // 校准采样次数

#endif /* CONFIG_H */
