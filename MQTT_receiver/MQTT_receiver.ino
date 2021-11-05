#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_NeoPixel.h>
#include "credentials_CloudMQTT.h"



#define PIN D3
#define ESP8266  // comment if you want to use the sketch on a Arduino board
#define MQTT     // uncomment if you want to foreward the message via MQTT
//#define OLED     // comment if you do nto have a OLED display


const long freq = 868E6;
const int SF = 7;
const long bw = 125E3;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2, PIN, NEO_GRB + NEO_KHZ800);

#ifdef OLED
#include "SSD1306.h"
SSD1306  display(0x3c, 4, 5);
#endif

#ifdef MQTT
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
// HA1ADM MQTT Addr
//IPAddress server (192, 168, 104, 9);
// HG2EBH MQTT Addr
IPAddress server (192, 168, 22, 20);
#define TOPIC "Greenhouse"
int counter, lastCounter; 


#endif

#ifdef MQTT
WiFiClient wifiClient;
PubSubClient client(wifiClient);
#endif

void setup() {
    pixels.begin(); // This initializes the NeoPixel library.
    pixels.setPixelColor(0, pixels.Color(0 , 0 , 0));
    pixels.show();
    pixels.setPixelColor(1, pixels.Color(0 , 0 , 0));
    pixels.show();
  Serial.begin(9600);
  Serial.println("LoRa Receiver");
#ifdef ESP8266
  LoRa.setPins(16, 17, 15); // set CS, reset, IRQ pin
#else
  LoRa.setPins(10, A0, 2); // set CS, reset, IRQ pin
#endif

#ifdef OLED
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "Lora to MQTT");
  display.display();
#endif

 if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");

    while (1);
  }

  LoRa.setSpreadingFactor(SF);
   LoRa.setSignalBandwidth(bw);
  Serial.println("LoRa Started");

#ifdef MQTT
  WiFi.mode(WIFI_STA);
  WiFi.begin(mySSID, myPASSWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //The ESP8266 tries to reconnect automatically when the connection is lost
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  client.setServer(server, 1883);
  client.setCallback(callback);

  if (client.connect("", MQTT_USERNAME, MQTT_KEY)) {
    client.publish(TOPIC"/STAT", "Greenhouse live!");

  }
       else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  Serial.println("MQTT Started");
#endif

  Serial.print("Frequency ");
  Serial.print(freq);
  Serial.print(" Bandwidth ");
  Serial.print(bw);
  Serial.print(" SF ");
  Serial.println(SF);

}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    String message = "";
    while (LoRa.available()) {
      message = message + ((char)LoRa.read());
    }
    String rssi = "\"RSSI\":\"" + String(LoRa.packetRssi()) + "\"";
    String jsonString = message;
    jsonString.replace("xxx", rssi);

    int ii = jsonString.indexOf("Count", 1);
    String count = jsonString.substring(ii + 8, ii + 13);
    counter = count.toInt();
    if (counter - lastCounter == 0) Serial.println("Repetition");
    lastCounter = counter;


    sendAck(message);
    String value1 = jsonString.substring(7, 8);  // Node id
    String value2 = jsonString.substring(23, 28); //counter
#ifdef OLED
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    displayText(40, 0, value1, 3);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    displayText(120, 0, String(LoRa.packetRssi()), 3);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    displayText(60, 35, count, 5);
    display.display();
#endif

#ifdef MQTT
    if (!client.connected()) {
      reconnect();
    }
    client.publish(TOPIC"/MESSAGE", jsonString.c_str());
#endif
  }
#ifdef MQTT
  client.loop();
#endif
}

#ifdef MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Greenhouse")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(TOPIC"/STAT", "I am live again");
      // ... and resubscribe
      //  client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
#endif
void sendAck(String message) {
  int check = 0;
  for (int i = 0; i < message.length(); i++) {
    check += message[i];
  }
  // Serial.print("/// ");
  LoRa.beginPacket();
  LoRa.print(String(check));
  LoRa.endPacket();
  Serial.print(message);
  Serial.print(" ");
  Serial.print("Ack Sent: ");
  Serial.println(check);
}

#ifdef OLED
void displayText(int x, int y, String tex, byte font ) {
  switch (font) {
    case 1:
      display.setFont(ArialMT_Plain_10);
      break;
    case 2:
      display.setFont(ArialMT_Plain_16);
      break;
    case 3:
      display.setFont(ArialMT_Plain_24);
      break;
    default:
      display.setFont(ArialMT_Plain_16);
      break;
  }
  display.drawString(x, y, tex);
}
#endif
