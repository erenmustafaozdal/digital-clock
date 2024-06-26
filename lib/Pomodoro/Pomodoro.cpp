#include "Pomodoro.h"
#include "Display.h"
#include <Arduino.h>

Pomodoro::Pomodoro(int lessonTime, int breakTime, int lessonCount)
    : lessonTime(lessonTime),
      breakTime(breakTime),
      lessonCount(lessonCount),
      currentLesson(0),
      previousMillis(0),
      interval(0),
      isBreak(false),
      active(false)
{
}

char *Pomodoro::timeToStr(int time)
{
    char *buffer = new char[10];       // time değerini saklamak için bir buffer oluştur
    sprintf(buffer, "%d", lessonTime); // time'ı string'e dönüştür
    return buffer;
}

void Pomodoro::start()
{
    Serial.println("Pomodoro start...");
    currentLesson = 0;
    isBreak = false;
    active = true;
    interval = lessonTime * 60000;
    previousMillis = millis();
}

void Pomodoro::update(Display display)
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

    unsigned long remainingTime = interval - elapsedTime;
    int minutes = remainingTime / 60000;
    int seconds = (remainingTime % 60000) / 1000;
    char *buffer;
    int lenBuffer;

    display.clear();
    display.print(0, 0, isBreak ? "Mola:" : "Odaklan:");
    buffer = this->timeToStr(minutes);
    display.print(0, 1, buffer);
    lenBuffer = strlen(buffer);

    display.print(lenBuffer, 1, ":");

    buffer = this->timeToStr(seconds);
    lenBuffer = lenBuffer + strlen(buffer);
    display.print(lenBuffer, 1, seconds < 10 ? "0" : "");
    display.print(seconds < 10 ? lenBuffer + 1 : lenBuffer, 1, buffer);
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
