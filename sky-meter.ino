
#include <Adafruit_BMP280.h>
#include <U8g2lib.h>
#include <stdio.h>

static Adafruit_BMP280 bmp(PA3);
static U8G2_ST75256_JLX19296_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ PA2, /* dc=*/ PA1, /* reset=*/ PA0);

#define PIN_BTN_1   PB9
#define PIN_BTN_2   PB6
#define PIN_BTN_3   PB4

void setup() {
    Serial.begin(115200);
    //delay(10000);

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

    attachInterrupt(
        digitalPinToInterrupt(PIN_BTN_1),
        [] () {
            Serial.println("BTN");
        },
        CHANGE
    );
}

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
    char s[20];

    u8g2.firstPage();
    do {
        snprintf(s, sizeof(s), "%d", 1*p);
        u8g2.setFont(u8g2_font_fub20_tn);
        u8g2.drawStr(0, 80, s);

        u8g2.setFont(u8g2_font_b10_b_t_japanese1);
        if (digitalRead(PIN_BTN_1) == LOW)
            u8g2.drawStr(0, 20, "btn-1");
        if (digitalRead(PIN_BTN_2) == LOW)
            u8g2.drawStr(64, 20, "btn-2");
        if (digitalRead(PIN_BTN_3) == LOW)
            u8g2.drawStr(128, 20, "btn-3");
    }  while( u8g2.nextPage() );

    delay(100);
}
