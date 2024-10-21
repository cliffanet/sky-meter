
#include <Adafruit_BMP280.h>
#include <U8g2lib.h>
#include <stdio.h>

static Adafruit_BMP280 bmp(PA3);
static U8G2_ST75256_JLX19296_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ PA2, /* dc=*/ PA1, /* reset=*/ PA0);

#define PIN_BTN_1   PC10
#define PIN_BTN_2   PB6
#define PIN_BTN_3   PB4

#define PIN_AN_1    PB15

void setup() {
    Serial.begin(115200);
    delay(5000);

    //HAL_InitTick(TICK_INT_PRIORITY);
    MX_RTC_Init();

    if (bmp.begin(BMP280_ADDRESS_ALT))
        Serial.println("BMP OK");
    else
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    
    if (u8g2.begin())
        Serial.println("Display OK");
    else
        Serial.println("Display ERROR");
    //u8g2.setPowerSave(false);
    //u8g2.setContrast(125);
    u8g2.clearDisplay();

    pinMode(PIN_BTN_1, INPUT_PULLUP);
    pinMode(PIN_BTN_2, INPUT_PULLUP);
    pinMode(PIN_BTN_3, INPUT_PULLUP);

    pinMode(PIN_AN_1, INPUT_ANALOG);

    attachInterrupt(
        digitalPinToInterrupt(PIN_BTN_1),
        [] () {
            Serial.println("BTN");
        },
        CHANGE
    );
}

static RTC_HandleTypeDef hrtc;

void loop() {
    /*
    digitalWrite(PC6, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(1000);                      // wait for a second
    digitalWrite(PC6, LOW);   // turn the LED off by making the voltage LOW
    delay(1000);                      // wait for a second
    static int n=0;
    Serial.print("test: ");
    Serial.println(++n);
    */

    int p = bmp.readPressure();
    char s[64];

    int a = analogRead(PIN_AN_1);
    auto tm = HAL_GetTick();
    
    //RTC_TimeTypeDef sTime;
    //RTC_DateTypeDef sDate;
    //HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    //HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    u8g2.firstPage();
    do {
        snprintf(s, sizeof(s), "%d", 1*p);
        u8g2.setFont(u8g2_font_fub20_tn);
        u8g2.drawStr(0, 80, s);

        snprintf(s, sizeof(s), "%d", a);
        u8g2.drawStr(110, 80, s);

        u8g2.setFont(u8g2_font_b10_b_t_japanese1);
        if (digitalRead(PIN_BTN_1) == LOW)
            u8g2.drawStr(0, 20, "btn-1");
        if (digitalRead(PIN_BTN_2) == LOW)
            u8g2.drawStr(64, 20, "btn-2");
        if (digitalRead(PIN_BTN_3) == LOW)
            u8g2.drawStr(128, 20, "btn-3");
        
        snprintf(s, sizeof(s), "%d", tm);
        u8g2.drawStr(10, 10, s);

        //snprintf(s, sizeof(s), "%02d.%02d.%02d / %02d.%02d.%02d",
        //    sDate.Date,     sDate.Month,    sDate.Year,
        //    sTime.Hours,    sTime.Minutes,  sTime.Seconds);
        //u8g2.drawStr(10, 40, s);
    }  while( u8g2.nextPage() );

    delay(100);
}

void _SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
#ifdef USBCON
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {};
#endif

    /* Configure the main internal regulator output voltage */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    /* Initializes the CPU, AHB and APB busses clocks */
    ///*
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 12;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    //*/
    /*
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
    RCC_OscInitStruct.PLL.PLLN = 85;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    /*
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                    | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    */

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }

#ifdef USBCON
    /* Initializes the peripherals clocks */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    //if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    //    Error_Handler();
    //}
#endif

}

static void MX_RTC_Init(void) {
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Serial.println("RTC init error");
        return;
    }
    
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    sTime.Hours = 0x10;
    sTime.Minutes = 0x20;
    sTime.Seconds = 0x30;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) {
        Serial.println("RTC SetTime error");
        return;
    }
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month = RTC_MONTH_MARCH;
    sDate.Date = 0x16;
    sDate.Year = 0x21;
    
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
        Serial.println("RTC SetDate error");
        return;
    }
    
    Serial.println("RTC OK");
}
