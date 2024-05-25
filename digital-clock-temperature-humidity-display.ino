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
const int defaultLessonTime = 1;
const int defaultBreakTime = 1;
const int defaultLessonCount = 2;

int mode = 0; // 0: clock, 1: settings, 2: pomodoro
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
int prevMinutes = -1;
int prevSeconds = -1;
bool prevIsBreak = true;

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

#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

int lessonEndMelody[] = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
};

int lessonEndNoteDurations[] = {
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2
};

int breakEndMelody[] = {
  NOTE_G4, NOTE_A4, NOTE_G4, NOTE_E4, NOTE_F4, NOTE_E4, NOTE_D4
};

int breakEndNoteDurations[] = {
  4, 4, 4, 4, 4, 4, 2
};

void playLessonEndMelody() {
  for (int thisNote = 0; thisNote < sizeof(lessonEndMelody) / sizeof(lessonEndMelody[0]); thisNote++) {
    int noteDuration = 1000 / lessonEndNoteDurations[thisNote];
    tone(buzzerPin, lessonEndMelody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzerPin);
  }
}

void playBreakEndMelody() {
  for (int thisNote = 0; thisNote < sizeof(breakEndMelody) / sizeof(breakEndMelody[0]); thisNote++) {
    int noteDuration = 1000 / breakEndNoteDurations[thisNote];
    tone(buzzerPin, breakEndMelody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzerPin);
  }
}

void setupProcess() {
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(backlightPin, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  Serial.begin(9600);
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
      if (mode == 1 || mode == 2) {
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
        mode = 2;
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

  Serial.print("Mod: ");
  Serial.println(mode);
  if (mode == 0) {
    if (currentMillis - lastClockUpdate >= 1000) {
      updateClockDate(false);
      lastClockUpdate = currentMillis;
    }
    if (currentMillis - lastTempHumUpdate >= 300000) {
      updateTempHum(false);
      lastTempHumUpdate = currentMillis;
    }
  } else if (mode == 1) {
    displaySettings();
  } else {
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
  static int prevLessonTime = -1;
  static int prevBreakTime = -1;
  static int prevLessonCount = -1;
  static int prevSettingMode = -1;

  if (settingMode != prevSettingMode) {
    lcd.clear();
    prevSettingMode = settingMode;

    // Display the appropriate static text based on settingMode
    switch (settingMode) {
      case 0:
        lcd.setCursor(0, 0);
        lcd.print("Ders suresi:");
        break;
      case 1:
        lcd.setCursor(0, 0);
        lcd.print("Teneffus suresi:");
        break;
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("Ders sayisi:");
        break;
    }
  }

  // Display dynamic values
  switch (settingMode) {
    case 0:
      prevBreakTime = -1;
      prevLessonCount = -1;

      if (lessonTime != prevLessonTime) {
        lcd.setCursor(0, 1);
        lcd.print("                "); // 16 boşluk, LCD'nin ikinci satırını tamamen temizler
        lcd.setCursor(0, 1);
        lcd.print(lessonTime);
        lcd.print(" dakika");
        prevLessonTime = lessonTime;
      }
      break;
    case 1:
      prevLessonTime = -1;
      prevLessonCount = -1;

      if (breakTime != prevBreakTime) {
        lcd.setCursor(0, 1);
        lcd.print("                "); // 16 boşluk, LCD'nin ikinci satırını tamamen temizler
        lcd.setCursor(0, 1);
        lcd.print(breakTime);
        lcd.print(" dakika");
        prevBreakTime = breakTime;
      }
      break;
    case 2:
      prevLessonTime = -1;
      prevBreakTime = -1;
      
      if (lessonCount != prevLessonCount) {
        lcd.setCursor(0, 1);
        lcd.print("                "); // 16 boşluk, LCD'nin ikinci satırını tamamen temizler
        lcd.setCursor(0, 1);
        lcd.print(lessonCount);
        prevLessonCount = lessonCount;
      }
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
    if (isBreak) {
      playBreakEndMelody(); // Teneffüs sonu melodisi çal
      isBreak = false;
      currentLesson++;
      if (currentLesson >= lessonCount) {
        pomodoroActive = false;
        displayClock();
        mode = 0;
        return;
      }
      interval = lessonTime * 60000; // Yeni ders süresi
    } else {
      playLessonEndMelody(); // Ders sonu melodisi çal
      isBreak = true;
      interval = breakTime * 60000; // Yeni teneffüs süresi
    }

    previousMillis = currentMillis;
  }

  unsigned long remainingTime = interval - elapsedTime;
  int minutes = remainingTime / 60000;
  int seconds = (remainingTime % 60000) / 1000;

  // Only update the mode line if it has changed
  if (isBreak != prevIsBreak) {
    lcd.setCursor(0, 0);
    lcd.print(isBreak ? "Teneffus:" : String(currentLesson + 1) + ". Ders:");
    prevIsBreak = isBreak;
  }

  // Only update the minutes if they have changed
  Serial.print("prevMinutes: ");
  Serial.println(prevMinutes);
  if (minutes != prevMinutes) {
    lcd.setCursor(0, 1);
    if (minutes < 10) lcd.print('0');
    lcd.print(minutes);
    prevMinutes = minutes;
  }

  // Always update the colon and seconds since they change every second
  lcd.setCursor(2, 1); // Position for the colon
  lcd.print(':');
  lcd.setCursor(3, 1); // Position for the seconds
  if (seconds < 10) lcd.print('0');
  lcd.print(seconds);

  prevSeconds = seconds;
}

void resetPomodoroDefaults() {
  lessonTime = defaultLessonTime;
  breakTime = defaultBreakTime;
  lessonCount = defaultLessonCount;
  pomodoroActive = false;
  mode = 0;
  prevMinutes = -1;
  prevSeconds = -1;
  prevIsBreak = true;
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
