/*
    Batt
*/

#include "stm32drv.h"
#include "batt.h"


#define BATT_PIN_INFO       GPIOB, GPIO_PIN_12
#define BATT_PIN_CHRG_CUR   GPIOB, GPIO_PIN_13
#define BATT_PIN_CHRG_IND   GPIOB, GPIO_PIN_14
#define BATT_PIN_CHRG_HI    GPIOB, GPIO_PIN_15

extern ADC_HandleTypeDef hadc1;

uint32_t analogRead()
{
  uint32_t ADCValue = 0;
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK) {
      ADCValue = HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);
  
  return ADCValue;
}


namespace batt {

    uint16_t raw() {
        return analogRead();
    }

    uint8_t val05() {
        auto r = raw();
        return
            r > 3880 ? 5 :
            r > 3670 ? 4 :
            r > 3550 ? 3 :
            r > 3430 ? 2 :
            r > 3310 ? 1 :
            0;
    }

    chrg_t charge() {
        if (HAL_GPIO_ReadPin(BATT_PIN_CHRG_IND) == GPIO_PIN_RESET)
            return CHRG_OFF;
        
        if (HAL_GPIO_ReadPin(BATT_PIN_CHRG_HI) == GPIO_PIN_SET)
            return CHRG_HI;

        return CHRG_STD;
    }

} // namespace batt 


