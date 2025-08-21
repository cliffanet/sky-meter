/*
    Batt
*/

#include "stm32drv.h"
#include "batt.h"
#include "power.h"
#include "err.h"


#if HWVER < 2
#define BATT_PIN_INFO       GPIOB, GPIO_PIN_12
#define BATT_PIN_CHRG_CUR   GPIOB, GPIO_PIN_13
#define BATT_PIN_CHRG_IND   GPIOB, GPIO_PIN_14
#define BATT_PIN_CHRG_HI    GPIOB, GPIO_PIN_15
#else
#define BATT_PIN_INFO       GPIOB, GPIO_PIN_1
#define BATT_PIN_CHRG_CUR   GPIOB, GPIO_PIN_0
#define BATT_PIN_CHRG_IND   GPIOB, GPIO_PIN_12
#define BATT_PIN_CHRG_HI    GPIOB, GPIO_PIN_13
#endif // HWVER

extern ADC_HandleTypeDef hadc1;

uint32_t analogRead()
{
  uint32_t ADCValue = 0;
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK) {
      ADCValue = HAL_ADC_GetValue(&hadc1);
  }
  else
      err::add(0x81);
  HAL_ADC_Stop(&hadc1);
  
  return ADCValue;
}


namespace batt {

    uint16_t raw() {
        return analogRead();
    }

    uint8_t val05() {
        auto r = raw();
        // 4.3 = 3945
        // 4.2 = 3850
        // 4.1 = 3757
        // 4.0 = 3665
        // 3.9 = 3572
        // 3.8 = 3480
        // 3.7 = 3385
        // 3.6 = 3290
        // 3.5 = 3197
        // 3.4 = 3105
        // 3.35 = 3060
        // 3.3 = 3040
        // 3.2 = 3040 - уже явно ошибка
        // где-то на 3.33 останавливается снижение показаний
        // и ниже всегда: 3030-3040
        // ниже - работает очень нестабильно
        // причём, проблемы только с дисплеем,
        // дисплёй:
        // - 3.14-3.15 на акк: работает стабильно
        // - 3.12-3.13 на акк: артефакты
        // - 3.10-3.11 на акк: гарантированно отключается
        // сам cpu работает примерно до 2.6-2.7в на акк
        // отключать надо где-то на 3100
        return
            r > 3700 ? 5 :
            r > 3580 ? 4 :
            r > 3450 ? 3 :
            r > 3350 ? 2 :
            r > 3200 ? 1 :
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


