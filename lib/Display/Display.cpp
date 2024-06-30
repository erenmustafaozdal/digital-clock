#include "Display.h"

Display::Display(int rs, int en, int d4, int d5, int d6, int d7)
    : lcd(rs, en, d4, d5, d6, d7) {}

void Display::initialize()
{
    lcd.begin(16, 2);
}

void Display::print(int col, int row, const char *message)
{
    lcd.setCursor(col, row);
    lcd.print(message);
}

void Display::clear()
{
    lcd.clear();
}
