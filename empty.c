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
#include "app_camera_uart.h"
#include "drv_jy61p.h"
#include "drv_encoder.h"

// 变量创建区
volatile uint32_t uwTick_Motor_Set_Point = 0;   // 控制Motor_Proc的执行速度
volatile uint32_t uwTick_IMU_Print_Point = 0;   // 控制IMU数据打印的执行速度

// 打印缓冲区
static char g_print_buffer[128];

// 校准状态
static uint8_t g_calibration_started = 0;

// 子函数声明区
void Motor_Proc(void);
void IMU_Proc(void);
void Calibration_Proc(void);

int main(void)
{
    SYSCFG_DL_init();
    MotorInit();
    
    // 初始化 UART0（摄像头）
    uart_init();
    camera_uart_init();
    
    // 初始化 UART1（IMU）
    jy61p_init();
    
    // 初始化编码器驱动
    EncoderInit();
    
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

    MotorASet(MOTOR_DIR_FORWARD, 150);  // 电机B正转，50%占空比

    while (1) {
        Motor_Proc();
        Calibration_Proc();
		IMU_Proc();
		
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
    }
}

// 电机处理任务函数 - 定期打印编码器速度
void Motor_Proc(void)
{
    if ((uwTick - uwTick_Motor_Set_Point) < 1000) {
        return;
    }
    uwTick_Motor_Set_Point = uwTick;

    // 读取编码器速度（mm/s）
    int16_t speed_a = EncoderGetSpeed(ENCODER_A);
    int16_t speed_b = EncoderGetSpeed(ENCODER_B);
    
    // 读取原始速度（脉冲/10ms）
    int16_t raw_speed_a = EncoderGetRawSpeed(ENCODER_A);
    int16_t raw_speed_b = EncoderGetRawSpeed(ENCODER_B);
    
    // 读取滤波后速度（脉冲/10ms）
    int16_t filtered_speed_a = EncoderGetFilteredSpeed(ENCODER_A);
    int16_t filtered_speed_b = EncoderGetFilteredSpeed(ENCODER_B);
    
    // 读取脉冲计数
    int16_t count_a = EncoderGetCount(ENCODER_A);
    int16_t count_b = EncoderGetCount(ENCODER_B);
    
    // 读取方向
    uint8_t dir_a = EncoderGetDirection(ENCODER_A);
    uint8_t dir_b = EncoderGetDirection(ENCODER_B);
    
    // 格式化编码器数据并通过串口发送
    // 格式：Motor_A: speed=XXX.XX mm/s, raw=XXX pulse/10ms, filtered=XXX pulse/10ms, count=XXX, dir=X
    // 将整数速度转换为浮点数显示（除以100得到小数点后两位）
    sprintf(g_print_buffer, "Motor_A: speed=%d mm/s, raw=%d, filtered=%d\r\n",
            speed_a, raw_speed_a, filtered_speed_a);
    
    // 通过 UART0 发送至上位机
    uart_send_data((uint8_t *)g_print_buffer, strlen(g_print_buffer));
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
        // int32_t yaw_int = angle.yaw_deg_x100 / 100;
        // int32_t yaw_frac = angle.yaw_deg_x100 % 100;
        
        // // 处理负数的小数部分
        // if (yaw_frac < 0) {
        //     yaw_frac = -yaw_frac;
        // }
        
        // sprintf(g_print_buffer, "YAW: %ld.%02ld deg\r\n", yaw_int, yaw_frac);
        
        // // 通过 UART0 发送至上位机（使用 sizeof 计算长度）
        // uart_send_data((uint8_t *)g_print_buffer, sizeof(g_print_buffer));
    }
}
