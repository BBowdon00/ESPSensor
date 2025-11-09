#ifndef SENSOR_BASE_H
#define SENSOR_BASE_H

#include <Arduino.h>

/**
 * @brief Abstract base class for all sensors
 * 
 * This class provides a common interface for all sensor implementations.
 * Each sensor must implement the initialization, reading, and data retrieval methods.
 */
class SensorBase {
protected:
    const char* sensorName;
    bool initialized;
    bool lastReadSuccess;
    
public:
    /**
     * @brief Constructor for SensorBase
     * @param name Name of the sensor
     */
    SensorBase(const char* name) : sensorName(name), initialized(false), lastReadSuccess(false) {}
    
    /**
     * @brief Virtual destructor
     */
    virtual ~SensorBase() {}
    
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
};

#endif // SENSOR_BASE_H
