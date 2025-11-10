# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-11-09

### Added
- Initial release of ESP32 Hydroponic Sensor Monitor firmware
- Support for SHT30 temperature and humidity sensor (I2C)
- Support for HC-SR04 ultrasonic distance sensor for water level measurement
- Support for Atlas Scientific pH sensor (analog version)
- Moving average filtering for all sensors with configurable window sizes
- WiFi connectivity with automatic reconnection (5-second intervals)
- MQTT publishing with QoS 0 for sensor data and QoS 1 for health messages
- OTA (Over-The-Air) firmware updates with password protection
- ESP32 watchdog timer for crash recovery (60-second timeout)
- Non-blocking main loop using millis() for timing
- LED status indicator with different blink patterns for system status
- Comprehensive error handling and graceful degradation
- Sensor value validation with min/max range checking
- Exponential backoff for MQTT reconnection
- Serial debug logging with optional verbose mode
- Modular sensor architecture with abstract base class
- Configuration file with clearly documented settings
- Comprehensive README with wiring diagrams and troubleshooting guide

### Technical Details
- Platform: ESP32-WROOM with Arduino framework
- Build System: PlatformIO
- Memory Usage: ~180KB program storage, ~30KB RAM
- Update Intervals: Sensors every 15s, Health every 60s
- MQTT Topic Structure: {project}/{node}/{deviceCategory}

### Dependencies
- Adafruit SHT31 Library v2.2.2+
- PubSubClient v2.8+
- ArduinoJson v6.21.3+
- Built-in: WiFi, ArduinoOTA, Wire, esp_task_wdt

[1.0.0]: https://github.com/BBowdon00/ESPSensor/releases/tag/v1.0.0
