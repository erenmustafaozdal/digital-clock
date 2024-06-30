#ifndef DISPLAY_H
#define DISPLAY_H

#include <LiquidCrystal.h>

class Display
{
public:
    Display(int rs, int en, int d4, int d5, int d6, int d7);
    void initialize();
    void print(int col, int row, const char *message);
    void clear();

private:
    LiquidCrystal lcd;
};

#endif
