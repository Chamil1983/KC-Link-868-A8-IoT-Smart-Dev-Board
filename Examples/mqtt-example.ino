/**
 * KC-Link PRO A8 MQTT Example
 * 
 * This example demonstrates how to connect the KC-Link PRO A8 to an MQTT broker
 * and publish sensor data while subscribing to control commands.
 * 
 * Features:
 * - Connects to WiFi and MQTT broker
 * - Publishes temperature, humidity and digital input states
 * - Subscribes to topics for relay control
 * - Handles reconnection to both WiFi and MQTT
 * - JSON formatting for data
 * 
 * Libraries required:
 * - PCF8574 library
 * - OneWire
 * - DallasTemperature
 * - DHT sensor library
 * - PubSubClient
 * - ArduinoJson
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
const int mqtt_port = 1883;
const char* mqtt_user = "YOUR_MQTT_USERNAME";  // Leave empty if no authentication
const char* mqtt_password = "YOUR_MQTT_PASSWORD";  // Leave empty if no authentication
const char* client_id = "kc-link-pro-a8";  // MQTT client ID

// MQTT Topics
const char* topic_temperature = "kc-link/temperature";
const char* topic_humidity = "kc-link/humidity";
const char* topic_inputs = "kc-link/inputs";
const char* topic_relays = "kc-link/relays";
const char* topic_relay_command = "kc-link/relay/set";
const char* topic_status = "kc-link/status";  // For availability reporting

// PCF8574 addresses
#define PCF8574_RELAY_ADDR 0x20  // Address for relay control
#define PCF8574_INPUT_ADDR 0x22  // Address for digital inputs

// Pin definitions for KC-Link PRO A8 V1.4
#define ANALOG_INPUT_1 34
#define ANALOG_INPUT_2 35
#define TEMP_SENSOR_1 14
#define DHTPIN 13       // DHT sensor on port 2
#define DHTTYPE DHT22   // DHT22 sensor type (can also be DHT11 or DHT21)

// Clients for WiFi and MQTT
WiFiClient espClient;
PubSubClient mqtt(espClient);

// I/O objects
PCF8574 relayModule(PCF8574_RELAY_ADDR);
PCF8574 inputModule(PCF8574_INPUT_ADDR);
OneWire oneWire(TEMP_SENSOR_1);
DallasTemperature tempSensor(&oneWire);
DHT dht(DHTPIN, DHTTYPE);

// Variables to track state
byte lastInputState = 0;
byte currentRelayState = 0;
unsigned long lastPublishTime = 0;
const long publishInterval = 60000;  // Publish every 60 seconds
unsigned long lastInputCheckTime = 0;
const long inputCheckInterval = 500;  // Check inputs every 500ms
bool mqttConnected = false;

// Function prototypes
void setupWiFi();
void reconnectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void publishTempHumidity();
void publishInputs();
void publishRelayState();
void publishStatus(const char* status);
void processRelayCommand(byte* payload, unsigned int length);
void checkInputChanges();

void setup() {
  // Initialize serial
  Serial.begin(115200);
  Serial.println(F("KC-Link PRO A8 MQTT Example"));
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize relay module
  if (relayModule.begin()) {
    Serial.println(F("Relay module initialized"));
    for (int i = 0; i < 8; i++) {
      relayModule.pinMode(i, OUTPUT);
      relayModule.digitalWrite(i, HIGH);  // Relays are active LOW, so set HIGH to turn OFF
    }
  } else {
    Serial.println(F("Error: Relay module not found!"));
  }
  
  // Initialize input module
  if (inputModule.begin()) {
    Serial.println(F("Input module initialized"));
    for (int i = 0; i < 8; i++) {
      inputModule.pinMode(i, INPUT);
    }
    lastInputState = inputModule.read8();
  } else {
    Serial.println(F("Error: Input module not found!"));
  }
  
  // Initialize temperature sensor
  tempSensor.begin();
  
  // Initialize DHT sensor
  dht.begin();
  
  // Setup WiFi
  setupWiFi();
  
  // Setup MQTT
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(callback);
  
  // Initial publish of status
  if (mqtt.connected()) {
    publishStatus("online");
  }
  
  Serial.println(F("Setup complete"));
}

void loop() {
  // Handle WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi disconnected. Reconnecting..."));
    setupWiFi();
  }
  
  // Handle MQTT connection
  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  
  // Handle MQTT communication
  mqtt.loop();
  
  // Check for input changes
  unsigned long currentMillis = millis();
  if (currentMillis - lastInputCheckTime > inputCheckInterval) {
    lastInputCheckTime = currentMillis;
    checkInputChanges();
  }
  
  // Publish sensor data periodically
  if (currentMillis - lastPublishTime > publishInterval) {
    lastPublishTime = currentMillis;
    publishTempHumidity();
    publishRelayState();
  }
}

// Setup WiFi connection
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
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
    Serial.println(F("WiFi connection failed. Will retry..."));
  }
}

// Connect/Reconnect to MQTT broker
void reconnectMQTT() {
  // Loop until we're reconnected
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    Serial.print(F("Attempting MQTT connection..."));
    
    // Attempt to connect with last will message for availability
    if (mqtt.connect(client_id, mqtt_user, mqtt_password, topic_status, 1, true, "offline")) {
      Serial.println(F("connected"));
      
      // Subscribe to control topics
      mqtt.subscribe(topic_relay_command);
      
      // Publish initial status
      publishStatus("online");
      mqttConnected = true;
      
      // Publish current state after reconnection
      publishInputs();
      publishRelayState();
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(mqtt.state());
      Serial.println(F(" trying again in 5 seconds"));
      delay(5000);
      attempts++;
    }
  }
  
  if (!mqtt.connected()) {
    mqttConnected = false;
    Serial.println(F("MQTT connection failed. Will retry later."));
  }
}

// MQTT message callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  // Check which topic received a message
  if (strcmp(topic, topic_relay_command) == 0) {
    processRelayCommand(payload, length);
  }
}

// Process relay command
void processRelayCommand(byte* payload, unsigned int length) {
  // Create buffer for the payload (add space for null terminator)
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  
  // Parse JSON command
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  
  // Process the command
  if (doc.containsKey("relay") && doc.containsKey("state")) {
    int relay = doc["relay"].as<int>();
    bool state = doc["state"].as<bool>();
    
    // Validate relay number
    if (relay >= 1 && relay <= 8) {
      // Convert to zero-based index
      int relayIndex = relay - 1;
      
      // Update relay state
      if (state) {
        relayModule.digitalWrite(relayIndex, LOW);  // Active LOW
        bitSet(currentRelayState, relayIndex);
      } else {
        relayModule.digitalWrite(relayIndex, HIGH);
        bitClear(currentRelayState, relayIndex);
      }
      
      Serial.print(F("Set relay "));
      Serial.print(relay);
      Serial.print(F(" to "));
      Serial.println(state ? F("ON") : F("OFF"));
      
      // Publish updated relay state
      publishRelayState();
    }
  } 
  // Toggle command
  else if (doc.containsKey("relay") && doc.containsKey("toggle") && doc["toggle"].as<bool>()) {
    int relay = doc["relay"].as<int>();
    
    // Validate relay number
    if (relay >= 1 && relay <= 8) {
      // Convert to zero-based index
      int relayIndex = relay - 1;
      
      // Get current state
      bool currentState = bitRead(currentRelayState, relayIndex);
      
      // Toggle state
      if (currentState) {
        relayModule.digitalWrite(relayIndex, HIGH);
        bitClear(currentRelayState, relayIndex);
      } else {
        relayModule.digitalWrite(relayIndex, LOW);  // Active LOW
        bitSet(currentRelayState, relayIndex);
      }
      
      Serial.print(F("Toggled relay "));
      Serial.print(relay);
      Serial.print(F(" to "));
      Serial.println(!currentState ? F("ON") : F("OFF"));
      
      // Publish updated relay state
      publishRelayState();
    }
  }
  // All relays command
  else if (doc.containsKey("all")) {
    bool state = doc["all"].as<bool>();
    
    // Set all relays
    for (int i = 0; i < 8; i++) {
      relayModule.digitalWrite(i, state ? LOW : HIGH);  // Active LOW
      if (state) {
        bitSet(currentRelayState, i);
      } else {
        bitClear(currentRelayState, i);
      }
    }
    
    Serial.print(F("Set all relays to "));
    Serial.println(state ? F("ON") : F("OFF"));
    
    // Publish updated relay state
    publishRelayState();
  }
}

// Check for changes in digital inputs
void checkInputChanges() {
  byte currentInputState = inputModule.read8();
  
  // Check if there are changes
  if (currentInputState != lastInputState) {
    Serial.println(F("Input state changed"));
    publishInputs();
    lastInputState = currentInputState;
  }
}

// Publish temperature and humidity data
void publishTempHumidity() {
  // Read temperature from DS18B20
  tempSensor.requestTemperatures();
  float tempC = tempSensor.getTempCByIndex(0);
  
  // Read DHT sensor
  float humidity = dht.readHumidity();
  float dhtTemp = dht.readTemperature();
  
  // Check if any reads failed and exit early
  if (isnan(humidity) || isnan(dhtTemp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  } else {
    // Publish DHT data
    DynamicJsonDocument humDoc(128);
    humDoc["humidity"] = humidity;
    humDoc["sensor"] = "DHT22";
    humDoc["port"] = 2;
    
    char humBuffer[128];
    serializeJson(humDoc, humBuffer);
    mqtt.publish(topic_humidity, humBuffer);
    
    Serial.print(F("Published humidity: "));
    Serial.print(humidity);
    Serial.println(F("%"));
  }
  
  // Publish DS18B20 data
  if (tempC != DEVICE_DISCONNECTED_C) {
    DynamicJsonDocument tempDoc(128);
    tempDoc["temperature"] = tempC;
    tempDoc["sensor"] = "DS18B20";
    tempDoc["port"] = 1;
    
    char tempBuffer[128];
    serializeJson(tempDoc, tempBuffer);
    mqtt.publish(topic_temperature, tempBuffer);
    
    Serial.print(F("Published temperature: "));
    Serial.print(tempC);
    Serial.println(F("°C"));
  } else {
    Serial.println(F("Failed to read from DS18B20 sensor!"));
  }
}

// Publish digital input states
void publishInputs() {
  if (!mqttConnected) return;
  
  byte inputStates = inputModule.read8();
  
  // Create JSON document
  DynamicJsonDocument doc(256);
  JsonArray inputs = doc.createNestedArray("inputs");
  
  // Add each input state to the array
  for (int i = 0; i < 8; i++) {
    bool state = bitRead(inputStates, i);
    JsonObject input = inputs.createNestedObject();
    input["id"] = i + 1;
    input["state"] = state;
  }
  
  // Serialize and publish
  char buffer[256];
  serializeJson(doc, buffer);
  mqtt.publish(topic_inputs, buffer);
  
  Serial.println(F("Published input states"));
}

// Publish relay states
void publishRelayState() {
  if (!mqttConnected) return;
  
  // Create JSON document
  DynamicJsonDocument doc(256);
  JsonArray relays = doc.createNestedArray("relays");
  
  // Add each relay state to the array
  for (int i = 0; i < 8; i++) {
    bool state = bitRead(currentRelayState, i);
    JsonObject relay = relays.createNestedObject();
    relay["id"] = i + 1;
    relay["state"] = state;
  }
  
  // Serialize and publish
  char buffer[256];
  serializeJson(doc, buffer);
  mqtt.publish(topic_relays, buffer);
  
  Serial.println(F("Published relay states"));
}

// Publish availability status
void publishStatus(const char* status) {
  if (mqtt.connected()) {
    mqtt.publish(topic_status, status, true);  // retained message
    Serial.print(F("Published status: "));
    Serial.println(status);
  }
}
