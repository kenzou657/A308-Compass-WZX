#ifndef DRV_BUZZER_H_
#define DRV_BUZZER_H_

void beep_on(void);
void beep_off(void);

/**
 * @brief 启动蜂鸣器鸣响1秒（非阻塞式）
 * @note 调用此函数后，需要在主循环中周期性调用 beep_1s_process()
 */
void beep_1s_start(void);

/**
 * @brief 蜂鸣器1秒鸣响状态机处理函数
 * @note 需要在主循环中周期性调用此函数
 */
void beep_1s_process(void);

#endif /* DRV_BUZZER_H_ */
