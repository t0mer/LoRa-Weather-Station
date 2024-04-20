#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>  // MQTT library
#include <ArduinoJson.h>   // JSON library

#define BAND 433E6 // Change this to your desired frequency band (433E6, 868E6, 915E6)
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
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

  // while (!client.connected()) {
  //   Serial.print("Attempting MQTT connection...");
  //   int rc = client.connect("ESP32Client", mqtt_username, mqtt_password);
  //   if (rc == 0) {
  //     Serial.println("connected");
      
  //   } else {
  //     Serial.print("failed, rc=");
  //     Serial.println(client.state());
  //     Serial.println(" try again in 5 seconds");
  //     delay(5000);
  //   }
  // }
}


void setup() {
  Serial.begin(115200);
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
    DynamicJsonDocument lorainfo(1024);
    lorainfo["rssi"] = rssi;
    lorainfo["snr"] = snr;
    lorainfo["sf"] = sf;
    lorainfo["frequency"] = frequency;
    char jsonBuffer[512];
    serializeJson(lorainfo, jsonBuffer);
    client.publish(mqtt_lora_info, jsonBuffer);
    
    // Data
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      char mqttPayload[LoRaData.length() + 1];  // Create a character array with enough space
      LoRaData.toCharArray(mqttPayload, sizeof(mqttPayload)); // Convert String to character array
      client.publish(mqtt_lora_payload, mqttPayload); // Publish the character array
    }
  }
}
