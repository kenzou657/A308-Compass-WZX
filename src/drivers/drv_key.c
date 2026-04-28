/*
 * 按键驱动实现
 * 
 * 功能：
 * - 按键硬件抽象
 * - 按键消抖处理（20ms延迟）
 * - 按键状态检测（按下/释放事件）
 */

#include "drv_key.h"
#include "ti_msp_dl_config.h"
#include "timer.h"

/* ==================== 按键消抖配置 ==================== */

#define KEY_DEBOUNCE_TIME_MS    20      /* 消抖延迟时间：20ms */
#define KEY_SCAN_PERIOD_MS      10      /* 扫描周期：10ms（建议） */
#define KEY_DEBOUNCE_COUNT      (KEY_DEBOUNCE_TIME_MS / KEY_SCAN_PERIOD_MS)  /* 消抖计数：2次 */

/* ==================== 按键状态机定义 ==================== */

typedef enum {
    KEY_FSM_RELEASED,       /* 状态0：释放状态 */
    KEY_FSM_PRESS_DEBOUNCE, /* 状态1：按下消抖中 */
    KEY_FSM_PRESSED,        /* 状态2：按下状态 */
    KEY_FSM_RELEASE_DEBOUNCE, /* 状态3：释放消抖中 */
} KeyFSM_t;

/* ==================== 按键控制块 ==================== */

typedef struct {
    KeyFSM_t fsm_state;         /* 状态机状态 */
    uint8_t debounce_count;     /* 消抖计数器 */
    KeyState_t current_state;   /* 当前按键状态 */
    KeyEvent_t event;           /* 按键事件 */
    uint32_t last_scan_time;    /* 上次扫描时间 */
} KeyControl_t;

/* ==================== 全局变量 ==================== */

static KeyControl_t g_keys[KEY_ID_COUNT];

/* ==================== 按键GPIO映射 ==================== */

/* 读取按键GPIO电平（低电平有效） */
static inline bool Key_ReadGPIO(KeyID_t key_id)
{
    uint32_t pin_value;
    
    switch (key_id) {
        case KEY_ID_K1:
            pin_value = DL_GPIO_readPins(GPIO_KEYS_PORT, GPIO_KEYS_K1_PIN);
            break;
        case KEY_ID_K2:
            pin_value = DL_GPIO_readPins(GPIO_KEYS_PORT, GPIO_KEYS_K2_PIN);
            break;
        case KEY_ID_K3:
            pin_value = DL_GPIO_readPins(GPIO_KEYS_PORT, GPIO_KEYS_K3_PIN);
            break;
        default:
            return false;
    }
    
    /* 低电平有效，返回true表示按下 */
    return (pin_value == 0);
}

/* ==================== 按键驱动实现 ==================== */

void Key_Init(void)
{
    /* 初始化所有按键控制块 */
    for (uint8_t i = 0; i < KEY_ID_COUNT; i++) {
        g_keys[i].fsm_state = KEY_FSM_RELEASED;
        g_keys[i].debounce_count = 0;
        g_keys[i].current_state = KEY_STATE_RELEASED;
        g_keys[i].event = KEY_EVENT_NONE;
        g_keys[i].last_scan_time = 0;
    }
}

void Key_Scan(void)
{
    /* 遍历所有按键 */
    for (uint8_t i = 0; i < KEY_ID_COUNT; i++) {
        KeyControl_t *key = &g_keys[i];
        bool gpio_pressed = Key_ReadGPIO((KeyID_t)i);
        
        /* 状态机处理 */
        switch (key->fsm_state) {
            case KEY_FSM_RELEASED:
                /* 状态0：释放状态 */
                if (gpio_pressed) {
                    /* 检测到按下，进入消抖状态 */
                    key->fsm_state = KEY_FSM_PRESS_DEBOUNCE;
                    key->debounce_count = 0;
                }
                break;
                
            case KEY_FSM_PRESS_DEBOUNCE:
                /* 状态1：按下消抖中 */
                if (gpio_pressed) {
                    /* 持续按下，增加消抖计数 */
                    key->debounce_count++;
                    if (key->debounce_count >= KEY_DEBOUNCE_COUNT) {
                        /* 消抖完成，确认按下 */
                        key->fsm_state = KEY_FSM_PRESSED;
                        key->current_state = KEY_STATE_PRESSED;
                        key->event = KEY_EVENT_PRESSED;
                    }
                } else {
                    /* 抖动，返回释放状态 */
                    key->fsm_state = KEY_FSM_RELEASED;
                    key->debounce_count = 0;
                }
                break;
                
            case KEY_FSM_PRESSED:
                /* 状态2：按下状态 */
                if (!gpio_pressed) {
                    /* 检测到释放，进入消抖状态 */
                    key->fsm_state = KEY_FSM_RELEASE_DEBOUNCE;
                    key->debounce_count = 0;
                }
                break;
                
            case KEY_FSM_RELEASE_DEBOUNCE:
                /* 状态3：释放消抖中 */
                if (!gpio_pressed) {
                    /* 持续释放，增加消抖计数 */
                    key->debounce_count++;
                    if (key->debounce_count >= KEY_DEBOUNCE_COUNT) {
                        /* 消抖完成，确认释放 */
                        key->fsm_state = KEY_FSM_RELEASED;
                        key->current_state = KEY_STATE_RELEASED;
                        key->event = KEY_EVENT_RELEASED;
                    }
                } else {
                    /* 抖动，返回按下状态 */
                    key->fsm_state = KEY_FSM_PRESSED;
                    key->debounce_count = 0;
                }
                break;
                
            default:
                /* 异常状态，重置 */
                key->fsm_state = KEY_FSM_RELEASED;
                key->debounce_count = 0;
                break;
        }
    }
}

KeyState_t Key_GetState(KeyID_t key_id)
{
    if (key_id >= KEY_ID_COUNT) {
        return KEY_STATE_RELEASED;
    }
    
    return g_keys[key_id].current_state;
}

KeyEvent_t Key_GetEvent(KeyID_t key_id)
{
    if (key_id >= KEY_ID_COUNT) {
        return KEY_EVENT_NONE;
    }
    
    KeyEvent_t event = g_keys[key_id].event;
    
    /* 读取后自动清除事件 */
    g_keys[key_id].event = KEY_EVENT_NONE;
    
    return event;
}

void Key_ClearEvent(KeyID_t key_id)
{
    if (key_id < KEY_ID_COUNT) {
        g_keys[key_id].event = KEY_EVENT_NONE;
    }
}

void Key_ClearAllEvents(void)
{
    for (uint8_t i = 0; i < KEY_ID_COUNT; i++) {
        g_keys[i].event = KEY_EVENT_NONE;
    }
}
