#define SERIAL_SPEED     115200  // serial baud rate


#include "wifi-secrets.h"
#include "influx-db-secrets.h"
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("weather");


#include <Wire.h>
#include <INA3221.h>

#define PRINT_DEC_POINTS 3       // decimal points to print


//INA3221_ADDR40_GND - you can setup a diffrentet addresses base on left side pins - def is x40
INA3221 ina_0(INA3221_ADDR40_GND);


#include <DHT.h>
#define DHT_SENSOR_PIN  18 // ESP32 pin GIOP21 connected to DHT22 sensor
#define UP_PIN  17 // power up all bits
#define DHT_SENSOR_TYPE DHT22
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

void setup() {
  Serial.begin(SERIAL_SPEED);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  //power sensors 
  //pinMode(4, OUTPUT);
  //digitalWrite(4, HIGH);
  pinMode(UP_PIN, OUTPUT);
  digitalWrite(UP_PIN, HIGH);

  dht_sensor.begin(); 
  
  ina_0.begin(&Wire);
  ina_0.reset();
  ina_0.setShuntRes(100, 100, 100);
  

  // Add tags
  //sensor.addTag("device", DEVICE);
  //sensor.addTag("SSID", WiFi.SSID());

  // Alternatively, set insecure connection to skip server certificate validation
  //client.setInsecure();

  // Accurate time is necessary for certificate validation and writing in batches
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {


  // read humidity
  float humi  = dht_sensor.readHumidity();
  // read temperature in Celsius
  float tempC = dht_sensor.readTemperature();

  //solar
  float a1 = ina_0.getCurrent(INA3221_CH1) * 1000;
  float v1 = ina_0.getVoltage(INA3221_CH1);
  //esp 32
  float a2 = ina_0.getCurrent(INA3221_CH2) * 1000;
  float v2 = ina_0.getVoltage(INA3221_CH2);
  //battery 
  float a3 = ina_0.getCurrent(INA3221_CH3) * 1000;
  float v3 = ina_0.getVoltage(INA3221_CH3);
  
  
  // Store measured value into point
  sensor.clearFields();
  
  // Store values
  
  sensor.addField("temperature", tempC);
  sensor.addField("humidity", humi);
  
  sensor.addField("solar-current", a1);
  sensor.addField("solar-voltage", v1);

  sensor.addField("esp32-current", a2);
  sensor.addField("esp32-voltage", v2);

  sensor.addField("battery-current", a3);
  sensor.addField("battery-voltage", v3);    
  
  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  //Wait 10s
  Serial.println("Wait 10s");
  delay(10000);
}
