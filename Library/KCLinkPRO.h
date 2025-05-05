// KCLinkPRO.h
// Main header file for the KC-Link PRO A8 library

#ifndef KC_LINK_PRO_H
#define KC_LINK_PRO_H

#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// Board version
#define BOARD_VERSION_1_4  // Comment this line for older versions

// PCF8574 addresses
#define PCF8574_RELAY_ADDR 0x20  // Address for relay control
#define PCF8574_INPUT_ADDR 0x22  // Address for digital inputs

// Pin definitions for KC-Link PRO A8 V1.4
#ifdef BOARD_VERSION_1_4
  #define ANALOG_INPUT_1 34
  #define ANALOG_INPUT_2 35
  #define TEMP_SENSOR_1 14
  #define TEMP_SENSOR_2 13
  #define TEMP_SENSOR_3 32
  #define TEMP_SENSOR_4 33
#else
  // Pin definitions for older versions
  #define ANALOG_INPUT_1 32
  #define ANALOG_INPUT_2 33
  #define TEMP_SENSOR_1 14
  #define TEMP_SENSOR_2 13
  #define TEMP_SENSOR_3 34
  #define TEMP_SENSOR_4 35
#endif

// Sensor types
enum SensorType {
  SENSOR_NONE = 0,
  SENSOR_DS18B20 = 1,
  SENSOR_DHT11 = 2,
  SENSOR_DHT22 = 3,
  SENSOR_DHT21 = 4
};

// Input change callback function type
typedef void (*InputChangeCallback)(int inputNumber, bool newState);

// Analog threshold callback function type
typedef void (*AnalogThresholdCallback)(int inputNumber, float value);

class KCLinkPRO {
public:
  // Constructor
  KCLinkPRO();
  
  // Initialization
  bool begin();
  
  // Relay control
  bool setRelay(int relayNumber, bool state);
  bool toggleRelay(int relayNumber);
  bool getRelayState(int relayNumber);
  bool setAllRelays(byte relayMask);
  
  // Digital inputs
  bool getDigitalInput(int inputNumber);
  byte getAllDigitalInputs();
  void onInputChange(InputChangeCallback callback);
  
  // Analog inputs
  int getAnalogInput(int inputNumber);
  float getAnalogVoltage(int inputNumber);
  void setAnalogThreshold(int inputNumber, float threshold, AnalogThresholdCallback callback);
  
  // Temperature sensors
  bool beginTemperatureSensor(int sensorNumber, SensorType type);
  float getTemperature(int sensorNumber);
  float getHumidity(int sensorNumber);
  
  // Network
  bool connectWiFi(const char* ssid, const char* password);
  bool beginEthernet();
  
private:
  // Internal objects
  PCF8574 _relayModule;
  PCF8574 _inputModule;
  
  // State variables
  byte _relayState;
  byte _lastInputState;
  
  // Analog monitoring
  float _analogThresholds[2];
  bool _analogThresholdExceeded[2];
  AnalogThresholdCallback _analogCallbacks[2];
  
  // Temperature sensors
  OneWire* _oneWireObjects[4];
  DallasTemperature* _dsTemperatureSensors[4];
  DHT* _dhtSensors[4];
  SensorType _sensorTypes[4];
  
  // Callback functions
  InputChangeCallback _inputChangeCallback;
  
  // Internal methods
  void checkInputChanges();
  void checkAnalogThresholds();
  int getPinForSensor(int sensorNumber);
};

#endif // KC_LINK_PRO_H

// KCLinkPRO.cpp
// Main implementation file for the KC-Link PRO A8 library

#include "KCLinkPRO.h"

// Constructor
KCLinkPRO::KCLinkPRO() : 
  _relayModule(PCF8574_RELAY_ADDR),
  _inputModule(PCF8574_INPUT_ADDR),
  _relayState(0),
  _lastInputState(0),
  _inputChangeCallback(nullptr)
{
  // Initialize arrays
  for (int i = 0; i < 2; i++) {
    _analogThresholds[i] = 0.0;
    _analogThresholdExceeded[i] = false;
    _analogCallbacks[i] = nullptr;
  }
  
  for (int i = 0; i < 4; i++) {
    _oneWireObjects[i] = nullptr;
    _dsTemperatureSensors[i] = nullptr;
    _dhtSensors[i] = nullptr;
    _sensorTypes[i] = SENSOR_NONE;
  }
}

// Initialize the board
bool KCLinkPRO::begin() {
  // Initialize I2C
  Wire.begin();
  
  // Initialize relay module
  if (!_relayModule.begin()) {
    Serial.println("Error: Relay module not found!");
    return false;
  }
  
  // Set all relay pins as OUTPUT and initialize to OFF (HIGH)
  for (int i = 0; i < 8; i++) {
    _relayModule.pinMode(i, OUTPUT);
    _relayModule.digitalWrite(i, HIGH); // Relays are active LOW
  }
  
  // Initialize input module
  if (!_inputModule.begin()) {
    Serial.println("Error: Input module not found!");
    return false;
  }
  
  // Set all input pins as INPUT
  for (int i = 0; i < 8; i++) {
    _inputModule.pinMode(i, INPUT);
  }
  
  // Read initial input state
  _lastInputState = _inputModule.read8();
  
  // Configure ADC resolution
  analogReadResolution(12); // 12-bit resolution (0-4095)
  
  return true;
}

// Set relay state (relayNumber: 1-8)
bool KCLinkPRO::setRelay(int relayNumber, bool state) {
  // Validate relay number
  if (relayNumber < 1 || relayNumber > 8) {
    return false;
  }
  
  // Convert to zero-based index
  int relayIndex = relayNumber - 1;
  
  // Update relay state
  if (state) {
    // Turn ON relay (active LOW)
    _relayModule.digitalWrite(relayIndex, LOW);
    bitSet(_relayState, relayIndex);
  } else {
    // Turn OFF relay
    _relayModule.digitalWrite(relayIndex, HIGH);
    bitClear(_relayState, relayIndex);
  }
  
  return true;
}

// Toggle relay state
bool KCLinkPRO::toggleRelay(int relayNumber) {
  // Validate relay number
  if (relayNumber < 1 || relayNumber > 8) {
    return false;
  }
  
  // Convert to zero-based index
  int relayIndex = relayNumber - 1;
  
  // Toggle relay
  bool currentState = bitRead(_relayState, relayIndex);
  return setRelay(relayNumber, !currentState);
}

// Get current relay state
bool KCLinkPRO::getRelayState(int relayNumber) {
  // Validate relay number
  if (relayNumber < 1 || relayNumber > 8) {
    return false;
  }
  
  // Convert to zero-based index
  int relayIndex = relayNumber - 1;
  
  // Return relay state
  return bitRead(_relayState, relayIndex);
}

// Set all relays at once using a bitmask
bool KCLinkPRO::setAllRelays(byte relayMask) {
  // Update all relays
  for (int i = 0; i < 8; i++) {
    bool state = bitRead(relayMask, i);
    _relayModule.digitalWrite(i, state ? LOW : HIGH); // Relays are active LOW
  }
  
  // Update state variable
  _relayState = relayMask;
  
  return true;
}

// Read a single digital input (inputNumber: 1-8)
bool KCLinkPRO::getDigitalInput(int inputNumber) {
  // Validate input number
  if (inputNumber < 1 || inputNumber > 8) {
    return false;
  }
  
  // Convert to zero-based index
  int inputIndex = inputNumber - 1;
  
  // Read and return input state
  return _inputModule.digitalRead(inputIndex);
}

// Read all digital inputs at once
byte KCLinkPRO::getAllDigitalInputs() {
  // Read all inputs
  return _inputModule.read8();
}

// Set up an input change callback
void KCLinkPRO::onInputChange(InputChangeCallback callback) {
  _inputChangeCallback = callback;
}

// Check for input changes (should be called in loop)
void KCLinkPRO::checkInputChanges() {
  // Skip if no callback is registered
  if (_inputChangeCallback == nullptr) {
    return;
  }
  
  // Read current input state
  byte currentInputState = _inputModule.read8();
  
  // Check if there are changes
  if (currentInputState != _lastInputState) {
    // Check each input
    for (int i = 0; i < 8; i++) {
      bool currentState = bitRead(currentInputState, i);
      bool lastState = bitRead(_lastInputState, i);
      
      // If state changed, call the callback
      if (currentState != lastState) {
        _inputChangeCallback(i + 1, currentState);
      }
    }
    
    // Update last state
    _lastInputState = currentInputState;
  }
}

// Read raw analog value (0-4095) from input (inputNumber: 1-2)
int KCLinkPRO::getAnalogInput(int inputNumber) {
  // Validate input number
  if (inputNumber < 1 || inputNumber > 2) {
    return -1;
  }
  
  // Select the appropriate pin
  int pin;
  if (inputNumber == 1) {
    pin = ANALOG_INPUT_1;
  } else {
    pin = ANALOG_INPUT_2;
  }
  
  // Read and return analog value
  return analogRead(pin);
}

// Read voltage (0-5V) from input
float KCLinkPRO::getAnalogVoltage(int inputNumber) {
  // Get raw analog value
  int rawValue = getAnalogInput(inputNumber);
  
  // Convert to voltage (0-5V)
  if (rawValue < 0) {
    return -1.0;
  }
  
  return rawValue * 5.0 / 4095.0;
}

// Set up analog threshold callback
void KCLinkPRO::setAnalogThreshold(int inputNumber, float threshold, AnalogThresholdCallback callback) {
  // Validate input number
  if (inputNumber < 1 || inputNumber > 2) {
    return;
  }
  
  // Store threshold and callback
  int index = inputNumber - 1;
  _analogThresholds[index] = threshold;
  _analogCallbacks[index] = callback;
  _analogThresholdExceeded[index] = false;
}

// Check analog thresholds (should be called in loop)
void KCLinkPRO::checkAnalogThresholds() {
  // Check both analog inputs
  for (int i = 0; i < 2; i++) {
    // Skip if no callback is registered
    if (_analogCallbacks[i] == nullptr) {
      continue;
    }
    
    // Read current voltage
    float voltage = getAnalogVoltage(i + 1);
    
    // Check threshold
    bool exceeded = (voltage >= _analogThresholds[i]);
    
    // If threshold state changed, call the callback
    if (exceeded != _analogThresholdExceeded[i]) {
      _analogThresholdExceeded[i] = exceeded;
      _analogCallbacks[i](i + 1, voltage);
    }
  }
}

// Get pin for temperature sensor
int KCLinkPRO::getPinForSensor(int sensorNumber) {
  switch (sensorNumber) {
    case 1: return TEMP_SENSOR_1;
    case 2: return TEMP_SENSOR_2;
    case 3: return TEMP_SENSOR_3;
    case 4: return TEMP_SENSOR_4;
    default: return -1;
  }
}

// Initialize temperature sensor
bool KCLinkPRO::beginTemperatureSensor(int sensorNumber, SensorType type) {
  // Validate sensor number
  if (sensorNumber < 1 || sensorNumber > 4) {
    return false;
  }
  
  // Get sensor pin
  int pin = getPinForSensor(sensorNumber);
  if (pin < 0) {
    return false;
  }
  
  // Store sensor type
  int index = sensorNumber - 1;
  _sensorTypes[index] = type;
  
  // Initialize sensor based on type
  switch (type) {
    case SENSOR_DS18B20:
      // Clean up existing objects if any
      if (_oneWireObjects[index] != nullptr) {
        delete _oneWireObjects[index];
      }
      if (_dsTemperatureSensors[index] != nullptr) {
        delete _dsTemperatureSensors[index];
      }
      
      // Create new objects
      _oneWireObjects[index] = new OneWire(pin);
      _dsTemperatureSensors[index] = new DallasTemperature(_oneWireObjects[index]);
      _dsTemperatureSensors[index]->begin();
      break;
      
    case SENSOR_DHT11:
    case SENSOR_DHT22:
    case SENSOR_DHT21:
      // Clean up existing object if any
      if (_dhtSensors[index] != nullptr) {
        delete _dhtSensors[index];
      }
      
      // Create new object with appropriate type
      uint8_t dhtType;
      if (type == SENSOR_DHT11) {
        dhtType = DHT11;
      } else if (type == SENSOR_DHT22) {
        dhtType = DHT22;
      } else {
        dhtType = DHT21;
      }
      
      _dhtSensors[index] = new DHT(pin, dhtType);
      _dhtSensors[index]->begin();
      break;
      
    default:
      // Unsupported sensor type
      _sensorTypes[index] = SENSOR_NONE;
      return false;
  }
  
  return true;
}

// Read temperature from a specific sensor
float KCLinkPRO::getTemperature(int sensorNumber) {
  // Validate sensor number
  if (sensorNumber < 1 || sensorNumber > 4) {
    return -999.0;  // Error value
  }
  
  // Get sensor index
  int index = sensorNumber - 1;
  
  // Check if sensor is initialized
  if (_sensorTypes[index] == SENSOR_NONE) {
    return -999.0;  // Error value
  }
  
  // Read temperature based on sensor type
  float temperature = -999.0;
  
  switch (_sensorTypes[index]) {
    case SENSOR_DS18B20:
      if (_dsTemperatureSensors[index] != nullptr) {
        _dsTemperatureSensors[index]->requestTemperatures();
        temperature = _dsTemperatureSensors[index]->getTempCByIndex(0);
      }
      break;
      
    case SENSOR_DHT11:
    case SENSOR_DHT22:
    case SENSOR_DHT21:
      if (_dhtSensors[index] != nullptr) {
        temperature = _dhtSensors[index]->readTemperature();
      }
      break;
      
    default:
      temperature = -999.0;  // Error value
  }
  
  return temperature;
}

// Read humidity from a specific sensor (for DHT sensors)
float KCLinkPRO::getHumidity(int sensorNumber) {
  // Validate sensor number
  if (sensorNumber < 1 || sensorNumber > 4) {
    return -999.0;  // Error value
  }
  
  // Get sensor index
  int index = sensorNumber - 1;
  
  // Check if sensor is a DHT type
  if (_sensorTypes[index] != SENSOR_DHT11 && 
      _sensorTypes[index] != SENSOR_DHT22 && 
      _sensorTypes[index] != SENSOR_DHT21) {
    return -999.0;  // Error value or not a humidity sensor
  }
  
  // Read humidity
  float humidity = -999.0;
  
  if (_dhtSensors[index] != nullptr) {
    humidity = _dhtSensors[index]->readHumidity();
  }
  
  return humidity;
}

// Connect to WiFi
bool KCLinkPRO::connectWiFi(const char* ssid, const char* password) {
  // Start WiFi connection
  WiFi.begin(ssid, password);
  
  // Wait for connection (with timeout)
  int timeout = 20;  // 20 seconds timeout
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    timeout--;
  }
  
  // Return connection status
  return (WiFi.status() == WL_CONNECTED);
}

// Start Ethernet connection
bool KCLinkPRO::beginEthernet() {
  // Note: Ethernet initialization depends on the specific Ethernet library
  // being used. This is a placeholder function.
  
  // For KC868-A8, the Ethernet chip is LAN8720A connected to ESP32
  // The actual implementation would use the appropriate Ethernet library
  
  return true;  // Placeholder return
}