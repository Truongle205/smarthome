#include "sensors.h"
#include <Arduino.h>
#include <DHT.h>

DHT dht(PIN_DHT, DHTTYPE);

float v_temp    = NAN;
float v_humi    = NAN;
int   v_lux     = 0;
int   v_ldrDock = 0;
bool  v_doorOpen = false;

// HC-SR04
float readDistanceCm() {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long duration = pulseIn(PIN_ECHO, HIGH, 30000); // timeout 30ms
    if (duration == 0) return -1.0f;                // timeout
    return duration * 0.0343f / 2.0f;
}

void sensorsInit() {
    dht.begin();
    analogReadResolution(12);

    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    digitalWrite(PIN_TRIG, LOW);
}

void readSensors() {
    // DHT11
    v_temp = dht.readTemperature();
    v_humi = dht.readHumidity();

    // LDR room lux (UC1)
    v_lux = analogRead(PIN_LDR_ADC);

    // LDR phone dock (UC9)
    v_ldrDock = analogRead(PIN_LDR_DOCK);

    // HC-SR04 (UC7)
    float dist = readDistanceCm();
    if (dist > 0) {
        v_doorOpen = (dist > DOOR_OPEN_CM);
    }
}
