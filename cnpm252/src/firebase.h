#pragma once
#include <Arduino.h>

extern String g_idToken;
extern String g_localId;
extern String DEVICE_NODE;

extern String lightOnTime;
extern String lightOffTime;
extern float  fanTempThreshold;
extern int    phoneAlertDelaySec;
extern int    phoneDockThreshold;
extern int    pomoWorkSec;
extern int    pomoBreakSec;

extern String gmailWebhookUrl;

bool firebaseAnonSignIn();
bool firebasePatchAll();        
void firebasePullActuators();   
void firebasePullConfig();      
void firebaseSendDoorAlert();   
