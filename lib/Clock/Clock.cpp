#include "Clock.h"

Clock::Clock(int clkPin, int datPin, int rstPin)
    : myRTC(clkPin, datPin, rstPin)
{
    myRTC.updateTime();
}

void Clock::updateTime()
{
    myRTC.updateTime();
}

void Clock::getDate(char *buffer)
{
    sprintf(buffer, "%02d.%02d.%04d", myRTC.dayofmonth, myRTC.month, myRTC.year);
}

void Clock::getTime(char *buffer)
{
    sprintf(buffer, "%02d:%02d:%02d", myRTC.hours, myRTC.minutes, myRTC.seconds);
}
