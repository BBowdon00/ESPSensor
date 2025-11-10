#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <esp_task_wdt.h>
#include "config.h"

// Sensor includes
#ifdef ENABLE_SHT30
#include "SHT30Sensor.h"
#endif

#ifdef ENABLE_HC_SR04
#include "HC_SR04Sensor.h"
#endif

#ifdef ENABLE_PH_SENSOR
#include "PHSensor.h"
#endif

// ==================== Global Objects ====================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Sensor instances
#ifdef ENABLE_SHT30
SHT30Sensor sht30Sensor;
#endif

#ifdef ENABLE_HC_SR04
HC_SR04Sensor waterLevelSensor(HC_SR04_TRIG_PIN, HC_SR04_ECHO_PIN);
#endif

#ifdef ENABLE_PH_SENSOR
PHSensor phSensor(PH_SENSOR_PIN);
#endif

// ==================== Timing Variables ====================
unsigned long lastSensorRead = 0;
unsigned long lastSensorPublish = 0;
unsigned long lastHealthMsg = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;
unsigned long mqttReconnectDelay = MQTT_RECONNECT_INITIAL_DELAY;

// ==================== LED Indicator ====================
#ifdef ENABLE_LED_INDICATOR
unsigned long lastLedBlink = 0;
bool ledState = false;
#endif

// ==================== Function Prototypes ====================
void setupWiFi();
void reconnectWiFi();
void setupMQTT();
void reconnectMQTT();
void setupOTA();
void initializeSensors();
void readSensors();
void publishSensorData();
void publishHealthMessage();
void updateLEDIndicator();

// ==================== Setup Function ====================
void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    Serial.println("\n\n");
    Serial.println("====================================");
    Serial.println("ESP32 Hydroponic Sensor Monitor");
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    Serial.println("====================================");
    
    #ifdef ENABLE_LED_INDICATOR
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    #endif
    
    // Initialize I2C for SHT30
    #ifdef ENABLE_SHT30
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("[I2C] Initialized on pins SDA=" + String(I2C_SDA) + ", SCL=" + String(I2C_SCL));
    #endif
    
    // Initialize WiFi
    setupWiFi();
    
    // Initialize MQTT
    setupMQTT();
    
    // Initialize OTA
    setupOTA();
    
    // Initialize sensors
    initializeSensors();
    
    // Initialize watchdog timer (60 seconds)
    Serial.println("[WDT] Configuring watchdog timer...");
    esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
    esp_task_wdt_add(NULL);
    Serial.println("[WDT] Watchdog timer enabled");
    
    Serial.println("\n[SYSTEM] Setup complete. Starting main loop...\n");
}

// ==================== Main Loop ====================
unsigned long lastStatusLog = 0;
#define STATUS_LOG_INTERVAL 300000  // Log status every 5 minutes

void loop() {
    // Reset watchdog timer
    esp_task_wdt_reset();
    
    // Handle OTA updates
    ArduinoOTA.handle();
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        reconnectWiFi();
    }
    
    // Check MQTT connection
    if (!mqttClient.connected()) {
        reconnectMQTT();
    } else {
        mqttClient.loop();
    }
    
    // Update LED indicator
    #ifdef ENABLE_LED_INDICATOR
    updateLEDIndicator();
    #endif
    
    unsigned long currentMillis = millis();
    
    // Periodic status logging (every 5 minutes)
    if (currentMillis - lastStatusLog >= STATUS_LOG_INTERVAL) {
        lastStatusLog = currentMillis;
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println("║     PERIODIC STATUS REPORT             ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.printf("[STATUS] Uptime: %lu seconds (%.2f hours)\n", 
                      currentMillis / 1000, (currentMillis / 1000) / 3600.0);
        Serial.printf("[STATUS] WiFi: %s (RSSI: %d dBm)\n", 
                      WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                      WiFi.RSSI());
        Serial.printf("[STATUS] MQTT: %s\n", 
                      mqttClient.connected() ? "Connected" : "Disconnected");
        Serial.printf("[STATUS] Free Heap: %d bytes (%.2f KB)\n", 
                      ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
        Serial.println("════════════════════════════════════════\n");
    }
    
    // Read sensors at regular intervals (for moving average data collection)
    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentMillis;
        Serial.printf("\n[LOOP] Next sensor read at: %lu ms (in %lu seconds)\n", 
                      currentMillis + SENSOR_READ_INTERVAL, 
                      SENSOR_READ_INTERVAL / 1000);
        readSensors();
    }
    
    // Publish sensor data at regular intervals
    if (currentMillis - lastSensorPublish >= SENSOR_PUBLISH_INTERVAL) {
        lastSensorPublish = currentMillis;
        Serial.printf("\n[LOOP] Next sensor publish at: %lu ms (in %lu seconds)\n", 
                      currentMillis + SENSOR_PUBLISH_INTERVAL, 
                      SENSOR_PUBLISH_INTERVAL / 1000);
        publishSensorData();
    }
    
    // Publish health message at regular intervals
    if (currentMillis - lastHealthMsg >= HEALTH_MSG_INTERVAL) {
        lastHealthMsg = currentMillis;
        Serial.printf("\n[LOOP] Next health message at: %lu ms (in %lu seconds)\n", 
                      currentMillis + HEALTH_MSG_INTERVAL, 
                      HEALTH_MSG_INTERVAL / 1000);
        publishHealthMessage();
    }
    
    // Small delay to prevent tight looping
    delay(10);
}

// ==================== WiFi Functions ====================
void setupWiFi() {
    Serial.println("\n[WiFi] Initializing WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print("[WiFi] Connecting to ");
    Serial.print(WIFI_SSID);
    
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_CONNECTION_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Connected!");
        Serial.print("[WiFi] IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WiFi] Signal Strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("\n[WiFi] Failed to connect. Will retry in background.");
    }
    
    lastWiFiAttempt = millis();
}

void reconnectWiFi() {
    unsigned long currentMillis = millis();
    
    // Only attempt reconnection at intervals
    if (currentMillis - lastWiFiAttempt < WIFI_RECONNECT_INTERVAL) {
        return;
    }
    
    lastWiFiAttempt = currentMillis;
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[WiFi] ⚠ Connection lost - attempting reconnection...");
        Serial.printf("[WiFi] Last successful connection: %lu ms ago\n", 
                      currentMillis - lastWiFiAttempt + WIFI_RECONNECT_INTERVAL);
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        // Wait briefly for connection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 10) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[WiFi] ✓ Reconnected successfully!");
            Serial.printf("[WiFi] IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] Signal Strength: %d dBm\n", WiFi.RSSI());
        } else {
            Serial.println("\n[WiFi] ✗ Reconnection failed, will retry");
        }
    }
}

// ==================== MQTT Functions ====================
void setupMQTT() {
    Serial.println("\n[MQTT] Configuring MQTT client...");
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setBufferSize(512);  // Increase buffer for JSON messages
    
    Serial.printf("[MQTT] Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    Serial.printf("[MQTT] Client ID: %s\n", MQTT_CLIENT_ID);
    
    // Attempt initial connection
    reconnectMQTT();
}

void reconnectMQTT() {
    // Only attempt if WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    unsigned long currentMillis = millis();
    
    // Check if we should attempt reconnection based on backoff delay
    if (currentMillis - lastMQTTAttempt < mqttReconnectDelay) {
        return;
    }
    
    lastMQTTAttempt = currentMillis;
    
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Attempting connection...");
        
        bool connected = false;
        if (strlen(MQTT_USER) > 0) {
            connected = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
        } else {
            connected = mqttClient.connect(MQTT_CLIENT_ID);
        }
        
        if (connected) {
            Serial.println("[MQTT] Connected!");
            mqttReconnectDelay = MQTT_RECONNECT_INITIAL_DELAY;  // Reset backoff
        } else {
            Serial.print("[MQTT] Connection failed, rc=");
            Serial.println(mqttClient.state());
            
            // Exponential backoff
            mqttReconnectDelay = min(mqttReconnectDelay * 2, (unsigned long)MQTT_RECONNECT_MAX_DELAY);
            Serial.printf("[MQTT] Will retry in %lu ms\n", mqttReconnectDelay);
        }
    }
}

// ==================== OTA Functions ====================
void setupOTA() {
    Serial.println("\n[OTA] Configuring OTA updates...");
    
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(OTA_PORT);
    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {
            type = "filesystem";
        }
        Serial.println("[OTA] Start updating " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update complete!");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    
    ArduinoOTA.begin();
    Serial.println("[OTA] OTA ready");
    Serial.printf("[OTA] Hostname: %s.local\n", OTA_HOSTNAME);
}

// ==================== Sensor Functions ====================
void initializeSensors() {
    Serial.println("\n[SENSORS] Initializing sensors...");
    
    #ifdef ENABLE_SHT30
    if (!sht30Sensor.begin()) {
        Serial.println("[SENSORS] WARNING: SHT30 initialization failed");
    }
    #endif
    
    #ifdef ENABLE_HC_SR04
    if (!waterLevelSensor.begin()) {
        Serial.println("[SENSORS] WARNING: HC-SR04 initialization failed");
    }
    #endif
    
    #ifdef ENABLE_PH_SENSOR
    if (!phSensor.begin()) {
        Serial.println("[SENSORS] WARNING: pH sensor initialization failed");
    }
    #endif
    
    Serial.println("[SENSORS] Sensor initialization complete\n");
}

void readSensors() {
    #ifdef DEBUG_VERBOSE
    Serial.printf("\n[SENSORS] Reading sensors for moving average (uptime: %lu s)\n", millis() / 1000);
    #endif
    
    int successCount = 0;
    int failCount = 0;
    
    #ifdef ENABLE_SHT30
    if (sht30Sensor.isInitialized()) {
        if (sht30Sensor.read()) {
            #ifdef DEBUG_VERBOSE
            Serial.printf("[SHT30] ✓ T:%.1f°C H:%.1f%%\n", 
                         sht30Sensor.getTemperature(), sht30Sensor.getHumidity());
            #endif
            successCount += 2;
        } else {
            Serial.println("[SHT30] ✗ Read failed");
            failCount += 2;
        }
    } else {
        failCount += 2;
    }
    #endif
    
    #ifdef ENABLE_HC_SR04
    if (waterLevelSensor.isInitialized()) {
        if (waterLevelSensor.read()) {
            #ifdef DEBUG_VERBOSE
            Serial.printf("[HC-SR04] ✓ %.1fcm\n", waterLevelSensor.getWaterLevel());
            #endif
            successCount++;
        } else {
            Serial.println("[HC-SR04] ✗ Read failed");
            failCount++;
        }
    } else {
        failCount++;
    }
    #endif
    
    #ifdef ENABLE_PH_SENSOR
    if (phSensor.isInitialized()) {
        if (phSensor.read()) {
            #ifdef DEBUG_VERBOSE
            Serial.printf("[pH] ✓ %.2f\n", phSensor.getPH());
            #endif
            successCount++;
        } else {
            Serial.println("[pH] ✗ Read failed");
            failCount++;
        }
    } else {
        failCount++;
    }
    #endif
    
    #ifdef DEBUG_VERBOSE
    if (failCount > 0) {
        Serial.printf("[SENSORS] Summary: %d ok, %d failed\n", successCount, failCount);
    }
    #endif
}

void publishSensorData() {
    if (!mqttClient.connected()) {
        Serial.println("\n[MQTT] ✗ Not connected, skipping sensor publish");
        return;
    }
    
    Serial.println("\n========================================");
    Serial.println("[MQTT] === Publishing Sensor Data ===");
    Serial.printf("[MQTT] Topic: %s\n", MQTT_TOPIC_SENSOR);
    Serial.println("========================================");
    
    int publishCount = 0;
    int failCount = 0;
    
    // Publish each sensor individually following the Hydroponic Monitor message format
    #ifdef ENABLE_SHT30
    if (sht30Sensor.isInitialized()) {
        // Publish temperature if majority of readings are valid
        if (sht30Sensor.hasValidTemperatureMajority()) {
            StaticJsonDocument<300> doc;
            doc["deviceType"] = "temperature";
            doc["deviceID"] = "1";
            doc["location"] = DEVICE_LOCATION;
            doc["value"] = String(sht30Sensor.getTemperature(), 2);
            doc["description"] = String(DEVICE_DESCRIPTION_PREFIX) + " - temperature";
            
            char buffer[300];
            serializeJson(doc, buffer);
            
            #ifdef DEBUG_VERBOSE
            Serial.printf("[MQTT] Temperature payload: %s\n", buffer);
            #endif
            
            if (mqttClient.publish(MQTT_TOPIC_SENSOR, buffer, false)) {
                Serial.printf("[MQTT] ✓ Temperature published: %.2f°C (%.1f%% success rate)\n", 
                             sht30Sensor.getTemperature(), sht30Sensor.getTemperatureSuccessRate());
                publishCount++;
            } else {
                Serial.println("[MQTT] ✗ Failed to publish temperature");
                failCount++;
            }
        } else {
            Serial.printf("[MQTT] ⊘ Skipping temperature (success rate: %.1f%%, need >50%%)\n", 
                         sht30Sensor.getTemperatureSuccessRate());
        }
        
        // Publish humidity if majority of readings are valid
        if (sht30Sensor.hasValidHumidityMajority()) {
            StaticJsonDocument<300> doc;
            doc["deviceType"] = "humidity";
            doc["deviceID"] = "1";
            doc["location"] = DEVICE_LOCATION;
            doc["value"] = String(sht30Sensor.getHumidity(), 2);
            doc["description"] = String(DEVICE_DESCRIPTION_PREFIX) + " - humidity";
            
            char buffer[300];
            serializeJson(doc, buffer);
            
            #ifdef DEBUG_VERBOSE
            Serial.printf("[MQTT] Humidity payload: %s\n", buffer);
            #endif
            
            if (mqttClient.publish(MQTT_TOPIC_SENSOR, buffer, false)) {
                Serial.printf("[MQTT] ✓ Humidity published: %.2f%% (%.1f%% success rate)\n", 
                             sht30Sensor.getHumidity(), sht30Sensor.getHumiditySuccessRate());
                publishCount++;
            } else {
                Serial.println("[MQTT] ✗ Failed to publish humidity");
                failCount++;
            }
        } else {
            Serial.printf("[MQTT] ⊘ Skipping humidity (success rate: %.1f%%, need >50%%)\n", 
                         sht30Sensor.getHumiditySuccessRate());
        }
    } else {
        Serial.println("[MQTT] ⊘ Skipping temperature/humidity (sensor not initialized)");
    }
    #endif
    
    #ifdef ENABLE_HC_SR04
    if (waterLevelSensor.isInitialized() && waterLevelSensor.hasValidMajority()) {
        StaticJsonDocument<300> doc;
        doc["deviceType"] = "waterLevel";
        doc["deviceID"] = "1";
        doc["location"] = DEVICE_LOCATION;
        doc["value"] = String(waterLevelSensor.getWaterLevel(), 1);
        doc["description"] = String(DEVICE_DESCRIPTION_PREFIX) + " - water level";
        
        char buffer[300];
        serializeJson(doc, buffer);
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[MQTT] Water level payload: %s\n", buffer);
        #endif
        
        if (mqttClient.publish(MQTT_TOPIC_SENSOR, buffer, false)) {
            Serial.printf("[MQTT] ✓ Water level published: %.1f cm\n", waterLevelSensor.getWaterLevel());
            publishCount++;
        } else {
            Serial.println("[MQTT] ✗ Failed to publish water level");
            failCount++;
        }
    } else {
        if (waterLevelSensor.isInitialized()) {
            Serial.printf("[MQTT] ⊘ Skipping water level (success rate: %.1f%%, need >50%%) - lid may be raised\n", 
                         waterLevelSensor.getSuccessRate());
        } else {
            Serial.println("[MQTT] ⊘ Skipping water level (sensor not ready)");
        }
    }
    #endif
    
    #ifdef ENABLE_PH_SENSOR
    if (phSensor.isInitialized() && phSensor.hasValidMajority()) {
        StaticJsonDocument<300> doc;
        doc["deviceType"] = "pH";
        doc["deviceID"] = "1";
        doc["location"] = DEVICE_LOCATION;
        doc["value"] = String(phSensor.getPH(), 2);
        doc["description"] = String(DEVICE_DESCRIPTION_PREFIX) + " - pH sensor";
        
        char buffer[300];
        serializeJson(doc, buffer);
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[MQTT] pH payload: %s\n", buffer);
        #endif
        
        if (mqttClient.publish(MQTT_TOPIC_SENSOR, buffer, false)) {
            Serial.printf("[MQTT] ✓ pH published: %.2f\n", phSensor.getPH());
            publishCount++;
        } else {
            Serial.println("[MQTT] ✗ Failed to publish pH");
            failCount++;
        }
    } else {
        if (phSensor.isInitialized()) {
            Serial.printf("[MQTT] ⊘ Skipping pH (success rate: %.1f%%, need >50%%)\n", 
                         phSensor.getSuccessRate());
        } else {
            Serial.println("[MQTT] ⊘ Skipping pH (sensor not ready)");
        }
    }
    #endif
    
    Serial.println("========================================");
    Serial.printf("[MQTT] Publish Summary: %d successful, %d failed\n", publishCount, failCount);
    Serial.println("========================================\n");
}

void publishHealthMessage() {
    if (!mqttClient.connected()) {
        Serial.println("\n[MQTT] ✗ Not connected, skipping health publish");
        return;
    }
    
    Serial.println("\n========================================");
    Serial.println("[MQTT] === Publishing Health Message ===");
    Serial.printf("[MQTT] Topic: %s\n", MQTT_TOPIC_HEALTH);
    Serial.println("========================================");
    
    StaticJsonDocument<512> doc;
    doc["deviceId"] = MQTT_CLIENT_ID;
    doc["status"] = "online";
    doc["uptime"] = millis() / 1000;  // seconds
    doc["firmwareVersion"] = FIRMWARE_VERSION;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["rssi"] = WiFi.RSSI();
    
    // Add sensor status
    JsonObject sensors = doc.createNestedObject("sensors");
    
    #ifdef ENABLE_SHT30
    sensors["temperature"] = sht30Sensor.isInitialized() ? "ok" : "error";
    sensors["humidity"] = sht30Sensor.isInitialized() ? "ok" : "error";
    #endif
    
    #ifdef ENABLE_HC_SR04
    sensors["waterLevel"] = waterLevelSensor.isInitialized() ? "ok" : "error";
    #endif
    
    #ifdef ENABLE_PH_SENSOR
    sensors["pH"] = phSensor.isInitialized() ? "ok" : "error";
    #endif
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    // Print health details
    Serial.printf("[HEALTH] Device ID: %s\n", MQTT_CLIENT_ID);
    Serial.printf("[HEALTH] Uptime: %lu seconds (%.2f hours)\n", 
                  millis() / 1000, (millis() / 1000) / 3600.0);
    Serial.printf("[HEALTH] Firmware: %s\n", FIRMWARE_VERSION);
    Serial.printf("[HEALTH] Free Heap: %d bytes (%.2f KB)\n", 
                  ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    Serial.printf("[HEALTH] WiFi RSSI: %d dBm\n", WiFi.RSSI());
    
    #ifdef DEBUG_VERBOSE
    Serial.printf("[HEALTH] JSON payload: %s\n", buffer);
    #endif
    
    // Use QoS 1 for health messages
    if (mqttClient.publish(MQTT_TOPIC_HEALTH, buffer, true)) {
        Serial.println("[MQTT] ✓ Health message published successfully");
    } else {
        Serial.println("[MQTT] ✗ Failed to publish health message");
    }
    
    Serial.println("========================================\n");
}

// ==================== LED Indicator Function ====================
#ifdef ENABLE_LED_INDICATOR
void updateLEDIndicator() {
    unsigned long currentMillis = millis();
    
    // Different blink patterns based on status
    int blinkInterval = 0;
    
    if (WiFi.status() != WL_CONNECTED) {
        // Fast blink when WiFi disconnected (200ms)
        blinkInterval = 200;
    } else if (!mqttClient.connected()) {
        // Medium blink when MQTT disconnected (500ms)
        blinkInterval = 500;
    } else {
        // Slow blink when all OK (2000ms)
        blinkInterval = 2000;
    }
    
    if (currentMillis - lastLedBlink >= blinkInterval) {
        lastLedBlink = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }
}
#endif
