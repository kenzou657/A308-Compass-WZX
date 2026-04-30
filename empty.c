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
#include "drv_key.h"
#include "app_camera_uart.h"
#include "app_task_manager.h"
#include "key_logic.h"
#include "oled.h"
#include "app_oled_display.h"
#include "app_vacuum_pump.h"
#include "app_gripper.h"
#include "drv_motor.h"
#include "config.h"

// ============ 变量创建区 ============
volatile uint32_t uwTick_Motor_Set_Point = 0;   // 控制 Motor_Proc 的执行速度
volatile uint32_t uwTick_Camera_Set_Point = 0;  // 控制 Camera_Proc 的执行速度
volatile uint32_t uwTick_OLED_Set_Point = 0;    // 控制 OLED_Proc 的执行速度
volatile uint32_t uwTick_Key_Set_Point = 0;     // 控制 Key_Scan 的执行速度

uint8_t motor_state = 0;

// ============ 子函数声明区 ============
void Motor_Proc(void);
void Camera_Proc(void);
void OLED_Proc(void);
void Key_Proc(void);

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
    OLED_ShowString(4, 0, (uint8_t *)"System Init", 12, 1);
    OLED_ShowString(4, 12, (uint8_t *)"Loading...", 12, 1);
    OLED_Refresh();
    
    // 初始化按键驱动
    Key_Init();
    
    // 初始化按键逻辑
    Key_Logic_Init();
    
    // 初始化任务管理器
    TaskManager_Init();
    
    // 初始化OLED显示模块
    OLED_Display_Init();

    Motor_Init();
    
    // 初始化真空泵应用
    VacuumPump_App_Init();
    
    // 初始化夹爪应用
    Gripper_App_Init();
    
    // 显示初始化完成
    OLED_Display_Update();


    

    while (1) {
        Key_Proc();             // 按键扫描和逻辑处理
        TaskManager_Update();   // 任务执行
        VacuumPump_App_Update(); // 真空泵状态机更新
        Gripper_App_Update();   // 夹爪状态机更新
        Camera_Proc();          // 摄像头数据处理
        OLED_Proc();            // OLED显示更新
        Motor_Proc();        // 电机控制（可选）
        // beep_1s_process();   // 蜂鸣器（可选）
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
    motor_state++;
    if(motor_state >= 4) {
        motor_state = 0;
        Motor_SetDuty(MOTOR_A, 150);
        Motor_SetDirection(MOTOR_A, MOTOR_DIR_FORWARD);
        Motor_SetDuty(MOTOR_B, 150);
        Motor_SetDirection(MOTOR_B, MOTOR_DIR_FORWARD);
    }

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
 * 执行周期：200ms
 * 功能：
 * - 显示当前任务ID
 * - 显示系统状态（IDLE/RUNNING）
 * - 显示按键提示
 */
void OLED_Proc(void)
{
    // 减速控制：每 200ms 执行一次
    if ((uwTick - uwTick_OLED_Set_Point) < 200) {
        return;
    }
    uwTick_OLED_Set_Point = uwTick;
    
    // 更新OLED显示
    OLED_Display_Update();
}

void HardFault_Handler(void) {
    while(1); // 在这里打断点！
    // 如果电机一转卡在这里，说明是电源/干扰把硬件震崩了，而不是代码逻辑错。
}