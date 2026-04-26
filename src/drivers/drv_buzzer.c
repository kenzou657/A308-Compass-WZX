#include "ti_msp_dl_config.h"
#include "timer.h"

// 蜂鸣器状态机状态定义
typedef enum {
    BEEP_IDLE = 0,      // 空闲状态
    BEEP_RUNNING = 1    // 鸣响中
} beep_state_t;

// 蜂鸣器状态机变量
static beep_state_t beep_state = BEEP_IDLE;
static uint32_t beep_start_tick = 0;

void beep_on(void) {
    DL_GPIO_setPins(GPIO_BEEP_PORT, GPIO_BEEP_USER_BEEP_PIN);
}

void beep_off(void) {
    DL_GPIO_clearPins(GPIO_BEEP_PORT, GPIO_BEEP_USER_BEEP_PIN);
}

/**
 * @brief 启动蜂鸣器鸣响1秒（非阻塞式）
 * @note 调用此函数后，需要在主循环中周期性调用 beep_1s_process()
 */
void beep_1s_start(void) {
    if (beep_state == BEEP_IDLE) {
        beep_on();                      // 打开蜂鸣器
        beep_start_tick = MW_Timer_GetTick();  // 记录起始时刻
        beep_state = BEEP_RUNNING;      // 进入鸣响状态
    }
}

/**
 * @brief 蜂鸣器1秒鸣响状态机处理函数
 * @note 需要在主循环中周期性调用此函数
 */
void beep_1s_process(void) {
    if (beep_state == BEEP_RUNNING) {
        if (MW_Timer_IsTimeout(beep_start_tick, 1000)) {
            beep_off();             // 关闭蜂鸣器
            beep_state = BEEP_IDLE; // 返回空闲状态
        }
    }
}