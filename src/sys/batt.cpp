/*
    Batt
*/

#include "stm32drv.h"
#include "batt.h"
#include "power.h"


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
        // 4.2 = 3850
        // 4.1 = 3760
        // 4.0 = 3667
        // 3.9 = 3575
        // 3.8 = 3485
        // 3.7 = 3410
        // 3.6 = 3360
        // 3.5 = 3310
        // 3.4 = 3310 - уже явно ошибка
        // ниже - работает очень нестабильно
        // причём, проблемы только с дисплеем,
        // сам cpu работает примерно до 2.6-2.7в на акк
        // отключать надо где-то на 3350
        return
            r > 3780 ? 5 :
            r > 3680 ? 4 :
            r > 3590 ? 3 :
            r > 3500 ? 2 :
            r > 3400 ? 1 :
            0;
    }

    chrg_t charge() {
        if (HAL_GPIO_ReadPin(BATT_PIN_CHRG_IND) == GPIO_PIN_SET)
            return CHRG_OFF;
        
        if (HAL_GPIO_ReadPin(BATT_PIN_CHRG_HI) == GPIO_PIN_SET)
            return CHRG_HI;

        return CHRG_STD;
    }

    void tick(bool force) {
        static uint8_t _c = 0;
        _c++;
        if (!force && (_c < 10))
            return;
        _c = 0;

        bool chrg = HAL_GPIO_ReadPin(BATT_PIN_CHRG_IND) == GPIO_PIN_RESET;
        bool ledr = HAL_GPIO_ReadPin(LED_PIN_RED) == GPIO_PIN_SET;

        if (chrg != ledr)
            HAL_GPIO_WritePin(LED_PIN_RED, chrg ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

} // namespace batt 


