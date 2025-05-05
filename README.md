# KC-Link PRO A8

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Arduino Compatible](https://img.shields.io/badge/Arduino-Compatible-green.svg)](https://www.arduino.cc/)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![ESPHome Compatible](https://img.shields.io/badge/ESPHome-Compatible-green.svg)](https://esphome.io/)
[![Home Assistant Compatible](https://img.shields.io/badge/Home_Assistant-Compatible-green.svg)](https://www.home-assistant.io/)
[![Release Version](https://img.shields.io/badge/Release-v1.1.0-green.svg)](https://github.com/mesa-automation/cortex-link-a8r-m/releases)

A professional ESP32-based IoT control board featuring 8 relay outputs, 8 optically isolated inputs, analog inputs, and multiple connectivity options including Ethernet and Wi-Fi.

<p align="center">

<img src="/Pictures/KC-Link_Layout.png" alt="Cortex Link A8F-M ESP32 Smart Relay Board" width="600"/>
  
</p>

## 🌟 Features

- **ESP32-WROOM-32 Microcontroller** with built-in Wi-Fi and Bluetooth
- **Dual Connectivity**: Wi-Fi and Ethernet (LAN8720A chip)
- **8 High-Power Relays**: Each supporting 10A at 240VAC
- **8 Opto-Isolated Digital Inputs**: For safe signal detection from external devices
- **2 Analog Inputs (0-5V)**: For monitoring analog sensors
- **Multiple Sensor Support**: 4 dedicated ports for temperature/humidity sensors
- **I²C Expansion**: Connect additional I²C devices
- **433MHz RF Support**: For wireless remote control applications
- **Modbus RTU Protocol**: Industrial standard communication
- **USB Interface**: With CH340C chip for easy programming

<p align="center">

<img src="/Pictures/Layout_Description.jpg" alt="Cortex Link A8F-M ESP32 Smart Relay Board" width="800"/>
  
</p>

## 📋 Table of Contents

- [Installation](#-installation)
- [Wiring Guide](#-wiring-guide)
- [Library Usage](https://github.com/Chamil1983/KC-Link-868-A8-IoT-Smart-Dev-Board/tree/main/Docs#-library-usage)
- [Examples](https://github.com/Chamil1983/KC-Link-868-A8-IoT-Smart-Dev-Board/tree/main/Docs#-examples)
- [API Reference](https://github.com/Chamil1983/KC-Link-868-A8-IoT-Smart-Dev-Board/tree/main/Docs#-api-reference)
- [Hardware Documentation](https://github.com/Chamil1983/KC-Link-868-A8-IoT-Smart-Dev-Board/tree/main/Docs#-hardware-documentation)
- [Troubleshooting](https://github.com/Chamil1983/KC-Link-868-A8-IoT-Smart-Dev-Board/tree/main/Docs#-troubleshooting)
- [Contributing](https://github.com/Chamil1983/KC-Link-868-A8-IoT-Smart-Dev-Board/tree/main/Docs#-contributing)
- [License](https://github.com/Chamil1983/KC-Link-868-A8-IoT-Smart-Dev-Board/tree/main/Docs#-license)

## 🔧 Installation

### Library Installation

1. **Using Arduino IDE Library Manager**:
   - Open Arduino IDE
   - Navigate to Sketch > Include Library > Manage Libraries
   - Search for "KC-Link PRO"
   - Click Install

2. **Manual Installation**:
   ```bash
   git clone https://github.com/mesa-iot/kc-link-pro-a8.git
   cp -r kc-link-pro-a8/src/KCLinkPRO [Arduino Library Path]/KCLinkPRO
   ```

### Board Setup

1. Install the ESP32 board package in Arduino IDE:
   - Go to File > Preferences
   - Add `https://dl.espressif.com/dl/package_esp32_index.json` to Additional Board Manager URLs
   - Go to Tools > Board > Boards Manager
   - Search for ESP32 and install

2. Select the correct board:
   - Board: "NodeMCU-32S"
   - Upload Speed: 115200
   - Flash Frequency: 80MHz
   - Flash Mode: QIO
   - Partition Scheme: Default

## 🔌 Wiring Guide

### Power Connection
- Connect 9-24V DC power supply to the power terminals (observe polarity)
- Verify power connection via the Power Status LED

### Digital Inputs
Connect dry contacts between the digital input terminals and GND:
```
Digital Input N <----> External Switch <----> GND
```

### Relay Outputs
Each relay provides three connection points:
- NO (Normally Open)
- COM (Common)
- NC (Normally Closed)

Example for controlling a lamp:
```
AC Power Line ----> Relay COM
                     |
Lamp -----------> Relay NO ----> AC Power Neutral
```

### Analog Inputs
- Connect analog voltage sources between the analog input terminals and GND
- Input range: 0- 5V DC

### Temperature Sensors
- Connect DS18B20, DHT11, DHT22, or DHT21 sensors to the dedicated sensor ports
- Each port provides 3.3V, GND, and data connections

## 📖 Library Usage

Include the library in your Arduino sketch:

```cpp
#include <KCLinkPRO.h>

// Initialize the board with default settings
KCLinkPRO board;

void setup() {
  // Initialize the board
  board.begin();
  
  // Turn on relay 1
  board.setRelay(1, true);
  
  // Read digital input 3
  bool input3State = board.getDigitalInput(3);
  
  // Read analog input 1
  int analog1Value = board.getAnalogInput(1);
}
```

## 🔍 Examples

The library includes several examples to help you get started:

- **BasicRelayControl**: Simple relay on/off control
- **DigitalInputs**: Reading and responding to digital inputs
- **AnalogInputs**: Reading analog voltage inputs
- **TemperatureSensor**: Reading temperature from DS18B20 sensors
- **WebServer**: Creating a web interface for control
- **MQTTControl**: Integration with MQTT for IoT applications
- **ModbusRTU**: Using Modbus protocol for industrial control
- **CompleteProject**: A full home automation example

Open examples from Arduino IDE: File > Examples > KCLinkPRO

## 📚 API Reference

### Relay Control

```cpp
// Set relay state (relayNumber: 1-8)
board.setRelay(int relayNumber, bool state);

// Toggle relay state
board.toggleRelay(int relayNumber);

// Get current relay state
bool relayState = board.getRelayState(int relayNumber);

// Set all relays at once using a bitmask
board.setAllRelays(byte relayMask);
```

### Digital Inputs

```cpp
// Read a single digital input (inputNumber: 1-8)
bool inputState = board.getDigitalInput(int inputNumber);

// Read all digital inputs at once (returns a byte where each bit corresponds to an input)
byte allInputs = board.getAllDigitalInputs();

// Set up an input change callback
board.onInputChange(void (*callback)(int inputNumber, bool newState));
```

### Analog Inputs

```cpp
// Read raw analog value (0-4095) from input (inputNumber: 1-2)
int rawValue = board.getAnalogInput(int inputNumber);

// Read voltage (0-5V) from input
float voltage = board.getAnalogVoltage(int inputNumber);

// Set up analog threshold callback
board.setAnalogThreshold(int inputNumber, float threshold, void (*callback)(int inputNumber, float value));
```

### Temperature Sensors

```cpp
// Initialize sensors
board.beginTemperatureSensors();

// Read temperature from a specific sensor (sensorNumber: 1-4)
float temperature = board.getTemperature(int sensorNumber);

// Read humidity (for DHT sensors)
float humidity = board.getHumidity(int sensorNumber);
```

### Network Communication

```cpp
// Connect to the WiFi
board.connectWiFi(const char* ssid, const char* password);

// Start Ethernet connection
board.beginEthernet();

// Start web server
board.beginWebServer();

// Connect to the MQTT broker
board.connectMQTT(const char* brokerIP, int port, const char* username, const char* password);
```

## 📂 Hardware Documentation

For detailed hardware specifications, please refer to the [Hardware Documentation](hardware.md), which includes:

- Board layout and dimensions
- Schematic diagrams
- Pin mappings
- Electrical specifications
- Connection diagrams

## ❓ Troubleshooting

Common issues and their solutions:

### No Power LED
- Check power supply connections and voltage (9- 24V DC)
- Verify polarity is correct

### Cannot connect via USB
- Install CH340C drivers
- Try a different USB cable
- Ensure proper COM port is selected in Arduino IDE

### Ethernet Not Working
- Check LAN cable connection
- Verify network settings and IP address
- Ensure the Ethernet chip is properly initialized in code

### I²C Communication Failure
- Check the I²C addresses:
  - Relay PCF8574: 0x20
  - Input PCF8574: 0x22
- Verify I²C wiring and pullup resistors

### More Issues
See the [Troubleshooting Guide](/Docs/troubleshooting.md) for additional help.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the project
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

Please read [CONTRIBUTING](/Docs/contributing.md) for details on our code of conduct and the process for submitting pull requests.

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- MESA Engineering Team
- ESP32 Community
- Arduino Community
- All contributors to this project



## Contact and Support

For technical support, please contact MESA:

- **Website:** [www.microcodeeng.com](https://www.microcodeeng.com)
- **Email:** microcode-eng@outlook.com

For issues related to this repository, please open an issue on GitHub.

---

<p align="center">
  <img src="LOGO/MESA_logo.png" alt="MESA Logo" width="200"/><br>

  <I align="center">Designed and manufactured by Microcode Embedded Systems and Automation (MESA)</i>
</p>

© 2025 MESA Electronics. All Rights Reserved.
