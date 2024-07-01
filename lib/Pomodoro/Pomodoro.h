#ifndef POMODORO_H
#define POMODORO_H

#include "Display.h"
class Pomodoro
{
public:
    Pomodoro(int lessonTime, int breakTime, int lessonCount);
    int lessonTime;
    int breakTime;
    int lessonCount;
    void settings(Display display);
    char *timeToStr(int time);
    void start();
    void update(Display display);
    bool isBreakTime();
    int getMinutesRemaining();
    int getSecondsRemaining();
    void resetDefaults();

private:
    int currentLesson;
    unsigned long previousMillis;
    unsigned long interval;
    bool isBreak;
    bool active;
};

#endif
