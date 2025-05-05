# KC-Link PRO A8 Hardware Documentation

## Overview

The KC-Link PRO A8 is a versatile ESP32-based controller board designed for IoT and automation applications. This document provides detailed hardware specifications, pin mappings, and connection information.

![KC-Link PRO A8 Board](./images/kc-link-pro-a8.jpg)

## Board Specifications

| Feature | Specification |
|---------|---------------|
| Dimensions | 180mm × 105mm |
| Power Input | 9-24V DC |
| Microcontroller | ESP32-WROOM-32 |
| Flash Memory | 4MB |
| RAM | 520KB SRAM |
| Operating Temperature | -10°C to 50°C |
| Mounting | DIN rail compatible |

## Key Components

### Microcontroller and Connectivity
- **ESP32-WROOM-32**: Main microcontroller with Wi-Fi and Bluetooth
- **LAN8720A**: Ethernet PHY chip for wired network connectivity
- **CH340C**: USB to UART bridge for programming and debugging

### I/O Expansion and Interfaces
- **PCF8574 (0x20)**: I²C I/O expander for relay control
- **PCF8574 (0x22)**: I²C I/O expander for digital inputs
- **433MHz RF**: RF transmitter and receiver modules (433M/T, 433M/R)
- **I²C Connector**: External I²C expansion port

### Inputs and Outputs
- **8 Relay Outputs**: 10A/240VAC relays with NO/NC/COM terminals
- **8 Digital Inputs**: Optically isolated inputs with LED indicators
- **2 Analog Inputs**: 0-5V analog input channels
- **4 Sensor Ports**: For temperature/humidity sensors (DS18B20/DHT)

## Pin Mapping

### ESP32 Pinout

| ESP32 GPIO | Function | Notes |
|------------|----------|-------|
| GPIO0 | BOOT/Download Button | Used for firmware flashing |
| GPIO1 | TX | Serial debugging |
| GPIO3 | RX | Serial debugging |
| GPIO4 | RS485_DE | RS485 direction control (optional) |
| GPIO5 | IIC_SCL | I²C clock line (PCF8574 communication) |
| GPIO12 | Ethernet PHY Power | For LAN8720A |
| GPIO13 | Temperature Sensor 2 | DHT22/DS18B20 data line |
| GPIO14 | Temperature Sensor 1 | DHT22/DS18B20 data line |
| GPIO16 | Ethernet Power | For LAN8720A |
| GPIO17 | EMAC_CLK | Ethernet MAC Clock |
| GPIO18 | EMAC_MDIO | Ethernet Management Data I/O |
| GPIO19 | EMAC_TXD0 | Ethernet Transmit Data 0 |
| GPIO21 | IIC_SDA | I²C data line (PCF8574 communication) |
| GPIO22 | IIC_SCL | I²C clock line (PCF8574 communication) |
| GPIO23 | EMAC_MDC | Ethernet Management Data Clock |
| GPIO25 | EMAC_RXD0 | Ethernet Receive Data 0 |
| GPIO26 | EMAC_RXD1 | Ethernet Receive Data 1 |
| GPIO27 | EMAC_CRS_DV | Ethernet Carrier Sense/Data Valid |
| GPIO32 | Temperature Sensor 3 | DHT22/DS18B20 data line |
| GPIO33 | Temperature Sensor 4 | DHT22/DS18B20 data line |
| GPIO34 | Analog Input 1 | ADC input (0-5V) |
| GPIO35 | Analog Input 2 | ADC input (0-5V) |
| GPIO36 | 433MHz RF Data | Data line for 433MHz RF receiver |
| GPIO39 | Reset Button | Hardware reset |

### I²C Device Addresses

| Device | I²C Address | Function |
|--------|-------------|----------|
| PCF8574 | 0x20 | Relay control |
| PCF8574 | 0x22 | Digital inputs |

### Relay Terminal Block

Each relay provides three connection points:

| Terminal | Description |
|----------|-------------|
| NO | Normally Open contact |
| COM | Common contact |
| NC | Normally Closed contact |

### Digital Input Terminal Block

| Terminal | Description |
|----------|-------------|
| IN1-IN8 | Digital input terminals |
| GND | Common ground connection |

### Analog Input Terminal Block

| Terminal | Description |
|----------|-------------|
| AI1 | Analog input 1 (0-5V) |
| AI2 | Analog input 2 (0-5V) |
| GND | Common ground connection |

## Circuit Sections

The KC-Link PRO A8 board is divided into several functional sections:

### Power Supply Section
- Input voltage: 9-24V DC
- 5V regulator (XL1509-5) for 5V circuits
- 3.3V regulator (LM1117-3V3) for ESP32 and logic

### ESP32 Core Section
- ESP32-WROOM-32 module
- Reset and download/boot buttons
- USB interface (CH340C)

### Ethernet Section
- LAN8720A Ethernet PHY
- RJ45 connector with integrated magnetics
- Link and activity LEDs

### Relay Section
- 8 relays (10A/240VAC)
- PCF8574 I/O expander for control
- ULN2003A relay drivers
- Status LEDs for each relay

### Digital Input Section
- 8 optically isolated inputs (EL357 optocouplers)
- PCF8574 I/O expander for reading
- Status LEDs for each input

### Analog Input Section
- 2 analog inputs (0-5V)
- OP-amp buffer circuit for protection

### Temperature Sensor Section
- 4 dedicated sensor ports
- Pull-up resistors for one-wire/DHT communication

### 433MHz RF Section
- 433MHz transmitter
- 433MHz receiver
- Data input/output connections

## Schematic Diagram

For the complete schematic diagram, please refer to the [KC-Link PRO A8 Schematic PDF](./datasheets/kc-link-pro-a8-schematic.pdf) in the datasheets folder.

## Board Layout

![Board Layout Diagram](./images/board-layout.png)

## Power Requirements

The KC-Link PRO A8 requires a 9-24V DC power supply with a minimum current rating of 1A. The power consumption varies depending on the number of active relays:

| Configuration | Approximate Power Consumption |
|---------------|-------------------------------|
| Board only (no active relays) | 1.5W |
| Each active relay | +0.5W |
| Full load (all 8 relays active) | 5.5W |

## Relay Specifications

| Parameter | Value |
|-----------|-------|
| Contact arrangement | SPDT (Single Pole Double Throw) |
| Rated load (resistive) | 10A at 240VAC / a0A at 30VDC |
| Max. switching voltage | 250VAC / 30VDC |
| Max. switching current | 10A |
| Expected mechanical life | 100,000 operations |
| Expected electrical life | 100,000 operations at rated load |
| Operating time | 10ms maximum |
| Release time | 5ms maximum |
| Coil voltage | 12VDC |
| Coil resistance | 400Ω ±10% |

## Digital Input Specifications

| Parameter | Value |
|-----------|-------|
| Input type | Optically isolated |
| Input voltage range | 5-24V DC |
| Input current | 5-10mA |
| Isolation voltage | 5000V |
| Response time | < 5ms |

## Analog Input Specifications

| Parameter | Value |
|-----------|-------|
| Input voltage range | 0-5V DC |
| Resolution | 12-bit (0-4095) |
| Input impedance | > 10kΩ |
| Sampling rate | Up to 1kHz |
| Accuracy | ±1% of full scale |

## Connection Diagrams

### Power Connection
```
DC Power Supply (9-24V) --> (+) Terminal --> Board
                        --> (-) Terminal --> Board
```

### Relay Connection (Example: Controlling a Light)
```
AC Power Line --- Relay COM
                    |
Light ----------- Relay NO --- AC Power Neutral
```

### Digital Input Connection (Example: Door Sensor)
```
+12V --- Door Sensor (Closed when door is closed) --- Input Terminal (IN1)
                                                   --- GND Terminal
```

### Analog Input Connection (Example: Temperature Sensor)
```
+5V --- Temperature Sensor --- Analog Input (AI1)
                           --- GND Terminal
```

### Temperature Sensor Connection (DS18B20)
```
Sensor Port --- Red Wire (+3.3V) --- DS18B20 VDD
             --- Black Wire (GND) --- DS18B20 GND
             --- Yellow Wire (Data) --- DS18B20 DQ
```

## Ethernet Configuration

The Ethernet interface uses the LAN8720A PHY chip with the following default configuration:
- Auto-negotiation for speed and duplex
- Activity and link status LEDs
- MDIO/MDC interface to ESP32

## Environmental Specifications

| Parameter | Value |
|-----------|-------|
| Operating temperature | -10°C to 50°C |
| Storage temperature | -20°C to 60°C |
| Operating humidity | 10% to 90% RH (non-condensing) |
| Storage humidity | 5% to 95% RH (non-condensing) |
| Protection class | IP20 (indoor use only) |

## Safety Considerations

- The KC-Link PRO A8 is designed for low voltage control applications.
- When controlling AC mains voltage through the relays, ensure proper electrical safety practices.
- Install the board in an appropriate enclosure when used in industrial environments.
- Maintain separation between high voltage wiring and low voltage/signal wiring.
- Follow local electrical codes and regulations for installation.

## Mechanical Dimensions

- Board dimensions: 180mm × 105mm × 30mm (L×W×H)
- Mounting hole diameter: 3.5mm
- Mounting hole spacing: 172mm × 97mm (center to center)

## Support and Warranty

The KC-Link PRO A8 comes with a standard 1-year warranty against manufacturing defects. For technical support, please contact the manufacturer or visit the support forum.

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| V1.0 | 2023-05-15 | Initial production release |
| V1.1 | 2023-09-22 | Improved analog input protection |
| V1.2 | 2023-12-10 | Enhanced Ethernet stability |
| V1.3 | 2024-03-05 | Added I²C expansion connector |
| V1.4 | 2024-04-18 | Modified sensor pin assignment |