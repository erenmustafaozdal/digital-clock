#ifndef POMODORO_H
#define POMODORO_H

class Pomodoro
{
public:
    Pomodoro(int lessonTime, int breakTime, int lessonCount);
    void start();
    void update();
    bool isBreakTime();
    int getMinutesRemaining();
    int getSecondsRemaining();
    void resetDefaults();

private:
    int lessonTime;
    int breakTime;
    int lessonCount;
    int currentLesson;
    unsigned long previousMillis;
    unsigned long interval;
    bool isBreak;
    bool active;
};

#endif
