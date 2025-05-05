# KC-Link PRO A8 API Reference

## Overview

This document provides a comprehensive reference for the KC-Link PRO A8 Arduino library. The library provides a simple and intuitive API for controlling all features of the KC-Link PRO A8 board.

## Class: KCLinkPRO

The main class that encapsulates all functionality of the KC-Link PRO A8 board.

### Constructor

```cpp
KCLinkPRO()
```

Creates a new KCLinkPRO object with default settings.

### Initialization

```cpp
bool begin()
```

Initializes the KC-Link PRO A8 board, setting up I2C communication, configuring I/O expanders, and preparing all peripherals.

**Returns:**
- `true` if initialization succeeds
- `false` if there's an error (e.g., I2C communication failure)

**Example:**
```cpp
KCLinkPRO board;

void setup() {
  Serial.begin(115200);
  
  if (!board.begin()) {
    Serial.println("Failed to initialize KC-Link PRO A8 board!");
    while (1);
  }
  
  Serial.println("Board initialized successfully.");
}
```

## Relay Control

### Set Relay State

```cpp
bool setRelay(int relayNumber, bool state)
```

Sets the state of a specific relay.

**Parameters:**
- `relayNumber`: The relay to control (1-8)
- `state`: The desired state (`true` = ON, `false` = OFF)

**Returns:**
- `true` if the operation succeeds
- `false` if the relay number is invalid

**Example:**
```cpp
// Turn ON relay 3
board.setRelay(3, true);

// Turn OFF relay 5
board.setRelay(5, false);
```

### Toggle Relay State

```cpp
bool toggleRelay(int relayNumber)
```

Toggles the state of a specific relay (ON to OFF, or OFF to ON).

**Parameters:**
- `relayNumber`: The relay to toggle (1-8)

**Returns:**
- `true` if the operation succeeds
- `false` if the relay number is invalid

**Example:**
```cpp
// Toggle relay 2 (if it's ON, turn it OFF; if it's OFF, turn it ON)
board.toggleRelay(2);
```

### Get Relay State

```cpp
bool getRelayState(int relayNumber)
```

Gets the current state of a specific relay.

**Parameters:**
- `relayNumber`: The relay to check (1-8)

**Returns:**
- `true` if the relay is ON
- `false` if the relay is OFF or the relay number is invalid

**Example:**
```cpp
// Check if relay 4 is on
if (board.getRelayState(4)) {
  Serial.println("Relay 4 is ON");
} else {
  Serial.println("Relay 4 is OFF");
}
```

### Set All Relays

```cpp
bool setAllRelays(byte relayMask)
```

Sets the state of all relays at once using a bitmask, where each bit corresponds to a relay.

**Parameters:**
- `relayMask`: Bitmask where bit 0 controls relay 1, bit 1 controls relay 2, etc.

**Returns:**
- `true` if the operation succeeds
- `false` if there's an error

**Example:**
```cpp
// Turn ON relays 1, 3, 5, 7 and turn OFF relays 2, 4, 6, 8
board.setAllRelays(0b01010101);

// Turn ON all relays
board.setAllRelays(0xFF);

// Turn OFF all relays
board.setAllRelays(0x00);
```

## Digital Inputs

### Get Digital Input State

```cpp
bool getDigitalInput(int inputNumber)
```

Reads the state of a specific digital input.

**Parameters:**
- `inputNumber`: The input to read (1-8)

**Returns:**
- `true` if the input is active (triggered)
- `false` if the input is inactive or the input number is invalid

**Example:**
```cpp
// Check if input 2 is triggered
if (board.getDigitalInput(2)) {
  Serial.println("Input 2 is triggered");
} else {
  Serial.println("Input 2 is not triggered");
}
```

### Get All Digital Inputs

```cpp
byte getAllDigitalInputs()
```

Reads the state of all digital inputs at once, returning a bitmask.

**Returns:**
- A byte where each bit represents an input state (bit 0 = input 1, bit 1 = input 2, etc.)

**Example:**
```cpp
// Read all inputs and print their states
byte inputStates = board.getAllDigitalInputs();
for (int i = 0; i < 8; i++) {
  bool state = bitRead(inputStates, i);
  Serial.print("Input ");
  Serial.print(i + 1);
  Serial.print(": ");
  Serial.println(state ? "ON" : "OFF");
}
```

### Register Input Change Callback

```cpp
void onInputChange(void (*callback)(int inputNumber, bool newState))
```

Registers a callback function that is called whenever a digital input changes state.

**Parameters:**
- `callback`: A function pointer to the callback function

**Callback Parameters:**
- `inputNumber`: The input that changed (1-8)
- `newState`: The new state of the input (`true` = active, `false` = inactive)

**Example:**
```cpp
void inputChangeHandler(int inputNumber, bool newState) {
  Serial.print("Input ");
  Serial.print(inputNumber);
  Serial.print(" changed to ");
  Serial.println(newState ? "ON" : "OFF");
  
  // Toggle corresponding relay when input changes
  board.toggleRelay(inputNumber);
}

void setup() {
  Serial.begin(115200);
  board.begin();
  
  // Register the callback
  board.onInputChange(inputChangeHandler);
}

void loop() {
  // The library will call inputChangeHandler when an input changes
  board.checkInputChanges();
  delay(10);
}
```

## Analog Inputs

### Get Analog Input Raw Value

```cpp
int getAnalogInput(int inputNumber)
```

Reads the raw ADC value from a specific analog input.

**Parameters:**
- `inputNumber`: The analog input to read (1-2)

**Returns:**
- The raw ADC value (0-4095)
- `-1` if the input number is invalid

**Example:**
```cpp
// Read raw value from analog input 1
int rawValue = board.getAnalogInput(1);
Serial.print("Analog Input 1 Raw Value: ");
Serial.println(rawValue);
```

### Get Analog Input Voltage

```cpp
float getAnalogVoltage(int inputNumber)
```

Reads the voltage from a specific analog input.

**Parameters:**
- `inputNumber`: The analog input to read (1-2)

**Returns:**
- The voltage (0-5V)
- `-1.0` if the input number is invalid

**Example:**
```cpp
// Read voltage from analog input 2
float voltage = board.getAnalogVoltage(2);
Serial.print("Analog Input 2 Voltage: ");
Serial.print(voltage, 2);
Serial.println("V");
```

### Register Analog Threshold Callback

```cpp
void setAnalogThreshold(int inputNumber, float threshold, void (*callback)(int inputNumber, float value))
```

Registers a callback function that is called when an analog input crosses a threshold value.

**Parameters:**
- `inputNumber`: The analog input to monitor (1-2)
- `threshold`: The threshold voltage (0-5V)
- `callback`: A function pointer to the callback function

**Callback Parameters:**
- `inputNumber`: The analog input that crossed the threshold (1-2)
- `value`: The current voltage value

**Example:**
```cpp
void analogThresholdHandler(int inputNumber, float value) {
  Serial.print("Analog Input ");
  Serial.print(inputNumber);
  Serial.print(" crossed threshold. Current value: ");
  Serial.print(value, 2);
  Serial.println("V");
  
  // Turn on relay when threshold is exceeded
  board.setRelay(inputNumber, true);
}

void setup() {
  Serial.begin(115200);
  board.begin();
  
  // Set threshold of 2.5V for analog input 1
  board.setAnalogThreshold(1, 2.5, analogThresholdHandler);
}

void loop() {
  // The library will call analogThresholdHandler when the threshold is crossed
  board.checkAnalogThresholds();
  delay(10);
}
```

## Temperature Sensors

### Initialize Temperature Sensor

```cpp
bool beginTemperatureSensor(int sensorNumber, SensorType type)
```

Initializes a temperature sensor connected to one of the sensor ports.

**Parameters:**
- `sensorNumber`: The sensor port to use (1-4)
- `type`: The type of sensor (`SENSOR_DS18B20`, `SENSOR_DHT11`, `SENSOR_DHT22`, or `SENSOR_DHT21`)

**Returns:**
- `true` if initialization succeeds
- `false` if there's an error

**Example:**
```cpp
// Initialize DS18B20 on sensor port 1
board.beginTemperatureSensor(1, SENSOR_DS18B20);

// Initialize DHT22 on sensor port 2
board.beginTemperatureSensor(2, SENSOR_DHT22);
```

### Get Temperature

```cpp
float getTemperature(int sensorNumber)
```

Reads the temperature from a specific sensor.

**Parameters:**
- `sensorNumber`: The sensor to read (1-4)

**Returns:**
- The temperature in degrees Celsius
- `-999.0` if there's an error or the sensor is not initialized

**Example:**
```cpp
// Read temperature from sensor 1
float temperature = board.getTemperature(1);
if (temperature > -999.0) {
  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println("°C");
} else {
  Serial.println("Error reading temperature");
}
```

### Get Humidity

```cpp
float getHumidity(int sensorNumber)
```

Reads the humidity from a specific DHT sensor.

**Parameters:**
- `sensorNumber`: The sensor to read (1-4)

**Returns:**
- The humidity percentage (0-100%)
- `-999.0` if there's an error, the sensor is not initialized, or the sensor is not a humidity sensor

**Example:**
```cpp
// Read humidity from sensor 2 (DHT22)
float humidity = board.getHumidity(2);
if (humidity > -999.0) {
  Serial.print("Humidity: ");
  Serial.print(humidity, 1);
  Serial.println("%");
} else {
  Serial.println("Error reading humidity");
}
```

## Network Communication

### Connect to WiFi

```cpp
bool connectWiFi(const char* ssid, const char* password)
```

Connects the board to a WiFi network.

**Parameters:**
- `ssid`: The WiFi network name
- `password`: The WiFi password

**Returns:**
- `true` if connection succeeds
- `false` if connection fails

**Example:**
```cpp
// Connect to WiFi
if (board.connectWiFi("MyNetwork", "MyPassword")) {
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
} else {
  Serial.println("Failed to connect to WiFi");
}
```

### Start Ethernet Connection

```cpp
bool beginEthernet()
```

Initializes the Ethernet connection.

**Returns:**
- `true` if initialization succeeds
- `false` if there's an error

**Example:**
```cpp
// Initialize Ethernet
if (board.beginEthernet()) {
  Serial.println("Ethernet initialized");
  // The actual IP address depends on your network configuration
} else {
  Serial.println("Failed to initialize Ethernet");
}
```

## Utility Functions

### Check Input Changes

```cpp
void checkInputChanges()
```

Checks for digital input state changes and triggers callbacks if registered.

This function should be called regularly in the `loop()` function if input change callbacks are used.

**Example:**
```cpp
void loop() {
  // Check for input changes
  board.checkInputChanges();
  
  // Your other code here
  
  delay(10);
}
```

### Check Analog Thresholds

```cpp
void checkAnalogThresholds()
```

Checks if analog inputs have crossed their thresholds and triggers callbacks if registered.

This function should be called regularly in the `loop()` function if analog threshold callbacks are used.

**Example:**
```cpp
void loop() {
  // Check analog thresholds
  board.checkAnalogThresholds();
  
  // Your other code here
  
  delay(10);
}
```

## Constants

### Sensor Types

```cpp
enum SensorType {
  SENSOR_NONE = 0,
  SENSOR_DS18B20 = 1,
  SENSOR_DHT11 = 2,
  SENSOR_DHT22 = 3,
  SENSOR_DHT21 = 4
};
```

Used to specify the type of temperature/humidity sensor when calling `beginTemperatureSensor()`.

## Type Definitions

```cpp
// Input change callback function type
typedef void (*InputChangeCallback)(int inputNumber, bool newState);

// Analog threshold callback function type
typedef void (*AnalogThresholdCallback)(int inputNumber, float value);
```

These type definitions are used for callback functions.

## Complete Example

```cpp
#include <KCLinkPRO.h>

KCLinkPRO board;

// Callback for digital input changes
void inputChangeHandler(int inputNumber, bool newState) {
  Serial.print("Input ");
  Serial.print(inputNumber);
  Serial.print(" changed to ");
  Serial.println(newState ? "ON" : "OFF");
  
  // Mirror input state to corresponding relay
  board.setRelay(inputNumber, newState);
}

// Callback for analog threshold
void analogThresholdHandler(int inputNumber, float value) {
  Serial.print("Analog Input ");
  Serial.print(inputNumber);
  Serial.print(" threshold crossed. Value: ");
  Serial.print(value, 2);
  Serial.println("V");
}

void setup() {
  Serial.begin(115200);
  Serial.println("KC-Link PRO A8 Example");
  
  // Initialize the board
  if (!board.begin()) {
    Serial.println("Failed to initialize board!");
    while (1);
  }
  
  // Initialize a DS18B20 temperature sensor on port 1
  board.beginTemperatureSensor(1, SENSOR_DS18B20);
  
  // Initialize a DHT22 humidity sensor on port 2
  board.beginTemperatureSensor(2, SENSOR_DHT22);
  
  // Set up callback for input changes
  board.onInputChange(inputChangeHandler);
  
  // Set up callback for analog threshold
  board.setAnalogThreshold(1, 2.5, analogThresholdHandler);
  
  // Connect to WiFi
  board.connectWiFi("MyNetwork", "MyPassword");
  
  // Turn on relay 1 initially
  board.setRelay(1, true);
  
  Serial.println("Setup complete");
}

void loop() {
  // Check for input changes
  board.checkInputChanges();
  
  // Check analog thresholds
  board.checkAnalogThresholds();
  
  // Read and print temperature every 5 seconds
  static unsigned long lastTempUpdate = 0;
  if (millis() - lastTempUpdate > 5000) {
    float temperature = board.getTemperature(1);
    float humidity = board.getHumidity(2);
    
    Serial.print("Temperature: ");
    Serial.print(temperature, 1);
    Serial.println("°C");
    
    Serial.print("Humidity: ");
    Serial.print(humidity, 1);
    Serial.println("%");
    
    lastTempUpdate = millis();
  }
  
  // Small delay to avoid excessive CPU usage
  delay(10);
}
```

This example initializes the board, sets up callbacks for input changes and analog thresholds, initializes temperature and humidity sensors, and periodically reads and displays sensor values.

## Error Handling

The library uses return values to indicate success or failure for most functions. For sensor readings, special values (like `-999.0`) are used to indicate errors.

Always check return values to ensure proper error handling in your application.

## Thread Safety

The KC-Link PRO A8 library is not thread-safe. If you're using it in a multi-threaded environment (e.g., with FreeRTOS on ESP32), make sure to implement proper synchronization mechanisms.

## Power Considerations

Activating multiple relays simultaneously can cause a current surge. If you're powering the board from a limited power source, consider adding delays between relay activations.

## Further Resources

For more information and advanced usage, please refer to:
- [Example sketches](../examples/)
- [Hardware documentation](./hardware.md)
- [Troubleshooting guide](./troubleshooting.md)
