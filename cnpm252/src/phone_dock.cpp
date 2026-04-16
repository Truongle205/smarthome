#include "phone_dock.h"
#include "actuators.h"
#include "sensors.h"
#include <Arduino.h>

//int phoneDockThreshold  = 800;   
//int phoneAlertDelaySec  = 30;   

static bool  s_phoneWasOnDock  = false;   
static bool  s_alertArmed      = false;  
static int   s_liftedSec       = 0;      
static bool  s_alertFired      = false; 

void phoneDockInit() {
    s_phoneWasOnDock = false;
    s_alertArmed     = false;
    s_liftedSec      = 0;
    s_alertFired     = false;
}

bool phoneIsOnDock() {
    return (v_ldrDock < phoneDockThreshold);
}

void phoneDockUpdate() {
    bool onDock = phoneIsOnDock();

    if (onDock) {
        if (s_alertFired) {
            setBuzzer(false);
            Serial.println("[DOCK] Phone placed back – buzzer OFF");
        }
        s_alertArmed  = false;
        s_liftedSec   = 0;
        s_alertFired  = false;
        s_phoneWasOnDock = true;
        return;
    }

    if (!s_phoneWasOnDock) {
        return;
    }

    if (!s_alertArmed) {
        s_alertArmed = true;
        s_liftedSec  = 0;
        Serial.printf("[DOCK] Phone lifted – alert in %d sec\n", phoneAlertDelaySec);
    }

    s_liftedSec++;

    if (!s_alertFired && s_liftedSec >= phoneAlertDelaySec) {
        s_alertFired = true;
        setBuzzer(true);
        Serial.printf("[DOCK] ALERT! Phone gone for %d sec – BUZZER ON\n", s_liftedSec);
    }

    s_phoneWasOnDock = false;
}
