#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <LoRa.h>
#include "DHT.h"
#include <ArduinoJson.h>   // JSON library

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
DHT dht(DHTPIN, DHTTYPE);
RTC_DATA_ATTR int bootCount = 0;
int TX_POWER = 20;


void setup() {
  // Disable Wi-Fi
  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  // delay(1000); // Wait for console to connect
  Serial.println("Starting");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  
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

  DynamicJsonDocument doc(1024);
  doc["Humidity"] = h;
  doc["Temperature"] = t;
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
