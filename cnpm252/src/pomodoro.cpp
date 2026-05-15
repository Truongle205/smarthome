#include "pomodoro.h"
#include "actuators.h"
#include <Arduino.h>

PomodoroState pomoState = POMO_IDLE;

static int s_countdown = 0;
static int s_buzzSecLeft = 0;

static void beepForSeconds(int seconds) {
    s_buzzSecLeft = seconds;
    setBuzzer(true);
}

void pomodoroStart() {
    pomoState = POMO_WORK;
    s_countdown = pomoWorkSec;
    s_buzzSecLeft = 0;
    setBuzzer(false);
    Serial.printf("[POMO] Started: %d min work\n", pomoWorkSec / 60);
}

void pomodoroStop() {
    pomoState = POMO_IDLE;
    s_countdown = 0;
    s_buzzSecLeft = 0;
    setBuzzer(false);
    Serial.println("[POMO] Stopped");
}

void pomodoroUpdate() {
    if (s_buzzSecLeft > 0) {
        s_buzzSecLeft--;
        if (s_buzzSecLeft == 0) setBuzzer(false);
    }

    if (pomoState == POMO_IDLE) return;

    if (s_countdown > 0) {
        s_countdown--;
        return;
    }

    if (pomoState == POMO_WORK) {
        pomoState = POMO_BREAK;
        s_countdown = pomoBreakSec;
        beepForSeconds(2);
        Serial.println("[POMO] Work done - break time");
    } else {
        pomoState = POMO_WORK;
        s_countdown = pomoWorkSec;
        beepForSeconds(2);
        Serial.println("[POMO] Break done - back to work");
    }
}

void pomodoroGetTimeLeft(int& minLeft, int& secLeft) {
    minLeft = s_countdown / 60;
    secLeft = s_countdown % 60;
}

bool pomodoroIsWork() {
    return (pomoState == POMO_WORK);
}
