#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "SensorBase.h"
#include "MovingAverage.h"
#include "config.h"

/**
 * @brief Atlas Scientific pH Sensor (Analog version)
 * 
 * Reads pH value from analog voltage output.
 * Default calibration: 0V = pH 0, 3.3V = pH 14
 * Applies moving average filtering for stable readings.
 */
class PHSensor : public SensorBase {
private:
    uint8_t analogPin;
    MovingAverage<float, PH_WINDOW> phAvg;
    float currentPH;
    
    /**
     * @brief Read raw ADC value and convert to pH
     * @return pH value (0-14 scale), or -1 on error
     */
    float readPH() {
        // Read ADC value (12-bit: 0-4095)
        int rawValue = analogRead(analogPin);
        
        // Convert to voltage (0-3.3V)
        float voltage = (rawValue / ADC_RESOLUTION) * 3.3;
        
        // Convert voltage to pH using linear calibration
        // Default: pH = (voltage / 3.3) * 14
        float ph = ((voltage - PH_VOLTAGE_MIN) / (PH_VOLTAGE_MAX - PH_VOLTAGE_MIN)) * 14.0;
        
        // Apply calibration adjustments
        ph = (ph * PH_CALIBRATION_SLOPE) + PH_CALIBRATION_OFFSET;
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[pH] ADC: %d, Voltage: %.3fV, pH: %.2f\n", rawValue, voltage, ph);
        #endif
        
        return ph;
    }
    
public:
    /**
     * @brief Constructor
     * @param pin Analog input pin number
     */
    PHSensor(uint8_t pin) : SensorBase("pH"), analogPin(pin), currentPH(7.0) {}
    
    /**
     * @brief Initialize the pH sensor
     * @return true if initialization successful, false otherwise
     */
    bool begin() override {
        Serial.println("[pH] Initializing sensor...");
        
        // Configure ADC
        pinMode(analogPin, INPUT);
        
        // ESP32 ADC configuration
        // Set ADC attenuation to 11dB for full 0-3.3V range
        analogSetAttenuation(ADC_11db);
        
        // Take a test reading
        delay(100);
        float testPH = readPH();
        
        if (testPH < 0 || testPH > 14) {
            Serial.printf("[pH] WARNING: Test reading out of range: %.2f\n", testPH);
        } else {
            Serial.printf("[pH] Test reading: %.2f\n", testPH);
        }
        
        Serial.println("[pH] Sensor initialized successfully");
        Serial.println("[pH] NOTE: Default calibration in use. Calibrate for accurate readings.");
        
        initialized = true;
        return true;
    }
    
    /**
     * @brief Read pH value from sensor
     * @return true if read successful, false otherwise
     */
    bool read() override {
        if (!initialized) {
            Serial.println("[pH] ERROR: Sensor not initialized");
            lastReadSuccess = false;
            return false;
        }
        
        float ph = readPH();
        
        // Validate range
        if (ph < PH_MIN || ph > PH_MAX) {
            Serial.printf("[pH] ERROR: pH out of range: %.2f\n", ph);
            lastReadSuccess = false;
            return false;
        }
        
        // Add to moving average
        phAvg.add(ph);
        
        // Update current value with averaged reading
        currentPH = phAvg.getAverage();
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[pH] Raw: %.2f | Avg: %.2f\n", ph, currentPH);
        #endif
        
        lastReadSuccess = true;
        return true;
    }
    
    /**
     * @brief Get averaged pH value
     * @return pH value (0-14 scale)
     */
    float getPH() const {
        return currentPH;
    }
    
    /**
     * @brief Get pH as formatted string
     * @param buffer Character buffer to store result
     * @param bufSize Size of buffer
     */
    void getPHString(char* buffer, size_t bufSize) const {
        snprintf(buffer, bufSize, "%.2f", currentPH);
    }
};

#endif // PH_SENSOR_H
