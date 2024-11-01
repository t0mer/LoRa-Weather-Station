#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <LoRa.h>
#include "DHT.h"
#include <ArduinoJson.h>   // JSON library
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BMP085.h>

#define BAND 433E6 // Change this to your desired frequency band (433E6, 868E6, 915E6)
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define TIME_TO_SLEEP 120 * 1e6  // Sleep for 120 seconds
// Define the LED pin
#define LED_PIN 2
#define DHTPIN 4         // DHT11 data pin
#define DHTTYPE DHT22   // DHT 11
#define RAIN_SENSOR_DIGITAL_PIN 15
#define RAIN_SENSOR_ANALOG_PIN 13
DHT dht(DHTPIN, DHTTYPE);
RTC_DATA_ATTR int bootCount = 0;
int TX_POWER = 20;

Adafruit_BMP085 bmp;
BH1750 lightMeter;

void setup() {
  // Disable Wi-Fi
  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RAIN_SENSOR_DIGITAL_PIN, INPUT);
  Wire.begin();  // Initialize I2C bus
  
  Serial.println("Starting");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  // Initialize BH1750
  if (!lightMeter.begin()) {
    Serial.println("Error initializing BH1750!");
    while (1);
  }
  Serial.println("BH1750 initialized.");

  // Initialize BMP180
  if (!bmp.begin()) {
    Serial.println("Error initializing BMP180!");
    while (1);
  }
  Serial.println("BMP180 initialized.");



}

void loop(){
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setTxPower(TX_POWER);
  LoRa.setSpreadingFactor(9);
  dht.begin();
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  int rainDetected = digitalRead(RAIN_SENSOR_DIGITAL_PIN);
  int rainIntensity = analogRead(RAIN_SENSOR_ANALOG_PIN);
  Serial.print("Rain Intensity: ");
  Serial.println(rainIntensity);




  float lux = lightMeter.readLightLevel();
  float pressure = bmp.readPressure() / 100.0;  // Convert Pa to hPa (or mbar)


  DynamicJsonDocument doc(1024);
  doc["Humidity"] = h;
  doc["Temperature"] = t;
  doc["light"] = lux ; 
  doc["pressure"] = pressure;

  if (rainDetected == LOW) {
    doc["raining"] = "Raining";
  } else {
    doc["raining"] = "Not Raining";
  }  

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  digitalWrite(LED_PIN, HIGH);
  sendLoRaMessage(String(jsonBuffer));
  LoRa.end(); // Shutdown LoRa cleanly
  digitalWrite(LED_PIN, LOW);

  delay(120000);



//  goToSleep();
}

void sendLoRaMessage(String message) {
  Serial.print("Sending packet: ");
  Serial.println(message);
  
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
}

void goToSleep() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP);
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_light_sleep_start();
  
}


float dBmToMilliwatts(float dBm) {
  return pow(10, dBm / 10);
}
