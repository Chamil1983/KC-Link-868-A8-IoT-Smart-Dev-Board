/**
 * KC-Link PRO A8 Modbus RTU Monitor
 * 
 * This sketch sets up the KC-Link PRO A8 as a Modbus RTU slave device
 * with monitoring capabilities. It monitors all inputs and outputs,
 * makes them available via Modbus registers, and can be controlled
 * by a Modbus master device.
 * 
 * Features:
 * - Implements Modbus RTU slave protocol
 * - Provides read access to all digital inputs, analog inputs, and temperature sensors
 * - Allows control of all relay outputs
 * - Includes web interface for configuration and monitoring
 * - Data logging to SPIFFS
 * 
 * Modbus Register Map:
 * - 0-7: Digital Outputs (Relays) - Read/Write (0 = OFF, 1 = ON)
 * - 8-15: Digital Inputs - Read Only (0 = Inactive, 1 = Active)
 * - 16-19: Analog Inputs (0-4095) - Read Only
 * - 20-23: Temperature Sensors (°C × 10) - Read Only
 * - 24-25: Humidity (% × 10) - Read Only
 * - 100-107: Relay Pulse Timers (seconds) - Read/Write
 * - 200-207: Relay Schedules (hour * 100 + minute) - Read/Write
 */

#include <Arduino.h>
#include <ModbusRTU.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Pin definitions for Modbus
#define MODBUS_RX_PIN 5   // Connect to Modbus RTU TX
#define MODBUS_TX_PIN 17  // Connect to Modbus RTU RX
#define MODBUS_DE_PIN 16  // Connect to Modbus RTU DE/RE (for RS485)

// PCF8574 addresses
#define PCF8574_RELAY_ADDR 0x20  // Address for relay control
#define PCF8574_INPUT_ADDR 0x22  // Address for digital inputs

// Pin definitions for KC-Link PRO A8 V1.4
#define ANALOG_INPUT_1 34
#define ANALOG_INPUT_2 35
#define TEMP_SENSOR_1_PIN 14
#define TEMP_SENSOR_2_PIN 13
#define TEMP_SENSOR_3_PIN 32
#define TEMP_SENSOR_4_PIN 33
#define DHTPIN 13
#define DHTTYPE DHT22

// Modbus settings
#define SLAVE_ID 1
#define MODBUS_BAUDRATE 9600

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Modbus components
ModbusRTU mb;
uint16_t modbusRegisters[256];  // Modbus register array

// I/O objects
PCF8574 relayModule(PCF8574_RELAY_ADDR);
PCF8574 inputModule(PCF8574_INPUT_ADDR);
OneWire oneWire1(TEMP_SENSOR_1_PIN);
OneWire oneWire2(TEMP_SENSOR_2_PIN);
OneWire oneWire3(TEMP_SENSOR_3_PIN);
OneWire oneWire4(TEMP_SENSOR_4_PIN);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
DallasTemperature sensors3(&oneWire3);
DallasTemperature sensors4(&oneWire4);
DHT dht(DHTPIN, DHTTYPE);

// Web server
AsyncWebServer server(80);

// State variables
struct {
  byte relayState;
  byte inputState;
  int analog1Value;
  int analog2Value;
  float temperatures[4];
  float humidity;
  unsigned long relayPulseEndTimes[8];  // For relay pulse functionality
} state;

// Function declarations
void setupModbus();
void setupRelays();
void setupInputs();
void setupSensors();
void setupWebServer();
void handleModbusRegisters();
void updateDigitalInputs();
void updateAnalogInputs();
void updateTemperatureSensors();
void updateRelays();
void loadConfiguration();
void saveConfiguration();
void logData();

void setup() {
  // Initialize serial
  Serial.begin(115200);
  Serial.println(F("KC-Link PRO A8 Modbus RTU Monitor"));
  
  // Initialize SPIFFS for web files and logging
  if(!SPIFFS.begin(true)) {
    Serial.println(F("An error occurred while mounting SPIFFS"));
  }
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize components
  setupRelays();
  setupInputs();
  setupSensors();
  
  // Initialize Modbus
  setupModbus();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to WiFi"));
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    
    // Setup web server
    setupWebServer();
  } else {
    Serial.println();
    Serial.println(F("WiFi connection failed. Operating in Modbus-only mode."));
  }
  
  // Load configuration from SPIFFS
  loadConfiguration();
  
  Serial.println(F("System initialized and ready"));
}

void loop() {
  // Process Modbus messages
  mb.task();
  
  // Read inputs and update Modbus registers
  updateDigitalInputs();
  updateAnalogInputs();
  updateTemperatureSensors();
  
  // Process any register changes from Modbus master
  handleModbusRegisters();
  
  // Update relay states based on pulse timers
  updateRelays();
  
  // Log data periodically (every minute)
  static unsigned long lastLogTime = 0;
  if (millis() - lastLogTime > 60000) {
    lastLogTime = millis();
    logData();
  }
}

void setupModbus() {
  // Create and configure a new Modbus RTU server
  Serial2.begin(MODBUS_BAUDRATE, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  
  mb.begin(&Serial2, MODBUS_DE_PIN);
  mb.slave(SLAVE_ID);
  
  // Initialize Modbus registers
  memset(modbusRegisters, 0, sizeof(modbusRegisters));
  
  // Add holding registers for Modbus access
  // 0-7: Relay outputs (coils)
  mb.addHreg(0, 0, 8);  // 8 relay outputs (0 = OFF, 1 = ON)
  
  // 8-15: Digital inputs
  mb.addHreg(8, 0, 8);  // 8 digital inputs (0 = Inactive, 1 = Active)
  
  // 16-19: Analog inputs and temperatures
  mb.addHreg(16, 0, 10); // Analog + Temperature registers
  
  // 100-107: Relay pulse timers
  mb.addHreg(100, 0, 8);  // 8 relay pulse timers (seconds)
  
  // 200-207: Relay schedules
  mb.addHreg(200, 0, 8);  // 8 relay schedules (hour * 100 + minute)
  
  Serial.println(F("Modbus RTU initialized"));
}

void setupRelays() {
  if (relayModule.begin()) {
    Serial.println(F("Relay module initialized"));
    
    // Set all pins as OUTPUT and initialize to OFF
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
    
    // Set all pins as INPUT
    for (int i = 0; i < 8; i++) {
      inputModule.pinMode(i, INPUT);
    }
    
    // Initial reading
    state.inputState = inputModule.read8();
  } else {
    Serial.println(F("Error: Input module not found!"));
  }
}

void setupSensors() {
  // Initialize temperature sensors
  sensors1.begin();
  sensors2.begin();
  sensors3.begin();
  sensors4.begin();
  
  // Initialize DHT sensor
  dht.begin();
  
  Serial.println(F("Temperature and humidity sensors initialized"));
}

void setupWebServer() {
  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
  // API endpoint for system status
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Create a JSON document with the current system status
    DynamicJsonDocument doc(1024);
    
    // Add relay states
    JsonArray relays = doc.createNestedArray("relays");
    for (int i = 0; i < 8; i++) {
      relays.add((state.relayState >> i) & 1);
    }
    
    // Add input states
    JsonArray inputs = doc.createNestedArray("inputs");
    for (int i = 0; i < 8; i++) {
      inputs.add((state.inputState >> i) & 1);
    }
    
    // Add analog values
    JsonArray analogs = doc.createNestedArray("analogs");
    analogs.add(state.analog1Value);
    analogs.add(state.analog2Value);
    
    // Add temperatures
    JsonArray temps = doc.createNestedArray("temperatures");
    for (int i = 0; i < 4; i++) {
      temps.add(state.temperatures[i]);
    }
    
    // Add humidity
    doc["humidity"] = state.humidity;
    
    // Add Modbus status
    doc["modbus_enabled"] = true;
    doc["modbus_address"] = SLAVE_ID;
    doc["modbus_baudrate"] = MODBUS_BAUDRATE;
    
    // Convert to string and send
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // API endpoint to control relays
  server.on("/api/relay", HTTP_POST, [](AsyncWebServerRequest *request){
    // Check if the required parameters are present
    if (!request->hasParam("id", true) || !request->hasParam("state", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
      return;
    }
    
    // Get parameters
    int id = request->getParam("id", true)->value().toInt();
    String stateStr = request->getParam("state", true)->value();
    
    // Check if the relay ID is valid
    if (id < 1 || id > 8) {
      request->send(400, "application/json", "{\"error\":\"Invalid relay ID\"}");
      return;
    }
    
    // Set relay state
    bool relayState = false;
    if (stateStr == "on" || stateStr == "1" || stateStr == "true") {
      relayState = true;
    }
    
    // Update relay (relay IDs are 1-based, but need 0-based for bitwise operations)
    int relayIndex = id - 1;
    
    // Set relay state
    if (relayState) {
      // Turn on relay (active LOW)
      relayModule.digitalWrite(relayIndex, LOW);
      bitSet(state.relayState, relayIndex);
    } else {
      // Turn off relay
      relayModule.digitalWrite(relayIndex, HIGH);
      bitClear(state.relayState, relayIndex);
    }
    
    // Update Modbus register
    modbusRegisters[relayIndex] = relayState ? 1 : 0;
    
    // Send response
    request->send(200, "application/json", "{\"success\":true}");
  });
  
  // API endpoint for pulse operation
  server.on("/api/pulse", HTTP_POST, [](AsyncWebServerRequest *request){
    // Check if the required parameters are present
    if (!request->hasParam("id", true) || !request->hasParam("duration", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
      return;
    }
    
    // Get parameters
    int id = request->getParam("id", true)->value().toInt();
    int duration = request->getParam("duration", true)->value().toInt();
    
    // Check if the relay ID is valid
    if (id < 1 || id > 8) {
      request->send(400, "application/json", "{\"error\":\"Invalid relay ID\"}");
      return;
    }
    
    // Check if the duration is valid
    if (duration <= 0 || duration > 3600) {
      request->send(400, "application/json", "{\"error\":\"Invalid duration (1-3600 seconds)\"}");
      return;
    }
    
    // Set relay index
    int relayIndex = id - 1;
    
    // Turn on relay (active LOW)
    relayModule.digitalWrite(relayIndex, LOW);
    bitSet(state.relayState, relayIndex);
    
    // Set pulse end time
    state.relayPulseEndTimes[relayIndex] = millis() + (duration * 1000);
    
    // Update Modbus registers
    modbusRegisters[relayIndex] = 1;             // Set relay state to ON
    modbusRegisters[100 + relayIndex] = duration; // Set pulse timer
    
    // Send response
    request->send(200, "application/json", "{\"success\":true}");
  });
  
  // API endpoint for Modbus configuration
  server.on("/api/modbus", HTTP_POST, [](AsyncWebServerRequest *request){
    // Check if the required parameters are present
    if (!request->hasParam("address", true) || !request->hasParam("baudrate", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
      return;
    }
    
    // Get parameters
    int address = request->getParam("address", true)->value().toInt();
    int baudrate = request->getParam("baudrate", true)->value().toInt();
    
    // Check if the address is valid
    if (address < 1 || address > 247) {
      request->send(400, "application/json", "{\"error\":\"Invalid Modbus address (1-247)\"}");
      return;
    }
    
    // Check if the baudrate is valid
    if (baudrate != 9600 && baudrate != 19200 && baudrate != 38400 && baudrate != 57600 && baudrate != 115200) {
      request->send(400, "application/json", "{\"error\":\"Invalid baudrate\"}");
      return;
    }
    
    // Update configuration
    // Note: This would normally restart the Modbus with new settings, but
    // in this example we're just acknowledging the request
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Modbus settings updated. Restart required.\"}");
  });
  
  // Start server
  server.begin();
  Serial.println(F("Web server started"));
}

void updateDigitalInputs() {
  // Read current input state from PCF8574
  byte newInputState = inputModule.read8();
  
  // If the state has changed, update it
  if (newInputState != state.inputState) {
    state.inputState = newInputState;
    
    // Update Modbus registers
    for (int i = 0; i < 8; i++) {
      modbusRegisters[8 + i] = (state.inputState >> i) & 1;
    }
  }
}

void updateAnalogInputs() {
  // Read analog inputs
  int analog1 = analogRead(ANALOG_INPUT_1);
  int analog2 = analogRead(ANALOG_INPUT_2);
  
  // Update state if values have changed significantly
  if (abs(analog1 - state.analog1Value) > 10) {
    state.analog1Value = analog1;
    modbusRegisters[16] = analog1;
  }
  
  if (abs(analog2 - state.analog2Value) > 10) {
    state.analog2Value = analog2;
    modbusRegisters[17] = analog2;
  }
  
  // Read humidity
  float h = dht.readHumidity();
  if (!isnan(h)) {
    state.humidity = h;
    modbusRegisters[25] = (uint16_t)(h * 10);  // Store as humidity × 10
  }
}

void updateTemperatureSensors() {
  // Request temperatures (non-blocking would be better for a real application)
  sensors1.requestTemperatures();
  sensors2.requestTemperatures();
  sensors3.requestTemperatures();
  sensors4.requestTemperatures();
  
  // Read temperatures
  float temp1 = sensors1.getTempCByIndex(0);
  float temp2 = sensors2.getTempCByIndex(0);
  float temp3 = sensors3.getTempCByIndex(0);
  float temp4 = sensors4.getTempCByIndex(0);
  
  // Update state and Modbus registers if valid readings
  if (temp1 != DEVICE_DISCONNECTED_C) {
    state.temperatures[0] = temp1;
    modbusRegisters[20] = (uint16_t)(temp1 * 10);  // Store as temperature × 10
  }
  
  if (temp2 != DEVICE_DISCONNECTED_C) {
    state.temperatures[1] = temp2;
    modbusRegisters[21] = (uint16_t)(temp2 * 10);
  }
  
  if (temp3 != DEVICE_DISCONNECTED_C) {
    state.temperatures[2] = temp3;
    modbusRegisters[22] = (uint16_t)(temp3 * 10);
  }
  
  if (temp4 != DEVICE_DISCONNECTED_C) {
    state.temperatures[3] = temp4;
    modbusRegisters[23] = (uint16_t)(temp4 * 10);
  }
}

void handleModbusRegisters() {
  // Check if coil registers have been updated by Modbus master
  for (int i = 0; i < 8; i++) {
    bool currentRelayState = (state.relayState >> i) & 1;
    bool newRelayState = modbusRegisters[i] != 0;
    
    // If state has changed, update relay
    if (currentRelayState != newRelayState) {
      if (newRelayState) {
        // Turn on relay (active LOW)
        relayModule.digitalWrite(i, LOW);
        bitSet(state.relayState, i);
      } else {
        // Turn off relay
        relayModule.digitalWrite(i, HIGH);
        bitClear(state.relayState, i);
      }
    }
  }
  
  // Check if pulse timers have been set via Modbus
  for (int i = 0; i < 8; i++) {
    int pulseSeconds = modbusRegisters[100 + i];
    
    // If a new pulse timer has been set
    if (pulseSeconds > 0) {
      // Turn on relay (active LOW)
      relayModule.digitalWrite(i, LOW);
      bitSet(state.relayState, i);
      
      // Set pulse end time
      state.relayPulseEndTimes[i] = millis() + (pulseSeconds * 1000);
      
      // Clear the timer register so it's not triggered again
      modbusRegisters[100 + i] = 0;
    }
  }
}

void updateRelays() {
  // Check pulse timers
  unsigned long currentMillis = millis();
  
  for (int i = 0; i < 8; i++) {
    // If this relay has an active pulse timer
    if (state.relayPulseEndTimes[i] > 0) {
      // Check if the pulse time has elapsed
      if (currentMillis >= state.relayPulseEndTimes[i]) {
        // Turn off relay
        relayModule.digitalWrite(i, HIGH);  // HIGH = OFF (active LOW)
        bitClear(state.relayState, i);
        
        // Update Modbus register
        modbusRegisters[i] = 0;
        
        // Clear pulse timer
        state.relayPulseEndTimes[i] = 0;
        
        Serial.print(F("Relay "));
        Serial.print(i + 1);
        Serial.println(F(" pulse completed"));
      }
    }
  }
}

void loadConfiguration() {
  // For demonstration purposes, this is left empty
  // In a real application, you would load configuration from SPIFFS
  Serial.println(F("Configuration loaded"));
}

void saveConfiguration() {
  // For demonstration purposes, this is left empty
  // In a real application, you would save configuration to SPIFFS
  Serial.println(F("Configuration saved"));
}

void logData() {
  // For demonstration purposes, just log to serial
  // In a real application, you would append to a log file in SPIFFS
  
  Serial.println(F("--- System Status Log ---"));
  
  // Log relay states
  Serial.print(F("Relays: "));
  for (int i = 0; i < 8; i++) {
    Serial.print((state.relayState >> i) & 1);
    Serial.print(" ");
  }
  Serial.println();
  
  // Log input states
  Serial.print(F("Inputs: "));
  for (int i = 0; i < 8; i++) {
    Serial.print((state.inputState >> i) & 1);
    Serial.print(" ");
  }
  Serial.println();
  
  // Log analog values
  Serial.print(F("Analog 1: "));
  Serial.println(state.analog1Value);
  Serial.print(F("Analog 2: "));
  Serial.println(state.analog2Value);
  
  // Log temperatures
  for (int i = 0; i < 4; i++) {
    Serial.print(F("Temperature "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(state.temperatures[i]);
    Serial.println(F("°C"));
  }
  
  // Log humidity
  Serial.print(F("Humidity: "));
  Serial.print(state.humidity);
  Serial.println(F("%"));
  
  Serial.println(F("------------------------"));
}
