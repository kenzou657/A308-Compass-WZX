/**
 * @file app_task_4_digit_recognition.c
 * @brief Task 4: 数字识别 - 实现文件
 * 
 * 功能描述：
 * - 启动陀螺仪校准
 * - 转动 -9000 度（-90°）后停止
 * - 读取摄像头 0x02 工作模式下的 id 值
 * - 根据 id 值鸣响蜂鸣器（id=3 则鸣响 3 声）
 * 
 * 状态机流程：
 * IDLE → INIT → GYRO_CALIB → ROTATING → WAITING_RECOGNITION → BEEPING → SUCCESS
 */

#include "app_task_4_digit_recognition.h"
#include "../drivers/drv_buzzer.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_jy61p.h"
#include "../app/app_camera_uart.h"
#include "../utils/timer.h"
#include <stdlib.h>

/* ==================== 任务四状态定义 ==================== */
#define TASK4_STATE_IDLE                0   /* 空闲状态 */
#define TASK4_STATE_INIT                1   /* 初始化状态 */
#define TASK4_STATE_GYRO_CALIB          2   /* 陀螺仪校准中 */
#define TASK4_STATE_ROTATING            3   /* 转动中 */
#define TASK4_STATE_WAITING_RECOGNITION 4   /* 等待识别 */
#define TASK4_STATE_BEEPING             5   /* 蜂鸣中 */
#define TASK4_STATE_SUCCESS             6   /* 成功完成 */
#define TASK4_STATE_TIMEOUT             7   /* 超时 */

/* ==================== 任务四参数定义 ==================== */
#define TASK4_GYRO_CALIBRATION_SAMPLES  20  /* 陀螺仪校准采样数 */
#define TASK4_ROTATION_TARGET_YAW       -9000  /* 目标转动角度：-90°（-9000 × 0.01°） */
#define TASK4_ROTATION_PWM              0   /* 原地转弯 PWM */
#define TASK4_ROTATION_DURATION         1000  /* 转动超时时间（ms） */
#define TASK4_RECOGNITION_TIMEOUT       3000  /* 识别超时时间（ms） */
#define TASK4_BEEP_INTERVAL             300   /* 蜂鸣间隔（ms） */
#define TASK4_TOTAL_TIMEOUT             15000 /* 总超时时间（ms） */

/* ==================== 任务上下文结构 ==================== */

typedef struct {
    uint8_t state;                      /* 任务状态 */
    uint32_t start_time;                /* 任务开始时间 */
    uint32_t elapsed_time;              /* 已运行时间 */
    uint32_t timeout_ms;                /* 总超时时间 */
    
    /* 陀螺仪校准相关 */
    uint8_t gyro_calibration_done;      /* 陀螺仪校准完成标志 */
    uint32_t calibration_start_time;    /* 校准开始时间 */
    
    /* 转动相关 */
    uint8_t rotation_complete;          /* 转动完成标志 */
    uint32_t rotation_start_time;       /* 转动开始时间 */
    
    /* 识别相关 */
    uint8_t recognized_digit;           /* 识别到的数字（1-5） */
    uint8_t recognition_complete;       /* 识别是否完成 */
    uint32_t recognition_start_time;    /* 识别开始时间 */
    
    /* 蜂鸣相关 */
    uint8_t beep_count;                 /* 已鸣响次数 */
    uint8_t beep_total;                 /* 需要鸣响总次数 */
    uint32_t beep_start_time;           /* 蜂鸣开始时间 */
    uint32_t last_beep_time;            /* 上次蜂鸣时间 */
    uint8_t beep_on;                    /* 蜂鸣器当前状态 */
    
    /* 调试信息 */
    uint32_t update_count;              /* 更新次数计数 */
} Task4_Context_t;

/* ==================== 全局变量 ==================== */

static Task4_Context_t g_task4_ctx;
extern volatile uint32_t uwTick;

/* ==================== 函数实现 ==================== */

/**
 * @brief 初始化 Task 4
 * 
 * 功能：
 * - 初始化任务状态为 INIT
 * - 启动陀螺仪零点校准
 * - 记录任务开始时间
 * - 设置超时时间
 */
void Task4_Init(void)
{
    /* 初始化任务状态为 INIT */
    g_task4_ctx.state = TASK4_STATE_INIT;
    
    /* 初始化时间相关变量 */
    g_task4_ctx.start_time = uwTick;
    g_task4_ctx.elapsed_time = 0;
    
    /* 设置总超时时间 */
    g_task4_ctx.timeout_ms = TASK4_TOTAL_TIMEOUT;
    
    /* 初始化陀螺仪校准标志 */
    g_task4_ctx.gyro_calibration_done = 0;
    g_task4_ctx.calibration_start_time = uwTick;
    
    /* 启动陀螺仪零点校准 */
    jy61p_start_calibration(TASK4_GYRO_CALIBRATION_SAMPLES);
    
    /* 初始化转动相关 */
    g_task4_ctx.rotation_complete = 0;
    g_task4_ctx.rotation_start_time = 0;
    
    /* 初始化识别相关 */
    g_task4_ctx.recognized_digit = 0;
    g_task4_ctx.recognition_complete = 0;
    g_task4_ctx.recognition_start_time = 0;
    
    /* 初始化蜂鸣相关 */
    g_task4_ctx.beep_count = 0;
    g_task4_ctx.beep_total = 0;
    g_task4_ctx.beep_start_time = 0;
    g_task4_ctx.last_beep_time = 0;
    g_task4_ctx.beep_on = 0;
    
    /* 初始化调试计数 */
    g_task4_ctx.update_count = 0;
    
    /* 初始化底盘 */
    ChassisInit();
}

/**
 * @brief Task 4 主循环
 * 
 * 功能：
 * - 等待陀螺仪校准完成
 * - 校准完成后启动转动 -9000 度
 * - 转动完成后等待摄像头识别
 * - 识别完成后根据 id 值鸣响蜂鸣器
 */
void Task4_Run(void)
{
    /* 检查任务状态 */
    if (g_task4_ctx.state == TASK4_STATE_IDLE || 
        g_task4_ctx.state == TASK4_STATE_SUCCESS ||
        g_task4_ctx.state == TASK4_STATE_TIMEOUT) {
        return;
    }
    
    /* 计算已运行时间 */
    g_task4_ctx.elapsed_time = uwTick - g_task4_ctx.start_time;
    
    /* 总超时检查 */
    if (g_task4_ctx.elapsed_time > g_task4_ctx.timeout_ms) {
        g_task4_ctx.state = TASK4_STATE_TIMEOUT;
        ChassisStop();  /* 停止运动 */
        return;
    }
    
    /* ========== INIT 状态：等待陀螺仪校准完成 ========== */
    if (g_task4_ctx.state == TASK4_STATE_INIT) {
        if (jy61p_is_calibration_done()) {
            g_task4_ctx.gyro_calibration_done = 1;
            g_task4_ctx.state = TASK4_STATE_ROTATING;
            g_task4_ctx.rotation_start_time = uwTick;
            
            /* 启动转动：-9000 度（-90°），原地转弯 */
            ChassisSetMotion(
                TASK4_ROTATION_TARGET_YAW,      /* 目标角度：-90° */
                TASK4_ROTATION_DURATION,        /* 转动超时时间 */
                TASK4_ROTATION_PWM,             /* 原地转弯 PWM=0 */
                0                               /* 前进方向 */
            );
        }
    }
    
    /* ========== ROTATING 状态：监控转动 ========== */
    if (g_task4_ctx.state == TASK4_STATE_ROTATING) {
        /* 更新底盘控制 */
        ChassisUpdate();
        
        /* 检查转动是否完成（通过底盘状态或时间） */
        uint32_t rotation_elapsed = uwTick - g_task4_ctx.rotation_start_time;
        if (rotation_elapsed > TASK4_ROTATION_DURATION) {
            g_task4_ctx.rotation_complete = 1;
            g_task4_ctx.state = TASK4_STATE_WAITING_RECOGNITION;
            g_task4_ctx.recognition_start_time = uwTick;
            
            /* 停止底盘运动 */
            ChassisStop();
            
            /* 等待一段时间后再开始识别（避免转动过程中的干扰） */
            delay_ms(200);
        }
    }
    
    /* ========== WAITING_RECOGNITION 状态：等待摄像头识别 ========== */
    if (g_task4_ctx.state == TASK4_STATE_WAITING_RECOGNITION) {
        uint32_t recognition_elapsed = uwTick - g_task4_ctx.recognition_start_time;
        
        /* 检查是否有识别结果 */
        if (g_camera_uart.rx_frame.valid && g_camera_uart.rx_frame.mode == CAMERA_MODE_DIGIT_RECOG) {
            if (g_camera_uart.rx_frame.id > 0 && g_camera_uart.rx_frame.id <= 5) {
                g_task4_ctx.recognized_digit = g_camera_uart.rx_frame.id;
                g_task4_ctx.recognition_complete = 1;
                g_task4_ctx.state = TASK4_STATE_BEEPING;
                g_task4_ctx.beep_total = g_task4_ctx.recognized_digit;
                g_task4_ctx.beep_count = 0;
                g_task4_ctx.beep_start_time = uwTick;
                g_task4_ctx.last_beep_time = uwTick;
                g_task4_ctx.beep_on = 0;
            }
        }
        
        // /* 识别超时检查 */
        // if (recognition_elapsed > TASK4_RECOGNITION_TIMEOUT) {
        //     g_task4_ctx.state = TASK4_STATE_TIMEOUT;
        //     beep_on();
        //     delay_ms(500);
        //     beep_off();
        // }
    }
    
    /* ========== BEEPING 状态：蜂鸣器鸣响 ========== */
    if (g_task4_ctx.state == TASK4_STATE_BEEPING) {
        uint32_t beep_elapsed = uwTick - g_task4_ctx.beep_start_time;
        uint32_t interval_elapsed = uwTick - g_task4_ctx.last_beep_time;
        
        /* 检查是否需要鸣响 */
        if (g_task4_ctx.beep_count < g_task4_ctx.beep_total) {
            if (interval_elapsed >= TASK4_BEEP_INTERVAL) {
                if (!g_task4_ctx.beep_on) {
                    /* 打开蜂鸣器 */
                    beep_on();
                    g_task4_ctx.beep_on = 1;
                    g_task4_ctx.last_beep_time = uwTick;
                } else if (interval_elapsed >= 100) {
                    /* 关闭蜂鸣器 */
                    beep_off();
                    g_task4_ctx.beep_on = 0;
                    g_task4_ctx.beep_count++;
                    g_task4_ctx.last_beep_time = uwTick;
                }
            }
        } else {
            /* 所有蜂鸣完成 */
            beep_off();
            g_task4_ctx.state = TASK4_STATE_SUCCESS;
        }
    }
    
    /* 更新计数器 */
    g_task4_ctx.update_count++;
}

/**
 * @brief 停止 Task 4
 * 
 * 功能：
 * - 停止底盘运动
 * - 关闭蜂鸣器
 * - 设置任务状态为 IDLE
 */
void Task4_Stop(void)
{
    /* 停止底盘运动 */
    ChassisStop();
    
    /* 关闭蜂鸣器 */
    beep_off();
    
    /* 设置任务状态为 IDLE */
    g_task4_ctx.state = TASK4_STATE_IDLE;
}

/**
 * @brief 重置 Task 4
 * 
 * 功能：
 * - 重置任务状态
 * - 清除运行时间
 * - 清除识别结果
 */
void Task4_Reset(void)
{
    g_task4_ctx.state = TASK4_STATE_IDLE;
    g_task4_ctx.start_time = 0;
    g_task4_ctx.elapsed_time = 0;
    g_task4_ctx.gyro_calibration_done = 0;
    g_task4_ctx.rotation_complete = 0;
    g_task4_ctx.recognized_digit = 0;
    g_task4_ctx.recognition_complete = 0;
    g_task4_ctx.beep_count = 0;
    g_task4_ctx.beep_total = 0;
    g_task4_ctx.beep_on = 0;
    g_task4_ctx.update_count = 0;
}

/**
 * @brief 获取 Task 4 的当前状态
 * 
 * @return 任务状态
 */
TaskState_t Task4_GetState(void)
{
    switch (g_task4_ctx.state) {
        case TASK4_STATE_SUCCESS:
            return TASK_STATE_SUCCESS;
        case TASK4_STATE_TIMEOUT:
            return TASK_STATE_TIMEOUT;
        case TASK4_STATE_IDLE:
            return TASK_STATE_IDLE;
        default:
            return TASK_STATE_RUNNING;
    }
}

/**
 * @brief 判断 Task 4 是否成功完成
 * 
 * @return true 表示成功，false 表示未成功
 */
bool Task4_IsSuccess(void)
{
    return (g_task4_ctx.state == TASK4_STATE_SUCCESS);
}

/**
 * @brief 获取识别到的数字
 * 
 * @return 识别到的数字（1-5），0 表示未识别
 */
uint8_t Task4_GetRecognizedDigit(void)
{
    return g_task4_ctx.recognized_digit;
}
