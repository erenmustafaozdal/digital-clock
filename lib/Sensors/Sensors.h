#ifndef SENSORS_H
#define SENSORS_H

#include <DHT.h>

class Sensors
{
public:
    Sensors(int dhtPin, int dhtType, int ldrPin);
    void initialize();
    float readTemperature();
    float readHumidity();
    int readLDR();

private:
    DHT _dht;
    int _ldrPin;
};

#endif
