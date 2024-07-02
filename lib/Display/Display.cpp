#include "Display.h"
#include <Arduino.h>

// Derece sembolü için özel karakter dizisi
byte degreeSymbol[8] = {
    B00111,
    B00101,
    B00111,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000};

Display::Display(int rs, int en, int d4, int d5, int d6, int d7, int backlightPin)
    : lcd(rs, en, d4, d5, d6, d7), _backlightPin(backlightPin) {}

void Display::initialize()
{
    pinMode(_backlightPin, OUTPUT);
    lcd.begin(16, 2);
    lcd.createChar(0, degreeSymbol); // Derece sembolünü özel karakter olarak tanımla
}

void Display::print(int col, int row, const char *message)
{
    lcd.setCursor(col, row);
    lcd.print(message);
}

void Display::print(int col, int row, const String &message)
{
    lcd.setCursor(col, row);
    lcd.print(message);
}

void Display::print(int col, int row, int message)
{
    lcd.setCursor(col, row);
    lcd.print(message);
}

void Display::print(int col, int row, float message)
{
    lcd.setCursor(col, row);
    lcd.print(message);
}

void Display::temp(int col, int row, const char *message)
{
    lcd.setCursor(col, row);
    lcd.print(message);
    lcd.setCursor(col + strlen(message), row);
    lcd.write(byte(0)); // Derece simgesini yazdırma
    lcd.setCursor(col + strlen(message) + 1, row);
    lcd.print("C");
}

void Display::clear()
{
    lcd.clear();
}
