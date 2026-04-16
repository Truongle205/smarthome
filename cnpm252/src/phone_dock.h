#pragma once
#include <Arduino.h>

//UC9 Phone dock

extern int  phoneDockThreshold;  
extern int  phoneAlertDelaySec;   

void phoneDockInit();
void phoneDockUpdate();           
bool phoneIsOnDock();  