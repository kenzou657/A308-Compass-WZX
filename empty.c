/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "timer.h"
#include "drv_buzzer.h"
#include "drv_led.h"
#include "drv_motor.h"
#include "drv_uart.h"
#include "drv_key.h"
#include "app_camera_uart.h"
#include "drv_jy61p.h"
#include "drv_encoder.h"
#include "drv_chassis.h"
#include "src/utils/pid.h"
#include "src/config.h"
#include "app_chassis_task.h"
#include "key_logic.h"
#include "oled.h"
#include "app_oled_display.h"

// 变量创建区
volatile uint32_t uwTick_Motor_Set_Point = 0;   // 控制Motor_Proc的执行速度
volatile uint32_t uwTick_IMU_Print_Point = 0;   // 控制IMU数据打印的执行速度
volatile uint32_t uwTick_PID_Control_Point = 0; // 控制PID实时控制的执行速度（CLOCK采样周期）
volatile uint32_t uwTick_Chassis_Control_Point = 0; // 控制小车底盘闭环控制的执行速度
volatile uint32_t uwTick_OLED_Set_Point = 0;    // 控制 OLED_Proc 的执行速度
volatile uint32_t uwTick_Key_Set_Point = 0;     // 控制 Key_Scan 的执行速度

// 打印缓冲区
static char g_print_buffer[128];

// 校准状态
static uint8_t g_calibration_started = 0;



// 子函数声明区
void Motor_Proc(void);
void IMU_Proc(void);
void Calibration_Proc(void);
void PID_Control_Proc(void);
void Chassis_Control_Proc(void);
void OLED_Proc(void);
void Key_Proc(void);

int main(void)
{
    SYSCFG_DL_init();
    MotorInit();
    
    // 初始化 UART0（摄像头）
    uart_init();
    camera_uart_init();

    // 初始化 UART1（IMU）
    jy61p_init();

     // 初始化 OLED 显示
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(4, 0, (uint8_t *)"System Init", 12, 1);
    OLED_ShowString(4, 12, (uint8_t *)"Loading...", 12, 1);
    OLED_Refresh();

    // 初始化按键驱动
    Key_Init();
    
    // 初始化按键逻辑
    Key_Logic_Init();

    // 初始化OLED显示模块
    OLED_Display_Init();
    
    // 初始化编码器驱动
    EncoderInit();
    
    // 初始化电机 PID 闭环控制
    MotorPID_Init();
    
    // 初始化小车底盘控制模块
    ChassisInit();
    
    // 使能编码器相关中断
    NVIC_EnableIRQ(GPIOB_INT_IRQn);      // GPIOB - 编码器GPIO中断
    NVIC_EnableIRQ(CLOCK_INST_INT_IRQN); // TIMA0 - 时钟定时器
    
    // 启动编码器采样
    EncoderStart();
    
    // 启动校准（采集 20 个样本）
    jy61p_start_calibration(20);
    g_calibration_started = 1;
    
    // 使能全局中断（必须在所有初始化完成后调用）
    __enable_irq();

    
    // 显示初始化完成
    OLED_Display_Update();

    

    while (1) {
        Motor_Proc();
        PID_Control_Proc();
        Calibration_Proc();
        IMU_Proc();
        Chassis_Control_Proc();  // 小车底盘闭环控制
        OLED_Proc();            // OLED显示更新
        Key_Proc();             // 按键扫描和逻辑处理
    }
}

// 校准处理函数
void Calibration_Proc(void)
{
    // 校准完成后，只执行一次提示
    if (g_calibration_started && jy61p_is_calibration_done()) {
        g_calibration_started = 0;  // 标记已处理
        
        // 可选：打印校准完成信息
        sprintf(g_print_buffer, "Calibration Done\r\n");
        uart_send_data((uint8_t *)g_print_buffer, sizeof(g_print_buffer));
        ChassisTaskStart();
    }
}

// 电机处理任务函数 - 定期打印编码器速度
void Motor_Proc(void)
{
    if ((uwTick - uwTick_Motor_Set_Point) < 20) {
        return;
    }
    uwTick_Motor_Set_Point = uwTick;

    // // 读取编码器速度（mm/s）
    // int16_t speed_a = EncoderGetSpeed(ENCODER_A);
    // int16_t speed_b = EncoderGetSpeed(ENCODER_B);
    
    // // 读取原始速度（脉冲/10ms）
    // int16_t raw_speed_a = EncoderGetRawSpeed(ENCODER_A);
    // int16_t raw_speed_b = EncoderGetRawSpeed(ENCODER_B);
    
    // // 读取滤波后速度（脉冲/10ms）
    // int16_t filtered_speed_a = EncoderGetFilteredSpeed(ENCODER_A);
    // int16_t filtered_speed_b = EncoderGetFilteredSpeed(ENCODER_B);
    
    // // 读取脉冲计数
    // int16_t count_a = EncoderGetCount(ENCODER_A);
    // int16_t count_b = EncoderGetCount(ENCODER_B);
    
    // // 读取方向
    // uint8_t dir_a = EncoderGetDirection(ENCODER_A);
    // uint8_t dir_b = EncoderGetDirection(ENCODER_B);
    
    // // 格式化编码器数据并通过串口发送
    // // 格式：Motor_A: speed=XXX.XX mm/s, raw=XXX pulse/10ms, filtered=XXX pulse/10ms, count=XXX, dir=X
    // // 将整数速度转换为浮点数显示（除以100得到小数点后两位）
    // sprintf(g_print_buffer, "%d, %d\n",
    //         speed_a, speed_b);
    
    // // 通过 UART0 发送至上位机
    // uart_send_data((uint8_t *)g_print_buffer, strlen(g_print_buffer));

    
}

void IMU_Proc(void)
{
    // 校准未完成时不打印数据
    if (!jy61p_is_calibration_done()) {
        return;
    }
    
    if ((uwTick - uwTick_IMU_Print_Point) < 1000) {
        return;
    }
    uwTick_IMU_Print_Point = uwTick;

    jy61p_angle_t angle;

    if (jy61p_get_angle(&angle) == 0) {
        // 使用 sprintf 格式化偏航角数据（整数格式，单位：0.01°）
        // yaw_deg_x100 = 100 表示 1.00°
        // 已应用零点偏移，相对于上电时的方向
        int32_t yaw_int = angle.yaw_deg_x100 / 100;
        int32_t yaw_frac = angle.yaw_deg_x100 % 100;
        
        // 处理负数的小数部分
        if (yaw_frac < 0) {
            yaw_frac = -yaw_frac;
        }
        
        sprintf(g_print_buffer, "YAW: %ld.%02ld deg\r\n", yaw_int, yaw_frac);
        
        // 通过 UART0 发送至上位机（使用 sizeof 计算长度）
        uart_send_data((uint8_t *)g_print_buffer, sizeof(g_print_buffer));
    }

    LEDG_TOGGLE();
    
}

/**
 * @brief PID 实时控制处理函数
 *
 * 在 CLOCK 定时器采样周期（20ms）内执行 PID 控制
 * 根据编码器反馈速度和目标速度计算 PID 输出，
 * 实时调整电机 PWM 占空比
 */
void PID_Control_Proc(void)
{
    // CLOCK 定时器采样周期为 20ms
    // 每个采样周期执行一次 PID 控制
    if ((uwTick - uwTick_PID_Control_Point) < 20) {
        return;
    }
    uwTick_PID_Control_Point = uwTick;
    
    // 执行 PID 闭环控制
    // 根据目标速度和编码器反馈速度计算 PID 输出
//    ForwardPID(10000);
		
    // Forward(500);  // 注释掉原有的简单前进控制，改用小车底盘闭环控制
}

/**
 * @brief 小车底盘闭环控制处理函数
 *
 * 功能：
 * - 基于陀螺仪 Yaw 角的闭环方向控制
 * - 基于运行时间的位移控制
 * - 差速控制实现转向
 *
 * 调用周期：10-20ms（建议）
 */
void Chassis_Control_Proc(void)
{
    // 控制周期：10ms
    if ((uwTick - uwTick_Chassis_Control_Point) < 20) {
        return;
    }
    uwTick_Chassis_Control_Point = uwTick;
    
    // 校准未完成时不执行控制
    if (!jy61p_is_calibration_done()) {
        return;
    }

    ChassisTaskUpdate();
    
    // 执行小车底盘闭环控制
    ChassisUpdate();

    uint8_t state = ChassisTaskGetState();
    
    // // 可选：打印调试信息（降低频率，避免影响性能）
    // static uint32_t debug_print_count = 0;
    // debug_print_count++;
    
    // if (debug_print_count >= 100) {  // 每 1 秒打印一次（100 * 10ms）
    //     debug_print_count = 0;
        
    //     Chassis_t *state = ChassisGetState();
    //     if (state->state == CHASSIS_STATE_MOVING) {
    //         // 格式化调试信息
    //         sprintf(g_print_buffer, "Chassis: Yaw=%d, Err=%d, PID=%d, L=%d, R=%d\r\n",
    //                 state->current_yaw, state->yaw_error, state->pid_output,
    //                 state->motor_left_pwm, state->motor_right_pwm);
            
    //         // 通过 UART0 发送至上位机
    //         uart_send_data((uint8_t *)g_print_buffer, strlen(g_print_buffer));
    //     }
    // }
}

// ============ 按键处理函数 ============
/**
 * @brief 按键处理函数（减速调用）
 *
 * 执行周期：10ms
 * 功能：
 * - 扫描按键状态
 * - 处理按键事件
 */
void Key_Proc(void)
{
    // 减速控制：每 10ms 执行一次
    if ((uwTick - uwTick_Key_Set_Point) < 10) {
        return;
    }
    uwTick_Key_Set_Point = uwTick;
    
    // 扫描按键状态
    Key_Scan();
    
    // 处理按键逻辑
    Key_Logic_Process();
}

// ============ OLED 显示减速函数 ============
/**
 * @brief OLED 显示处理函数（减速调用）
 *
 * 执行周期：500ms
 * 功能：
 * - 显示当前任务ID
 * - 显示系统状态（IDLE/RUNNING）
 * - 显示按键提示
 */
void OLED_Proc(void)
{
    // 减速控制：每 200ms 执行一次
    if ((uwTick - uwTick_OLED_Set_Point) < 500) {
        return;
    }
    uwTick_OLED_Set_Point = uwTick;
    
    // 更新OLED显示
    OLED_Display_Update();
}

