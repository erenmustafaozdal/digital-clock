#include "Sensors.h"
#include <Arduino.h>

Sensors::Sensors(int dhtPin, int dhtType, int ldrPin)
    : _dht(dhtPin, dhtType), _ldrPin(ldrPin) {}

void Sensors::initialize()
{
    _dht.begin();
    pinMode(_ldrPin, INPUT);
}

int Sensors::readTemperature()
{
    int temperature = _dht.readTemperature();

    Serial.print("Sıcaklık alındı: ");
    Serial.print(temperature);
    Serial.println("°C");

    return temperature;
}

int Sensors::readHumidity()
{
    int humidity = _dht.readTemperature();

    Serial.print("Nem alındı: %");
    Serial.println(humidity);

    return humidity;
}

int Sensors::readLDR()
{
    return analogRead(_ldrPin);
}
