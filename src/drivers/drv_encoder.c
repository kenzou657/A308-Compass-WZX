/**
 * @file drv_encoder.c
 * @brief 编码器驱动模块 - 实现文件
 * 
 * 实现编码器脉冲读取和速度计算功能
 * 使用整数运算，避免浮点数
 */

#include "drv_encoder.h"

/* ============ 全局变量 ============ */

// 编码器状态数组
static EncoderState_t g_encoder_state[ENCODER_COUNT] = {0};

/* ============ 初始化函数 ============ */

/**
 * @brief 初始化编码器驱动模块
 */
void EncoderInit(void)
{
    // 初始化编码器A状态
    g_encoder_state[ENCODER_A].pulse_count = 0;
    g_encoder_state[ENCODER_A].speed_raw = 0;
    g_encoder_state[ENCODER_A].speed_filtered = 0;
    g_encoder_state[ENCODER_A].speed_mmps = 0;
    g_encoder_state[ENCODER_A].direction = 1;  // 默认正转
    
    // 初始化编码器B状态
    g_encoder_state[ENCODER_B].pulse_count = 0;
    g_encoder_state[ENCODER_B].speed_raw = 0;
    g_encoder_state[ENCODER_B].speed_filtered = 0;
    g_encoder_state[ENCODER_B].speed_mmps = 0;
    g_encoder_state[ENCODER_B].direction = 1;  // 默认正转
}

/**
 * @brief 启动编码器采样
 * @note 启用GPIO中断和CLOCK定时器
 */
void EncoderStart(void)
{
    // 启用GPIO中断（编码器A、B的A、B两相）
    DL_GPIO_enableInterrupt(GPIO_ENCODER_PORT,
        GPIO_ENCODER_E1A_PIN | GPIO_ENCODER_E1B_PIN |
        GPIO_ENCODER_E2A_PIN | GPIO_ENCODER_E2B_PIN);
    
    // 启动CLOCK定时器（TIMA0，用于速度计算）
    DL_TimerA_startCounter(CLOCK_INST);
}

/**
 * @brief 停止编码器采样
 * @note 禁用GPIO中断和CLOCK定时器
 */
void EncoderStop(void)
{
    // 禁用GPIO中断（编码器A、B的A、B两相）
    DL_GPIO_disableInterrupt(GPIO_ENCODER_PORT,
        GPIO_ENCODER_E1A_PIN | GPIO_ENCODER_E1B_PIN |
        GPIO_ENCODER_E2A_PIN | GPIO_ENCODER_E2B_PIN);
    
    // 停止CLOCK定时器（TIMA0）
    DL_TimerA_stopCounter(CLOCK_INST);
}

/* ============ 数据访问函数 ============ */

/**
 * @brief 获取编码器脉冲计数
 */
int16_t EncoderGetCount(uint8_t encoder_id)
{
    if (encoder_id >= ENCODER_COUNT) {
        return 0;
    }
    return g_encoder_state[encoder_id].pulse_count;
}

/**
 * @brief 重置编码器脉冲计数
 */
void EncoderResetCount(uint8_t encoder_id)
{
    if (encoder_id >= ENCODER_COUNT) {
        return;
    }
    g_encoder_state[encoder_id].pulse_count = 0;
}

/**
 * @brief 获取编码器原始速度（脉冲/10ms）
 */
int16_t EncoderGetRawSpeed(uint8_t encoder_id)
{
    if (encoder_id >= ENCODER_COUNT) {
        return 0;
    }
    return g_encoder_state[encoder_id].speed_raw;
}

/**
 * @brief 获取编码器滤波后速度（脉冲/10ms）
 */
int16_t EncoderGetFilteredSpeed(uint8_t encoder_id)
{
    if (encoder_id >= ENCODER_COUNT) {
        return 0;
    }
    return g_encoder_state[encoder_id].speed_filtered;
}

/**
 * @brief 获取编码器线速度（mm/s）
 */
int16_t EncoderGetSpeed(uint8_t encoder_id)
{
    if (encoder_id >= ENCODER_COUNT) {
        return 0;
    }
    return g_encoder_state[encoder_id].speed_mmps;
}

/**
 * @brief 获取编码器方向
 */
uint8_t EncoderGetDirection(uint8_t encoder_id)
{
    if (encoder_id >= ENCODER_COUNT) {
        return 1;
    }
    return g_encoder_state[encoder_id].direction;
}

/**
 * @brief 获取编码器状态结构体指针（供ISR使用）
 */
EncoderState_t* EncoderGetState(uint8_t encoder_id)
{
    if (encoder_id >= ENCODER_COUNT) {
        return NULL;
    }
    return &g_encoder_state[encoder_id];
}

/* ============ 速度计算函数 ============ */

/**
 * @brief 计算编码器速度（在CLOCK中断中调用）
 *
 * 算法流程：
 * 1. 保存当前脉冲计数作为原始速度
 * 2. 一阶低通滤波：speed_filtered = α*speed_raw + (1-α)*speed_prev
 * 3. 转换为线速度：v(mm/s) = (speed_filtered * ENCODER_SPEED_RATIO_NUM) / ENCODER_SPEED_RATIO_DEN
 * 4. 清零脉冲计数器
 *
 * 整数运算说明：
 * - 滤波系数α使用整数表示：SPEED_FILTER_ALPHA (0-1000)
 * - 滤波公式：speed_filtered = (α*speed_raw + (1000-α)*speed_prev) / 1000
 * - 速度转换：speed_mmps = (speed_filtered * 14030) / 1000
 */
void EncoderCalculateSpeed(void)
{
    int32_t temp;  // 临时变量，用于中间计算
    
    // 处理编码器A
    // 1. 保存原始速度（脉冲/10ms）
    g_encoder_state[ENCODER_A].speed_raw = g_encoder_state[ENCODER_A].pulse_count;
    
    // 2. 一阶低通滤波（整数运算）
    // speed_filtered = (α*speed_raw + (1000-α)*speed_prev) / 1000
    temp = (int32_t)SPEED_FILTER_ALPHA * g_encoder_state[ENCODER_A].speed_raw;
    temp += (int32_t)(1000 - SPEED_FILTER_ALPHA) * g_encoder_state[ENCODER_A].speed_filtered;
    g_encoder_state[ENCODER_A].speed_filtered = (int16_t)(temp / 1000);
    
    // 3. 转换为线速度（mm/s）
    // speed_mmps = (speed_filtered * 1403)
    g_encoder_state[ENCODER_A].speed_mmps = (int32_t)g_encoder_state[ENCODER_A].speed_filtered * ENCODER_SPEED_RATIO_NUM;
    
    // 4. 更新方向标志
    if (g_encoder_state[ENCODER_A].speed_filtered > 0) {
        g_encoder_state[ENCODER_A].direction = 1;  // 正转
    } else if (g_encoder_state[ENCODER_A].speed_filtered < 0) {
        g_encoder_state[ENCODER_A].direction = 0;  // 反转
    }
    // 速度为0时保持上次方向
    
    // 5. 清零脉冲计数器
    g_encoder_state[ENCODER_A].pulse_count = 0;
    
    // 处理编码器B（同样的流程）
    // 1. 保存原始速度（脉冲/10ms）
    g_encoder_state[ENCODER_B].speed_raw = g_encoder_state[ENCODER_B].pulse_count;
    
    // 2. 一阶低通滤波（整数运算）
    temp = (int32_t)SPEED_FILTER_ALPHA * g_encoder_state[ENCODER_B].speed_raw;
    temp += (int32_t)(1000 - SPEED_FILTER_ALPHA) * g_encoder_state[ENCODER_B].speed_filtered;
    g_encoder_state[ENCODER_B].speed_filtered = (int16_t)(temp / 1000);
    
    // 3. 转换为线速度（mm/s）
    g_encoder_state[ENCODER_B].speed_mmps = (int32_t)g_encoder_state[ENCODER_B].speed_filtered * ENCODER_SPEED_RATIO_NUM;
    
    // 4. 更新方向标志
    if (g_encoder_state[ENCODER_B].speed_filtered > 0) {
        g_encoder_state[ENCODER_B].direction = 1;  // 正转
    } else if (g_encoder_state[ENCODER_B].speed_filtered < 0) {
        g_encoder_state[ENCODER_B].direction = 0;  // 反转
    }
    
    // 5. 清零脉冲计数器
    g_encoder_state[ENCODER_B].pulse_count = 0;
}
