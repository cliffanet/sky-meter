/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

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
#define led_red_Pin GPIO_PIN_0
#define led_red_GPIO_Port GPIOA
#define led_blue_Pin GPIO_PIN_1
#define led_blue_GPIO_Port GPIOA
#define sdcard_cd_Pin GPIO_PIN_2
#define sdcard_cd_GPIO_Port GPIOA
#define sdcard_en_Pin GPIO_PIN_3
#define sdcard_en_GPIO_Port GPIOA
#define sdcard_cs_Pin GPIO_PIN_4
#define sdcard_cs_GPIO_Port GPIOA
#define chg_cur_Pin GPIO_PIN_0
#define chg_cur_GPIO_Port GPIOB
#define batt_info_Pin GPIO_PIN_1
#define batt_info_GPIO_Port GPIOB
#define bmp280_cs_Pin GPIO_PIN_10
#define bmp280_cs_GPIO_Port GPIOB
#define chg_ind_Pin GPIO_PIN_12
#define chg_ind_GPIO_Port GPIOB
#define chg_hi_Pin GPIO_PIN_13
#define chg_hi_GPIO_Port GPIOB
#define display_en_Pin GPIO_PIN_15
#define display_en_GPIO_Port GPIOB
#define display_cs_Pin GPIO_PIN_8
#define display_cs_GPIO_Port GPIOA
#define display_rs_dc_Pin GPIO_PIN_9
#define display_rs_dc_GPIO_Port GPIOA
#define display_rst_Pin GPIO_PIN_10
#define display_rst_GPIO_Port GPIOA
#define display_light_Pin GPIO_PIN_15
#define display_light_GPIO_Port GPIOA
#define btn_dn_Pin GPIO_PIN_7
#define btn_dn_GPIO_Port GPIOB
#define btn_dn_EXTI_IRQn EXTI9_5_IRQn
#define btn_sel_Pin GPIO_PIN_8
#define btn_sel_GPIO_Port GPIOB
#define btn_sel_EXTI_IRQn EXTI9_5_IRQn
#define btn_up_Pin GPIO_PIN_9
#define btn_up_GPIO_Port GPIOB
#define btn_up_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
