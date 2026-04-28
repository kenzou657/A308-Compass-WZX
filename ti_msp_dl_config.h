/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
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

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     80000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2500
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for PWM_MOTOR */
#define PWM_MOTOR_INST                                                     TIMG0
#define PWM_MOTOR_INST_IRQHandler                               TIMG0_IRQHandler
#define PWM_MOTOR_INST_INT_IRQN                                 (TIMG0_INT_IRQn)
#define PWM_MOTOR_INST_CLK_FREQ                                         40000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_MOTOR_C0_PORT                                             GPIOA
#define GPIO_PWM_MOTOR_C0_PIN                                     DL_GPIO_PIN_12
#define GPIO_PWM_MOTOR_C0_IOMUX                                  (IOMUX_PINCM34)
#define GPIO_PWM_MOTOR_C0_IOMUX_FUNC                 IOMUX_PINCM34_PF_TIMG0_CCP0
#define GPIO_PWM_MOTOR_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_MOTOR_C1_PORT                                             GPIOA
#define GPIO_PWM_MOTOR_C1_PIN                                     DL_GPIO_PIN_13
#define GPIO_PWM_MOTOR_C1_IOMUX                                  (IOMUX_PINCM35)
#define GPIO_PWM_MOTOR_C1_IOMUX_FUNC                 IOMUX_PINCM35_PF_TIMG0_CCP1
#define GPIO_PWM_MOTOR_C1_IDX                                DL_TIMER_CC_1_INDEX



/* Defines for ENCODER1A */
#define ENCODER1A_INST                                                   (TIMG7)
#define ENCODER1A_INST_IRQHandler                               TIMG7_IRQHandler
#define ENCODER1A_INST_INT_IRQN                                 (TIMG7_INT_IRQn)
#define ENCODER1A_INST_LOAD_VALUE                                        (9999U)
/* GPIO defines for channel 0 */
#define GPIO_ENCODER1A_C0_PORT                                             GPIOA
#define GPIO_ENCODER1A_C0_PIN                                     DL_GPIO_PIN_23
#define GPIO_ENCODER1A_C0_IOMUX                                  (IOMUX_PINCM53)
#define GPIO_ENCODER1A_C0_IOMUX_FUNC                 IOMUX_PINCM53_PF_TIMG7_CCP0

/* Defines for ENCODER2A */
#define ENCODER2A_INST                                                   (TIMG6)
#define ENCODER2A_INST_IRQHandler                               TIMG6_IRQHandler
#define ENCODER2A_INST_INT_IRQN                                 (TIMG6_INT_IRQn)
#define ENCODER2A_INST_LOAD_VALUE                                        (9999U)
/* GPIO defines for channel 0 */
#define GPIO_ENCODER2A_C0_PORT                                             GPIOA
#define GPIO_ENCODER2A_C0_PIN                                     DL_GPIO_PIN_21
#define GPIO_ENCODER2A_C0_IOMUX                                  (IOMUX_PINCM46)
#define GPIO_ENCODER2A_C0_IOMUX_FUNC                 IOMUX_PINCM46_PF_TIMG6_CCP0





/* Defines for CLOCK */
#define CLOCK_INST                                                       (TIMA0)
#define CLOCK_INST_IRQHandler                                   TIMA0_IRQHandler
#define CLOCK_INST_INT_IRQN                                     (TIMA0_INT_IRQn)
#define CLOCK_INST_LOAD_VALUE                                             (999U)




/* Defines for I2C_OLED */
#define I2C_OLED_INST                                                       I2C1
#define I2C_OLED_INST_IRQHandler                                 I2C1_IRQHandler
#define I2C_OLED_INST_INT_IRQN                                     I2C1_INT_IRQn
#define I2C_OLED_BUS_SPEED_HZ                                             100000
#define GPIO_I2C_OLED_SDA_PORT                                             GPIOA
#define GPIO_I2C_OLED_SDA_PIN                                     DL_GPIO_PIN_30
#define GPIO_I2C_OLED_IOMUX_SDA                                   (IOMUX_PINCM5)
#define GPIO_I2C_OLED_IOMUX_SDA_FUNC                    IOMUX_PINCM5_PF_I2C1_SDA
#define GPIO_I2C_OLED_SCL_PORT                                             GPIOA
#define GPIO_I2C_OLED_SCL_PIN                                     DL_GPIO_PIN_29
#define GPIO_I2C_OLED_IOMUX_SCL                                   (IOMUX_PINCM4)
#define GPIO_I2C_OLED_IOMUX_SCL_FUNC                    IOMUX_PINCM4_PF_I2C1_SCL


/* Defines for UART_CAM */
#define UART_CAM_INST                                                      UART0
#define UART_CAM_INST_FREQUENCY                                          4000000
#define UART_CAM_INST_IRQHandler                                UART0_IRQHandler
#define UART_CAM_INST_INT_IRQN                                    UART0_INT_IRQn
#define GPIO_UART_CAM_RX_PORT                                              GPIOA
#define GPIO_UART_CAM_TX_PORT                                              GPIOA
#define GPIO_UART_CAM_RX_PIN                                       DL_GPIO_PIN_1
#define GPIO_UART_CAM_TX_PIN                                       DL_GPIO_PIN_0
#define GPIO_UART_CAM_IOMUX_RX                                    (IOMUX_PINCM2)
#define GPIO_UART_CAM_IOMUX_TX                                    (IOMUX_PINCM1)
#define GPIO_UART_CAM_IOMUX_RX_FUNC                     IOMUX_PINCM2_PF_UART0_RX
#define GPIO_UART_CAM_IOMUX_TX_FUNC                     IOMUX_PINCM1_PF_UART0_TX
#define UART_CAM_BAUD_RATE                                                (9600)
#define UART_CAM_IBRD_4_MHZ_9600_BAUD                                       (26)
#define UART_CAM_FBRD_4_MHZ_9600_BAUD                                        (3)
/* Defines for UART_IMU */
#define UART_IMU_INST                                                      UART1
#define UART_IMU_INST_FREQUENCY                                          4000000
#define UART_IMU_INST_IRQHandler                                UART1_IRQHandler
#define UART_IMU_INST_INT_IRQN                                    UART1_INT_IRQn
#define GPIO_UART_IMU_RX_PORT                                              GPIOA
#define GPIO_UART_IMU_TX_PORT                                              GPIOA
#define GPIO_UART_IMU_RX_PIN                                      DL_GPIO_PIN_18
#define GPIO_UART_IMU_TX_PIN                                      DL_GPIO_PIN_17
#define GPIO_UART_IMU_IOMUX_RX                                   (IOMUX_PINCM40)
#define GPIO_UART_IMU_IOMUX_TX                                   (IOMUX_PINCM39)
#define GPIO_UART_IMU_IOMUX_RX_FUNC                    IOMUX_PINCM40_PF_UART1_RX
#define GPIO_UART_IMU_IOMUX_TX_FUNC                    IOMUX_PINCM39_PF_UART1_TX
#define UART_IMU_BAUD_RATE                                              (115200)
#define UART_IMU_IBRD_4_MHZ_115200_BAUD                                      (2)
#define UART_IMU_FBRD_4_MHZ_115200_BAUD                                     (11)





/* Defines for DMA_UART0_RX */
#define DMA_UART0_RX_CHAN_ID                                                 (1)
#define UART_CAM_INST_DMA_TRIGGER                            (DMA_UART0_RX_TRIG)
/* Defines for DMA_IMU_RX */
#define DMA_IMU_RX_CHAN_ID                                                   (0)
#define UART_IMU_INST_DMA_TRIGGER                            (DMA_UART1_RX_TRIG)


/* Port definition for Pin Group GPIO_BEEP */
#define GPIO_BEEP_PORT                                                   (GPIOA)

/* Defines for USER_BEEP: GPIOA.27 with pinCMx 60 on package pin 31 */
#define GPIO_BEEP_USER_BEEP_PIN                                 (DL_GPIO_PIN_27)
#define GPIO_BEEP_USER_BEEP_IOMUX                                (IOMUX_PINCM60)
/* Port definition for Pin Group GPIO_LEDS */
#define GPIO_LEDS_PORT                                                   (GPIOB)

/* Defines for USER_LED_G: GPIOB.27 with pinCMx 58 on package pin 29 */
#define GPIO_LEDS_USER_LED_G_PIN                                (DL_GPIO_PIN_27)
#define GPIO_LEDS_USER_LED_G_IOMUX                               (IOMUX_PINCM58)
/* Port definition for Pin Group GPIO_MOTOR_DIR */
#define GPIO_MOTOR_DIR_PORT                                              (GPIOA)

/* Defines for MOTOR_AIN1: GPIOA.2 with pinCMx 7 on package pin 42 */
#define GPIO_MOTOR_DIR_MOTOR_AIN1_PIN                            (DL_GPIO_PIN_2)
#define GPIO_MOTOR_DIR_MOTOR_AIN1_IOMUX                           (IOMUX_PINCM7)
/* Defines for MOTOR_AIN2: GPIOA.3 with pinCMx 8 on package pin 43 */
#define GPIO_MOTOR_DIR_MOTOR_AIN2_PIN                            (DL_GPIO_PIN_3)
#define GPIO_MOTOR_DIR_MOTOR_AIN2_IOMUX                           (IOMUX_PINCM8)
/* Defines for MOTOR_BIN1: GPIOA.4 with pinCMx 9 on package pin 44 */
#define GPIO_MOTOR_DIR_MOTOR_BIN1_PIN                            (DL_GPIO_PIN_4)
#define GPIO_MOTOR_DIR_MOTOR_BIN1_IOMUX                           (IOMUX_PINCM9)
/* Defines for MOTOR_BIN2: GPIOA.5 with pinCMx 10 on package pin 45 */
#define GPIO_MOTOR_DIR_MOTOR_BIN2_PIN                            (DL_GPIO_PIN_5)
#define GPIO_MOTOR_DIR_MOTOR_BIN2_IOMUX                          (IOMUX_PINCM10)
/* Port definition for Pin Group GPIO_ENCODER */
#define GPIO_ENCODER_PORT                                                (GPIOA)

/* Defines for ENCODER1B: GPIOA.24 with pinCMx 54 on package pin 25 */
#define GPIO_ENCODER_ENCODER1B_PIN                              (DL_GPIO_PIN_24)
#define GPIO_ENCODER_ENCODER1B_IOMUX                             (IOMUX_PINCM54)
/* Defines for ENCODER2B: GPIOA.22 with pinCMx 47 on package pin 18 */
#define GPIO_ENCODER_ENCODER2B_PIN                              (DL_GPIO_PIN_22)
#define GPIO_ENCODER_ENCODER2B_IOMUX                             (IOMUX_PINCM47)




/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_PWM_MOTOR_init(void);
void SYSCFG_DL_ENCODER1A_init(void);
void SYSCFG_DL_ENCODER2A_init(void);
void SYSCFG_DL_CLOCK_init(void);
void SYSCFG_DL_I2C_OLED_init(void);
void SYSCFG_DL_UART_CAM_init(void);
void SYSCFG_DL_UART_IMU_init(void);
void SYSCFG_DL_DMA_init(void);

void SYSCFG_DL_SYSTICK_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
