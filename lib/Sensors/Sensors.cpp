#include "Sensors.h"
#include <Arduino.h>

Sensors::Sensors(int dhtPin, int dhtType, int ldrPin)
    : _dht(dhtPin, dhtType), _ldrPin(ldrPin) {}

void Sensors::initialize()
{
    _dht.begin();
    pinMode(_ldrPin, INPUT);
}

float Sensors::readTemperature()
{
    return _dht.readTemperature();
}

float Sensors::readHumidity()
{
    return _dht.readHumidity();
}

int Sensors::readLDR()
{
    return analogRead(_ldrPin);
}
