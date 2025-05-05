/*
 * KC-Link PRO A8 - Modbus RTU Example
 * 
 * This example demonstrates how to use the KC-Link PRO A8 with Modbus RTU protocol
 * to read and control relays, digital inputs, and analog inputs.
 * 
 * Based on the KC868-A series Modbus protocol documentation.
 * 
 * Function codes used:
 * - 01: Read digital output (relay) state
 * - 02: Read digital input state
 * - 03: Read analog input state
 * - 05: Set ON/OFF one channel of digital output
 * - 0F: Set ON/OFF multi-channel digital outputs
 * 
 * Hardware Required:
 * - KC-Link PRO A8 board
 * - RS485 to USB converter (for connecting to a computer)
 * - 9-24V DC power supply
 */

#include <Arduino.h>
#include <ModbusRTU.h>

// Serial port for Modbus communication
// On ESP32, you can use Serial1 or Serial2 for Modbus while keeping Serial for debugging
#define MODBUS_SERIAL Serial2
#define MODBUS_DE_PIN 4  // Data Enable pin for RS485 converter (if needed)

// Define board settings
#define MODBUS_ADDRESS 1  // Modbus slave address (1-247)
#define BAUD_RATE 9600   // Default baud rate

// Create Modbus slave object
ModbusRTU slave;

// Storage for Modbus registers
uint16_t modbusRegisters[32];  // Storage for holding register values

// Function to set a relay based on Modbus command
void setRelay(uint8_t relay, bool state) {
  // Safety check
  if (relay < 1 || relay > 8) {
    return;
  }
  
  // Set the corresponding bit in the register
  if (state) {
    bitSet(modbusRegisters[0], relay - 1);
  } else {
    bitClear(modbusRegisters[0], relay - 1);
  }
  
  // TODO: Add actual relay control code here
  // This would interface with the PCF8574 to control the physical relay
  
  Serial.print("Relay ");
  Serial.print(relay);
  Serial.print(" set to ");
  Serial.println(state ? "ON" : "OFF");
}

// Function to read a digital input
bool readDigitalInput(uint8_t input) {
  // Safety check
  if (input < 1 || input > 8) {
    return false;
  }
  
  // TODO: Add actual digital input reading code here
  // This would interface with the PCF8574 to read the physical input
  
  // For demonstration, we'll return the bit from the register
  return bitRead(modbusRegisters[1], input - 1);
}

// Function to read an analog input
uint16_t readAnalogInput(uint8_t input) {
  // Safety check
  if (input < 1 || input > 2) {
    return 0;
  }
  
  // TODO: Add actual analog input reading code here
  // This would use analogRead() on the appropriate pin
  
  // For demonstration, we'll return the value from the register
  return modbusRegisters[2 + input - 1];
}

void setup() {
  // Initialize debug serial port
  Serial.begin(115200);
  Serial.println("KC-Link PRO A8 Modbus RTU Example");
  
  // Initialize Modbus communication
  MODBUS_SERIAL.begin(BAUD_RATE, SERIAL_8N1);
  
  // If using a DE/RE pin for RS485 direction control
  if (MODBUS_DE_PIN >= 0) {
    pinMode(MODBUS_DE_PIN, OUTPUT);
  }
  
  // Initialize Modbus slave
  slave.begin(&MODBUS_SERIAL, MODBUS_DE_PIN);
  slave.slave(MODBUS_ADDRESS);
  
  // Initialize register values
  memset(modbusRegisters, 0, sizeof(modbusRegisters));
  
  Serial.println("Modbus RTU slave initialized with address: " + String(MODBUS_ADDRESS));
  Serial.println("Waiting for Modbus master commands...");
}

void loop() {
  // Process Modbus messages
  slave.task();
  
  // Simulate digital input changes (for demonstration)
  static unsigned long lastInputUpdate = 0;
  if (millis() - lastInputUpdate > 5000) {
    // Toggle input 1 every 5 seconds for demonstration
    bool currentState = readDigitalInput(1);
    modbusRegisters[1] = currentState ? 0 : 1;  // Toggle bit 0
    
    lastInputUpdate = millis();
  }
  
  // Simulate analog input changes (for demonstration)
  static unsigned long lastAnalogUpdate = 0;
  if (millis() - lastAnalogUpdate > 1000) {
    // Simulate changing analog values
    modbusRegisters[2] = (modbusRegisters[2] + 10) % 1024;  // Analog 1
    modbusRegisters[3] = (modbusRegisters[3] + 5) % 1024;   // Analog 2
    
    lastAnalogUpdate = millis();
  }
  
  // Add any other non-blocking code here
  yield();
}

// Example Modbus packet for reference:
// Read relay states: 01 01 00 00 00 08 3D CC
// Response:          01 01 01 01 51 88 (relay 1 ON, others OFF)
// 
// Turn ON relay 1:   01 05 00 00 FF 00 8C 3A
// Response:          01 05 00 00 FF 00 8C 3A (echo)
// 
// Turn OFF relay 1:  01 05 00 00 00 00 CD CA
// Response:          01 05 00 00 00 00 CD CA (echo)
//
// Read digital inputs: 01 02 00 00 00 08 79 CC
// Response:           01 02 01 03 A1 90 (inputs 1 and 2 active)
//
// Read analog inputs: 01 03 00 00 00 02 C4 0B
// Response:           01 03 04 01 F4 02 58 70 DC
//                     (analog 1 = 0x01F4 = 500, analog 2 = 0x0258 = 600)
