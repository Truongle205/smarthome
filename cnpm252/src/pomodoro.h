#pragma once
#include <Arduino.h>

enum PomodoroState {
    POMO_IDLE = 0,
    POMO_WORK,
    POMO_BREAK
};

extern PomodoroState pomoState;
extern int pomoWorkSec;   
extern int pomoBreakSec;   

void pomodoroStart();
void pomodoroStop();
void pomodoroUpdate();          

void pomodoroGetTimeLeft(int& minLeft, int& secLeft);
bool pomodoroIsWork();
