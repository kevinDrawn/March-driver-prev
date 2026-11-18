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
#include "stm32h7xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TEMPERATURE_Pin GPIO_PIN_4
#define TEMPERATURE_GPIO_Port GPIOF
#define DRV_nSCS_Pin GPIO_PIN_0
#define DRV_nSCS_GPIO_Port GPIOC
#define ISENC_Pin GPIO_PIN_2
#define ISENC_GPIO_Port GPIOC
#define DRV_MOSI_Pin GPIO_PIN_3
#define DRV_MOSI_GPIO_Port GPIOC
#define ENC_CS_Pin GPIO_PIN_4
#define ENC_CS_GPIO_Port GPIOA
#define ENC_CLOCK_Pin GPIO_PIN_5
#define ENC_CLOCK_GPIO_Port GPIOA
#define ENC_MISO_Pin GPIO_PIN_6
#define ENC_MISO_GPIO_Port GPIOA
#define INLB_Pin GPIO_PIN_0
#define INLB_GPIO_Port GPIOB
#define INLC_Pin GPIO_PIN_1
#define INLC_GPIO_Port GPIOB
#define STLINK_MOSI_Pin GPIO_PIN_2
#define STLINK_MOSI_GPIO_Port GPIOB
#define ISENA_Pin GPIO_PIN_12
#define ISENA_GPIO_Port GPIOF
#define ISENB_Pin GPIO_PIN_14
#define ISENB_GPIO_Port GPIOF
#define INLA_Pin GPIO_PIN_8
#define INLA_GPIO_Port GPIOE
#define INHA_Pin GPIO_PIN_9
#define INHA_GPIO_Port GPIOE
#define INHB_Pin GPIO_PIN_11
#define INHB_GPIO_Port GPIOE
#define INHC_Pin GPIO_PIN_13
#define INHC_GPIO_Port GPIOE
#define DRV_DRVOFF_Pin GPIO_PIN_15
#define DRV_DRVOFF_GPIO_Port GPIOE
#define DRV_SCK_Pin GPIO_PIN_10
#define DRV_SCK_GPIO_Port GPIOB
#define DRV_MISO_Pin GPIO_PIN_14
#define DRV_MISO_GPIO_Port GPIOB
#define RESET_ETH_Pin GPIO_PIN_8
#define RESET_ETH_GPIO_Port GPIOD
#define BRAKE_PWM_Pin GPIO_PIN_6
#define BRAKE_PWM_GPIO_Port GPIOC
#define ENC_SYNC_Pin GPIO_PIN_9
#define ENC_SYNC_GPIO_Port GPIOC
#define STLINK_SCK_Pin GPIO_PIN_10
#define STLINK_SCK_GPIO_Port GPIOC
#define STLINK_MISO_Pin GPIO_PIN_11
#define STLINK_MISO_GPIO_Port GPIOC
#define STLINK_nSCS_Pin GPIO_PIN_0
#define STLINK_nSCS_GPIO_Port GPIOD
#define STLINK_NRST_Pin GPIO_PIN_1
#define STLINK_NRST_GPIO_Port GPIOD
#define ENC_MOSI_Pin GPIO_PIN_7
#define ENC_MOSI_GPIO_Port GPIOD
#define LED_GPIO_Pin GPIO_PIN_9
#define LED_GPIO_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
