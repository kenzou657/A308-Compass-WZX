/*
 * OLED显示管理模块头文件
 * 
 * 功能：
 * - 显示任务选择界面
 * - 显示参数设置界面
 * - 显示任务运行状态
 */

#ifndef APP_OLED_DISPLAY_H
#define APP_OLED_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* ==================== OLED显示接口 ==================== */

/*
 * 初始化OLED显示模块
 */
void OLED_Display_Init(void);

/*
 * 显示任务选择界面
 * 
 * 功能：
 * - 显示当前选中的任务
 * - 显示任务名称
 */
void OLED_Display_TaskSelection(void);

/*
 * 显示参数设置界面
 * 
 * 功能：
 * - 显示目的地1和目的地2
 * - 显示当前正在设置的参数（用反显表示）
 */
void OLED_Display_ParamSetting(void);

/*
 * 显示任务运行状态
 * 
 * 功能：
 * - 显示当前运行的任务
 * - 显示运行时间
 */
void OLED_Display_TaskRunning(void);

/*
 * 更新OLED显示
 * 
 * 功能：
 * - 根据系统状态更新显示内容
 * 
 * 调用位置：主循环中定期调用
 */
void OLED_Display_Update(void);

#endif /* APP_OLED_DISPLAY_H */
