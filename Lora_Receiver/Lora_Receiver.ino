#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>  // MQTT library
#include <ArduinoJson.h>   // JSON library
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BAND 433E6 // Change this to your desired frequency band (433E6, 868E6, 915E6)
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String LoRaData;

// WiFi credentials
const char* ssid = "";
const char* password = "";

// MQTT broker
const char* mqtt_server = "";
const int mqtt_port = 1883;
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* mqtt_lora_info = "station/radio";
const char* mqtt_lora_payload = "station/data";
const char* mqtt_clientid = "wstation";
const char* thingsboardServer = ""; // e.g., "demo.thingsboard.io"
const String accessToken = "";



WiFiClient wifiClient;
PubSubClient client(wifiClient);


void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void reconnect() {
  
 while (!client.connected()) {
     Serial.printf("The client %s connects to the public mqtt broker\n", mqtt_clientid);
     if (client.connect(mqtt_clientid, mqtt_username, mqtt_password)) {
         Serial.println("Public mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(5000);
     }
 }

}

void sendDataToThingsBoard(const String& payload) {
  HTTPClient http;
  
  String url = String(thingsboardServer) + "/api/v1/" + accessToken + "/telemetry";

  http.begin(url);

  http.addHeader("Content-Type", "application/json");

  // String payload = payload; // Example telemetry data

  int httpResponseCode = http.POST(payload);

  if(httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}


void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);


  Serial.println("LoRa Receiver Test");
  setup_wifi();
  client.setServer( mqtt_server, mqtt_port);
  reconnect();
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(9);
  LoRa.setGain(6);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("Received packet:");
    // RSSI
    int rssi = LoRa.packetRssi();
    // SNR
    float snr = LoRa.packetSnr();
    // Spreading Factor
    int sf = LoRa.getSpreadingFactor();
    // Frequency
    int frequency = LoRa.getFrequency();


    display.setTextColor(WHITE);
    display.setCursor(0, 20);
    display.println("RSSI: " + String(rssi) + "dbm");
    display.setCursor(0, 30);
    display.println("SNR: " + String(snr));
    display.display();

    DynamicJsonDocument lorainfo(1024);
    lorainfo["rssi"] = rssi;
    lorainfo["snr"] = snr;
    lorainfo["sf"] = sf;
    lorainfo["frequency"] = frequency;
    char radioinfo[512];
    serializeJson(lorainfo, radioinfo);
    client.publish(mqtt_lora_info, radioinfo);
    sendDataToThingsBoard(radioinfo);
    
    // Data
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      char mqttPayload[LoRaData.length() + 1];  // Create a character array with enough space
      LoRaData.toCharArray(mqttPayload, sizeof(mqttPayload)); // Convert String to character array
      client.publish(mqtt_lora_payload, mqttPayload); // Publish the character array
      sendDataToThingsBoard(mqttPayload);
    }
  }
}



