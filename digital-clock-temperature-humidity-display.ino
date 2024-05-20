#include <LiquidCrystal.h>
#include <virtuabotixRTC.h>
#include <DHT.h>

const int CLK_PIN = 11;
const int DAT_PIN = 12;
const int RST_PIN = 13;
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;

#define DHTPIN 8     // DHT11 sensörünün bağlı olduğu pin
#define DHTTYPE DHT11   // DHT11 kullanıyoruz

int temperature, humidity;
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

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.createChar(0, degreeSymbol); // Derece sembolünü özel karakter olarak tanımla
  dht.begin();
  // myRTC.setDS1302Time(30, 27, 22, 1, 20, 5, 2024);
  // analogWrite(A0,100);// Adjust Contrast VO
  // analogWrite(8,120);// Turn on Backlight
}

void loop() {
  lcd.clear();

  // Saat işlemleri 
  myRTC.updateTime();

  // İki haneli format için string kullanımı
  char dateBuffer[11];
  char timeBuffer[9];

  sprintf(dateBuffer, "%02d.%02d.%04d", myRTC.dayofmonth, myRTC.month, myRTC.year);
  sprintf(timeBuffer, "%02d:%02d:%02d", myRTC.hours, myRTC.minutes, myRTC.seconds);

  Serial.print(dateBuffer);
  Serial.print(" ");
  Serial.println(timeBuffer);
  
  lcd.setCursor(0, 0);
  lcd.print(dateBuffer);
  lcd.setCursor(0, 1);
  lcd.print(timeBuffer);


  // Sıcaklik ve Nem işlemleri
  int temperature = dht.readTemperature();
  int humidity = dht.readHumidity();

  lcd.setCursor(12, 0);
  lcd.print(temperature);
  lcd.write(byte(0));
  lcd.print("C");
  
  lcd.setCursor(13, 1);
  lcd.print("%");
  lcd.print(humidity);

  Serial.print("Sıcaklık: ");
  Serial.print(temperature);
  Serial.print("°C");
  Serial.print(" - ");
  Serial.print("Nem: %");
  Serial.println(humidity);

  delay(1000);
}
