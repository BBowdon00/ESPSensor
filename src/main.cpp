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
    
    // Read sensors at regular intervals
    unsigned long currentMillis = millis();
    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentMillis;
        readSensors();
        publishSensorData();
    }
    
    // Publish health message at regular intervals
    if (currentMillis - lastHealthMsg >= HEALTH_MSG_INTERVAL) {
        lastHealthMsg = currentMillis;
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
        Serial.println("[WiFi] Reconnecting...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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
    Serial.println("\n[SENSORS] Reading all sensors...");
    
    #ifdef ENABLE_SHT30
    if (sht30Sensor.isInitialized()) {
        if (sht30Sensor.read()) {
            Serial.printf("[SENSORS] Temperature: %.2f°C, Humidity: %.2f%%\n", 
                         sht30Sensor.getTemperature(), sht30Sensor.getHumidity());
        }
    }
    #endif
    
    #ifdef ENABLE_HC_SR04
    if (waterLevelSensor.isInitialized()) {
        if (waterLevelSensor.read()) {
            Serial.printf("[SENSORS] Water Level: %.2f mm\n", waterLevelSensor.getDistance());
        }
    }
    #endif
    
    #ifdef ENABLE_PH_SENSOR
    if (phSensor.isInitialized()) {
        if (phSensor.read()) {
            Serial.printf("[SENSORS] pH: %.2f\n", phSensor.getPH());
        }
    }
    #endif
}

void publishSensorData() {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Not connected, skipping sensor publish");
        return;
    }
    
    Serial.println("\n[MQTT] Publishing sensor data...");
    
    // Publish each sensor individually following the message format
    #ifdef ENABLE_SHT30
    if (sht30Sensor.isInitialized() && sht30Sensor.isLastReadSuccess()) {
        StaticJsonDocument<200> doc;
        doc["type"] = "temperature";
        doc["value"] = String(sht30Sensor.getTemperature(), 2);
        doc["unit"] = "°C";
        
        char buffer[200];
        serializeJson(doc, buffer);
        
        if (mqttClient.publish(MQTT_TOPIC_SENSOR, buffer, false)) {
            Serial.println("[MQTT] ✓ Temperature published");
        } else {
            Serial.println("[MQTT] ✗ Failed to publish temperature");
        }
        
        // Publish humidity
        doc.clear();
        doc["type"] = "humidity";
        doc["value"] = String(sht30Sensor.getHumidity(), 2);
        doc["unit"] = "%";
        
        serializeJson(doc, buffer);
        
        if (mqttClient.publish(MQTT_TOPIC_SENSOR, buffer, false)) {
            Serial.println("[MQTT] ✓ Humidity published");
        } else {
            Serial.println("[MQTT] ✗ Failed to publish humidity");
        }
    }
    #endif
    
    #ifdef ENABLE_HC_SR04
    if (waterLevelSensor.isInitialized() && waterLevelSensor.isLastReadSuccess()) {
        StaticJsonDocument<200> doc;
        doc["type"] = "waterLevel";
        doc["value"] = String(waterLevelSensor.getDistance(), 2);
        doc["unit"] = "mm";
        
        char buffer[200];
        serializeJson(doc, buffer);
        
        if (mqttClient.publish(MQTT_TOPIC_SENSOR, buffer, false)) {
            Serial.println("[MQTT] ✓ Water level published");
        } else {
            Serial.println("[MQTT] ✗ Failed to publish water level");
        }
    }
    #endif
    
    #ifdef ENABLE_PH_SENSOR
    if (phSensor.isInitialized() && phSensor.isLastReadSuccess()) {
        StaticJsonDocument<200> doc;
        doc["type"] = "pH";
        doc["value"] = String(phSensor.getPH(), 2);
        doc["unit"] = "";
        
        char buffer[200];
        serializeJson(doc, buffer);
        
        if (mqttClient.publish(MQTT_TOPIC_SENSOR, buffer, false)) {
            Serial.println("[MQTT] ✓ pH published");
        } else {
            Serial.println("[MQTT] ✗ Failed to publish pH");
        }
    }
    #endif
}

void publishHealthMessage() {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Not connected, skipping health publish");
        return;
    }
    
    Serial.println("\n[MQTT] Publishing health message...");
    
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
    
    // Use QoS 1 for health messages
    if (mqttClient.publish(MQTT_TOPIC_HEALTH, buffer, true)) {
        Serial.println("[MQTT] ✓ Health message published");
    } else {
        Serial.println("[MQTT] ✗ Failed to publish health message");
    }
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
