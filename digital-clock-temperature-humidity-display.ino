#include <LiquidCrystal.h>
#include <virtuabotixRTC.h>
#include <DHT.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Bounce2.h>  // Bounce2 kütüphanesini dahil ediyoruz

#define DHTPIN 8       // DHT11 sensörünün bağlı olduğu pin
#define DHTTYPE DHT11  // DHT11 kullanıyoruz
#define LDR_PIN A3     // LDR sensörünün bağlı olduğu analog pin
#define PIR_PIN 2     // PIR sensörünün bağlı olduğu analog pin

const int CLK_PIN = 11;
const int DAT_PIN = 12;
const int RST_PIN = 13;
const int rs = 10, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
const int button1Pin = A0;  // Analog pin A0 buton 1 olarak kullanılıyor
const int button2Pin = A1;  // Analog pin A1 buton 2 olarak kullanılıyor
const int buzzerPin = A2;   // Analog pin A2 buzzer olarak kullanılıyor
const int backlightPin = 9; // PWM pin for LCD backlight control

DHT dht(DHTPIN, DHTTYPE);
virtuabotixRTC myRTC(CLK_PIN, DAT_PIN, RST_PIN);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Derece sembolü için özel karakter dizisi
byte degreeSymbol[8] = {
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

// Pomodoro varsayılan değerleri
const int defaultLessonTime = 3;
const int defaultBreakTime = 1;
const int defaultLessonCount = 2;

int mode = 0; // 0: clock, 1: pomodoro
int settingMode = 0; // 0: lesson time, 1: break time, 2: lesson count
int lessonTime = defaultLessonTime; // in minutes
int breakTime = defaultBreakTime; // in minutes
int lessonCount = defaultLessonCount; // number of lessons
int currentLesson = 0;
unsigned long previousMillis = 0;
unsigned long interval = 0; // in milliseconds
unsigned long lastTempHumUpdate = 0;
unsigned long lastClockUpdate = 0;

bool isBreak = false; // Track whether it's break or lesson time
bool pomodoroActive = false; // Track whether Pomodoro is active

char prevDateBuffer[11];
char prevTimeBuffer[6];
char prevTempBuffer[6];
char prevHumBuffer[4];

bool isSleep = false; // Uyku modunda mı
volatile bool motionDetected = false;
static unsigned long lastActivityTime = millis();

unsigned long button1PressTime = 0; // Buton 1 basılı kalma süresi

// Bounce kütüphanesi için buton nesneleri
Bounce button1 = Bounce(); 
Bounce button2 = Bounce();

void setupProcess() {
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(backlightPin, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  // Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.createChar(0, degreeSymbol); // Derece sembolünü özel karakter olarak tanımla
  dht.begin();
  // Başlangıçta birkaç kez sensörden veri okuyarak güvenilir sonuç al
  for (int i = 0; i < 3; i++) {
    updateTempHum(true);
    delay(100);
  }
  displayClock();  // Display initial clock data

  // Bounce kütüphanesi için butonları başlat
  button1.attach(button1Pin);
  button1.interval(50); // Debounce süresi 50 ms
  button2.attach(button2Pin);
  button2.interval(50); // Debounce süresi 50 ms
}

void setup() {
  setupProcess();
}

void loop() {
  interrupts();    // enable interrupts for Due and Nano V3

  unsigned long currentMillis = millis();

  // Bounce kütüphanesi ile butonların durumunu güncelle
  button1.update();
  button2.update();

  // Read LDR value and adjust backlight brightness
  int ldrValue = analogRead(LDR_PIN);  // LDR sensöründen gelen değeri okur
  int brightness = map(ldrValue, 0, 300, 0, 75);  // LDR değerini 0-75 aralığına dönüştürür
  analogWrite(backlightPin, brightness);  // LCD'nin arka ışık parlaklığını ayarlar

  // Button 1 press logic
  if (button1.read() == LOW) {
    if (button1PressTime == 0) {
      button1PressTime = currentMillis;
    } else if (currentMillis - button1PressTime > 1000) {
      // Long press detected for button 1
      if (mode == 1) {
        resetPomodoroDefaults();
      }
      button1PressTime = 0; // Prevent multiple triggers
    }
  } else {
    button1PressTime = 0;
  }

  if (button1.fell()) {
    if (mode == 0) {
      mode = 1;
      settingMode = 0;
      lcd.clear();
    } else {
      settingMode = (settingMode + 1) % 3;
      if (settingMode == 0) {
        mode = 0;
        startPomodoro();
        lcd.clear();
      }
    }
  }

  if (button2.fell()) {
    if (mode == 1) {
      switch (settingMode) {
        case 0:
          lessonTime++;
          if (lessonTime > 40) lessonTime = 1;
          break;
        case 1:
          breakTime++;
          if (breakTime > 20) breakTime = 1;
          break;
        case 2:
          lessonCount++;
          if (lessonCount > 3) lessonCount = 1;
          break;
      }
    }
  }

  if (mode == 0) {
    if (currentMillis - lastClockUpdate >= 1000) {
      updateClockDate(false);
      lastClockUpdate = currentMillis;
    }
    if (currentMillis - lastTempHumUpdate >= 300000) {
      updateTempHum(false);
      lastTempHumUpdate = currentMillis;
    }
  } else {
    displaySettings();
  }

  if (pomodoroActive) {
    updatePomodoro();
  }

  // Check for inactivity to enter sleep mode
  int activity = digitalRead(PIR_PIN); //Sensörden okuma yapıyoruz.
  if (!isSleep && activity == HIGH) {
    lastActivityTime = currentMillis; // Reset activity time after waking up
  }
  if (currentMillis - lastActivityTime > 60000) { // 1 minute of inactivity
    enterSleep();
    lastActivityTime = currentMillis; // Reset activity time after waking up
  }
}

void displayClock() {
  lcd.clear();
  updateClockDate(true);
  updateTempHum(true);
}

void updateClockDate(bool fullUpdate) {
  myRTC.updateTime();

  char dateBuffer[11];
  char timeBuffer[9];

  sprintf(dateBuffer, "%02d.%02d.%04d", myRTC.dayofmonth, myRTC.month, myRTC.year);
  sprintf(timeBuffer, "%02d:%02d:%02d", myRTC.hours, myRTC.minutes, myRTC.seconds);

  for (int i = 0; i < 10; i++) {
    if (fullUpdate || dateBuffer[i] != prevDateBuffer[i]) {
      lcd.setCursor(i, 0);
      lcd.print(dateBuffer[i]);
      prevDateBuffer[i] = dateBuffer[i];
    }
  }

  for (int i = 0; i < 5; i++) {
    if (fullUpdate || timeBuffer[i] != prevTimeBuffer[i]) {
      lcd.setCursor(i, 1);
      lcd.print(timeBuffer[i]);
      prevTimeBuffer[i] = timeBuffer[i];
    }
  }
}

void updateTempHum(bool fullUpdate) {
  int temperature = dht.readTemperature();
  int humidity = dht.readHumidity();

  char tempBuffer[4];
  char humBuffer[4];

  sprintf(tempBuffer, "%2d", temperature); // Derece sembolü olmadan sıcaklığı yazdırma
  sprintf(humBuffer, "%%%d", humidity); // "%" for humidity

  for (int i = 0; i < 2; i++) {
    if (fullUpdate || tempBuffer[i] != prevTempBuffer[i]) {
      lcd.setCursor(12 + i, 0);
      lcd.print(tempBuffer[i]);
      prevTempBuffer[i] = tempBuffer[i];
    }
  }
  // Derece sembolünü ekle
  lcd.setCursor(12 + strlen(tempBuffer), 0);
  lcd.write(byte(0));
  lcd.setCursor(12 + strlen(tempBuffer) + 1, 0);
  lcd.write("C");

  for (int i = 0; i < 3; i++) {
    if (fullUpdate || humBuffer[i] != prevHumBuffer[i]) {
      lcd.setCursor(13 + i, 1);
      lcd.print(humBuffer[i]);
      prevHumBuffer[i] = humBuffer[i];
    }
  }
}

void displaySettings() {
  lcd.clear();
  switch (settingMode) {
    case 0:
      lcd.print("Ders suresi:");
      lcd.setCursor(0, 1);
      lcd.print(lessonTime);
      lcd.print(" dakika");
      break;
    case 1:
      lcd.print("Teneffus suresi:");
      lcd.setCursor(0, 1);
      lcd.print(breakTime);
      lcd.print(" dakika");
      break;
    case 2:
      lcd.print("Ders sayisi:");
      lcd.setCursor(0, 1);
      lcd.print(lessonCount);
      break;
  }
}

void startPomodoro() {
  currentLesson = 0;
  isBreak = false;
  pomodoroActive = true;
  interval = lessonTime * 60000; // Ders süresi
  previousMillis = millis();
}

void updatePomodoro() {
  unsigned long currentMillis = millis();
  unsigned long elapsedTime = currentMillis - previousMillis;

  if (elapsedTime >= interval) {
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);

    if (isBreak) {
      isBreak = false;
      currentLesson++;
      if (currentLesson >= lessonCount) {
        pomodoroActive = false;
        return;
      }
      interval = lessonTime * 60000; // Yeni ders süresi
    } else {
      isBreak = true;
      interval = breakTime * 60000; // Yeni teneffüs süresi
    }

    previousMillis = currentMillis;
  }

  unsigned long remainingTime = interval - elapsedTime;
  int minutes = remainingTime / 60000;
  int seconds = (remainingTime % 60000) / 1000;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(isBreak ? "Teneffus:" : "Ders:");
  lcd.setCursor(0, 1);
  lcd.print(minutes);
  lcd.print(":");
  lcd.print(seconds < 10 ? "0" : "");
  lcd.print(seconds);
}

void resetPomodoroDefaults() {
  lessonTime = defaultLessonTime;
  breakTime = defaultBreakTime;
  lessonCount = defaultLessonCount;
  pomodoroActive = false;
  mode = 0;
  displayClock();
}

void wakeUp() {
  isSleep = false;
}

void enterSleep() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // Disable ADC to save power
  ADCSRA &= ~(1 << ADEN);
  sleep_bod_disable();
  sleep_enable();
  
  isSleep = true;
  // Allow wake up pin to trigger interrupt on low.
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), wakeUp, CHANGE);
  setupProcess();

  sleep_mode();
  // The processor will continue here after waking up
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(PIR_PIN));
}
