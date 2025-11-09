#ifndef HC_SR04_SENSOR_H
#define HC_SR04_SENSOR_H

#include "SensorBase.h"
#include "MovingAverage.h"
#include "config.h"

/**
 * @brief HC-SR04 Ultrasonic Distance Sensor
 * 
 * Measures distance using ultrasonic time-of-flight.
 * Used for water level monitoring in hydroponic systems.
 * Applies moving average filtering to reduce noise.
 */
class HC_SR04Sensor : public SensorBase {
private:
    uint8_t trigPin;
    uint8_t echoPin;
    MovingAverage<float, WATER_LEVEL_WINDOW> distanceAvg;
    float currentDistance;
    
    /**
     * @brief Measure distance using ultrasonic sensor
     * @return Distance in millimeters, or -1 on error
     */
    float measureDistance() {
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
    
public:
    /**
     * @brief Constructor
     * @param trig Trigger pin number
     * @param echo Echo pin number
     */
    HC_SR04Sensor(uint8_t trig, uint8_t echo) 
        : SensorBase("HC-SR04"), trigPin(trig), echoPin(echo), currentDistance(0.0) {}
    
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
        float testReading = measureDistance();
        
        if (testReading < 0) {
            Serial.println("[HC-SR04] WARNING: Initial test reading failed, but sensor initialized");
        } else {
            Serial.printf("[HC-SR04] Test reading: %.2f mm\n", testReading);
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
            lastReadSuccess = false;
            return false;
        }
        
        float distance = measureDistance();
        
        // Check for error
        if (distance < 0) {
            Serial.println("[HC-SR04] ERROR: Timeout or invalid reading");
            lastReadSuccess = false;
            return false;
        }
        
        // Validate range
        if (distance < WATER_LEVEL_MIN || distance > WATER_LEVEL_MAX) {
            Serial.printf("[HC-SR04] ERROR: Distance out of range: %.2f mm\n", distance);
            lastReadSuccess = false;
            return false;
        }
        
        // Add to moving average
        distanceAvg.add(distance);
        
        // Update current value with averaged reading
        currentDistance = distanceAvg.getAverage();
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[HC-SR04] Raw: %.2f mm | Avg: %.2f mm\n", distance, currentDistance);
        #endif
        
        lastReadSuccess = true;
        return true;
    }
    
    /**
     * @brief Get averaged distance value
     * @return Distance in millimeters
     */
    float getDistance() const {
        return currentDistance;
    }
    
    /**
     * @brief Get distance as formatted string
     * @param buffer Character buffer to store result
     * @param bufSize Size of buffer
     */
    void getDistanceString(char* buffer, size_t bufSize) const {
        snprintf(buffer, bufSize, "%.2f", currentDistance);
    }
};

#endif // HC_SR04_SENSOR_H
