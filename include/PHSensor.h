#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "SensorBase.h"
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
    float currentPH;
    
    /**
     * @brief Convert voltage to pH using Atlas Scientific piecewise linear calibration
     * @param voltage_mV Voltage reading in millivolts
     * @return pH value using 3-point calibration method
     */
    float readPHFromVoltage(float voltage_mV) {
        // Atlas Scientific piecewise linear method
        if (voltage_mV > PH_CAL_MID) { 
            // High voltage = low pH (acidic range: pH 4-7)
            // Uses low_cal and mid_cal calibration points
            return 7.0 - 3.0 / (PH_CAL_LOW - PH_CAL_MID) * (voltage_mV - PH_CAL_MID);
        } else {
            // Low voltage = high pH (basic range: pH 7-10) 
            // Uses mid_cal and high_cal calibration points
            return 7.0 - 3.0 / (PH_CAL_MID - PH_CAL_HIGH) * (voltage_mV - PH_CAL_MID);
        }
    }

    /**
     * @brief Read raw ADC value and convert to pH
     * @return pH value (0-14 scale), or -1 on error
     */
    float readPH() {
        // Debug: Read raw ADC values first
        int totalRawADC = 0;
        float voltage_mV = 0;
        
        for (int i = 0; i < PH_VOLTAGE_AVERAGING; ++i) {
            int rawADC = analogRead(analogPin);
            totalRawADC += rawADC;
            // ESP32 ADC with compensation (Atlas Scientific method)
            voltage_mV += rawADC / 4095.0 * 3300.0 + ESP32_ADC_OFFSET_MV;
        }
        voltage_mV /= PH_VOLTAGE_AVERAGING;
        int avgRawADC = totalRawADC / PH_VOLTAGE_AVERAGING;
        
        // Debug output for troubleshooting
        Serial.printf("[pH] DEBUG: Pin %d, Raw ADC: %d, Voltage: %.1fmV\n", 
                      analogPin, avgRawADC, voltage_mV);
        
        // Convert voltage to pH using Atlas Scientific piecewise linear calibration
        float ph = readPHFromVoltage(voltage_mV);
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[pH] Voltage: %.1fmV, pH: %.2f, Range: %s\n", 
                      voltage_mV, ph, 
                      (voltage_mV > PH_CAL_MID) ? "Acidic(4-7)" : "Basic(7-10)");
        #endif
        
        return ph;
    }
    
public:
    /**
     * @brief Constructor
     * @param pin Analog input pin number
     */
    PHSensor(uint8_t pin) : SensorBase("pH", true), analogPin(pin), currentPH(7.0) {}
    
    /**
     * @brief Initialize the pH sensor
     * @return true if initialization successful, false otherwise
     */
    bool begin() override {
        Serial.println("[pH] Initializing sensor...");
        
        // Configure ADC
        pinMode(analogPin, INPUT);
        
        // ESP32 ADC configuration - detailed setup
        Serial.printf("[pH] Configuring ADC on pin %d (ADC1_CH6)\n", analogPin);
        
        // Set ADC attenuation to 11dB for full 0-3.3V range
        analogSetAttenuation(ADC_11db);
        
        // Additional ESP32 ADC setup
        analogSetWidth(12);  // 12-bit resolution (0-4095)
        
        // Test raw ADC reading first
        delay(100);
        int rawTest = analogRead(analogPin);
        Serial.printf("[pH] Raw ADC test reading: %d (should be 0-4095)\n", rawTest);
        
        if (rawTest == 0) {
            Serial.println("[pH] WARNING: ADC reading 0 - check wiring and sensor connection!");
            Serial.println("[pH] Troubleshooting:");
            Serial.println("[pH] 1. Verify sensor is connected to pin 34");
            Serial.println("[pH] 2. Check sensor power supply (3.3V or 5V)");
            Serial.println("[pH] 3. Verify sensor output is within 0-3.3V range");
            Serial.println("[pH] 4. Test with multimeter on pin 34");
        }
        
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
            addFailureToAverage();  // Record failure in moving average
            lastReadSuccess = false;
            return false;
        }
        
        float ph = readPH();
        
        // Validate range
        if (ph < PH_MIN || ph > PH_MAX) {
            Serial.printf("[pH] ERROR: pH out of range: %.2f\n", ph);
            addFailureToAverage();  // Record failure in moving average
            lastReadSuccess = false;
            return false;
        }
        
        // Add to moving average
        addToAverage(ph);
        
        // Update current value with averaged reading
        currentPH = getAverage(ph);
        
        #ifdef DEBUG_VERBOSE
        Serial.printf("[pH] Raw: %.2f | Avg: %.2f | Success: %.1f%% (%zu valid)\n", 
                      ph, currentPH, getSuccessRate(), getValidReadingCount());
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
