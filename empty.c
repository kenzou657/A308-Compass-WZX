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
#include "timer.h"
#include "drv_buzzer.h"
#include "drv_led.h"
#include "drv_uart.h"
#include "app_camera_uart.h"
#include "oled.h"

// ============ 变量创建区 ============
volatile uint32_t uwTick_Motor_Set_Point = 0;   // 控制 Motor_Proc 的执行速度
volatile uint32_t uwTick_Camera_Set_Point = 0;  // 控制 Camera_Proc 的执行速度
volatile uint32_t uwTick_OLED_Set_Point = 0;    // 控制 OLED_Proc 的执行速度

// ============ 子函数声明区 ============
void Motor_Proc(void);
void Camera_Proc(void);
void OLED_Proc(void);

int main(void)
{
    SYSCFG_DL_init();
    
    // 初始化串口驱动
    uart_init();
    
    // 初始化摄像头通信模块
    camera_uart_init();
    
    // 初始化 OLED 显示
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(4, 0, (uint8_t *)"Camera System", 12, 1);
    OLED_ShowString(4, 12, (uint8_t *)"Initializing...", 12, 1);
    OLED_Refresh();

    while (1) {
        Motor_Proc();
        Camera_Proc();
        // OLED_Proc();
        // beep_1s_process();
    }
}

// ============ 电机减速任务函数 ============
/**
 * @brief 电机处理函数（减速调用）
 *
 * 执行周期：3000ms
 */
void Motor_Proc(void)
{
    if ((uwTick - uwTick_Motor_Set_Point) < 1000) {
        return;
    }
    uwTick_Motor_Set_Point = uwTick;

    // 电机控制逻辑在此处添加
    // beep_1s_start();
    LEDG_TOGGLE();
}

// ============ 摄像头通信减速函数 ============
/**
 * @brief 摄像头通信处理函数（减速调用）
 *
 * 执行周期：100ms
 * 功能：
 * - 获取摄像头接收到的数据
 * - 处理摄像头命令
 * - 更新调试数据，便于断点观察
 * 
 * 注意：摄像头接收使用中断处理，无需减速函数
 *      接收数据通过 camera_uart_rx_callback() 在中断中解析
 */
void Camera_Proc(void)
{
    // 减速控制：每 100ms 执行一次
    if ((uwTick - uwTick_Camera_Set_Point) < 100) {
        return;
    }
    uwTick_Camera_Set_Point = uwTick;
    
    // 获取最新接收的摄像头数据
    camera_frame_data_t frame;
    if (camera_uart_get_rx_frame(&frame) == 0) {
        // 成功获取摄像头数据
        // 此处可添加数据处理逻辑
        // 例如：根据 Mode 和 ID 进行相应的控制
        
        // 示例：根据工作模式处理
        if (frame.mode == CAMERA_MODE_LINE_TRACKING) {
            // 寻线模式处理
            // 使用 frame.data_x 和 frame.data_y 进行控制
        } else if (frame.mode == CAMERA_MODE_DIGIT_RECOG) {
            // 数字识别模式处理
            // 使用 frame.id 获取识别的数字
        }
    }
    
    // 此处可添加定期发送命令到摄像头的逻辑
    // 例如：camera_uart_send_frame(mode, id, data_x, data_y);
}

// ============ OLED 显示减速函数 ============
/**
 * @brief OLED 显示处理函数（减速调用）
 *
 * 执行周期：500ms
 * 功能：
 * - 显示摄像头数据（Mode、ID、Data_X、Data_Y）
 * - 显示系统状态信息
 * - 支持数字、字母、字符串显示
 */
void OLED_Proc(void)
{
    // 减速控制：每 500ms 执行一次
    if ((uwTick - uwTick_OLED_Set_Point) < 1000) {
        return;
    }
    uwTick_OLED_Set_Point = uwTick;
    
    // 清空显示缓冲
    // OLED_Clear();
    
    // 格式化字符串缓冲
    uint8_t display_buf[32];
    
    // 第 0 行：标题
    OLED_ShowString(4, 0, (uint8_t *)"System Status        ", 12, 1);
    
    // 第 1 行：ID
    camera_frame_data_t frame;
    if (camera_uart_get_rx_history(0, &frame) == 0) {
        sprintf((char *)display_buf, "ID: %02d           ", frame.id);
    } else {
        sprintf((char *)display_buf, "ID: --");
    }
    OLED_ShowString(4, 12, display_buf, 12, 1);
    
    // 第 2 行：陀螺仪偏航角（Yaw）
    // TODO: 从陀螺仪获取偏航角数据，此处显示示例值
    sprintf((char *)display_buf, "Yaw: 0000 deg            ");
    OLED_ShowString(4, 24, display_buf, 12, 1);
    
    // 第 3 行：当前任务序号
    // TODO: 从任务管理器获取当前任务序号
    sprintf((char *)display_buf, "Task: 0                  ");
    OLED_ShowString(4, 36, display_buf, 12, 1);
    
    // 第 4 行：预留或其他信息
    OLED_ShowString(4, 48, (uint8_t *)"Ready                ", 12, 1);
    
    // 刷新显示
    OLED_Refresh();
}
