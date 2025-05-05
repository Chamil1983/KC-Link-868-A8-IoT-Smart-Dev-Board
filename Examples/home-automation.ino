/**
 * KC-Link PRO A8 Smart Home Automation Example
 * 
 * This sketch demonstrates a complete home automation system using the KC-Link PRO A8.
 * It integrates temperature/humidity monitoring, motion detection, light control,
 * time-based automation, and a web interface.
 * 
 * Features:
 * - Temperature/humidity monitoring with threshold alerts
 * - Motion detection for automated lighting
 * - Light intensity dependent control
 * - Time-based scheduling for devices
 * - Web interface for manual control and configuration
 * - Data logging to SD card (optional)
 * 
 * Hardware:
 * - KC-Link PRO A8 board
 * - DS18B20 temperature sensors (2)
 * - DHT22 humidity sensor
 * - PIR motion detector (connected to digital input)
 * - LDR light sensor (connected to analog input)
 * - Relay-controlled devices (lights, fan, heating/cooling)
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Wire.h>
#include <PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "SPIFFS.h"

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// PCF8574 addresses
#define PCF8574_RELAY_ADDR 0x20  // Address for relay control
#define PCF8574_INPUT_ADDR 0x22  // Address for digital inputs

// Pin definitions for KC-Link PRO A8 V1.4
#define ANALOG_LIGHT_SENSOR 34   // Analog input for light sensor
#define INDOOR_TEMP_SENSOR_PIN 14 // Indoor temperature sensor
#define OUTDOOR_TEMP_SENSOR_PIN 32 // Outdoor temperature sensor
#define DHT_PIN 13               // DHT22 sensor
#define DHTTYPE DHT22            // DHT sensor type

// Relay assignments
#define RELAY_LIVING_ROOM_LIGHT 1
#define RELAY_KITCHEN_LIGHT 2
#define RELAY_BEDROOM_LIGHT 3
#define RELAY_BATHROOM_LIGHT 4
#define RELAY_FAN 5
#define RELAY_AC 6
#define RELAY_HEATER 7
#define RELAY_WATER_PUMP 8

// Input assignments
#define INPUT_MOTION_LIVING_ROOM 1
#define INPUT_MOTION_KITCHEN 2
#define INPUT_MOTION_BEDROOM 3
#define INPUT_MOTION_BATHROOM 4
#define INPUT_DOOR_SENSOR 5
#define INPUT_WINDOW_SENSOR 6
#define INPUT_WATER_LEVEL 7
#define INPUT_SPARE 8

// Thresholds
const float HIGH_TEMP_THRESHOLD = 27.0;  // °C
const float LOW_TEMP_THRESHOLD = 18.0;   // °C
const float HIGH_HUMIDITY_THRESHOLD = 65.0; // %
const float LOW_HUMIDITY_THRESHOLD = 30.0;  // %
const int LIGHT_THRESHOLD = 500;         // Analog value (0-4095)

// Time intervals
const unsigned long SENSOR_READ_INTERVAL = 30000;  // 30 seconds
const unsigned long MOTION_TIMEOUT = 300000;       // 5 minutes

// Objects
PCF8574 relayModule(PCF8574_RELAY_ADDR);
PCF8574 inputModule(PCF8574_INPUT_ADDR);
OneWire oneWireIndoor(INDOOR_TEMP_SENSOR_PIN);
OneWire oneWireOutdoor(OUTDOOR_TEMP_SENSOR_PIN);
DallasTemperature indoorTempSensor(&oneWireIndoor);
DallasTemperature outdoorTempSensor(&oneWireOutdoor);
DHT dht(DHT_PIN, DHTTYPE);
AsyncWebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

// State variables
struct SystemState {
  float indoorTemp;
  float outdoorTemp;
  float humidity;
  int lightLevel;
  bool motionDetected[4];  // For 4 motion sensors
  bool doorOpen;
  bool windowOpen;
  bool waterLevelLow;
  unsigned long lastMotionTime[4];
  byte relayState;
  byte inputState;
  bool autoMode;
  bool heatingMode;
  bool coolingMode;
  bool schedulingEnabled;
} state;

// Schedule structure
struct ScheduleItem {
  int relay;
  int hour;
  int minute;
  bool turnOn;
  bool enabled;
  bool daysOfWeek[7];  // Sunday to Saturday
};

// Maximum of 20 scheduled events
ScheduleItem schedules[20];
int scheduleCount = 0;

// Function prototypes
void setupWiFi();
void setupRelays();
void setupInputs();
void setupSensors();
void setupTime();
void setupWebServer();
void readSensors();
void checkInputs();
void applyRules();
void handleSchedules();
void setRelay(int relay, bool state);
void toggleRelay(int relay);
bool getRelayState(int relay);
bool getInputState(int input);
void processMotionDetection(int sensorId, bool detected);
void loadSettings();
void saveSettings();
String getTimeString();
String getSystemStateJson();

void setup() {
  // Initialize serial
  Serial.begin(115200);
  Serial.println(F("KC-Link PRO A8 Smart Home Automation System"));
  
  // Initialize file system for settings
  if (!SPIFFS.begin(true)) {
    Serial.println(F("An error occurred while mounting SPIFFS"));
  }
  
  // Load settings from file
  loadSettings();
  
  // Initialize I2C
  Wire.begin();
  
  // Setup components
  setupRelays();
  setupInputs();
  setupSensors();
  setupWiFi();
  setupTime();
  setupWebServer();
  
  Serial.println(F("System initialized and ready"));
}

void loop() {
  // Update time
  timeClient.update();
  
  // Read sensor data periodically
  static unsigned long lastSensorReadTime = 0;
  if (millis() - lastSensorReadTime > SENSOR_READ_INTERVAL) {
    lastSensorReadTime = millis();
    readSensors();
  }
  
  // Check inputs
  checkInputs();
  
  // Apply automation rules if auto mode is enabled
  if (state.autoMode) {
    applyRules();
  }
  
  // Handle scheduled events
  handleSchedules();
  
  // Small delay to avoid hogging CPU
  delay(100);
}

void setupWiFi() {
  Serial.println(F("Connecting to WiFi..."));
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println(F("WiFi connection failed! Operating in offline mode."));
  }
}

void setupRelays() {
  if (relayModule.begin()) {
    Serial.println(F("Relay module initialized"));
    for (int i = 0; i < 8; i++) {
      relayModule.pinMode(i, OUTPUT);
      relayModule.digitalWrite(i, HIGH);  // Relays are active LOW, so HIGH = OFF
    }
    state.relayState = 0;  // All relays off
  } else {
    Serial.println(F("Error: Relay module not found!"));
  }
}

void setupInputs() {
  if (inputModule.begin()) {
    Serial.println(F("Input module initialized"));
    for (int i = 0; i < 8; i++) {
      inputModule.pinMode(i, INPUT);
    }
    state.inputState = inputModule.read8();
  } else {
    Serial.println(F("Error: Input module not found!"));
  }
  
  // Initialize motion detection timestamps
  for (int i = 0; i < 4; i++) {
    state.lastMotionTime[i] = 0;
    state.motionDetected[i] = false;
  }
}

void setupSensors() {
  // Initialize temperature sensors
  indoorTempSensor.begin();
  outdoorTempSensor.begin();
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initial sensor readings
  readSensors();
  
  Serial.println(F("All sensors initialized"));
}

void setupTime() {
  timeClient.begin();
  timeClient.update();
  
  Serial.println(F("NTP client initialized"));
  Serial.print(F("Current time: "));
  Serial.println(getTimeString());
}

void setupWebServer() {
  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
  // API to get current system state
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request) {
    String jsonResponse = getSystemStateJson();
    request->send(200, "application/json", jsonResponse);
  });
  
  // API to control relays
  server.on("/api/relay", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("id", true) && request->hasParam("state", true)) {
      int relayId = request->getParam("id", true)->value().toInt();
      String stateStr = request->getParam("state", true)->value();
      
      if (relayId >= 1 && relayId <= 8) {
        if (stateStr == "toggle") {
          toggleRelay(relayId);
        } else {
          bool newState = (stateStr == "on" || stateStr == "1" || stateStr == "true");
          setRelay(relayId, newState);
        }
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid relay ID\"}");
      }
    } else {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
    }
  });
  
  // API to set auto mode
  server.on("/api/mode", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("auto", true)) {
      String autoStr = request->getParam("auto", true)->value();
      state.autoMode = (autoStr == "on" || autoStr == "1" || autoStr == "true");
      saveSettings();
      request->send(200, "application/json", "{\"success\":true}");
    } else {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
    }
  });
  
  // API to set heating/cooling mode
  server.on("/api/hvac", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool paramsOk = false;
    
    if (request->hasParam("heating", true)) {
      String heatingStr = request->getParam("heating", true)->value();
      state.heatingMode = (heatingStr == "on" || heatingStr == "1" || heatingStr == "true");
      paramsOk = true;
    }
    
    if (request->hasParam("cooling", true)) {
      String coolingStr = request->getParam("cooling", true)->value();
      state.coolingMode = (coolingStr == "on" || coolingStr == "1" || coolingStr == "true");
      paramsOk = true;
    }
    
    if (paramsOk) {
      saveSettings();
      request->send(200, "application/json", "{\"success\":true}");
    } else {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
    }
  });
  
  // API to manage schedules
  server.on("/api/schedules", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.createNestedArray("schedules");
    
    for (int i = 0; i < scheduleCount; i++) {
      JsonObject schedObj = array.createNestedObject();
      schedObj["id"] = i;
      schedObj["relay"] = schedules[i].relay;
      schedObj["hour"] = schedules[i].hour;
      schedObj["minute"] = schedules[i].minute;
      schedObj["turnOn"] = schedules[i].turnOn;
      schedObj["enabled"] = schedules[i].enabled;
      
      JsonArray daysArray = schedObj.createNestedArray("days");
      for (int j = 0; j < 7; j++) {
        daysArray.add(schedules[i].daysOfWeek[j]);
      }
    }
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
  });
  
  // Handle adding, updating, and deleting schedules
  AsyncCallbackJsonWebHandler* createScheduleHandler = new AsyncCallbackJsonWebHandler("/api/schedules", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (request->method() == HTTP_POST) {
      // Add new schedule
      if (scheduleCount < 20) {
        JsonObject jsonObj = json.as<JsonObject>();
        
        if (jsonObj.containsKey("relay") && jsonObj.containsKey("hour") && 
            jsonObj.containsKey("minute") && jsonObj.containsKey("turnOn")) {
          
          int idx = scheduleCount++;
          schedules[idx].relay = jsonObj["relay"];
          schedules[idx].hour = jsonObj["hour"];
          schedules[idx].minute = jsonObj["minute"];
          schedules[idx].turnOn = jsonObj["turnOn"];
          schedules[idx].enabled = jsonObj.containsKey("enabled") ? jsonObj["enabled"] : true;
          
          // Process days of week
          if (jsonObj.containsKey("days") && jsonObj["days"].is<JsonArray>()) {
            JsonArray days = jsonObj["days"].as<JsonArray>();
            for (int i = 0; i < 7 && i < days.size(); i++) {
              schedules[idx].daysOfWeek[i] = days[i];
            }
          } else {
            // Default to all days enabled
            for (int i = 0; i < 7; i++) {
              schedules[idx].daysOfWeek[i] = true;
            }
          }
          
          saveSettings();
          request->send(200, "application/json", "{\"success\":true,\"id\":" + String(idx) + "}");
        } else {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing required parameters\"}");
        }
      } else {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Maximum number of schedules reached\"}");
      }
    } else if (request->method() == HTTP_PUT) {
      // Update existing schedule
      JsonObject jsonObj = json.as<JsonObject>();
      
      if (jsonObj.containsKey("id")) {
        int id = jsonObj["id"];
        
        if (id >= 0 && id < scheduleCount) {
          if (jsonObj.containsKey("relay")) schedules[id].relay = jsonObj["relay"];
          if (jsonObj.containsKey("hour")) schedules[id].hour = jsonObj["hour"];
          if (jsonObj.containsKey("minute")) schedules[id].minute = jsonObj["minute"];
          if (jsonObj.containsKey("turnOn")) schedules[id].turnOn = jsonObj["turnOn"];
          if (jsonObj.containsKey("enabled")) schedules[id].enabled = jsonObj["enabled"];
          
          // Process days of week
          if (jsonObj.containsKey("days") && jsonObj["days"].is<JsonArray>()) {
            JsonArray days = jsonObj["days"].as<JsonArray>();
            for (int i = 0; i < 7 && i < days.size(); i++) {
              schedules[id].daysOfWeek[i] = days[i];
            }
          }
          
          saveSettings();
          request->send(200, "application/json", "{\"success\":true}");
        } else {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid schedule ID\"}");
        }
      } else {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing schedule ID\"}");
      }
    } else if (request->method() == HTTP_DELETE) {
      // Delete a schedule
      JsonObject jsonObj = json.as<JsonObject>();
      
      if (jsonObj.containsKey("id")) {
        int id = jsonObj["id"];
        
        if (id >= 0 && id < scheduleCount) {
          // Remove the schedule by shifting all items after it
          for (int i = id; i < scheduleCount - 1; i++) {
            schedules[i] = schedules[i + 1];
          }
          scheduleCount--;
          
          saveSettings();
          request->send(200, "application/json", "{\"success\":true}");
        } else {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid schedule ID\"}");
        }
      } else {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing schedule ID\"}");
      }
    }
  });
  server.addHandler(createScheduleHandler);
  
  // Start the server
  server.begin();
  Serial.println(F("Web server started"));
}

void readSensors() {
  // Read indoor temperature
  indoorTempSensor.requestTemperatures();
  state.indoorTemp = indoorTempSensor.getTempCByIndex(0);
  
  // Read outdoor temperature
  outdoorTempSensor.requestTemperatures();
  state.outdoorTemp = outdoorTempSensor.getTempCByIndex(0);
  
  // Read humidity
  float h = dht.readHumidity();
  if (!isnan(h)) {
    state.humidity = h;
  }
  
  // Read light level
  state.lightLevel = analogRead(ANALOG_LIGHT_SENSOR);
  
  Serial.println(F("Sensor readings:"));
  Serial.print(F("Indoor temperature: "));
  Serial.print(state.indoorTemp);
  Serial.println(F("°C"));
  
  Serial.print(F("Outdoor temperature: "));
  Serial.print(state.outdoorTemp);
  Serial.println(F("°C"));
  
  Serial.print(F("Humidity: "));
  Serial.print(state.humidity);
  Serial.println(F("%"));
  
  Serial.print(F("Light level: "));
  Serial.println(state.lightLevel);
}

void checkInputs() {
  // Read all inputs
  byte newInputState = inputModule.read8();
  
  // Only process if there's a change
  if (newInputState != state.inputState) {
    // Check motion sensors
    for (int i = 0; i < 4; i++) {
      bool oldState = bitRead(state.inputState, i);
      bool newState = bitRead(newInputState, i);
      
      if (oldState != newState) {
        processMotionDetection(i + 1, newState);
      }
    }
    
    // Check door sensor
    bool doorState = bitRead(newInputState, INPUT_DOOR_SENSOR - 1);
    if (doorState != state.doorOpen) {
      state.doorOpen = doorState;
      Serial.print(F("Door is now "));
      Serial.println(state.doorOpen ? F("OPEN") : F("CLOSED"));
    }
    
    // Check window sensor
    bool windowState = bitRead(newInputState, INPUT_WINDOW_SENSOR - 1);
    if (windowState != state.windowOpen) {
      state.windowOpen = windowState;
      Serial.print(F("Window is now "));
      Serial.println(state.windowOpen ? F("OPEN") : F("CLOSED"));
    }
    
    // Check water level
    bool waterLevel = bitRead(newInputState, INPUT_WATER_LEVEL - 1);
    if (waterLevel != state.waterLevelLow) {
      state.waterLevelLow = waterLevel;
      Serial.print(F("Water level is "));
      Serial.println(state.waterLevelLow ? F("LOW") : F("OK"));
      
      // Automatic water pump control
      if (state.autoMode) {
        setRelay(RELAY_WATER_PUMP, state.waterLevelLow);
      }
    }
    
    // Update input state
    state.inputState = newInputState;
  }
}

void processMotionDetection(int sensorId, bool detected) {
  if (sensorId < 1 || sensorId > 4) return;
  
  // Update motion state
  state.motionDetected[sensorId - 1] = detected;
  state.lastMotionTime[sensorId - 1] = detected ? millis() : state.lastMotionTime[sensorId - 1];
  
  Serial.print(F("Motion detector "));
  Serial.print(sensorId);
  Serial.print(F(" is "));
  Serial.println(detected ? F("ACTIVE") : F("INACTIVE"));
  
  // Automatic light control based on motion
  if (state.autoMode) {
    // Map sensor to corresponding light
    int relayId = 0;
    switch (sensorId) {
      case INPUT_MOTION_LIVING_ROOM:
        relayId = RELAY_LIVING_ROOM_LIGHT;
        break;
      case INPUT_MOTION_KITCHEN:
        relayId = RELAY_KITCHEN_LIGHT;
        break;
      case INPUT_MOTION_BEDROOM:
        relayId = RELAY_BEDROOM_LIGHT;
        break;
      case INPUT_MOTION_BATHROOM:
        relayId = RELAY_BATHROOM_LIGHT;
        break;
    }
    
    // Only turn on light if motion detected AND light level is low
    if (relayId > 0) {
      if (detected && state.lightLevel < LIGHT_THRESHOLD) {
        setRelay(relayId, true);
      }
    }
  }
}

void applyRules() {
  // Check for motion timeouts and turn off lights if needed
  unsigned long currentMillis = millis();
  
  for (int i = 0; i < 4; i++) {
    if (!state.motionDetected[i] && (currentMillis - state.lastMotionTime[i] > MOTION_TIMEOUT)) {
      // Turn off the corresponding light if no motion for timeout period
      int relayId = i + 1;  // Simple mapping for example
      if (getRelayState(relayId)) {
        setRelay(relayId, false);
        Serial.print(F("Auto turning off light "));
        Serial.print(relayId);
        Serial.println(F(" due to inactivity"));
      }
    }
  }
  
  // Temperature control logic
  if (state.indoorTemp > HIGH_TEMP_THRESHOLD && state.coolingMode) {
    // Turn on AC if too hot
    if (!getRelayState(RELAY_AC)) {
      setRelay(RELAY_AC, true);
      setRelay(RELAY_HEATER, false);  // Make sure heater is off
      Serial.println(F("Auto turning ON AC due to high temperature"));
    }
  } else if (state.indoorTemp < LOW_TEMP_THRESHOLD && state.heatingMode) {
    // Turn on heater if too cold
    if (!getRelayState(RELAY_HEATER)) {
      setRelay(RELAY_HEATER, true);
      setRelay(RELAY_AC, false);  // Make sure AC is off
      Serial.println(F("Auto turning ON heater due to low temperature"));
    }
  } else {
    // Temperature is in comfortable range
    if (getRelayState(RELAY_AC) || getRelayState(RELAY_HEATER)) {
      setRelay(RELAY_AC, false);
      setRelay(RELAY_HEATER, false);
      Serial.println(F("Auto turning OFF HVAC - temperature in comfort range"));
    }
  }
  
  // Humidity control logic
  if (state.humidity > HIGH_HUMIDITY_THRESHOLD) {
    // Turn on fan if humidity is too high
    if (!getRelayState(RELAY_FAN)) {
      setRelay(RELAY_FAN, true);
      Serial.println(F("Auto turning ON fan due to high humidity"));
    }
  } else if (state.humidity < LOW_HUMIDITY_THRESHOLD) {
    // Could turn on a humidifier if available
  } else {
    // Humidity is in comfortable range
    if (getRelayState(RELAY_FAN) && state.humidity < HIGH_HUMIDITY_THRESHOLD - 5) {
      setRelay(RELAY_FAN, false);
      Serial.println(F("Auto turning OFF fan - humidity in comfort range"));
    }
  }
}

void handleSchedules() {
  if (!state.schedulingEnabled) return;
  
  // Get current time components
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentSecond = timeClient.getSeconds();
  int currentDay = (timeClient.getDay() + 6) % 7;  // Convert from NTP (0=Sunday) to our format (0=Monday)
  
  // Only check schedules once per minute, at the beginning of the minute
  static int lastMinuteChecked = -1;
  if (currentMinute != lastMinuteChecked && currentSecond < 5) {
    lastMinuteChecked = currentMinute;
    
    // Check each schedule
    for (int i = 0; i < scheduleCount; i++) {
      ScheduleItem *schedule = &schedules[i];
      
      // Skip disabled schedules
      if (!schedule->enabled) continue;
      
      // Check if this is the right day
      if (!schedule->daysOfWeek[currentDay]) continue;
      
      // Check if this is the right time
      if (schedule->hour == currentHour && schedule->minute == currentMinute) {
        // Execute the scheduled action
        setRelay(schedule->relay, schedule->turnOn);
        
        Serial.print(F("Scheduled action: Relay "));
        Serial.print(schedule->relay);
        Serial.print(F(" turned "));
        Serial.println(schedule->turnOn ? F("ON") : F("OFF"));
      }
    }
  }
}

void setRelay(int relay, bool state) {
  if (relay < 1 || relay > 8) return;
  
  // Convert to zero-based index
  int relayIndex = relay - 1;
  
  // Update relay
  relayModule.digitalWrite(relayIndex, state ? LOW : HIGH);  // Active LOW
  
  // Update state variable
  if (state) {
    bitSet(state.relayState, relayIndex);
  } else {
    bitClear(state.relayState, relayIndex);
  }
  
  Serial.print(F("Relay "));
  Serial.print(relay);
  Serial.print(F(" set to "));
  Serial.println(state ? F("ON") : F("OFF"));
}

void toggleRelay(int relay) {
  if (relay < 1 || relay > 8) return;
  
  // Convert to zero-based index
  int relayIndex = relay - 1;
  
  // Toggle state
  bool currentState = bitRead(state.relayState, relayIndex);
  setRelay(relay, !currentState);
}

bool getRelayState(int relay) {
  if (relay < 1 || relay > 8) return false;
  
  // Convert to zero-based index
  int relayIndex = relay - 1;
  
  // Return relay state
  return bitRead(state.relayState, relayIndex);
}

bool getInputState(int input) {
  if (input < 1 || input > 8) return false;
  
  // Convert to zero-based index
  int inputIndex = input - 1;
  
  // Return input state
  return bitRead(state.inputState, inputIndex);
}

String getTimeString() {
  char buffer[20];
  sprintf(buffer, "%02d:%02d:%02d", timeClient.getHours(), 
          timeClient.getMinutes(), timeClient.getSeconds());
  return String(buffer);
}

String getSystemStateJson() {
  DynamicJsonDocument doc(1024);
  
  // Add sensor data
  doc["indoor_temp"] = state.indoorTemp;
  doc["outdoor_temp"] = state.outdoorTemp;
  doc["humidity"] = state.humidity;
  doc["light_level"] = state.lightLevel;
  
  // Add input states
  JsonObject inputs = doc.createNestedObject("inputs");
  inputs["motion_living_room"] = state.motionDetected[0];
  inputs["motion_kitchen"] = state.motionDetected[1];
  inputs["motion_bedroom"] = state.motionDetected[2];
  inputs["motion_bathroom"] = state.motionDetected[3];
  inputs["door_open"] = state.doorOpen;
  inputs["window_open"] = state.windowOpen;
  inputs["water_level_low"] = state.waterLevelLow;
  
  // Add relay states
  JsonObject relays = doc.createNestedObject("relays");
  relays["living_room_light"] = getRelayState(RELAY_LIVING_ROOM_LIGHT);
  relays["kitchen_light"] = getRelayState(RELAY_KITCHEN_LIGHT);
  relays["bedroom_light"] = getRelayState(RELAY_BEDROOM_LIGHT);
  relays["bathroom_light"] = getRelayState(RELAY_BATHROOM_LIGHT);
  relays["fan"] = getRelayState(RELAY_FAN);
  relays["ac"] = getRelayState(RELAY_AC);
  relays["heater"] = getRelayState(RELAY_HEATER);
  relays["water_pump"] = getRelayState(RELAY_WATER_PUMP);
  
  // Add system modes
  JsonObject modes = doc.createNestedObject("modes");
  modes["auto"] = state.autoMode;
  modes["heating"] = state.heatingMode;
  modes["cooling"] = state.coolingMode;
  modes["scheduling"] = state.schedulingEnabled;
  
  // Add time information
  doc["time"] = getTimeString();
  doc["day_of_week"] = timeClient.getDay();
  
  // Serialize to string
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

void loadSettings() {
  // Initialize with defaults
  state.autoMode = true;
  state.heatingMode = true;
  state.coolingMode = true;
  state.schedulingEnabled = true;
  scheduleCount = 0;
  
  // Try to load from file
  if (SPIFFS.exists("/settings.json")) {
    File file = SPIFFS.open("/settings.json", "r");
    if (file) {
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, file);
      
      if (!error) {
        // Load system modes
        if (doc.containsKey("modes")) {
          JsonObject modes = doc["modes"];
          state.autoMode = modes.containsKey("auto") ? modes["auto"] : true;
          state.heatingMode = modes.containsKey("heating") ? modes["heating"] : true;
          state.coolingMode = modes.containsKey("cooling") ? modes["cooling"] : true;
          state.schedulingEnabled = modes.containsKey("scheduling") ? modes["scheduling"] : true;
        }
        
        // Load schedules
        if (doc.containsKey("schedules") && doc["schedules"].is<JsonArray>()) {
          JsonArray schedulesArray = doc["schedules"].as<JsonArray>();
          scheduleCount = min((int)schedulesArray.size(), 20);
          
          for (int i = 0; i < scheduleCount; i++) {
            JsonObject schedObj = schedulesArray[i];
            schedules[i].relay = schedObj["relay"];
            schedules[i].hour = schedObj["hour"];
            schedules[i].minute = schedObj["minute"];
            schedules[i].turnOn = schedObj["turnOn"];
            schedules[i].enabled = schedObj.containsKey("enabled") ? schedObj["enabled"] : true;
            
            // Load days of week
            if (schedObj.containsKey("days") && schedObj["days"].is<JsonArray>()) {
              JsonArray days = schedObj["days"].as<JsonArray>();
              for (int j = 0; j < 7 && j < days.size(); j++) {
                schedules[i].daysOfWeek[j] = days[j];
              }
            } else {
              // Default to all days enabled
              for (int j = 0; j < 7; j++) {
                schedules[i].daysOfWeek[j] = true;
              }
            }
          }
        }
        
        Serial.println(F("Settings loaded successfully"));
      } else {
        Serial.println(F("Failed to parse settings file"));
      }
      
      file.close();
    }
  } else {
    Serial.println(F("No settings file found, using defaults"));
    saveSettings();  // Create the file with defaults
  }
}

void saveSettings() {
  DynamicJsonDocument doc(4096);
  
  // Save system modes
  JsonObject modes = doc.createNestedObject("modes");
  modes["auto"] = state.autoMode;
  modes["heating"] = state.heatingMode;
  modes["cooling"] = state.coolingMode;
  modes["scheduling"] = state.schedulingEnabled;
  
  // Save schedules
  JsonArray schedulesArray = doc.createNestedArray("schedules");
  for (int i = 0; i < scheduleCount; i++) {
    JsonObject schedObj = schedulesArray.createNestedObject();
    schedObj["relay"] = schedules[i].relay;
    schedObj["hour"] = schedules[i].hour;
    schedObj["minute"] = schedules[i].minute;
    schedObj["turnOn"] = schedules[i].turnOn;
    schedObj["enabled"] = schedules[i].enabled;
    
    JsonArray days = schedObj.createNestedArray("days");
    for (int j = 0; j < 7; j++) {
      days.add(schedules[i].daysOfWeek[j]);
    }
  }
  
  // Write to file
  File file = SPIFFS.open("/settings.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println(F("Settings saved successfully"));
  } else {
    Serial.println(F("Failed to open settings file for writing"));
  }
}
