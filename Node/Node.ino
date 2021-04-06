// HG2EBH Lora Greenhouse Node

//#define SOIL_M   // Uncomment if you want to use a analog Soil moisture sensor on 

// One Wire libs
#include <OneWire.h>
#include <DS18B20.h>
// DHT Libs
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
// Lora Libs
#include <SPI.h>
#include <LoRa.h>
// LowPower
#include <LowPower.h>

#define DHTPIN A5 
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);
float dht_temp, dht_humi;

#define ONE_WIRE_BUS A4

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);
float DS_temp;

const int AI_1 = A3;  //Analog input pin for Battery Voltage measurement
const int AI_2 = A2;  //Analog input pin for soil moisture sensor

float batt_v = 0.0;
float temp=0.0;
float r1=180000.0;
float r2=100000.0;

// Lora constants
const int dev_id = 2;
const unsigned TX_INTERVAL_MINUTES = 1;
const unsigned TX_INTERVAL = 60 * TX_INTERVAL_MINUTES;

const long freq = 868E6;
const int SF = 7;
const long bw = 125E3;

int counter = 1, messageLostCounter = 0;
int temp1, temp2, temp3, temp4, soil_m;


void setup() {
  Serial.begin(9600);
  while (!Serial);
  // Initialize DHT device.
  dht.begin();
  // Initialize DS18B20 device.
  sensor.begin();

  LoRa.setPins(10, A0, 2); // set CS, reset, IRQ pin


  Serial.println("LoRa Sender");

  if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(SF);
  //  LoRa.setSignalBandwidth(bw);

  Serial.print("Frequency ");
  Serial.print(freq);
  Serial.print(" Bandwidth ");
  Serial.print(bw);
  Serial.print(" SF ");
  Serial.println(SF);

}

void loop() {
  char message[90];
  // Read DHT Sensor
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    dht_temp = 0.0;
  }
  else {
    dht_temp = event.temperature;
  }
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    dht_humi = 0.0;
  }
  else {
    dht_humi = event.relative_humidity;
  }
  // Read Battery Voltage
  temp = (analogRead(AI_1) * 3.3) / 1024.0; 
  batt_v = temp / (r2/(r1+r2));
  // Read DS18B20
  sensor.requestTemperatures();  
  while (!sensor.isConversionComplete());  // wait until sensor is ready
  DS_temp = sensor.getTempC();
  // Read Soil moisture sensor if present
  #ifdef SOIL_M
  soil_m = analogRead(AI_2);
  #endif

  temp1 = batt_v*100;
  temp2 = dht_temp*100;
  temp3 = dht_humi*100;
  temp4 = DS_temp*100;
  
  #ifdef SOIL_M
  sprintf(message, "{\"N#\":\"%1d\",\"Batt\":\"%3d\",\"Count\":\"%05d\",\"Lost\":\"%03d\",\"DHT_T\":\"%3d\",\"DHT_H\":\"%3d\",\"DS_T\":\"%3d\",\"SM\":\"%4d\",xxx}",dev_id, temp1, counter, messageLostCounter, temp2, temp3, temp4, soil_m);
  #else
  sprintf(message, "{\"N#\":\"%1d\",\"Batt\":\"%3d\",\"Count\":\"%05d\",\"Lost\":\"%03d\",\"DHT_T\":\"%3d\",\"DHT_H\":\"%3d\",\"DS_T\":\"%3d\",xxx}",dev_id, temp1, counter, messageLostCounter, temp2, temp3, temp4);
  #endif
  
  // Debug Sensor values
  //printDebug;
  // Send Message over LoRa
    sendMessage(message);
    
  int nackCounter = 0;
  while (!receiveAck(message) && nackCounter <= 5) {

    Serial.println(" refused ");
    Serial.print(nackCounter);
    LoRa.sleep();
    delay(1000);
    sendMessage(message);
    nackCounter++;
  }

  if (nackCounter >= 5) {
    Serial.println("");
    Serial.println("--------------- MESSAGE LOST ---------------------");
    messageLostCounter++;
    delay(100);
  } else {
    Serial.println("Acknowledged ");
  }
  counter++;

  Serial.println("Falling asleep");
  delay(100);
    for (int i=0; i<int((TX_INTERVAL/8)+dev_id); i++) {
       LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
            }
  delay(200);

}

bool receiveAck(String message) {
  String ack;
  Serial.print(" Waiting for Ack ");
  bool stat = false;
  unsigned long entry = millis();
  while (stat == false && millis() - entry < 2000) {
    if (LoRa.parsePacket()) {
      ack = "";
      while (LoRa.available()) {
        ack = ack + ((char)LoRa.read());
      }
      int check = 0;
      for (int i = 0; i < message.length(); i++) {
        check += message[i];
      }
      Serial.print(" Ack ");
      Serial.print(ack);
      Serial.print(" Check ");
      Serial.print(check);
      if (ack.toInt() == check) {
        Serial.print(" Checksum OK ");
        stat = true;
      }
    }
  }
  return stat;
}

void sendMessage(String message) {
  Serial.print(message);
  // send packet
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
}

void printDebug () {
  Serial.println(F("DHT22 Sensor Data"));
    Serial.print(F("Temperature: "));
    Serial.print(dht_temp);
    Serial.println(F("°C"));
    Serial.print(F("Humidity: "));
    Serial.print(dht_humi);
    Serial.println(F("%"));
    Serial.println(F("#####################"));
    Serial.println(F("Battery "));
    Serial.print(batt_v);
    Serial.println(F("V"));
    Serial.println(F("#####################"));
    Serial.println(F("DS18B20 "));
    Serial.print(F("Temp: "));
    Serial.print(DS_temp);
    Serial.println(F("°C"));
    Serial.println(F("#####################"));
    Serial.println(F("Soil moisture: "));
    Serial.print(soil_m);
    Serial.println(F("#####################"));    

}
