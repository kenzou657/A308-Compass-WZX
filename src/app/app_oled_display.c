/*
 * OLED显示管理模块实现
 * 
 * 功能：
 * - 显示任务选择界面
 * - 显示参数设置界面
 * - 显示任务运行状态
 */

#include "app_oled_display.h"
#include "oled.h"
#include "key_logic.h"
#include "app_task_manager.h"

/* ==================== 显示常量定义 ==================== */

#define OLED_CHAR_WIDTH     6   /* 字符宽度（6x8字体） */
#define OLED_CHAR_HEIGHT    8   /* 字符高度 */
#define OLED_LINE_HEIGHT    8  /* 行高（16像素） */

/* ==================== 静态变量 ==================== */

static uint32_t s_last_update_time = 0;
static uint32_t s_update_interval = 100;  /* 更新间隔：100ms */

/* 静态缓冲区，避免栈溢出 */
static uint8_t s_display_buf[64];

/* ==================== 辅助函数 ==================== */

/*
 * 显示反显文本（用于高亮显示）
 */
static void OLED_ShowStringReverse(uint8_t x, uint8_t y, uint8_t *str, uint8_t size)
{
    OLED_ShowString(x, y, str, size, 0);  /* mode=0表示反显 */
}

/*
 * 显示正常文本
 */
static void OLED_ShowStringNormal(uint8_t x, uint8_t y, uint8_t *str, uint8_t size)
{
    OLED_ShowString(x, y, str, size, 1);  /* mode=1表示正常显示 */
}

/* ==================== OLED显示实现 ==================== */

void OLED_Display_Init(void)
{
    /* OLED已在系统初始化中完成，这里仅初始化显示模块的内部状态 */
    s_last_update_time = 0;
}

void OLED_Display_TaskSelection(void)
{
    /* 清屏 */
    OLED_Clear();
    
    /* 显示标题 */
    OLED_ShowString(0, 0, (uint8_t *)"Task Selection", 8, 1);
    
    /* 显示当前任务 */
    TaskID_t current_task = TaskManager_GetCurrentTaskID();
    
    /* 使用静态缓冲区显示任务ID */
    sprintf((char *)s_display_buf, "Task %d", current_task);
    OLED_ShowString(0, OLED_LINE_HEIGHT, s_display_buf, 8, 1);
    
    /* 显示操作提示 */
    OLED_ShowString(0, OLED_LINE_HEIGHT * 4, (uint8_t *)"K1:Prev K2:Next", 8, 1);
    OLED_ShowString(0, OLED_LINE_HEIGHT * 5, (uint8_t *)"K3:Start", 8, 1);
    OLED_ShowString(0, OLED_LINE_HEIGHT * 6, (uint8_t *)"K2 Long:Param", 8, 1);
    
    /* 刷新显示 */
    OLED_Refresh();
}

void OLED_Display_ParamSetting(void)
{
    /* 清屏 */
    OLED_Clear();
    
    /* 显示标题 */
    OLED_ShowString(0, 0, (uint8_t *)"Param Setting", 8, 1);
    
    /* 获取参数设置 */
    ParamSetting_t *param = Key_Logic_GetParamSetting();
    
    /* 显示目的地1 */
    sprintf((char *)s_display_buf, "Dest1: %d", param->destination_1);
    if (param->current_param == PARAM_DESTINATION_1) {
        OLED_ShowStringReverse(0, OLED_LINE_HEIGHT, s_display_buf, 8);
    } else {
        OLED_ShowStringNormal(0, OLED_LINE_HEIGHT, s_display_buf, 8);
    }
    
    /* 显示目的地2 */
    sprintf((char *)s_display_buf, "Dest2: %d", param->destination_2);
    if (param->current_param == PARAM_DESTINATION_2) {
        OLED_ShowStringReverse(0, OLED_LINE_HEIGHT * 2, s_display_buf, 8);
    } else {
        OLED_ShowStringNormal(0, OLED_LINE_HEIGHT * 2, s_display_buf, 8);
    }
    
    /* 显示操作提示 */
    OLED_ShowString(0, OLED_LINE_HEIGHT * 4, (uint8_t *)"K2:Inc Value", 8, 1);
    OLED_ShowString(0, OLED_LINE_HEIGHT * 5, (uint8_t *)"K2 Long:Switch", 8, 1);
    OLED_ShowString(0, OLED_LINE_HEIGHT * 6, (uint8_t *)"K3:Confirm", 8, 1);
    
    /* 刷新显示 */
    OLED_Refresh();
}

void OLED_Display_TaskRunning(void)
{
    /* 清屏 */
    OLED_Clear();
    
    /* 显示标题 */
    OLED_ShowString(0, 0, (uint8_t *)"Task Running", 8, 1);

    ParamSetting_t *param = Key_Logic_GetParamSetting();

        /* 显示当前任务 */
    TaskID_t current_task = TaskManager_GetCurrentTaskID();
    
    /* 使用静态缓冲区显示任务ID */
    sprintf((char *)s_display_buf, "Task %d", current_task);
    OLED_ShowString(0, OLED_LINE_HEIGHT * 2, s_display_buf, 8, 1);
    
    /* 使用静态缓冲区避免栈溢出 */
    sprintf((char *)s_display_buf, "Dest1:%d,Dest2:%d", param->destination_1, param->destination_2);
    OLED_ShowString(0, OLED_LINE_HEIGHT * 3, s_display_buf, 8, 1);
    
    /* 显示操作提示 */
    OLED_ShowString(0, OLED_LINE_HEIGHT * 4, (uint8_t *)"K3:Stop", 8, 1);
    
    /* 刷新显示 */
    OLED_Refresh();
}

void OLED_Display_Update(void)
{
    /* 获取系统状态 */
    SystemState_t state = Key_Logic_GetSystemState();
    
    /* 根据系统状态更新显示 */
    switch (state) {
        case SYSTEM_STATE_IDLE:
            OLED_Display_TaskSelection();
            break;
            
        case SYSTEM_STATE_PARAM_SETTING:
            OLED_Display_ParamSetting();
            break;
            
        case SYSTEM_STATE_RUNNING:
            OLED_Display_TaskRunning();
            break;
            
        default:
            break;
    }
}
