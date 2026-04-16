#pragma once
#include <Arduino.h>
#include <DHT.h>

// DHT11
#define PIN_DHT         4
#define DHTTYPE         DHT11

// ADC
#define PIN_LDR_ADC     1      // LDR room lux (UC1)
#define PIN_LDR_DOCK    3      // LDR phone dock (UC9)

#define PIN_TRIG        5
#define PIN_ECHO        7
#define DOOR_OPEN_CM    15    

#define PIN_LIGHT       48
#define PIN_BUZZER      10
#define PIN_FAN         18

#define SDA_PIN 8
#define SCL_PIN 9

extern float v_temp, v_humi;
extern int   v_lux;           
extern int   v_ldrDock;        
extern bool  v_doorOpen;       

void sensorsInit();
void readSensors();
float readDistanceCm();        
