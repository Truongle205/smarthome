#include "lcd.h"
#include "sensors.h"
#include <LiquidCrystal_I2C.h>

static LiquidCrystal_I2C lcd(0x27, 16, 2);

void lcdInit() {
    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  Smart Home    ");
    lcd.setCursor(0, 1);
    lcd.print("  Initializing..");
}

// UC2:
void lcdShowSensors() {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (!isnan(v_temp)) {
        lcd.printf("Temp: %.1f C    ", v_temp);
    } else {
        lcd.print("Temp: -- C      ");
    }
    lcd.setCursor(0, 1);
    if (!isnan(v_humi)) {
        lcd.printf("Humi: %.1f %%   ", v_humi);
    } else {
        lcd.print("Humi: -- %      ");
    }
}

void lcdShowPomodoro(int minLeft, int secLeft, bool isWork) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(isWork ? "  FOCUS TIME    " : "   BREAK TIME   ");
    lcd.setCursor(0, 1);
    char buf[17];
    snprintf(buf, sizeof(buf), "    %02d:%02d        ", minLeft, secLeft);
    lcd.print(buf);
}

void lcdShowDoorAlert() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("!! DOOR OPENED !!");
    lcd.setCursor(0, 1);
    lcd.print("Check your door ");
}

void lcdClear() {
    lcd.clear();
}
