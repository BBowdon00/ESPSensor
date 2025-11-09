#ifndef CONFIG_H
#define CONFIG_H

// ==================== WiFi Configuration ====================
// Replace these with your WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIFI_RECONNECT_INTERVAL 5000  // milliseconds
#define WIFI_CONNECTION_TIMEOUT 20000 // milliseconds

// ==================== MQTT Configuration ====================
// Replace these with your MQTT broker details
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "esp32_1"
#define MQTT_USER ""  // Leave empty if no authentication
#define MQTT_PASSWORD ""  // Leave empty if no authentication

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
#define HC_SR04_TRIG_PIN 5
#define HC_SR04_ECHO_PIN 18

// Atlas Scientific pH Sensor (Analog)
#define PH_SENSOR_PIN 34        // ADC1_CH6, use ADC1 pins (32-39) for WiFi compatibility

// ==================== Sensor Configuration ====================
// Enable/Disable sensors (comment out to disable)
#define ENABLE_SHT30
#define ENABLE_HC_SR04
#define ENABLE_PH_SENSOR

// Moving Average Window Sizes
#define TEMP_HUMIDITY_WINDOW 5
#define WATER_LEVEL_WINDOW 10
#define PH_WINDOW 5

// Sensor Validation Ranges
#define TEMP_MIN -40.0
#define TEMP_MAX 125.0
#define HUMIDITY_MIN 0.0
#define HUMIDITY_MAX 100.0
#define WATER_LEVEL_MIN 20.0   // mm
#define WATER_LEVEL_MAX 4000.0 // mm
#define PH_MIN 0.0
#define PH_MAX 14.0

// HC-SR04 Specifics
#define HC_SR04_TIMEOUT 30000  // microseconds (30ms)
#define SOUND_SPEED 343.0      // m/s at 20°C

// pH Sensor Calibration (Analog version)
// Default linear calibration: 0V = pH 0, 3.3V = pH 14
#define PH_VOLTAGE_MIN 0.0
#define PH_VOLTAGE_MAX 3.3
#define PH_CALIBRATION_OFFSET 0.0   // Adjust after calibration
#define PH_CALIBRATION_SLOPE 1.0     // Adjust after calibration
#define ADC_RESOLUTION 4095.0        // 12-bit ADC

// ==================== Timing Configuration ====================
#define SENSOR_READ_INTERVAL 15000   // milliseconds (15 seconds)
#define HEALTH_MSG_INTERVAL 60000    // milliseconds (60 seconds)
#define WATCHDOG_TIMEOUT 60          // seconds

// ==================== Firmware Version ====================
#define FIRMWARE_VERSION "1.0.0"

// ==================== Debug Configuration ====================
#define SERIAL_BAUD_RATE 115200
// Uncomment to enable verbose debug output
// #define DEBUG_VERBOSE

// ==================== LED Indicator (Optional) ====================
#define LED_PIN 2  // Built-in LED on most ESP32 boards
#define ENABLE_LED_INDICATOR

#endif // CONFIG_H
