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
#include "stm32f0xx_hal.h"

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
#define BLUE_LED_Pin GPIO_PIN_13
#define BLUE_LED_GPIO_Port GPIOC
#define F2_Pin GPIO_PIN_0
#define F2_GPIO_Port GPIOA
#define F1_Pin GPIO_PIN_1
#define F1_GPIO_Port GPIOA
#define GAS_VALVE_Pin GPIO_PIN_2
#define GAS_VALVE_GPIO_Port GPIOA
#define GAS_IND_Pin GPIO_PIN_3
#define GAS_IND_GPIO_Port GPIOA
#define PETROL_IND_Pin GPIO_PIN_4
#define PETROL_IND_GPIO_Port GPIOA
#define FLASHER_L_Pin GPIO_PIN_5
#define FLASHER_L_GPIO_Port GPIOA
#define BATTERY_Pin GPIO_PIN_6
#define BATTERY_GPIO_Port GPIOA
#define ENGINE_RPM_Pin GPIO_PIN_15
#define ENGINE_RPM_GPIO_Port GPIOB
#define SW_HORN_Pin GPIO_PIN_10
#define SW_HORN_GPIO_Port GPIOA
#define SW_GAS_Pin GPIO_PIN_15
#define SW_GAS_GPIO_Port GPIOA
#define SW_HAZARD_Pin GPIO_PIN_3
#define SW_HAZARD_GPIO_Port GPIOB
#define SW_FLASHER_L_Pin GPIO_PIN_4
#define SW_FLASHER_L_GPIO_Port GPIOB
#define SW_FLASHER_R_Pin GPIO_PIN_5
#define SW_FLASHER_R_GPIO_Port GPIOB
#define HORN_Pin GPIO_PIN_6
#define HORN_GPIO_Port GPIOB
#define FLASHER_R_Pin GPIO_PIN_7
#define FLASHER_R_GPIO_Port GPIOB
#define F4_Pin GPIO_PIN_8
#define F4_GPIO_Port GPIOB
#define F3_Pin GPIO_PIN_9
#define F3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
