/**
 * @file drv_encoder.h
 * @brief 编码器驱动模块 - 头文件
 * 
 * 功能：
 * - 两路直流有刷电机编码器脉冲读取
 * - 通过捕获A相上升沿，判断B相电平实现方向识别
 * - 脉冲计数转换为速度（mm/s）
 * - 使用整数运算，避免浮点数
 * 
 * 硬件连接：
 * - 电机1编码器：A相PA12(TIMG0_CCP0)，B相PB20(GPIO)
 * - 电机2编码器：A相PA15(TIMA1_CCP0)，B相PB25(GPIO)
 * 
 * 使用方法：
 * 1. 在main()中调用 EncoderInit() 初始化
 * 2. 在主循环中调用 EncoderGetSpeed() 获取速度
 * 3. 中断处理由 isr_encoder.c 实现
 */

#ifndef _DRV_ENCODER_H_
#define _DRV_ENCODER_H_

#include "ti_msp_dl_config.h"
#include "src/config.h"

/* ============ 编码器编号定义 ============ */
#define ENCODER_A             0   // 电机A编码器
#define ENCODER_B             1   // 电机B编码器
#define ENCODER_COUNT         2   // 编码器总数

/* ============ 编码器状态结构体 ============ */
typedef struct {
    volatile int16_t pulse_count;      // 脉冲计数（支持正反转，-32768~+32767）
    volatile int16_t speed_raw;        // 原始速度（脉冲/10ms）
    volatile int16_t speed_filtered;   // 滤波后速度（脉冲/10ms）
    volatile int16_t speed_mmps;       // 线速度（mm/s）
    volatile uint8_t direction;        // 方向标志（0=反转，1=正转）
} EncoderState_t;

/* ============ 编码器驱动函数声明 ============ */

/**
 * @brief 初始化编码器驱动模块
 * @note 在main函数中调用SYSCFG_DL_init()后调用此函数
 * @note SysConfig已配置E1A、E2A、CLOCK定时器，此函数仅初始化状态
 */
void EncoderInit(void);

/**
 * @brief 启动编码器采样
 * @note 启动E1A、E2A捕获定时器和CLOCK定时器
 */
void EncoderStart(void);

/**
 * @brief 停止编码器采样
 * @note 停止E1A、E2A捕获定时器和CLOCK定时器
 */
void EncoderStop(void);

/**
 * @brief 获取编码器脉冲计数
 * @param encoder_id: 编码器编号 (ENCODER_A 或 ENCODER_B)
 * @return 脉冲计数（正数=正转，负数=反转）
 */
int16_t EncoderGetCount(uint8_t encoder_id);

/**
 * @brief 重置编码器脉冲计数
 * @param encoder_id: 编码器编号 (ENCODER_A 或 ENCODER_B)
 */
void EncoderResetCount(uint8_t encoder_id);

/**
 * @brief 获取编码器原始速度（脉冲/10ms）
 * @param encoder_id: 编码器编号 (ENCODER_A 或 ENCODER_B)
 * @return 原始速度（脉冲/10ms）
 */
int16_t EncoderGetRawSpeed(uint8_t encoder_id);

/**
 * @brief 获取编码器滤波后速度（脉冲/10ms）
 * @param encoder_id: 编码器编号 (ENCODER_A 或 ENCODER_B)
 * @return 滤波后速度（脉冲/10ms）
 */
int16_t EncoderGetFilteredSpeed(uint8_t encoder_id);

/**
 * @brief 获取编码器线速度（mm/s）
 * @param encoder_id: 编码器编号 (ENCODER_A 或 ENCODER_B)
 * @return 线速度（mm/s，正数=正转，负数=反转）
 */
int16_t EncoderGetSpeed(uint8_t encoder_id);

/**
 * @brief 获取编码器方向
 * @param encoder_id: 编码器编号 (ENCODER_A 或 ENCODER_B)
 * @return 方向标志（0=反转，1=正转）
 */
uint8_t EncoderGetDirection(uint8_t encoder_id);

/**
 * @brief 获取编码器状态结构体指针（供ISR使用）
 * @param encoder_id: 编码器编号 (ENCODER_A 或 ENCODER_B)
 * @return 编码器状态结构体指针
 */
EncoderState_t* EncoderGetState(uint8_t encoder_id);

/* ============ 速度计算函数（供CLOCK ISR调用）============ */

/**
 * @brief 计算编码器速度（在CLOCK中断中调用）
 * @note 将脉冲计数转换为速度，执行滤波，清零计数器
 */
void EncoderCalculateSpeed(void);

#endif /* _DRV_ENCODER_H_ */
