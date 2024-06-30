#include "Pomodoro.h"
#include <Arduino.h>

Pomodoro::Pomodoro(int lessonTime, int breakTime, int lessonCount)
    : lessonTime(lessonTime), breakTime(breakTime), lessonCount(lessonCount), currentLesson(0), previousMillis(0), interval(0), isBreak(false), active(false) {}

void Pomodoro::start()
{
    currentLesson = 0;
    isBreak = false;
    active = true;
    interval = lessonTime * 60000;
    previousMillis = millis();
}

void Pomodoro::update()
{
    if (!active)
        return;

    unsigned long currentMillis = millis();
    unsigned long elapsedTime = currentMillis - previousMillis;

    if (elapsedTime >= interval)
    {
        // Add buzzer logic here if needed

        if (isBreak)
        {
            isBreak = false;
            currentLesson++;
            if (currentLesson >= lessonCount)
            {
                active = false;
                return;
            }
            interval = lessonTime * 60000;
        }
        else
        {
            isBreak = true;
            interval = breakTime * 60000;
        }
        previousMillis = currentMillis;
    }
}

bool Pomodoro::isBreakTime()
{
    return isBreak;
}

int Pomodoro::getMinutesRemaining()
{
    unsigned long remainingTime = interval - (millis() - previousMillis);
    return remainingTime / 60000;
}

int Pomodoro::getSecondsRemaining()
{
    unsigned long remainingTime = interval - (millis() - previousMillis);
    return (remainingTime % 60000) / 1000;
}

void Pomodoro::resetDefaults()
{
    lessonTime = 25; // Default lesson time
    breakTime = 5;   // Default break time
    lessonCount = 4; // Default lesson count
    active = false;
}
