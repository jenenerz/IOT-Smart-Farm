/*
  Smart Agriculture Sensor Node - Sensor Data Logger
  - Reads LDR (Light) and DHT11 (Temperature/Humidity)
  - Outputs all sensor data to Serial Monitor
  - FIXED: Always reads all sensors and reports data
  - FIXED: Adjusted LDR threshold for proper calibration
  - FIXED: Better DHT11 error handling
  
  NOTE: WiFi transmission will be added after sensor verification
*/

#include <DHT.h>
#include <WiFiS3.h>

//  -------------------------------
// Node identity (unique per node)
// -------------------------------
const char SENSOR_ID[] = "NODE_01";
const char LOCATION_NAME[] = "North_Field";

// -------------------------------
// WiFi / Server configuration
// -------------------------------
const char* ssid = "PLDTHOMEFIBRff238";
const char* password = "PLDTWIFIt2erm";
const char* serverAddress = "192.168.1.13";
const int serverPort = 8000;
const char* serverPath = "/sensor-data";

WiFiClient client;

// -------------------------------
// Pin configuration
// DHT11 Connect as:
//   VCC → 5V
//   GND → GND
//   DATA → GPIO4 (D2)
// LDR Connect as voltage divider:
//   5V → LDR → A0 → 10kΩ resistor → GND
// -------------------------------
const int LDR_PIN = A0;     // Analog pin for LDR (A0 on ESP8266)
const int DHT_PIN = 4;      // Digital pin for DHT11 data line (GPIO4/D2)
const int DHT_TYPE = DHT11;

// -------------------------------
// Threshold and timing - ADJUSTED FOR BETTER CALIBRATION
// -------------------------------
const int LDR_DAY_THRESHOLD = 100;             // LOWERED from 400 to match actual readings
const int LDR_MIN_READING = 10;                // Expected minimum (dark)
const int LDR_MAX_READING = 1023;              // ESP8266 ADC max (10-bit)
const unsigned long CHECK_INTERVAL_MS = 5000; // 5 seconds
const bool DEBUG_MODE = true;                  // Enable detailed sensor diagnostics

DHT dht(DHT_PIN, DHT_TYPE);
unsigned long lastDHTReadTime = 0;
const unsigned long DHT_READ_INTERVAL = 2000; // DHT11 needs 2 seconds between reads

void connectWiFi();
void sendSensorData(int lightRawValue, float temperatureC, float humidityPercent, bool isDayTime, const String& dhtStatus);

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial to stabilize
  
  Serial.println("\n\n=== Smart Agriculture Sensor Node - ESP8266 ===");
  Serial.print("SensorID: ");
  Serial.print(SENSOR_ID);
  Serial.print(" | Location: ");
  Serial.println(LOCATION_NAME);
  Serial.print("LDR Day Threshold: ");
  Serial.println(LDR_DAY_THRESHOLD);
  Serial.print("Debug Mode: ");
  Serial.println(DEBUG_MODE ? "ON" : "OFF");
  Serial.println("Initializing DHT11...");
  
  // Initialize DHT sensor
  dht.begin();
  delay(1000);

  Serial.println("Connecting to WiFi...");
  connectWiFi();
  
  Serial.println("DHT11 initialized.");
  Serial.println("\nSystem ready. Starting sensor readings...");
  Serial.println("============================================\n");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost. Reconnecting...");
    connectWiFi();
  }

  unsigned long currentTime = millis();
  
  // 1) Read light sensor (always)
  int lightRawValue = analogRead(LDR_PIN);
  
  // 2) Read DHT11 (respect minimum interval between reads)
  float temperatureC = -999;
  float humidityPercent = -999;
  String dhtStatus = "SKIPPED";
  
  if ((currentTime - lastDHTReadTime) >= DHT_READ_INTERVAL) {
    temperatureC = dht.readTemperature();
    humidityPercent = dht.readHumidity();
    lastDHTReadTime = currentTime;
    
    if (isnan(temperatureC) || isnan(humidityPercent)) {
      dhtStatus = "ERROR";
    } else {
      dhtStatus = "OK";
    }
  }
  
  // 3) Determine day/night status for transmission logic
  bool isDayTime = (lightRawValue >= LDR_DAY_THRESHOLD);
  String dayNightStatus = isDayTime ? "DAY" : "NIGHT";
  
  // 4) Always display sensor data (DEBUG_MODE ensures monitoring)
  Serial.print("=== READ "); 
  Serial.print(dayNightStatus);
  Serial.print(" | Time: ");
  Serial.print(millis() / 1000);
  Serial.println("s ===");
  
  Serial.print("  SensorID: ");
  Serial.print(SENSOR_ID);
  Serial.print(" | Location: ");
  Serial.println(LOCATION_NAME);
  
  Serial.print("  Light Level: ");
  Serial.print(lightRawValue);
  Serial.print(" (Threshold: ");
  Serial.print(LDR_DAY_THRESHOLD);
  Serial.println(")");
  
  if (dhtStatus == "OK") {
    Serial.print("  Temperature: ");
    Serial.print(temperatureC, 1);
    Serial.println("°C");
    
    Serial.print("  Humidity: ");
    Serial.print(humidityPercent, 1);
    Serial.println("%");
  } else if (dhtStatus == "ERROR") {
    Serial.println("  [ERROR] DHT11 read failed - check wiring and pin");
  } else {
    Serial.println("  [INFO] Waiting for next DHT11 read window...");
  }
  Serial.println();
  
  // 5) Edge computing rule: only transmit crop data when daytime
  sendSensorData(lightRawValue, temperatureC, humidityPercent, isDayTime, dhtStatus);
  
  delay(CHECK_INTERVAL_MS);
}

void connectWiFi() {
  Serial.print("[WiFi] Connecting to: ");
  Serial.println(ssid);

  int status = WiFi.begin(ssid, password);
  int attempts = 0;

  while (status != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
    attempts++;
  }
  Serial.println();

  if (status == WL_CONNECTED) {
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("[WiFi] Failed. Status: ");
    Serial.println(status);
  }
}

void sendSensorData(int lightRawValue, float temperatureC, float humidityPercent, bool isDayTime, const String& dhtStatus) {
  if (!isDayTime) {
    Serial.println("[TX] Nighttime detected. Data not sent.");
    return;
  }

  if (dhtStatus != "OK") {
    Serial.println("[TX] Daytime, but DHT data is invalid. Data not sent.");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TX] Skipped send: WiFi not connected.");
    return;
  }

  String payload = "{";
  payload += "\"sensor_id\":\"" + String(SENSOR_ID) + "\",";
  payload += "\"location\":\"" + String(LOCATION_NAME) + "\",";
  payload += "\"light\":" + String(lightRawValue) + ",";
  payload += "\"temperature_c\":" + String(temperatureC, 1) + ",";
  payload += "\"humidity_percent\":" + String(humidityPercent, 1) + ",";
  payload += "\"day_night\":\"DAY\"";
  payload += "}";

  Serial.print("[TX] Connecting to ");
  Serial.print(serverAddress);
  Serial.print(":");
  Serial.println(serverPort);

  if (client.connect(serverAddress, serverPort)) {
    Serial.println("[TX] Connected. Sending daytime payload...");

    client.print("POST ");
    client.print(serverPath);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serverAddress);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(payload.length());
    client.println("Connection: close");
    client.println();
    client.println(payload);

    unsigned long startWait = millis();
    while (!client.available() && (millis() - startWait < 2000)) {
      delay(10);
    }

    while (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }

    client.stop();
    Serial.println("[TX] Send complete. Connection closed.");
  } else {
    Serial.println("[TX] Connection failed.");
  }
}
 