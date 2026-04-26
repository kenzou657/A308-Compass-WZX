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

/* ==================== 编码器参数 ==================== */
/* 编码器分辨率：每转脉冲数 */
#define ENCODER_PPR              (20)

/* 轮子参数 */
#define WHEEL_RADIUS             (25)        /* 轮半径(mm) */
#define WHEEL_DISTANCE           (120)       /* 轮距(mm)，两轮中心距离 */

/* 采样周期(ms)，对应CLOCK定时器频率100Hz */
#define ENCODER_SAMPLE_PERIOD    (10)

/* ==================== PWM参数 ==================== */
/* PWM周期和频率 */
#define PWM_PERIOD               (2000)      /* PWM周期计数值 */
#define PWM_FREQ                 (40000000)  /* PWM时钟频率(Hz) */
#define PWM_MAX_DUTY             (2000)      /* 最大占空比 */

/* ==================== PID控制参数 ==================== */
/* 位置式PID系数（输入输出都是速度mm/s） */
#define PID_KP                   (1.5f)      /* 比例系数 */
#define PID_KI                   (0.3f)      /* 积分系数 */
#define PID_KD                   (0.1f)      /* 微分系数 */

/* PID限幅参数（速度单位mm/s） */
#define PID_I_MAX                (500)       /* 积分项限幅(mm/s) */
#define PID_OUT_MAX              (500)       /* 输出限幅(mm/s)，最大速度 */

/* 速度到占空比的转换系数
 * duty = (speed × SPEED_TO_DUTY_SCALE) / 1000
 * 其中 MAX_SPEED对应PWM_MAX_DUTY
 * SPEED_TO_DUTY_SCALE = (PWM_MAX_DUTY × 1000) / MAX_SPEED
 */
#define SPEED_TO_DUTY_SCALE      ((PWM_MAX_DUTY * 1000) / MAX_SPEED)

/* ==================== 速度限制 ==================== */
#define MAX_SPEED                (500)       /* 最大速度(mm/s) */
#define MIN_SPEED                (50)        /* 最小速度(mm/s) */

/* ==================== 电机ID定义 ==================== */
#define MOTOR_A                  (0)         /* 电机A */
#define MOTOR_B                  (1)         /* 电机B */
#define MOTOR_COUNT              (2)         /* 电机总数 */

/* ==================== 方向定义 ==================== */
#define MOTOR_DIR_STOP           (0)         /* 停止 */
#define MOTOR_DIR_FORWARD        (1)         /* 前进 */
#define MOTOR_DIR_BACKWARD       (-1)        /* 后退 */

/* ==================== 计算常数 ==================== */
/* 脉冲转速度的系数：(2π × R) / (N × T)
 * 其中 R=轮半径(mm), N=编码器PPR, T=采样周期(ms)
 * 单位转换：mm/s = (脉冲数 × 2π × R) / (N × T × 1000)
 * 为避免浮点运算，使用整数比例：
 * speed_mm_s = (pulse_count × SPEED_SCALE) / 1000
 */
#define SPEED_SCALE              ((2 * 314 * WHEEL_RADIUS) / (ENCODER_PPR * ENCODER_SAMPLE_PERIOD))

#endif /* CONFIG_H */
