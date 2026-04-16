#pragma once
#include <Arduino.h>
#include "sensors.h"

enum LightMode {
    LIGHT_MODE_MANUAL = 0,
    LIGHT_MODE_AUTO   = 1    
};

// (UC5) 
enum FanState {
    FAN_OFF    = 0,
    FAN_MANUAL,
    FAN_AUTO              
};

extern bool      s_light;
extern bool      s_buzz;
extern FanState  fanState;
extern LightMode lightMode;
extern float     fanTempThreshold;  

extern String lightOnTime;
extern String lightOffTime;

void actuatorsInit();
void setLight(bool on);
void setBuzzer(bool on);
void fanSetMode(FanState st);
void actuatorsStateMachineUpdate(unsigned long nowSec);
int  getNowMinutes();
int  timeStringToMinutes(const String& timeStr);
