# ESP32 Hydroponic Sensor Monitor

A PlatformIO-based ESP32 firmware for monitoring environmental sensors in hydroponic systems. The firmware reads multiple sensors, applies moving average filtering, and publishes data to an MQTT broker over WiFi.

## Features

- **Multi-Sensor Support**
  - SHT30 Temperature and Humidity Sensor (I2C)
  - HC-SR04 Ultrasonic Distance Sensor (Water Level)
  - Atlas Scientific pH Sensor (Analog)
  
- **Data Processing**
  - Moving average filtering for stable readings
  - Configurable window sizes per sensor type
  - Out-of-range value rejection
  
- **Connectivity**
  - WiFi with automatic reconnection
  - MQTT publish (QoS 0 for sensor data, QoS 1 for health messages)
  - OTA (Over-The-Air) firmware updates
  
- **Reliability**
  - ESP32 watchdog timer (60-second timeout)
  - Non-blocking operation
  - Graceful degradation on sensor failures
  - LED status indicator

## Hardware Requirements

### Microcontroller
- ESP32-WROOM development board
- USB cable for power and programming

### Sensors
1. **SHT30 Temperature & Humidity Sensor**
   - Interface: I2C
   - Operating Range: -40°C to 125°C, 0-100% RH

2. **HC-SR04 Ultrasonic Distance Sensor**
   - Interface: Digital (Trigger/Echo)
   - Range: 20mm to 4000mm

3. **Atlas Scientific pH Sensor (Analog)**
   - Interface: Analog (0-3.3V)
   - Range: 0-14 pH

## Wiring Diagram

```
ESP32-WROOM Pinout:
┌─────────────────────────────────┐
│                                 │
│  3V3  ──────────── SHT30 VCC    │
│  GND  ──────────── SHT30 GND    │
│  GPIO 21 (SDA) ─── SHT30 SDA    │
│  GPIO 22 (SCL) ─── SHT30 SCL    │
│                                 │
│  5V   ──────────── HC-SR04 VCC  │
│  GND  ──────────── HC-SR04 GND  │
│  GPIO 5 ───────── HC-SR04 TRIG  │
│  GPIO 18 ──────── HC-SR04 ECHO  │
│                                 │
│  3V3  ──────────── pH VCC       │
│  GND  ──────────── pH GND       │
│  GPIO 34 (ADC1_6)─ pH OUT       │
│                                 │
│  GPIO 2 ───────── Built-in LED  │
│                                 │
└─────────────────────────────────┘

Notes:
- Use 3.3V for SHT30 and pH sensor
- Use 5V for HC-SR04
- GPIO 34 is ADC1_CH6 (ADC1 pins work with WiFi)
- Built-in LED on GPIO 2 for status indication
```

### Detailed Pin Connections

#### SHT30 Temperature & Humidity Sensor (I2C)
| SHT30 Pin | ESP32 Pin | Notes |
|-----------|-----------|-------|
| VCC       | 3.3V      | Power supply |
| GND       | GND       | Ground |
| SDA       | GPIO 21   | I2C Data (default) |
| SCL       | GPIO 22   | I2C Clock (default) |

#### HC-SR04 Ultrasonic Sensor
| HC-SR04 Pin | ESP32 Pin | Notes |
|-------------|-----------|-------|
| VCC         | 5V        | Requires 5V |
| GND         | GND       | Ground |
| TRIG        | GPIO 5    | Trigger pulse output |
| ECHO        | GPIO 18   | Echo pulse input (5V tolerant) |

⚠️ **Important**: While the HC-SR04 outputs 5V on ECHO, most ESP32 GPIO pins are 5V tolerant. If you want to be safe, use a voltage divider (10kΩ and 20kΩ resistors) to reduce 5V to 3.3V.

#### Atlas Scientific pH Sensor (Analog)
| pH Sensor Pin | ESP32 Pin | Notes |
|---------------|-----------|-------|
| VCC           | 3.3V      | Power supply |
| GND           | GND       | Ground |
| OUT           | GPIO 34   | Analog output (ADC1_CH6) |

⚠️ **Important**: Use ADC1 pins (GPIO 32-39) for analog sensors when using WiFi, as ADC2 conflicts with WiFi.

## Software Setup

### Prerequisites

1. **PlatformIO IDE**
   - Install [Visual Studio Code](https://code.visualstudio.com/)
   - Install [PlatformIO Extension](https://platformio.org/install/ide?install=vscode)

2. **Required Libraries** (automatically installed by PlatformIO)
   - Adafruit SHT31 Library
   - PubSubClient (MQTT)
   - ArduinoJson
   - Built-in: WiFi, ArduinoOTA, Wire, esp_task_wdt

### Installation Steps

1. **Clone or Download this Repository**
   ```bash
   git clone https://github.com/BBowdon00/ESPSensor.git
   cd ESPSensor
   ```

2. **Configure Settings**
   
   Edit `include/config.h` and update the following:
   
   ```cpp
   // WiFi credentials
   #define WIFI_SSID "Your_WiFi_SSID"
   #define WIFI_PASSWORD "Your_WiFi_Password"
   
   // MQTT broker
   #define MQTT_BROKER "192.168.1.100"  // Your broker IP
   #define MQTT_PORT 1883
   #define MQTT_CLIENT_ID "esp32_1"     // Unique ID for each device
   
   // OTA password
   #define OTA_PASSWORD "your_secure_password"
   ```

3. **Enable/Disable Sensors**
   
   In `include/config.h`, comment out sensors you don't want to use:
   
   ```cpp
   #define ENABLE_SHT30        // Temperature & Humidity
   #define ENABLE_HC_SR04      // Water Level
   #define ENABLE_PH_SENSOR    // pH
   ```

4. **Build and Upload**
   
   Using PlatformIO:
   ```bash
   # Build the project
   pio run
   
   # Upload to ESP32 (via USB)
   pio run --target upload
   
   # Monitor serial output
   pio device monitor
   ```
   
   Or use the PlatformIO buttons in VS Code.

## Configuration

### Sensor Calibration

#### pH Sensor Calibration

The pH sensor uses default linear calibration (0V = pH 0, 3.3V = pH 14). For accurate readings:

1. **Calibrate with Standard Solutions**
   - Use pH 4, pH 7, and pH 10 calibration solutions
   - Take voltage readings at each pH point
   
2. **Update Calibration in `config.h`**
   ```cpp
   #define PH_CALIBRATION_OFFSET 0.0   // Adjust based on pH 7 reading
   #define PH_CALIBRATION_SLOPE 1.0     // Adjust based on slope
   ```

3. **Two-Point Calibration Example**
   - If voltage at pH 7 = 1.65V (should be mid-range)
   - If voltage at pH 4 = 0.95V
   - Slope = (7 - 4) / (1.65 - 0.95) = 4.29
   - Update: `PH_CALIBRATION_SLOPE 4.29`

### Moving Average Window Sizes

Adjust in `config.h` based on your stability requirements:

```cpp
#define TEMP_HUMIDITY_WINDOW 5    // 5 samples
#define WATER_LEVEL_WINDOW 10     // 10 samples (more noise)
#define PH_WINDOW 5               // 5 samples
```

### Timing Configuration

```cpp
#define SENSOR_READ_INTERVAL 15000   // 15 seconds
#define HEALTH_MSG_INTERVAL 60000    // 60 seconds
```

## MQTT Message Format

### Sensor Data (Topic: `grow/esp32_1/sensor`)

Each sensor publishes individual messages with QoS 0:

```json
{
  "type": "temperature",
  "value": "23.45",
  "unit": "°C"
}
```

```json
{
  "type": "humidity",
  "value": "65.32",
  "unit": "%"
}
```

```json
{
  "type": "waterLevel",
  "value": "150.25",
  "unit": "mm"
}
```

```json
{
  "type": "pH",
  "value": "6.85",
  "unit": ""
}
```

### Health Message (Topic: `grow/esp32_1/device`)

Published every 60 seconds with QoS 1:

```json
{
  "deviceId": "esp32_1",
  "status": "online",
  "uptime": 3600,
  "firmwareVersion": "1.0.0",
  "freeHeap": 234567,
  "rssi": -45,
  "sensors": {
    "temperature": "ok",
    "humidity": "ok",
    "waterLevel": "ok",
    "pH": "ok"
  }
}
```

## OTA Updates

### First-Time Setup

1. **Ensure ESP32 is on the network**
   - Device hostname: `esp32_1.local`

2. **Update via PlatformIO**
   ```bash
   # Edit platformio.ini and uncomment OTA settings
   upload_protocol = espota
   upload_port = esp32_1.local
   upload_flags = 
       --auth=your_ota_password
   
   # Upload
   pio run --target upload
   ```

3. **Using Arduino IDE**
   - Go to Tools → Port
   - Select "esp32_1 at 192.168.x.x"
   - Upload normally

## LED Status Indicator

The built-in LED (GPIO 2) shows system status:

- **Fast Blink (200ms)**: WiFi disconnected
- **Medium Blink (500ms)**: MQTT disconnected
- **Slow Blink (2000ms)**: All systems operational

## Troubleshooting

### WiFi Connection Issues

**Problem**: ESP32 won't connect to WiFi

**Solutions**:
- Verify SSID and password in `config.h`
- Check WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Move ESP32 closer to router
- Check serial monitor for connection attempts
- Verify WiFi network is operational

### MQTT Connection Issues

**Problem**: Can't connect to MQTT broker

**Solutions**:
- Verify broker IP address and port
- Test broker with mosquitto_pub/sub:
  ```bash
  mosquitto_sub -h 192.168.1.100 -t "grow/#" -v
  ```
- Check firewall rules on broker
- Verify broker is running: `sudo systemctl status mosquitto`
- Check if authentication is required

### Sensor Reading Errors

**Problem**: SHT30 not detected

**Solutions**:
- Check I2C wiring (SDA, SCL)
- Verify 3.3V power connection
- Try I2C scanner sketch to detect address
- Check for loose connections

**Problem**: HC-SR04 timeout errors

**Solutions**:
- Verify 5V power supply (not 3.3V)
- Check trigger and echo pin connections
- Ensure no obstacles within 20mm of sensor
- Test sensor with separate sketch

**Problem**: pH readings seem incorrect

**Solutions**:
- Perform calibration with standard solutions
- Check analog pin connection (GPIO 34)
- Verify 3.3V power supply
- Update calibration constants in config.h
- Allow sensor to stabilize (10-15 samples)

### OTA Update Issues

**Problem**: Can't find ESP32 for OTA update

**Solutions**:
- Verify ESP32 is connected to WiFi
- Check mDNS hostname: `ping esp32_1.local`
- Use IP address instead of hostname
- Verify OTA password matches
- Check firewall allows port 3232

### General Debugging

Enable verbose debugging in `config.h`:
```cpp
#define DEBUG_VERBOSE
```

This will show:
- Raw sensor readings
- Averaged values
- Detailed connection attempts
- ADC values for pH sensor

## Project Structure

```
ESPSensor/
├── include/
│   ├── config.h              # Configuration file
│   ├── MovingAverage.h       # Moving average template class
│   ├── SensorBase.h          # Abstract sensor base class
│   ├── SHT30Sensor.h         # Temperature/humidity sensor
│   ├── HC_SR04Sensor.h       # Ultrasonic distance sensor
│   └── PHSensor.h            # pH sensor
├── src/
│   └── main.cpp              # Main application code
├── platformio.ini            # PlatformIO configuration
├── .gitignore               # Git ignore file
└── README.md                # This file
```

## Adding More Sensors

The modular design makes it easy to add new sensors:

1. **Create New Sensor Class**
   ```cpp
   // include/NewSensor.h
   #include "SensorBase.h"
   
   class NewSensor : public SensorBase {
   public:
       NewSensor() : SensorBase("NewSensor") {}
       bool begin() override { /* init code */ }
       bool read() override { /* read code */ }
       // Add getter methods
   };
   ```

2. **Add to config.h**
   ```cpp
   #define ENABLE_NEW_SENSOR
   #define NEW_SENSOR_PIN 35
   ```

3. **Instantiate in main.cpp**
   ```cpp
   #ifdef ENABLE_NEW_SENSOR
   #include "NewSensor.h"
   NewSensor newSensor;
   #endif
   ```

4. **Add to Initialization and Reading**
   ```cpp
   #ifdef ENABLE_NEW_SENSOR
   newSensor.begin();
   newSensor.read();
   // Publish data
   #endif
   ```

## Future Enhancements

Planned features for future versions:

- [ ] EC (Electrical Conductivity) sensor support
- [ ] Light intensity sensor (LUX)
- [ ] CO2 sensor integration
- [ ] Relay control for pumps/lights
- [ ] MQTT command subscription (control actuators)
- [ ] SD card data logging
- [ ] Web dashboard (ESP32 web server)
- [ ] Multi-node coordination
- [ ] Deep sleep mode for battery operation
- [ ] Alert notifications (Telegram, email)

## Performance Characteristics

- **Power Consumption**: ~80-120mA @ 5V (active)
- **Memory Usage**: ~180KB program storage, ~30KB RAM
- **Network Latency**: <100ms (sensor read to MQTT publish)
- **Sensor Update Rate**: 15 seconds (configurable)
- **WiFi Reconnect Time**: 5-10 seconds
- **MQTT Reconnect Time**: 1-60 seconds (exponential backoff)

## License

This project is open source and available for educational and commercial use.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

For issues, questions, or suggestions:
- Open an issue on GitHub
- Check the troubleshooting section above
- Review serial monitor output with DEBUG_VERBOSE enabled

## Version History

### v1.0.0 (Current)
- Initial release
- Support for SHT30, HC-SR04, and pH sensors
- WiFi and MQTT connectivity
- OTA updates
- Moving average filtering
- Watchdog timer
- LED status indicator

## Acknowledgments

- Built with PlatformIO and Arduino framework
- Uses Adafruit SHT31 library
- MQTT via PubSubClient library
- JSON formatting with ArduinoJson

---

**Happy Growing! 🌱**