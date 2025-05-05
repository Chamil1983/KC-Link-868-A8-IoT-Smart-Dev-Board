# Required Libraries for KC-Link PRO A8

This document provides a comprehensive guide to all the libraries required for programming the KC-Link PRO A8 with Arduino IDE, including installation instructions and GitHub links.

## Core Libraries

### 1. PCF8574 Library
**Description:** Essential library for interfacing with the PCF8574 I/O expanders used to control relays and read digital inputs on the KC-Link PRO A8.

**Installation:**
- **Arduino Library Manager Method:**
  1. In Arduino IDE, go to Sketch > Include Library > Manage Libraries
  2. Search for "PCF8574"
  3. Install "PCF8574" by Renzo Mischianti or "PCF8574_library" by xreef

- **GitHub Method:**
  1. Download: [https://github.com/xreef/PCF8574_library](https://github.com/xreef/PCF8574_library)
  2. Extract and place in your Arduino/libraries folder
  3. Restart Arduino IDE

**Usage Notes:**
- For relay control on the KC-Link PRO A8, use address `0x20`
- For digital inputs on the KC-Link PRO A8, use address `0x22`

### 2. OneWire Library
**Description:** Required for communication with OneWire devices such as DS18B20 temperature sensors.

**Installation:**
- **Arduino Library Manager Method:**
  1. In Arduino IDE, go to Sketch > Include Library > Manage Libraries
  2. Search for "OneWire"
  3. Install "OneWire" by Paul Stoffregen

- **GitHub Method:**
  1. Download: [https://github.com/PaulStoffregen/OneWire](https://github.com/PaulStoffregen/OneWire)
  2. Extract and place in your Arduino/libraries folder
  3. Restart Arduino IDE

### 3. DallasTemperature Library
**Description:** Used for interfacing with Dallas temperature sensors (DS18B20) connected to the sensor ports.

**Installation:**
- **Arduino Library Manager Method:**
  1. In Arduino IDE, go to Sketch > Include Library > Manage Libraries
  2. Search for "DallasTemperature"
  3. Install "DallasTemperature" by Miles Burton

- **GitHub Method:**
  1. Download: [https://github.com/milesburton/Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library)
  2. Extract and place in your Arduino/libraries folder
  3. Restart Arduino IDE

### 4. DHT Sensor Library
**Description:** Used for interfacing with DHT11/DHT22/DHT21 temperature and humidity sensors.

**Installation:**
- **Arduino Library Manager Method:**
  1. In Arduino IDE, go to Sketch > Include Library > Manage Libraries
  2. Search for "DHT sensor library"
  3. Install "DHT sensor library" by Adafruit

- **GitHub Method:**
  1. Download: [https://github.com/adafruit/DHT-sensor-library](https://github.com/adafruit/DHT-sensor-library)
  2. Extract and place in your Arduino/libraries folder
  3. Install the Adafruit Unified Sensor library as well (dependency)
  4. Restart Arduino IDE

### 5. Ethernet Library (for LAN8720)
**Description:** For using the built-in Ethernet functionality of the KC-Link PRO A8 (used with the LAN8720A chip).

**Installation:**
- **Already Included:** The Ethernet library is included with the ESP32 Arduino Core
- **Configuration Notes:** 
  - The KC-Link PRO A8 uses the LAN8720A Ethernet PHY chip
  - Default pin connections:
    ```
    ETH_PHY_MDC: 23
    ETH_PHY_MDIO: 18
    ETH_PHY_ADDR: 0
    ETH_CLK_MODE: ETH_CLOCK_GPIO0_IN
    ```

## Communication Libraries

### 6. PubSubClient
**Description:** MQTT client library for implementing MQTT communication with brokers.

**Installation:**
- **Arduino Library Manager Method:**
  1. In Arduino IDE, go to Sketch > Include Library > Manage Libraries
  2. Search for "PubSubClient"
  3. Install "PubSubClient" by Nick O'Leary

- **GitHub Method:**
  1. Download: [https://github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient)
  2. Extract and place in your Arduino/libraries folder
  3. Restart Arduino IDE

### 7. ArduinoJson
**Description:** JSON parsing and creation library, useful for API communication and data storage.

**Installation:**
- **Arduino Library Manager Method:**
  1. In Arduino IDE, go to Sketch > Include Library > Manage Libraries
  2. Search for "ArduinoJson"
  3. Install "ArduinoJson" by Benoit Blanchon

- **GitHub Method:**
  1. Download: [https://github.com/bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson)
  2. Extract and place in your Arduino/libraries folder
  3. Restart Arduino IDE

**Note:** Recommended to use version 6.x or later

### 8. AsyncTCP (Optional - For Advanced Web Server)
**Description:** Asynchronous TCP library for ESP32, basis for advanced web server implementations.

**Installation:**
- **GitHub Method:**
  1. Download: [https://github.com/me-no-dev/AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
  2. Extract and place in your Arduino/libraries folder
  3. Restart Arduino IDE

### 9. ESPAsyncWebServer (Optional - For Advanced Web Server)
**Description:** Asynchronous Web Server library for ESP32, provides a powerful way to create web interfaces.

**Installation:**
- **GitHub Method:**
  1. Download: [https://github.com/me-no-dev/ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
  2. Extract and place in your Arduino/libraries folder
  3. Restart Arduino IDE
  4. Requires AsyncTCP library as a dependency

## ModBus Libraries

### 10. ModbusRTU Library
**Description:** Provides ModBus RTU client and server functionality for industrial communication.

**Installation:**
- **Arduino Library Manager Method:**
  1. In Arduino IDE, go to Sketch > Include Library > Manage Libraries
  2. Search for "ModbusRTU"
  3. Install "ModbusRTU" by Alexander Emelianov (emelianov)

- **GitHub Method:**
  1. Download: [https://github.com/emelianov/modbus-esp8266](https://github.com/emelianov/modbus-esp8266)
  2. Extract and place in your Arduino/libraries folder
  3. Restart Arduino IDE

## Installation Troubleshooting

If you encounter issues with library installation, try these solutions:

1. **Library Dependencies:** Some libraries require other libraries to function. Check the documentation for any prerequisites.

2. **Library Conflicts:** If you have multiple libraries with similar functionality, they may conflict. Try using the library specified in this document.

3. **ESP32 Board Support:** Ensure you have the ESP32 board support package installed:
   - Go to File > Preferences
   - Add `https://dl.espressif.com/dl/package_esp32_index.json` to the "Additional Board Manager URLs" field
   - Go to Tools > Board > Boards Manager
   - Search for "esp32" and install the ESP32 board package by Espressif Systems

4. **Compilation Errors:**
   - Check that you have selected the correct board (NodeMCU-32S) in the Arduino IDE
   - Verify that the libraries you're using are compatible with ESP32
   - Some libraries may need modifications to work with ESP32

## Board-Specific Configuration

When working with the KC-Link PRO A8, use these default parameters:

```cpp
// PCF8574 Addresses
#define PCF8574_RELAY_ADDR 0x20  // Address for relay control
#define PCF8574_INPUT_ADDR 0x22  // Address for digital inputs

// Analog Input Pins for V1.4
#define ANALOG_INPUT_1 34
#define ANALOG_INPUT_2 35

// Temperature Sensor Pins for V1.4
#define TEMP_SENSOR_1 14
#define TEMP_SENSOR_2 13
#define TEMP_SENSOR_3 32
#define TEMP_SENSOR_4 33

// Ethernet Configuration for LAN8720A
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_ADDR 0
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
#define ETH_PHY_POWER -1
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
```

## Additional Resources

- [KinCony KC868-A8 Official Documentation](https://www.kincony.com/arduino-esp32-8-channel-relay-module-kc868-a8.html)
- [ESP32 Arduino Core Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [KC868-A8 GitHub Examples](https://github.com/kincony/KC868-A8-examples)

## Compatibility Notes

- All libraries mentioned in this document have been tested with ESP32 Arduino Core version 2.0.0 and later
- The KC-Link PRO A8 is based on the ESP32-WROOM-32 module
- Board selection in Arduino IDE should be "NodeMCU-32S"

---

*This document serves as a reference for installing and using the required libraries for the KC-Link PRO A8 board. For more detailed information on each library's usage, please refer to the respective library's documentation or examples.*
