#include <Arduino.h>
#include "Clock.h"
#include "Sensors.h"
#include "Display.h"

// Pin definitions
#define DHTPIN 8      // DHT11 sensörünün bağlı olduğu pin
#define DHTTYPE DHT11 // DHT11 kullanıyoruz
#define LDR_PIN A3    // LDR sensörünün bağlı olduğu analog pin
#define CLK_PIN 11
#define DAT_PIN 12
#define RST_PIN 13
#define RS 10
#define EN 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7

// Pomodoro settings
const int defaultLessonTime = 25;
const int defaultBreakTime = 5;
const int defaultLessonCount = 4;

// Instances of classes
Clock clock(CLK_PIN, DAT_PIN, RST_PIN);
Display display(RS, EN, D4, D5, D6, D7);
Sensors sensors(DHTPIN, DHTTYPE, LDR_PIN);

void setup()
{
  Serial.begin(9600);
  display.initialize();
  sensors.initialize();
  // Initial display setup
  char dateBuffer[11];
  char timeBuffer[9];
  clock.getDate(dateBuffer);
  clock.getTime(timeBuffer);
  display.print(0, 0, dateBuffer);
  display.print(0, 1, timeBuffer);
}

void loop()
{
  int ldrValue = sensors.readLDR();

  // LDR değerine göre ekran parlaklığını ayarla (örnek olarak)
  int brightness = map(ldrValue, 0, 1023, 0, 255);
  analogWrite(9, brightness); // Pin 9'da LCD arka ışığını ayarlama

  // Update clock display every second
  static unsigned long lastClockUpdate = 0;
  if (millis() - lastClockUpdate >= 1000)
  {
    clock.updateTime();
    char dateBuffer[11];
    char timeBuffer[9];
    clock.getDate(dateBuffer);
    clock.getTime(timeBuffer);
    display.print(0, 0, dateBuffer);
    display.print(0, 1, timeBuffer);
    lastClockUpdate = millis();
  }

  // Update temperature and humidity every 5 minutes
  static unsigned long lastTempHumUpdate = 0;
  if (lastTempHumUpdate == 0 or millis() - lastTempHumUpdate >= 300000)
  {
    float temperature = sensors.readTemperature();
    float humidity = sensors.readHumidity();
    char tempBuffer[10];                    // Adjust buffer size as needed
    dtostrf(temperature, 2, 1, tempBuffer); // (değer, toplam genişlik, ondalık basamak sayısı, hedef buffer)
    strcat(tempBuffer, "C");                // Append degree symbol
    char humBuffer[10];                     // Adjust buffer size as needed
    dtostrf(humidity, 2, 0, humBuffer);     // (value, width, precision, buffer)
    strcat(humBuffer, "%");                 // Append percentage symbol
    display.print(12, 0, tempBuffer);
    display.print(12, 1, humBuffer);
    lastTempHumUpdate = millis();
  }
}