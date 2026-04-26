/*
 * 电机驱动模块头文件
 * 
 * 功能：
 * - 编码器脉冲采集和速度计算
 * - PWM占空比控制
 * - PID闭环速度控制
 * - 电机运动控制（前进、后退、转弯）
 */

#ifndef DRV_MOTOR_H
#define DRV_MOTOR_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== 数据结构定义 ==================== */

/* 编码器数据结构 */
typedef struct {
    int32_t pulse_count;        /* 脉冲计数 */
    int32_t last_pulse_count;   /* 上次脉冲计数 */
    int16_t speed;              /* 实时速度(mm/s) */
} Encoder_t;

/* PID控制器数据结构 */
typedef struct {
    float kp;                   /* 比例系数 */
    float ki;                   /* 积分系数 */
    float kd;                   /* 微分系数 */
    float error;                /* 当前误差 */
    float last_error;           /* 上次误差 */
    float integral;             /* 积分项累积 */
    int16_t target_speed;       /* 目标速度(mm/s) */
    int16_t output;             /* 控制输出(PWM占空比) */
    bool enabled;               /* PID是否启用 */
} PID_t;

/* 电机控制结构 */
typedef struct {
    Encoder_t encoder;          /* 编码器数据 */
    PID_t pid;                  /* PID控制器 */
    uint16_t pwm_duty;          /* 当前PWM占空比(0-2000) */
    int8_t direction;           /* 当前方向(-1/0/1) */
} Motor_t;

/* ==================== 初始化函数 ==================== */

/*
 * 初始化电机驱动模块
 * 
 * 功能：
 * - 初始化编码器和CLOCK定时器
 * - 初始化PID参数
 * - 初始化PWM和方向控制GPIO
 */
void Motor_Init(void);

/*
 * 初始化编码器和计时器
 * 
 * 功能：
 * - 启动ENCODER1A/2A捕获定时器
 * - 启动CLOCK定时器（100Hz）
 * - 使能相关中断
 */
void Motor_EncoderInit(void);

/*
 * 初始化PID参数
 */
void Motor_PIDInit(void);

/* ==================== 编码器函数 ==================== */

/*
 * 获取电机脉冲计数
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 * 
 * 返回：
 *   脉冲计数值
 */
int32_t Motor_GetEncoderCount(uint8_t motor_id);

/*
 * 重置电机脉冲计数
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 */
void Motor_ResetEncoder(uint8_t motor_id);

/*
 * 计算电机实时速度
 * 
 * 功能：
 * - 根据脉冲计数计算速度
 * - 在CLOCK中断中调用（100Hz）
 * 
 * 公式：
 *   speed = (pulse_count × SPEED_SCALE) / 1000
 */
void Motor_CalcSpeed(void);

/*
 * 获取电机实时速度
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 * 
 * 返回：
 *   实时速度(mm/s)
 */
int16_t Motor_GetSpeed(uint8_t motor_id);

/* ==================== PWM控制函数 ==================== */

/*
 * 设置电机PWM占空比
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 *   duty - PWM占空比 (0-2000)
 * 
 * 说明：
 * - duty=0 时PWM输出为0%
 * - duty=2000 时PWM输出为100%
 * - 使用DriverLib函数DL_TimerG_setCaptureCompareValue()更新
 */
void Motor_SetDuty(uint8_t motor_id, uint16_t duty);

/*
 * 获取电机当前PWM占空比
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 * 
 * 返回：
 *   PWM占空比 (0-2000)
 */
uint16_t Motor_GetDuty(uint8_t motor_id);

/* ==================== 方向控制函数 ==================== */

/*
 * 设置电机方向
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 *   dir - 方向 (MOTOR_DIR_FORWARD/MOTOR_DIR_BACKWARD/MOTOR_DIR_STOP)
 * 
 * 说明：
 * - MOTOR_DIR_FORWARD (1)：前进
 * - MOTOR_DIR_BACKWARD (-1)：后退
 * - MOTOR_DIR_STOP (0)：停止
 * 
 * TB6612芯片方向控制：
 * 电机A (AIN1, AIN2):
 *   - 前进：AIN1=1, AIN2=0
 *   - 后退：AIN1=0, AIN2=1
 *   - 停止：AIN1=0, AIN2=0
 * 
 * 电机B (BIN1, BIN2):
 *   - 前进：BIN1=1, BIN2=0
 *   - 后退：BIN1=0, BIN2=1
 *   - 停止：BIN1=0, BIN2=0
 */
void Motor_SetDirection(uint8_t motor_id, int8_t dir);

/*
 * 获取电机当前方向
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 * 
 * 返回：
 *   方向 (MOTOR_DIR_FORWARD/MOTOR_DIR_BACKWARD/MOTOR_DIR_STOP)
 */
int8_t Motor_GetDirection(uint8_t motor_id);

/* ==================== PID控制函数 ==================== */

/*
 * 设置电机目标速度（启用PID控制）
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 *   speed - 目标速度(mm/s)，正值前进，负值后退
 * 
 * 功能：
 * - 设置目标速度
 * - 启用PID控制
 * - 自动设置电机方向
 * 
 * 说明：
 * - 实际速度通过编码器反馈计算
 * - PID在CLOCK中断中更新（100Hz）
 */
void Motor_SetTargetSpeed(uint8_t motor_id, int16_t speed);

/*
 * 获取电机目标速度
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 * 
 * 返回：
 *   目标速度(mm/s)
 */
int16_t Motor_GetTargetSpeed(uint8_t motor_id);

/*
 * 更新PID控制
 *
 * 功能：
 * - 计算速度误差(mm/s)
 * - 执行PID算法（位置式）
 * - 输出目标速度(mm/s)
 * - 转换为PWM占空比
 * - 在CLOCK中断中调用（100Hz）
 *
 * PID算法（输入输出都是速度mm/s）：
 *   error = target_speed - actual_speed  (mm/s)
 *   P = Kp × error
 *   I = Ki × ∫error (带限幅 ±500mm/s)
 *   D = Kd × d(error)/dt
 *   output_speed = P + I + D  (mm/s，限幅 ±500mm/s)
 *
 * 速度到占空比转换：
 *   duty = (output_speed × SPEED_TO_DUTY_SCALE) / 1000
 */
void Motor_PIDUpdate(void);

/*
 * 禁用PID控制
 * 
 * 参数：
 *   motor_id - 电机ID (MOTOR_A 或 MOTOR_B)
 */
void Motor_DisablePID(uint8_t motor_id);

/* ==================== 运动控制函数 ==================== */

/*
 * 前进
 * 
 * 参数：
 *   speed - 速度(mm/s)
 * 
 * 功能：
 * - 两个电机同速前进
 * - 启用PID闭环控制
 */
void Motor_MoveForward(int16_t speed);

/*
 * 后退
 * 
 * 参数：
 *   speed - 速度(mm/s)
 * 
 * 功能：
 * - 两个电机同速后退
 * - 启用PID闭环控制
 */
void Motor_MoveBackward(int16_t speed);

/*
 * 转弯（差速控制）
 * 
 * 参数：
 *   left_speed - 左轮速度(mm/s)
 *   right_speed - 右轮速度(mm/s)
 * 
 * 功能：
 * - 设置左右轮不同速度实现转弯
 * - 启用PID闭环控制
 * 
 * 例子：
 * - Motor_Turn(200, 100)：左轮200mm/s，右轮100mm/s，向右转
 * - Motor_Turn(100, 200)：左轮100mm/s，右轮200mm/s，向左转
 * - Motor_Turn(200, -100)：左轮前进，右轮后退，原地转
 */
void Motor_Turn(int16_t left_speed, int16_t right_speed);

/*
 * 停止
 * 
 * 功能：
 * - 两个电机停止
 * - 禁用PID控制
 */
void Motor_Stop(void);

/* ==================== 中断处理函数 ==================== */

/*
 * 编码器A相中断处理（ENCODER1A_IRQHandler）
 * 
 * 功能：
 * - 捕获E1A上升沿
 * - 读取E1B电平判断方向
 * - 更新脉冲计数
 * 
 * 说明：
 * - 此函数由SysConfig自动调用
 * - 在isr/isr_motor.c中实现
 */
void Motor_Encoder1A_ISR(void);

/*
 * 编码器B相中断处理（ENCODER2A_IRQHandler）
 * 
 * 功能：
 * - 捕获E2A上升沿
 * - 读取E2B电平判断方向
 * - 更新脉冲计数
 * 
 * 说明：
 * - 此函数由SysConfig自动调用
 * - 在isr/isr_motor.c中实现
 */
void Motor_Encoder2A_ISR(void);

/*
 * 计时器中断处理（CLOCK_IRQHandler）
 * 
 * 功能：
 * - 计算电机实时速度
 * - 执行PID控制
 * - 更新PWM占空比
 * 
 * 说明：
 * - 此函数由SysConfig自动调用
 * - 在isr/isr_motor.c中实现
 * - 调用频率：100Hz（10ms）
 */
void Motor_Clock_ISR(void);

#endif /* DRV_MOTOR_H */
