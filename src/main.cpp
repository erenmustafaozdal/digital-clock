#include <Arduino.h>
#include "Clock.h"
#include "Sensors.h"
#include "Display.h"
#include "Pomodoro.h"

// Function prototypes
void displayLCD();
void displaySettings();
void updateClockDate();
void updateTempHum();
void handleButtons();
void resetToDefaults();

// Pin definitions
#define DHTPIN 8      // DHT11 sensörünün bağlı olduğu pin
#define DHTTYPE DHT11 // DHT11 kullanıyoruz
#define LDR_PIN A0    // LDR sensörünün bağlı olduğu analog pin
#define CLK_PIN 11
#define DAT_PIN 12
#define RST_PIN 13
#define RS 10
#define EN 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define BACKLIGHT_PIN 9 // PWM pin for LCD backlight control
#define BUTTON1_PIN A2  // Analog pin A2 buton 1 olarak kullanılıyor
#define BUTTON2_PIN A3  // Analog pin A3 buton 2 olarak kullanılıyor

// Pomodoro settings
const int defaultLessonTime = 25;
const int defaultBreakTime = 5;
const int defaultLessonCount = 4;

int mode = 0; // 0: clock, 1: settings, 2: pomodoro
int settingStep = 0;

// Instances of classes
Clock clock(CLK_PIN, DAT_PIN, RST_PIN);
Display display(RS, EN, D4, D5, D6, D7, BACKLIGHT_PIN);
Sensors sensors(DHTPIN, DHTTYPE, LDR_PIN);
Pomodoro pomodoro(defaultLessonTime, defaultBreakTime, defaultLessonCount);

void setup()
{
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  display.initialize();
  sensors.initialize();
  displayLCD();
}

void loop()
{
  handleButtons();

  int ldrValue = sensors.readLDR();

  // LDR değerine göre ekran parlaklığını ayarla (örnek olarak)
  int brightness = map(ldrValue, 0, 1023, 0, 255);
  analogWrite(9, brightness); // Pin 9'da LCD arka ışığını ayarlama

  if (mode == 0)
  {
    // Update clock display every second
    static unsigned long lastClockUpdate = 0;
    if (millis() - lastClockUpdate >= 1000)
    {
      updateClockDate();
      lastClockUpdate = millis();
    }

    // Update temperature and humidity every 5 minutes
    static unsigned long lastTempHumUpdate = 0;
    if (millis() - lastTempHumUpdate >= 300000)
    {
      updateTempHum();
      lastTempHumUpdate = millis();
    }
  }
  else if (mode == 1)
  {
    displaySettings();
  }
  else if (mode == 2)
  {
    pomodoro.update(display);
  }

  delay(100);
}

void handleButtons()
{
  static unsigned long lastDebounceTime1 = 0;
  static unsigned long lastDebounceTime2 = 0;
  static bool lastButtonState1 = HIGH;
  static bool lastButtonState2 = HIGH;

  bool reading1 = digitalRead(BUTTON1_PIN);
  bool reading2 = digitalRead(BUTTON2_PIN);

  if ((millis() - lastDebounceTime1) > 50)
  {
    if (reading1 == LOW && lastButtonState1 == HIGH)
    {
      lastDebounceTime1 = millis();
      Serial.println("1. butona basıldı.");
      if (mode == 0)
      {
        mode = 1; // Switch to settings mode
        settingStep = 0;
      }
      else if (mode == 1)
      {
        settingStep++;
        if (settingStep > 2)
        {
          mode = 2; // Start Pomodoro
          pomodoro.start();
        }
      }
    }
  }

  if ((millis() - lastDebounceTime2) > 50)
  {
    if (reading2 == LOW && lastButtonState2 == HIGH)
    {
      lastDebounceTime2 = millis();
      Serial.println("2. butona basıldı.");
      if (mode == 1)
      {
        if (settingStep == 0)
        {
          pomodoro.lessonTime++;
          if (pomodoro.lessonTime > 40)
            pomodoro.lessonTime = 1;
        }
        else if (settingStep == 1)
        {
          pomodoro.breakTime++;
          if (pomodoro.breakTime > 20)
            pomodoro.breakTime = 1;
        }
        else if (settingStep == 2)
        {
          pomodoro.lessonCount++;
          if (pomodoro.lessonCount > 5)
            pomodoro.lessonCount = 1;
        }
      }
    }
  }

  unsigned long button1PressTime = 0;
  const unsigned long longPressDuration = 3000; // 3 saniye

  if (reading1 == LOW && lastButtonState1 == HIGH)
  {
    button1PressTime = millis();
  }

  if (reading1 == HIGH && lastButtonState1 == LOW)
  {
    if (button1PressTime != 0 && millis() - button1PressTime >= longPressDuration)
    {
      if (mode != 0)
      {
        Serial.println("Resetleme işlemi.");
        resetToDefaults();
      }
    }
    button1PressTime = 0;
  }

  lastButtonState1 = reading1;
  lastButtonState2 = reading2;
}

void displaySettings()
{
  display.clear();

  if (settingStep == 0)
  {
    display.print(0, 0, "Odaklanma:");
    display.print(0, 1, pomodoro.lessonTime);
    display.print(2, 1, " dakika");
  }
  else if (settingStep == 1)
  {
    display.print(0, 0, "Mola:");
    display.print(0, 1, pomodoro.breakTime);
    display.print(2, 1, " dakika");
  }
  else if (settingStep == 2)
  {
    display.print(0, 0, pomodoro.lessonCount);
    display.print(1, 0, " defa");
    display.print(0, 1, "mola olacak.");
  }
}

void resetToDefaults()
{
  pomodoro.lessonTime = defaultLessonTime;
  pomodoro.breakTime = defaultBreakTime;
  pomodoro.lessonCount = defaultLessonCount;
  mode = 0;
  displayLCD();
}

void displayLCD()
{
  display.clear();
  updateClockDate();
  updateTempHum();
}

void updateClockDate()
{
  clock.updateTime();
  char dateBuffer[11];
  char timeBuffer[9];
  clock.getDate(dateBuffer);
  clock.getTime(timeBuffer);
  display.print(0, 0, dateBuffer);
  display.print(0, 1, timeBuffer);
}

void updateTempHum()
{
  int temperature = sensors.readTemperature();
  int humidity = sensors.readHumidity();

  char tempBuffer[4];                      // Adjust buffer size as needed
  sprintf(tempBuffer, "%2d", temperature); // Derece sembolü olmadan sıcaklığı yazdırma
  display.temp(12, 0, tempBuffer);

  char humBuffer[3];                    // Adjust buffer size as needed
  sprintf(humBuffer, "%%%d", humidity); // "%" for humidity
  display.print(13, 1, humBuffer);
}
