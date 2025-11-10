#ifndef CONFIG_H
#define CONFIG_H

// ==================== WiFi Configuration ====================
// Replace these with your WiFi credentials
#define WIFI_SSID "Verizon_7VP4RL"
#define WIFI_PASSWORD "chili-nay6-claw"
#define WIFI_RECONNECT_INTERVAL 10000  // milliseconds
#define WIFI_CONNECTION_TIMEOUT 30000 // milliseconds

// ==================== MQTT Configuration ====================
// Replace these with your MQTT broker details
#define MQTT_BROKER "m0rb1d-server.mynetworksettings.com"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "esp32_1"
#define MQTT_USER ""  // Leave empty if no authentication
#define MQTT_PASSWORD ""  // Leave empty if no authentication

// Device Information (for MQTT message payload)
#define DEVICE_LOCATION "tent"
#define DEVICE_DESCRIPTION_PREFIX "ESP32 sensor node"

// MQTT Topics - Follow pattern: {project}/{node}/{deviceCategory}
#define MQTT_PROJECT "grow"
#define MQTT_NODE "esp32_1"
#define MQTT_TOPIC_SENSOR "grow/esp32_1/sensor"
#define MQTT_TOPIC_HEALTH "grow/esp32_1/device"

// MQTT Reconnection settings
#define MQTT_RECONNECT_INITIAL_DELAY 1000  // milliseconds
#define MQTT_RECONNECT_MAX_DELAY 60000     // milliseconds

// ==================== OTA Configuration ====================
#define OTA_HOSTNAME "esp32_1"
#define OTA_PASSWORD "your_ota_password"  // Change this!
#define OTA_PORT 3232

// ==================== Sensor Pin Configuration ====================
// SHT30 (I2C)
#define SHT30_I2C_ADDRESS 0x44  // Default I2C address
#define I2C_SDA 21              // ESP32 default SDA pin
#define I2C_SCL 22              // ESP32 default SCL pin

// HC-SR04 Ultrasonic Sensor
#define HC_SR04_TRIG_PIN 25
#define HC_SR04_ECHO_PIN 35

// Atlas Scientific pH Sensor (Analog)
#define PH_SENSOR_PIN 34        // ADC1_CH6, use ADC1 pins (32-39) for WiFi compatibility

// ==================== Sensor Configuration ====================
// Enable/Disable sensors (comment out to disable)
//#define ENABLE_SHT30
//#define ENABLE_HC_SR04
#define ENABLE_PH_SENSOR

// Moving Average Window Sizes (optimized for 1-second reads, 15-second publishes)
#define TEMP_HUMIDITY_WINDOW 15  // 15 samples over 15 seconds - excellent for stable readings
#define WATER_LEVEL_WINDOW 15    // 15 samples over 15 seconds - standard responsiveness
#define PH_WINDOW 60             // 60 samples over 60 seconds - ultra-stable pH for plant health

// Sensor Validation Ranges
#define TEMP_MIN 0.0
#define TEMP_MAX 50.0
#define HUMIDITY_MIN 0.0
#define HUMIDITY_MAX 100.0
#define WATER_LEVEL_MIN 20.0   // mm
#define WATER_LEVEL_MAX 4000.0 // mm
#define PH_MIN 0.0
#define PH_MAX 14.0

// HC-SR04 Specifics
#define HC_SR04_TIMEOUT 30000     // microseconds (30ms)
#define SOUND_SPEED 343.0         // m/s at 20°C
#define CONTAINER_HEIGHT_CM 38.0  // Total container height in centimeters
#define MIN_WATER_LEVEL_CM 2.0    // Minimum expected water level (cm)
#define MAX_WATER_LEVEL_CM 35.0   // Maximum expected water level (cm)

// pH Sensor Calibration (Analog version)
// ==================== ATLAS SCIENTIFIC 3-POINT CALIBRATION ====================
// 1. Connect pH probe to pin 34
// 2. Upload firmware and monitor serial output  
// 3. Test in pH 7.0 buffer - record voltage reading → Update PH_CAL_MID
// 4. Test in pH 4.0 buffer - record voltage reading → Update PH_CAL_LOW
// 5. Test in pH 10.0 buffer - record voltage reading → Update PH_CAL_HIGH
// 6. Update the three constants below and reflash firmware
// 7. Verify accuracy across pH range - should be ±0.1 pH or better
// ============================================================================
#define PH_VOLTAGE_MIN 0.265         // Minimum expected voltage (pH 14)
#define PH_VOLTAGE_MAX 3.0           // Maximum expected voltage (pH 0)

// Atlas Scientific 3-point calibration values (in millivolts)
// Default values from Atlas Surveyor library - UPDATE AFTER CALIBRATION
#define PH_CAL_MID 1420     // pH 7.0 buffer calibration point (mV)
#define PH_CAL_LOW 1880     // pH 4.0 buffer calibration point (mV) 
#define PH_CAL_HIGH 955.0     // pH 10.0 buffer calibration point (mV)

// ESP32 ADC compensation (Atlas Scientific standard)
#define ESP32_ADC_OFFSET_MV 130    // Compensate for ESP32 ADC nonlinearity
#define PH_VOLTAGE_AVERAGING 20       // Number of readings to average for stability
#define ADC_RESOLUTION 4095.0        // 12-bit ADC

// ==================== Timing Configuration ====================
#define SENSOR_READ_INTERVAL 1000    // milliseconds (1 second) - for moving average data collection
#define SENSOR_PUBLISH_INTERVAL 15000 // milliseconds (15 seconds) - for MQTT publishing
#define HEALTH_MSG_INTERVAL 60000    // milliseconds (60 seconds)
#define WATCHDOG_TIMEOUT 60          // seconds

// Data freshness configuration
#define MAX_DATA_AGE_MS 30000        // milliseconds (30 seconds) - max age for data to be considered fresh for publishing

// ==================== Firmware Version ====================
#define FIRMWARE_VERSION "1.0.0"

// ==================== Debug Configuration ====================
#define SERIAL_BAUD_RATE 115200
// Enable verbose debug output (comment out to disable detailed sensor logs)
#define DEBUG_VERBOSE

// ==================== LED Indicator (Optional) ====================
#define LED_PIN 2  // Built-in LED on most ESP32 boards
#define ENABLE_LED_INDICATOR

#endif // CONFIG_H
