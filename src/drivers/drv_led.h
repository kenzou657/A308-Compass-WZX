#ifndef _BSP_LED_H_
#define _BSP_LED_H_

#include "ti_msp_dl_config.h"

// 封装底层接口，方便以后更换引脚
#define LEDG_ON()      DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_G_PIN)
#define LEDG_OFF()     DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_G_PIN)
#define LEDG_TOGGLE()  DL_GPIO_togglePins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_G_PIN)


#endif