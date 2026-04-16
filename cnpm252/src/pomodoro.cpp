#include "pomodoro.h"
#include "actuators.h"
#include <Arduino.h>

PomodoroState pomoState   = POMO_IDLE;
//int pomoWorkSec  = 25 * 60;
//int pomoBreakSec = 5  * 60;

static int  s_countdown  = 0;
static bool s_buzzFired  = false;

void pomodoroStart() {
    pomoState   = POMO_WORK;
    s_countdown = pomoWorkSec;
    s_buzzFired = false;
    Serial.printf("[POMO] Started: %d min work\n", pomoWorkSec / 60);
}

void pomodoroStop() {
    pomoState = POMO_IDLE;
    setBuzzer(false);
    Serial.println("[POMO] Stopped");
}

void pomodoroUpdate() {
    if (pomoState == POMO_IDLE) return;

    if (s_countdown > 0) {
        s_countdown--;
        return;
    }
    if (pomoState == POMO_WORK) {
        pomoState   = POMO_BREAK;
        s_countdown = pomoBreakSec;
        setBuzzer(true);
        s_buzzFired = true;
        Serial.println("[POMO] Work done! Break time.");
    } else {
        pomoState   = POMO_WORK;
        s_countdown = pomoWorkSec;
        setBuzzer(true);
        s_buzzFired = true;
        Serial.println("[POMO] Break done! Back to work.");
    }
}

void pomodoroGetTimeLeft(int& minLeft, int& secLeft) {
    minLeft = s_countdown / 60;
    secLeft = s_countdown % 60;
}

bool pomodoroIsWork() {
    return (pomoState == POMO_WORK);
}
