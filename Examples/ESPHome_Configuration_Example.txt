# KC-Link PRO A8 - ESPHome Configuration Example
# This configuration allows integration with Home Assistant

# Basic device configuration
esphome:
  name: kc_link_pro_a8
  friendly_name: KC-Link PRO A8
  platform: ESP32
  board: nodemcu-32s

# Enable logging
logger:
  level: INFO
  
# Enable Home Assistant API
api:
  encryption:
    key: "YOUR_ENCRYPTION_KEY"  # Generate with 'esphome secrets encrypt-secrets'

# Enable OTA updates
ota:
  password: "YOUR_OTA_PASSWORD"

# WiFi credentials
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  
  # Enable fallback hotspot in case of connection failure
  ap:
    ssid: "KC-Link Pro Fallback"
    password: "configme123"

# Enable fallback to Ethernet if WiFi fails
ethernet:
  type: LAN8720
  mdc_pin: GPIO23
  mdio_pin: GPIO18
  clk_mode: GPIO17_OUT
  phy_addr: 0
  power_pin: GPIO12
  
# Enable time synchronization
time:
  - platform: homeassistant
    id: homeassistant_time

# Enable I2C bus for the PCF8574 expanders
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  id: bus_a

# PCF8574 for relay control (address 0x20)
pcf8574:
  - id: 'pcf8574_relays'
    address: 0x20
    pcf8575: false
    i2c_id: bus_a

# PCF8574 for digital inputs (address 0x22)
pcf8574:
  - id: 'pcf8574_inputs'
    address: 0x22
    pcf8575: false
    i2c_id: bus_a

# Define relays (8 channels)
switch:
  # Relay 1
  - platform: gpio
    name: "Relay 1"
    pin:
      pcf8574: pcf8574_relays
      number: 0
      mode: OUTPUT
      inverted: true  # Relays are active LOW
    id: relay_1
    
  # Relay 2
  - platform: gpio
    name: "Relay 2"
    pin:
      pcf8574: pcf8574_relays
      number: 1
      mode: OUTPUT
      inverted: true
    id: relay_2
    
  # Relay 3
  - platform: gpio
    name: "Relay 3"
    pin:
      pcf8574: pcf8574_relays
      number: 2
      mode: OUTPUT
      inverted: true
    id: relay_3
    
  # Relay 4
  - platform: gpio
    name: "Relay 4"
    pin:
      pcf8574: pcf8574_relays
      number: 3
      mode: OUTPUT
      inverted: true
    id: relay_4
    
  # Relay 5
  - platform: gpio
    name: "Relay 5"
    pin:
      pcf8574: pcf8574_relays
      number: 4
      mode: OUTPUT
      inverted: true
    id: relay_5
    
  # Relay 6
  - platform: gpio
    name: "Relay 6"
    pin:
      pcf8574: pcf8574_relays
      number: 5
      mode: OUTPUT
      inverted: true
    id: relay_6
    
  # Relay 7
  - platform: gpio
    name: "Relay 7"
    pin:
      pcf8574: pcf8574_relays
      number: 6
      mode: OUTPUT
      inverted: true
    id: relay_7
    
  # Relay 8
  - platform: gpio
    name: "Relay 8"
    pin:
      pcf8574: pcf8574_relays
      number: 7
      mode: OUTPUT
      inverted: true
    id: relay_8

# Define digital inputs (8 channels)
binary_sensor:
  # Input 1
  - platform: gpio
    name: "Input 1"
    pin:
      pcf8574: pcf8574_inputs
      number: 0
      mode: INPUT
      inverted: false
    id: input_1
    filters:
      - delayed_on: 10ms
      - delayed_off: 10ms
      
  # Input 2
  - platform: gpio
    name: "Input 2"
    pin:
      pcf8574: pcf8574_inputs
      number: 1
      mode: INPUT
      inverted: false
    id: input_2
    filters:
      - delayed_on: 10ms
      - delayed_off: 10ms
      
  # Input 3
  - platform: gpio
    name: "Input 3"
    pin:
      pcf8574: pcf8574_inputs
      number: 2
      mode: INPUT
      inverted: false
    id: input_3
    filters:
      - delayed_on: 10ms
      - delayed_off: 10ms
      
  # Input 4
  - platform: gpio
    name: "Input 4"
    pin:
      pcf8574: pcf8574_inputs
      number: 3
      mode: INPUT
      inverted: false
    id: input_4
    filters:
      - delayed_on: 10ms
      - delayed_off: 10ms
      
  # Input 5
  - platform: gpio
    name: "Input 5"
    pin:
      pcf8574: pcf8574_inputs
      number: 4
      mode: INPUT
      inverted: false
    id: input_5
    filters:
      - delayed_on: 10ms
      - delayed_off: 10ms
      
  # Input 6
  - platform: gpio
    name: "Input 6"
    pin:
      pcf8574: pcf8574_inputs
      number: 5
      mode: INPUT
      inverted: false
    id: input_6
    filters:
      - delayed_on: 10ms
      - delayed_off: 10ms
      
  # Input 7
  - platform: gpio
    name: "Input 7"
    pin:
      pcf8574: pcf8574_inputs
      number: 6
      mode: INPUT
      inverted: false
    id: input_7
    filters:
      - delayed_on: 10ms
      - delayed_off: 10ms
      
  # Input 8
  - platform: gpio
    name: "Input 8"
    pin:
      pcf8574: pcf8574_inputs
      number: 7
      mode: INPUT
      inverted: false
    id: input_8
    filters:
      - delayed_on: 10ms
      - delayed_off: 10ms

# Define analog inputs (2 channels)
sensor:
  # Analog Input 1
  - platform: adc
    pin: GPIO34
    name: "Analog Input 1"
    update_interval: 5s
    attenuation: 11db
    filters:
      - multiply: 5.0  # Convert to voltage (0-5V)
    unit_of_measurement: "V"
    accuracy_decimals: 2
    
  # Analog Input 2
  - platform: adc
    pin: GPIO35
    name: "Analog Input 2"
    update_interval: 5s
    attenuation: 11db
    filters:
      - multiply: 5.0  # Convert to voltage (0-5V)
    unit_of_measurement: "V"
    accuracy_decimals: 2
    
  # DS18B20 Temperature Sensor 1
  - platform: dallas
    address: 0x000000000000  # Replace with actual sensor address
    name: "Temperature Sensor 1"
    update_interval: 30s
    
  # DHT22 Temperature/Humidity Sensor 2
  - platform: dht
    model: DHT22
    pin: GPIO13
    temperature:
      name: "Temperature Sensor 2"
    humidity:
      name: "Humidity Sensor 2"
    update_interval: 30s

# System sensors
  - platform: wifi_signal
    name: "WiFi Signal"
    update_interval: 60s
    
  - platform: uptime
    name: "Uptime"
    
# Create automation for linking inputs to relays
# (Example: Input 1 toggles Relay 1)
automation:
  - trigger:
      platform: state
      entity_id: binary_sensor.input_1
      to: "ON"
    action:
      - switch.toggle: relay_1
      
  - trigger:
      platform: state
      entity_id: binary_sensor.input_2
      to: "ON"
    action:
      - switch.toggle: relay_2

# Display dashboard on web interface
web_server:
  port: 80
  auth:
    username: admin
    password: !secret web_password

# Status LED
status_led:
  pin: 
    number: GPIO2
    inverted: true
