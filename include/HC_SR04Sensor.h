#ifndef HC_SR04_SENSOR_H
#define HC_SR04_SENSOR_H

#include "SensorBase.h"
#include "config.h"

/**
 * @brief HC-SR04 Ultrasonic Water Level Sensor
 * 
 * Measures water level using ultrasonic distance sensor mounted on container lid.
 * Sensor points down into container, measures distance to water surface.
 * Converts distance to actual water level: water_level = container_height - distance
 * Used for water level monitoring in hydroponic reservoir systems.
 * Applies moving average filtering to reduce noise.
 */
class HC_SR04Sensor : public SensorBase {
private:
    uint8_t trigPin;
    uint8_t echoPin;
    float currentWaterLevel;
    float lastRawDistance;
    
    /**
     * @brief Measure raw distance using ultrasonic sensor
     * @return Distance in millimeters, or -1 on error
     */
    float measureRawDistance() {
        // Send 10us pulse to trigger
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        // Read echo pulse duration (in microseconds)
        long duration = pulseIn(echoPin, HIGH, HC_SR04_TIMEOUT);
        
        // Check for timeout or invalid reading
        if (duration == 0) {
            return -1.0;
        }
        
        // Calculate distance: distance = (time * speed_of_sound) / 2
        // Speed of sound = 343 m/s = 0.343 mm/us
        // Distance (mm) = (duration (us) * 0.343) / 2
        float distance = (duration * 0.343) / 2.0;
        
        return distance;
    }
    
    /**
     * @brief Convert raw distance to water level
     * @param distanceMm Raw distance measurement in millimeters
     * @return Water level in centimeters, or -1 on error
     */
    float convertToWaterLevel(float distanceMm) {
        if (distanceMm < 0) {
            return -1.0;  // Error propagation
        }
        
        // Convert distance from mm to cm
        float distanceCm = distanceMm / 10.0;
        
        // Calculate water level: container_height - distance_to_water
        float waterLevelCm = CONTAINER_HEIGHT_CM - distanceCm;
        
        return waterLevelCm;
    }
    
public:
    /**
     * @brief Constructor
     * @param trig Trigger pin number
     * @param echo Echo pin number
     */
    HC_SR04Sensor(uint8_t trig, uint8_t echo) 
        : SensorBase("HC-SR04", true), trigPin(trig), echoPin(echo), currentWaterLevel(0.0), lastRawDistance(0.0) {}
    
    /**
     * @brief Initialize the HC-SR04 sensor
     * @return true if initialization successful, false otherwise
     */
    bool begin() override {
        Serial.println("[HC-SR04] Initializing sensor...");
        
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
        
        digitalWrite(trigPin, LOW);
        
        // Test reading
        delay(100);
        float testDistance = measureRawDistance();
        
        if (testDistance < 0) {
            Serial.println("[HC-SR04] WARNING: Initial test reading failed, but sensor initialized");
        } else {
            float testWaterLevel = convertToWaterLevel(testDistance);
            Serial.printf("[HC-SR04] Test reading - Distance: %.1f mm, Water Level: %.1f cm\n", 
                         testDistance, testWaterLevel);
        }
        
        Serial.println("[HC-SR04] Sensor initialized successfully");
        initialized = true;
        return true;
    }
    
    /**
     * @brief Read distance from sensor
     * @return true if read successful, false otherwise
     */
    bool read() override {
        if (!initialized) {
            Serial.println("[HC-SR04] ERROR: Sensor not initialized");
            markFailedRead();
            return false;
        }
        
        // Measure raw distance
        float rawDistance = measureRawDistance();
        lastRawDistance = rawDistance;
        
        // Check for sensor error
        if (rawDistance < 0) {
            Serial.println("[HC-SR04] ERROR: Timeout or invalid reading");
            addFailureToAverage();  // Record failure in moving average
            lastReadSuccess = false;
            return false;
        }
        
        // Convert to water level
        float waterLevel = convertToWaterLevel(rawDistance);
        
        // Validate water level range - only add valid readings to moving average
        if (waterLevel < MIN_WATER_LEVEL_CM || waterLevel > MAX_WATER_LEVEL_CM) {
            Serial.printf("[HC-SR04] WARNING: Water level out of range: %.1f cm (distance: %.1f mm) - NOT added to average\n", 
                         waterLevel, rawDistance);
            
            // Check if this might be a raised lid condition
            if (waterLevel < MIN_WATER_LEVEL_CM) {
                Serial.println("[HC-SR04] Possible raised lid or empty container detected");
            }
            
            addFailureToAverage();  // Record failure in moving average
            lastReadSuccess = false;
            return false;  // Don't contaminate moving average with bad readings
        }
        
        // Only add valid readings to moving average
        addToAverage(waterLevel);
        
        // Update current value with averaged reading
        currentWaterLevel = getAverage(waterLevel);
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[HC-SR04] Raw: %.1fmm -> WaterLevel: %.1fcm | Avg: %.1fcm | Success: %.1f%% (%zu valid)\n", 
                      rawDistance, waterLevel, currentWaterLevel, 
                      getSuccessRate(), getValidReadingCount());
        #endif
        
        lastReadSuccess = true;
        return true;
    }
    
    /**
     * @brief Get averaged water level value
     * @return Water level in centimeters
     */
    float getWaterLevel() const {
        return currentWaterLevel;
    }
    
    /**
     * @brief Get water level as formatted string
     * @param buffer Character buffer to store result
     * @param bufSize Size of buffer
     */
    void getWaterLevelString(char* buffer, size_t bufSize) const {
        snprintf(buffer, bufSize, "%.1f", currentWaterLevel);
    }
    
    /**
     * @brief Get last raw distance measurement (for debugging)
     * @return Raw distance in millimeters
     */
    float getLastRawDistance() const {
        return lastRawDistance;
    }
    
    // Legacy method for compatibility with existing code
    float getDistance() const {
        return getWaterLevel();
    }
};

#endif // HC_SR04_SENSOR_H
