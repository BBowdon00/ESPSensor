#ifndef SENSOR_BASE_H
#define SENSOR_BASE_H

#include <Arduino.h>
#include "MovingAverage.h"

/**
 * @brief Abstract base class for all sensors
 * 
 * This class provides a common interface for all sensor implementations.
 * Each sensor must implement the initialization, reading, and data retrieval methods.
 * Includes optional moving average functionality for sensor smoothing.
 */
class SensorBase {
protected:
    const char* sensorName;
    bool initialized;
    bool lastReadSuccess;
    unsigned long lastSuccessfulReadTime;  // millis() when last successful read occurred
    
    // Moving average functionality (optional - can be nullptr if not used)
    MovingAverage<float, 15>* movingAverage;  // Default 15-sample window
    bool useMovingAverage;
    
public:
    /**
     * @brief Constructor for SensorBase
     * @param name Name of the sensor
     * @param enableAvg Whether to enable moving average functionality
     */
    SensorBase(const char* name, bool enableAvg = false) 
        : sensorName(name), initialized(false), lastReadSuccess(false), 
          lastSuccessfulReadTime(0), movingAverage(nullptr), useMovingAverage(enableAvg) {
        if (useMovingAverage) {
            movingAverage = new MovingAverage<float, 15>();
        }
    }
    
    /**
     * @brief Virtual destructor
     */
    virtual ~SensorBase() {
        if (movingAverage) {
            delete movingAverage;
        }
    }
    
    /**
     * @brief Initialize the sensor
     * @return true if initialization successful, false otherwise
     */
    virtual bool begin() = 0;
    
    /**
     * @brief Read current sensor value(s)
     * @return true if read successful, false otherwise
     */
    virtual bool read() = 0;
    
    /**
     * @brief Get sensor name
     * @return Sensor name as const char*
     */
    const char* getName() const {
        return sensorName;
    }
    
    /**
     * @brief Check if sensor is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const {
        return initialized;
    }
    
    /**
     * @brief Check if last read was successful
     * @return true if last read successful, false otherwise
     */
    bool isLastReadSuccess() const {
        return lastReadSuccess;
    }
    
    /**
     * @brief Add a successful value to the moving average (if enabled)
     * @param value Value to add to the average
     * @return true if value was added, false if averaging not enabled
     */
    bool addToAverage(float value) {
        if (useMovingAverage && movingAverage) {
            movingAverage->add(value);
            return true;
        }
        return false;
    }
    
    /**
     * @brief Record a failed reading in the moving average (if enabled)
     * @return true if failure was recorded, false if averaging not enabled
     */
    bool addFailureToAverage() {
        if (useMovingAverage && movingAverage) {
            movingAverage->addFailure();
            return true;
        }
        return false;
    }
    
    /**
     * @brief Get the current moving average (if enabled)
     * @param defaultValue Value to return if averaging not enabled or no samples
     * @return Averaged value or defaultValue
     */
    float getAverage(float defaultValue = 0.0f) const {
        if (useMovingAverage && movingAverage) {
            return movingAverage->getAverage();
        }
        return defaultValue;
    }
    
    /**
     * @brief Check if moving average buffer is full
     * @return true if buffer is full, false otherwise
     */
    bool isAverageBufferFull() const {
        if (useMovingAverage && movingAverage) {
            return movingAverage->isFull();
        }
        return false;
    }
    
    /**
     * @brief Check if moving average is enabled
     * @return true if enabled, false otherwise
     */
    bool isMovingAverageEnabled() const {
        return useMovingAverage;
    }
    
    /**
     * @brief Check if more than half the readings in the moving average window are valid
     * @return true if > 50% of recent readings are valid
     */
    bool hasValidMajority() const {
        if (useMovingAverage && movingAverage) {
            return movingAverage->hasValidMajority();
        }
        return lastReadSuccess;  // Fallback to simple success check
    }
    
    /**
     * @brief Get the success rate of recent readings
     * @return Success rate as percentage (0.0 to 100.0)
     */
    float getSuccessRate() const {
        if (useMovingAverage && movingAverage) {
            return movingAverage->getSuccessRate();
        }
        return lastReadSuccess ? 100.0f : 0.0f;  // Fallback
    }
    
    /**
     * @brief Get count of valid readings in moving average window
     * @return Number of valid readings
     */
    size_t getValidReadingCount() const {
        if (useMovingAverage && movingAverage) {
            return movingAverage->getValidCount();
        }
        return lastReadSuccess ? 1 : 0;  // Fallback
    }
    
    /**
     * @brief Mark a successful read (updates timestamp)
     * Call this when a sensor read succeeds to track data freshness
     */
    void markSuccessfulRead() {
        lastReadSuccess = true;
        lastSuccessfulReadTime = millis();
    }
    
    /**
     * @brief Mark a failed read
     * Call this when a sensor read fails
     */
    void markFailedRead() {
        lastReadSuccess = false;
        // Don't update lastSuccessfulReadTime - keep the timestamp of last good read
    }
    
    /**
     * @brief Check if sensor data is fresh enough for publishing
     * @param maxAgeMs Maximum age in milliseconds for data to be considered fresh
     * @return true if data is fresh, false if stale or never read
     */
    bool isDataFresh(unsigned long maxAgeMs = 60000) const {
        if (lastSuccessfulReadTime == 0) {
            return false;  // Never had a successful read
        }
        
        unsigned long currentTime = millis();
        
        // Handle millis() rollover (happens every ~50 days)
        if (currentTime < lastSuccessfulReadTime) {
            // Rollover occurred, assume data is stale for safety
            return false;
        }
        
        return (currentTime - lastSuccessfulReadTime) <= maxAgeMs;
    }
    
    /**
     * @brief Get time since last successful read in milliseconds
     * @return Time in ms, or 0 if never read successfully
     */
    unsigned long getTimeSinceLastSuccess() const {
        if (lastSuccessfulReadTime == 0) {
            return 0;
        }
        
        unsigned long currentTime = millis();
        if (currentTime < lastSuccessfulReadTime) {
            // Handle rollover
            return 0;
        }
        
        return currentTime - lastSuccessfulReadTime;
    }
};

#endif // SENSOR_BASE_H
