#ifndef SHT30_SENSOR_H
#define SHT30_SENSOR_H

#include "SensorBase.h"
#include "MovingAverage.h"
#include "config.h"
#include <Adafruit_SHT31.h>

/**
 * @brief SHT30 Temperature and Humidity Sensor
 * 
 * Reads temperature and humidity from SHT30 sensor over I2C.
 * Applies moving average filtering for stable readings.
 */
class SHT30Sensor : public SensorBase {
private:
    Adafruit_SHT31 sht;
    MovingAverage<float, TEMP_HUMIDITY_WINDOW> tempAvg;
    MovingAverage<float, TEMP_HUMIDITY_WINDOW> humidityAvg;
    float currentTemp;
    float currentHumidity;
    
public:
    /**
     * @brief Constructor
     */
    SHT30Sensor() : SensorBase("SHT30"), currentTemp(0.0), currentHumidity(0.0) {}
    
    /**
     * @brief Initialize the SHT30 sensor
     * @return true if initialization successful, false otherwise
     */
    bool begin() override {
        Serial.println("[SHT30] Initializing sensor...");
        
        if (!sht.begin(SHT30_I2C_ADDRESS)) {
            Serial.println("[SHT30] ERROR: Failed to initialize sensor");
            initialized = false;
            return false;
        }
        
        Serial.println("[SHT30] Sensor initialized successfully");
        initialized = true;
        return true;
    }
    
    /**
     * @brief Read temperature and humidity from sensor
     * @return true if read successful, false otherwise
     */
    bool read() override {
        if (!initialized) {
            Serial.println("[SHT30] ERROR: Sensor not initialized");
            lastReadSuccess = false;
            return false;
        }
        
        float temp = sht.readTemperature();
        float humidity = sht.readHumidity();
        
        // Check if readings are valid
        if (isnan(temp) || isnan(humidity)) {
            Serial.println("[SHT30] ERROR: Failed to read sensor");
            lastReadSuccess = false;
            return false;
        }
        
        // Validate temperature range
        if (temp < TEMP_MIN || temp > TEMP_MAX) {
            Serial.printf("[SHT30] ERROR: Temperature out of range: %.2f°C\n", temp);
            lastReadSuccess = false;
            return false;
        }
        
        // Validate humidity range
        if (humidity < HUMIDITY_MIN || humidity > HUMIDITY_MAX) {
            Serial.printf("[SHT30] ERROR: Humidity out of range: %.2f%%\n", humidity);
            lastReadSuccess = false;
            return false;
        }
        
        // Add to moving average
        tempAvg.add(temp);
        humidityAvg.add(humidity);
        
        // Update current values with averaged readings
        currentTemp = tempAvg.getAverage();
        currentHumidity = humidityAvg.getAverage();
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[SHT30] Raw: T=%.2f°C, H=%.2f%% | Avg: T=%.2f°C, H=%.2f%%\n", 
                      temp, humidity, currentTemp, currentHumidity);
        #endif
        
        lastReadSuccess = true;
        return true;
    }
    
    /**
     * @brief Get averaged temperature value
     * @return Temperature in Celsius
     */
    float getTemperature() const {
        return currentTemp;
    }
    
    /**
     * @brief Get averaged humidity value
     * @return Humidity in percentage
     */
    float getHumidity() const {
        return currentHumidity;
    }
    
    /**
     * @brief Get temperature as formatted string
     * @param buffer Character buffer to store result
     * @param bufSize Size of buffer
     */
    void getTemperatureString(char* buffer, size_t bufSize) const {
        snprintf(buffer, bufSize, "%.2f", currentTemp);
    }
    
    /**
     * @brief Get humidity as formatted string
     * @param buffer Character buffer to store result
     * @param bufSize Size of buffer
     */
    void getHumidityString(char* buffer, size_t bufSize) const {
        snprintf(buffer, bufSize, "%.2f", currentHumidity);
    }
};

#endif // SHT30_SENSOR_H
