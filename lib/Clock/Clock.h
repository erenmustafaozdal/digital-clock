#ifndef CLOCK_H
#define CLOCK_H

#include <virtuabotixRTC.h>

class Clock
{
public:
    Clock(int clkPin, int datPin, int rstPin);
    void updateTime();
    void getDate(char *buffer);
    void getTime(char *buffer);

private:
    virtuabotixRTC myRTC;
};

#endif
