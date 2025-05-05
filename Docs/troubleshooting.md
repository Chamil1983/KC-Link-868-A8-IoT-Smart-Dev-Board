# KC-Link PRO A8 Troubleshooting Guide

This guide provides solutions for common problems encountered with the KC-Link PRO A8 board. If you're experiencing issues, please follow the troubleshooting steps below.

## Table of Contents

- [Power Issues](#power-issues)
- [Communication Problems](#communication-problems)
- [Relay Control Issues](#relay-control-issues)
- [Digital Input Problems](#digital-input-problems)
- [Analog Input Problems](#analog-input-problems)
- [Temperature Sensor Issues](#temperature-sensor-issues)
- [Network Connectivity Issues](#network-connectivity-issues)
- [Software and Programming](#software-and-programming)
- [Factory Reset](#factory-reset)
- [Support and Assistance](#support-and-assistance)

## Power Issues

### Board Not Powering Up

**Symptoms:**
- No power LED illumination
- No response from the board

**Troubleshooting Steps:**

1. **Check Power Supply:**
   - Verify that the power supply is providing 9-24V DC
   - Measure voltage at the power input terminals
   - Ensure correct polarity (+ and - connections)

2. **Check Power LED:**
   - If the power LED is off, check the power supply fuse if present
   - Inspect the board for any visible damage

3. **Inspect Power Regulator:**
   - Check if the 5V regulator (XL1509-5) is overheating
   - Measure the 5V output using a multimeter

**Solution:**
- Replace power supply if voltage is incorrect
- If the regulator is damaged, the board may need repair

### Intermittent Power Issues

**Symptoms:**
- Board resets unexpectedly
- Relays activate/deactivate randomly

**Troubleshooting Steps:**

1. **Check Power Supply Load:**
   - Ensure your power supply can handle the load (min 1A recommended)
   - Add a larger capacitor (1000µF/25V) to the power input for stabilization

2. **Measure Voltage During Operation:**
   - Monitor voltage when activating multiple relays
   - Voltage should not drop below 9V

**Solution:**
- Use a power supply with higher current rating
- Add additional filtering capacitors
- Sequence relay operations instead of activating multiple relays simultaneously

## Communication Problems

### I²C Communication Failure

**Symptoms:**
- "Error: Relay module not found!" or "Error: Input module not found!" messages
- Relays or inputs not responding

**Troubleshooting Steps:**

1. **Check I²C Connections:**
   - Verify SDA and SCL connections are not loose
   - Check for solder bridges or broken traces on the PCB

2. **Verify I²C Addresses:**
   - Run an I²C scanner sketch to confirm device addresses
   - Relay PCF8574 should be at address 0x20
   - Input PCF8574 should be at address 0x22

```cpp
// I²C Scanner Example
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);
  Serial.println("I²C Scanner");
}

void loop() {
  byte error, address;
  int devices = 0;
  
  Serial.println("Scanning...");
  
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I²C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println();
      devices++;
    }
  }
  
  if (devices == 0) {
    Serial.println("No I²C devices found");
  }
  
  delay(5000);
}
```

3. **Check for I²C Conflicts:**
   - Disconnect any external I²C devices that might cause conflicts
   - Try changing I²C speed (in Wire.begin())

**Solution:**
- Verify correct PCF8574 addresses in your code
- Replace the PCF8574 IC if damaged
- Check and repair any damaged traces on the PCB

### USB Connection Issues

**Symptoms:**
- Cannot connect to the board via USB
- Arduino IDE doesn't recognize the COM port

**Troubleshooting Steps:**

1. **Check USB Cable:**
   - Try a different USB cable
   - Ensure the cable supports data transfer (not charge-only)

2. **Check USB Driver:**
   - Install CH340C drivers if not already installed
   - Windows: Check Device Manager for errors

3. **Try Different USB Ports:**
   - Some USB hubs may not provide enough power
   - Connect directly to a computer USB port

**Solution:**
- Install proper CH340C drivers for your operating system
- Use a known good USB cable
- If the CH340C chip is damaged, the board will need repair

## Relay Control Issues

### Relays Not Activating

**Symptoms:**
- Relays don't click when activated in software
- Relay status LEDs don't illuminate

**Troubleshooting Steps:**

1. **Check Relay Power:**
   - Verify 12V is present at the relay coils
   - Check if ULN2003A driver IC is getting warm

2. **Test With Simple Sketch:**
   - Use the basic relay test sketch to toggle relays

```cpp
#include <Wire.h>
#include <PCF8574.h>

#define PCF8574_RELAY_ADDR 0x20

PCF8574 relayModule(PCF8574_RELAY_ADDR);

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  if (!relayModule.begin()) {
    Serial.println("PCF8574 relay module not found!");
    while(1);
  }
  
  // Set all pins as OUTPUT
  for (int i = 0; i < 8; i++) {
    relayModule.pinMode(i, OUTPUT);
    relayModule.digitalWrite(i, HIGH); // Relays are active LOW
  }
}

void loop() {
  // Test each relay
  for (int relay = 0; relay < 8; relay++) {
    Serial.print("Testing Relay ");
    Serial.println(relay + 1);
    
    // Turn ON
    relayModule.digitalWrite(relay, LOW);
    delay(1000);
    
    // Turn OFF
    relayModule.digitalWrite(relay, HIGH);
    delay(500);
  }
}
```

3. **Check PCF8574 Outputs:**
   - Use a multimeter to check if PCF8574 outputs change state
   - Measure voltage at ULN2003A inputs

**Solution:**
- Verify correct I²C address for relay PCF8574
- Check ULN2003A driver and replace if faulty
- Check relay connections and replace relay if damaged

### Relays Chattering or Unstable

**Symptoms:**
- Relays rapidly switch on/off
- Relays make buzzing sounds

**Troubleshooting Steps:**

1. **Check Power Supply:**
   - Ensure power supply can provide enough current
   - Monitor voltage when multiple relays are active

2. **Check for EMI/RFI:**
   - Keep high voltage wires away from signal wires
   - Add snubber circuits for inductive loads

**Solution:**
- Use a more capable power supply
- Add decoupling capacitors (100nF) near the relay driver
- Add flyback diodes for inductive loads

## Digital Input Problems

### Inputs Not Detecting

**Symptoms:**
- Digital inputs don't respond to external signals
- Input status LEDs don't illuminate

**Troubleshooting Steps:**

1. **Check Input Voltage:**
   - Verify external voltage is between 5-24V DC
   - Check polarity of input connections

2. **Test Input Circuit:**
   - Use a multimeter to verify voltage at optocoupler inputs
   - Check if input LED indicators light up

3. **Verify I²C Communication:**
   - Run I²C scanner to verify PCF8574 address (0x22)
   - Test with simple input reading sketch

```cpp
#include <Wire.h>
#include <PCF8574.h>

#define PCF8574_INPUT_ADDR 0x22

PCF8574 inputModule(PCF8574_INPUT_ADDR);

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  if (!inputModule.begin()) {
    Serial.println("PCF8574 input module not found!");
    while(1);
  }
  
  // Set all pins as INPUT
  for (int i = 0; i < 8; i++) {
    inputModule.pinMode(i, INPUT);
  }
}

void loop() {
  // Read all inputs
  byte inputs = inputModule.read8();
  
  // Display input states
  Serial.print("Inputs: ");
  for (int i = 0; i < 8; i++) {
    bool state = bitRead(inputs, i);
    Serial.print(state ? "1" : "0");
  }
  Serial.println();
  
  delay(500);
}
```

**Solution:**
- Verify correct voltage and polarity for inputs
- Check optocoupler and replace if damaged
- Verify correct PCF8574 address in code

### Input Status Unstable

**Symptoms:**
- Inputs fluctuate or give false readings
- Intermittent input detection

**Troubleshooting Steps:**

1. **Check Wiring:**
   - Ensure proper connections and wire gauge
   - Keep input wires away from high voltage or noisy sources

2. **Add Debounce:**
   - Implement software debounce for mechanical contacts

```cpp
// Add to your code for debounce
#define DEBOUNCE_DELAY 50  // ms

byte lastInputState = 0;
unsigned long lastDebounceTime[8] = {0};
byte debouncedState = 0;

void loop() {
  byte currentInputs = inputModule.read8();
  
  // Check each input for changes
  for (int i = 0; i < 8; i++) {
    bool currentState = bitRead(currentInputs, i);
    bool lastState = bitRead(lastInputState, i);
    
    // If state changed, reset debounce timer
    if (currentState != lastState) {
      lastDebounceTime[i] = millis();
    }
    
    // If enough time has passed, accept the change
    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      if (currentState != bitRead(debouncedState, i)) {
        if (currentState) {
          bitSet(debouncedState, i);
        } else {
          bitClear(debouncedState, i);
        }
        
        // Process the debounced input change here
        Serial.print("Input ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(currentState ? "ON" : "OFF");
      }
    }
  }
  
  lastInputState = currentInputs;
  delay(10);
}
```

**Solution:**
- Add pull-up or pull-down resistors for stability
- Implement software debouncing
- Use shielded cables for input connections

## Analog Input Problems

### Incorrect Analog Readings

**Symptoms:**
- Analog readings are inaccurate or inconsistent
- Values drift or fluctuate

**Troubleshooting Steps:**

1. **Check Input Range:**
   - Verify input voltage is within 0-5V range
   - Use a multimeter to confirm actual voltage

2. **Calibrate Readings:**
   - Apply known voltages and record ADC values
   - Create a calibration curve or scaling factor

```cpp
// Example of analog input calibration
// Apply known voltages (e.g., 0V, 1V, 2.5V, 5V) and record ADC values
// Then adjust the formula accordingly

float getVoltage(int adcValue) {
  // Example calibration formula:
  return (adcValue * 5.0 / 4095.0) * 0.992 + 0.01;
}
```

3. **Filter Noisy Readings:**
   - Implement averaging to stabilize readings

```cpp
// Simple averaging filter
#define SAMPLES 10

float getFilteredVoltage(int pin) {
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  int average = sum / SAMPLES;
  return (average * 5.0 / 4095.0);
}
```

**Solution:**
- Implement software filtering and calibration
- Add hardware filtering (RC filter) if needed
- Use shielded cables for analog inputs

### No Analog Readings

**Symptoms:**
- Analog input always reads 0 or maximum value
- No response to voltage changes

**Troubleshooting Steps:**

1. **Check Pin Assignments:**
   - Verify correct GPIO pins for analog inputs
   - For V1.4, should be GPIO34 and GPIO35

2. **Check Input Circuit:**
   - Measure voltage at analog input pins
   - Verify op-amp buffer circuits if present

3. **Test Direct GPIO Readings:**
   - Bypass library and read directly from GPIO

```cpp
void setup() {
  Serial.begin(115200);
  
  // Set ADC resolution
  analogReadResolution(12);
}

void loop() {
  // Read analog inputs directly
  int analog1 = analogRead(34);  // Use 32 for older versions
  int analog2 = analogRead(35);  // Use 33 for older versions
  
  Serial.print("Analog 1: ");
  Serial.print(analog1);
  Serial.print(" (");
  Serial.print(analog1 * 5.0 / 4095.0, 2);
  Serial.print("V), Analog 2: ");
  Serial.print(analog2);
  Serial.print(" (");
  Serial.print(analog2 * 5.0 / 4095.0, 2);
  Serial.println("V)");
  
  delay(500);
}
```

**Solution:**
- Verify correct pin assignments for your board version
- Check for hardware damage to analog input circuit
- Replace op-amp buffer if damaged

## Temperature Sensor Issues

### Sensor Not Detected

**Symptoms:**
- Temperature readings return error values
- "No devices found" message in serial monitor

**Troubleshooting Steps:**

1. **Check Wiring:**
   - Verify correct connections (VCC, GND, Data)
   - Check for loose connections or damaged cables

2. **Verify Sensor Type:**
   - Confirm sensor type matches library configuration
   - DS18B20 requires different library than DHT sensors

3. **Check Pull-up Resistor:**
   - Ensure data line has a 4.7kΩ pull-up resistor
   - Add external pull-up if needed

**Solution:**
- Verify correct sensor port and connections
- Replace sensor if damaged
- Ensure correct libraries are installed

### Inaccurate Temperature Readings

**Symptoms:**
- Temperature readings are far from expected values
- Readings fluctuate wildly

**Troubleshooting Steps:**

1. **Check Sensor Placement:**
   - Ensure sensor is not near heat sources
   - Verify sensor is not in direct sunlight

2. **Check Power Supply:**
   - Ensure stable power to the sensor
   - Try powering the sensor from an external 3.3V source

3. **Implement Filtering:**
   - Add averaging to stabilize readings

```cpp
float getFilteredTemperature(int sensorNumber) {
  float sum = 0;
  int validReadings = 0;
  
  // Take multiple readings
  for (int i = 0; i < 5; i++) {
    float temp = board.getTemperature(sensorNumber);
    if (temp > -100) {  // Valid reading
      sum += temp;
      validReadings++;
    }
    delay(100);
  }
  
  // Return average if we have valid readings
  if (validReadings > 0) {
    return sum / validReadings;
  } else {
    return -999.0;  // Error value
  }
}
```

**Solution:**
- Implement filtering algorithm
- Relocate sensor away from heat sources
- Replace sensor if consistently inaccurate

## Network Connectivity Issues

### WiFi Connection Problems

**Symptoms:**
- Cannot connect to WiFi network
- Frequent disconnections

**Troubleshooting Steps:**

1. **Check Network Settings:**
   - Verify SSID and password are correct
   - Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)

2. **Check Signal Strength:**
   - Place the board closer to the router
   - Use an external antenna if needed

3. **Reduce Interference:**
   - Keep the board away from metal objects
   - Minimize nearby electronics that could cause interference

**Solution:**
- Use an external antenna for better reception
- Update ESP32 firmware to latest version
- Consider using Ethernet instead for more reliable connection

### Ethernet Connection Problems

**Symptoms:**
- Cannot establish Ethernet connection
- No IP address assigned

**Troubleshooting Steps:**

1. **Check Ethernet Cable:**
   - Try a different Ethernet cable
   - Verify cable is properly seated in the RJ45 jack

2. **Check Network Configuration:**
   - Verify DHCP is enabled on your router
   - Try setting a static IP address

3. **Check Hardware:**
   - Verify Ethernet LEDs light up when connected
   - Check LAN8720A connections and power

**Solution:**
- Ensure LAN8720A chip is properly powered
- Check Ethernet library configuration
- Use correct pin definitions for your board version

## Software and Programming

### Upload Fails

**Symptoms:**
- "Failed to connect to ESP32" error in Arduino IDE
- Upload timeout errors

**Troubleshooting Steps:**

1. **Enter Boot Mode:**
   - Hold BOOT button while uploading
   - Release after "Connecting...." appears
   - Some boards require pressing RESET after connecting

2. **Check USB Connection:**
   - Try different USB ports
   - Replace USB cable

3. **Verify Board and Port Selection:**
   - Select "NodeMCU-32S" from board list
   - Select correct COM port

**Solution:**
- Hold BOOT button during upload process
- Use shorter/better quality USB cable
- Reinstall ESP32 board package if necessary

### Library Conflicts

**Symptoms:**
- Compilation errors about redefined symbols
- Unexpected behavior when using multiple libraries

**Troubleshooting Steps:**

1. **Check Library Versions:**
   - Ensure compatible versions of all libraries
   - Update libraries to latest versions

2. **Resolve I²C Conflicts:**
   - Ensure only one instance of Wire is initialized
   - Check for conflicts in I²C address usage

3. **Memory Issues:**
   - Reduce program size if approaching limits
   - Check for memory leaks in long-running applications

**Solution:**
- Update all libraries to latest versions
- Resolve conflicting definitions
- Optimize code to reduce memory usage

## Factory Reset

If all else fails, performing a factory reset can sometimes resolve issues caused by corrupted settings or misconfiguration.

### Performing a Factory Reset

1. **Hardware Reset:**
   - Press and hold the RESET button for 10 seconds
   - This will reboot the ESP32 microcontroller

2. **Flash Reset Firmware:**
   - Download the reset firmware from our website
   - Follow the flashing instructions to restore factory settings

3. **Clear EEPROM:**
   - Upload the EEPROM clearing sketch

```cpp
#include <EEPROM.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Clearing EEPROM...");
  
  // Initialize EEPROM
  EEPROM.begin(512);
  
  // Clear all EEPROM data
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0xFF);
  }
  
  // Commit changes
  EEPROM.commit();
  
  Serial.println("EEPROM cleared.");
}

void loop() {
  // Nothing to do here
}
```

## Support and Assistance

If you've tried the troubleshooting steps above and are still experiencing issues, please reach out for support:

- **Documentation:** Review the [complete documentation](https://www.mesa-electronics.com/docs/kc-link-pro-a8)
- **Community Forum:** Visit our [community forum](https://forum.mesa-electronics.com)
- **Email Support:** Contact us at support@mesa-electronics.com
- **GitHub Issues:** Report issues on our [GitHub repository](https://github.com/mesa-iot/kc-link-pro-a8/issues)

When requesting support, please provide:
- Board version (look for "Ver x.x" marking on the PCB)
- Detailed description of the problem
- Steps to reproduce the issue
- Any error messages or logs
- Your code (if applicable)
- Hardware setup details

---

This troubleshooting guide covers the most common issues with the KC-Link PRO A8 board. For additional help or for issues not covered here, please contact technical support.
