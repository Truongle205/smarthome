#include "phone_dock.h"
#include "actuators.h"
#include "sensors.h"
#include "pomodoro.h"
#include <Arduino.h>

// UC9: During Pomodoro WORK phase, warn user if the phone/device is removed
// from the dock for longer than phoneAlertDelaySec.
static bool s_wasOnDock = false;
static int  s_liftedSec = 0;
static bool s_alertFired = false;

void phoneDockInit() {
    s_wasOnDock = false;
    s_liftedSec = 0;
    s_alertFired = false;
}

bool phoneIsOnDock() {
    return (v_ldrDock < phoneDockThreshold);
}

void phoneDockUpdate() {
    if (!pomodoroIsWork()) {
        s_liftedSec = 0;
        s_alertFired = false;
        s_wasOnDock = phoneIsOnDock();
        return;
    }

    const bool onDock = phoneIsOnDock();

    if (onDock) {
        if (s_alertFired) {
            setBuzzer(false);
            Serial.println("[DOCK] Phone/device returned - buzzer OFF");
        }
        s_wasOnDock = true;
        s_liftedSec = 0;
        s_alertFired = false;
        return;
    }

    // If the system started with no phone/device on dock, do not alert immediately.
    if (!s_wasOnDock && s_liftedSec == 0) {
        return;
    }

    s_liftedSec++;
    if (!s_alertFired && s_liftedSec >= phoneAlertDelaySec) {
        s_alertFired = true;
        setBuzzer(true);
        Serial.printf("[DOCK] ALERT: off dock for %d sec\n", s_liftedSec);
    }
}
