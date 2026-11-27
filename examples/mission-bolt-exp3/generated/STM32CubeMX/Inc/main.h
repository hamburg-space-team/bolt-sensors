/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u0xx_ll_adc.h"
#include "stm32u0xx_ll_comp.h"
#include "stm32u0xx_ll_exti.h"
#include "stm32u0xx_ll_dac.h"
#include "stm32u0xx_ll_dma.h"
#include "stm32u0xx.h"
#include "stm32u0xx_ll_i2c.h"
#include "stm32u0xx_ll_pwr.h"
#include "stm32u0xx_ll_crs.h"
#include "stm32u0xx_ll_rcc.h"
#include "stm32u0xx_ll_bus.h"
#include "stm32u0xx_ll_system.h"
#include "stm32u0xx_ll_cortex.h"
#include "stm32u0xx_ll_utils.h"
#include "stm32u0xx_ll_tim.h"
#include "stm32u0xx_ll_usart.h"
#include "stm32u0xx_ll_gpio.h"

#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif /* USE_FULL_ASSERT */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OSC_IN_Pin LL_GPIO_PIN_0
#define OSC_IN_GPIO_Port GPIOF
#define OSC_EN_Pin LL_GPIO_PIN_1
#define OSC_EN_GPIO_Port GPIOF
#define PD_IN_Pin LL_GPIO_PIN_1
#define PD_IN_GPIO_Port GPIOA
#define LIFI_TX_Pin LL_GPIO_PIN_2
#define LIFI_TX_GPIO_Port GPIOA
#define DAC_THR_Pin LL_GPIO_PIN_4
#define DAC_THR_GPIO_Port GPIOA
#define DBG_REF_Pin LL_GPIO_PIN_10
#define DBG_REF_GPIO_Port GPIOB
#define TX_EDGE_MAX_Pin LL_GPIO_PIN_11
#define TX_EDGE_MAX_GPIO_Port GPIOB
#define STATE_LED_Pin LL_GPIO_PIN_12
#define STATE_LED_GPIO_Port GPIOB
#define WD_LED_Pin LL_GPIO_PIN_13
#define WD_LED_GPIO_Port GPIOB
#define OPA_EN_Pin LL_GPIO_PIN_8
#define OPA_EN_GPIO_Port GPIOA
#define HOST_TX_Pin LL_GPIO_PIN_9
#define HOST_TX_GPIO_Port GPIOA
#define HOST_RX_Pin LL_GPIO_PIN_10
#define HOST_RX_GPIO_Port GPIOA
#define EDGE_Pin LL_GPIO_PIN_11
#define EDGE_GPIO_Port GPIOA
#define SWDIO_Pin LL_GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin LL_GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define LIFI_RX_Pin LL_GPIO_PIN_15
#define LIFI_RX_GPIO_Port GPIOA
#define PPS_Pin LL_GPIO_PIN_3
#define PPS_GPIO_Port GPIOB
#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority,
                                                                 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority,
                                                                 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority,
                                                                 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority,
                                                                 1 bit  for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority,
                                                                 0 bit  for subpriority */
#endif

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
