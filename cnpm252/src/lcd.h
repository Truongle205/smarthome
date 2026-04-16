#pragma once
#include <Arduino.h>

void lcdInit();
void lcdShowSensors();          // UC2 
void lcdShowPomodoro(int minLeft, int secLeft, bool isWork); // UC2/UC6
void lcdShowDoorAlert();        // UC7 
void lcdClear();
