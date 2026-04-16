#include "actuators.h"
#include "firebase.h"
#include "sensors.h"
#include <Arduino.h>
#include <time.h>

bool      s_light = false;
bool      s_buzz  = false;
FanState  fanState = FAN_OFF;
LightMode lightMode = LIGHT_MODE_AUTO;
// float     fanTempThreshold = 30.0f;

// Get current time in minutes (HH * 60 + MM)
int getNowMinutes() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return -1;
    return timeinfo.tm_hour * 60 + timeinfo.tm_min;
}

// Convert time string "HH:MM" to minutes
int timeStringToMinutes(const String& timeStr) {
    if (timeStr.length() != 5 || timeStr.charAt(2) != ':') return -1;
    int hour   = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60)
        return hour * 60 + minute;
    return -1;
}

// Initialize actuator pins
void actuatorsInit() {
    pinMode(PIN_LIGHT,  OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_FAN,    OUTPUT);
    digitalWrite(PIN_LIGHT,  LOW);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_FAN,    LOW);
}

// Set light state
void setLight(bool on) {
    s_light = on;
    digitalWrite(PIN_LIGHT, on ? HIGH : LOW);
}

// Set buzzer state
void setBuzzer(bool on) {
    s_buzz = on;
    digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

// Set fan mode (OFF / MANUAL / AUTO)
void fanSetMode(FanState st) {
    fanState = st;
}

// Main state machine update for actuators
void actuatorsStateMachineUpdate(unsigned long nowSec) {

    // UC3 / UC4: Automatic light control based on schedule
    if (lightMode == LIGHT_MODE_AUTO) {
        int nowMins = getNowMinutes();
        int onMins  = timeStringToMinutes(lightOnTime);
        int offMins = timeStringToMinutes(lightOffTime);

        if (nowMins != -1 && onMins != -1 && offMins != -1) {
            bool shouldOn;

            // Normal case: ON time < OFF time (same day)
            if (onMins < offMins)
                shouldOn = (nowMins >= onMins && nowMins < offMins);
            else
                // Overnight case (crosses midnight)
                shouldOn = (nowMins >= onMins || nowMins < offMins);

            if (shouldOn != s_light) {
                setLight(shouldOn);
                Serial.printf("[LIGHT] AUTO → %s  (now=%d, schedule=%s-%s)\n",
                              shouldOn ? "ON" : "OFF", nowMins,
                              lightOnTime.c_str(), lightOffTime.c_str());
            }
        } else {
            if (nowMins == -1)
                Serial.println("[LIGHT] AUTO: NTP not ready");
            else
                Serial.printf("[LIGHT] AUTO: invalid schedule (on=%d off=%d)\n", onMins, offMins);
        }
    } else {
        // Manual light control
        digitalWrite(PIN_LIGHT, s_light ? HIGH : LOW);
    }

    // UC5: Fan control
    switch (fanState) {
        case FAN_OFF:
            digitalWrite(PIN_FAN, LOW);
            break;

        case FAN_MANUAL:
            digitalWrite(PIN_FAN, HIGH);
            break;

        case FAN_AUTO:
            // Turn on fan if temperature exceeds threshold
            if (!isnan(v_temp))
                digitalWrite(PIN_FAN, (v_temp > fanTempThreshold) ? HIGH : LOW);
            break;
    }
}