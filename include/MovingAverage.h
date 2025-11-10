#ifndef MOVING_AVERAGE_H
#define MOVING_AVERAGE_H

#include <Arduino.h>

/**
 * @brief Template class for calculating moving average of sensor readings
 * @tparam T Data type (float, int, etc.)
 * @tparam SIZE Window size for moving average
 */
template <typename T, size_t SIZE>
class MovingAverage {
private:
    T buffer[SIZE];
    bool validBuffer[SIZE];  // Track which entries are valid vs failed reads
    size_t index;
    size_t count;
    T sum;
    size_t validCount;  // Count of valid readings in the window
    
public:
    MovingAverage() : index(0), count(0), sum(0), validCount(0) {
        for (size_t i = 0; i < SIZE; i++) {
            buffer[i] = 0;
            validBuffer[i] = false;
        }
    }
    
    /**
     * @brief Add a new successful value to the moving average
     * @param value New value to add
     */
    void add(T value) {
        addReading(value, true);
    }
    
    /**
     * @brief Record a failed reading (no value, just track the failure)
     */
    void addFailure() {
        addReading(T(0), false);  // Use default value, mark as invalid
    }
    
    /**
     * @brief Add a reading (success or failure) to the moving average
     * @param value Value to add (ignored if failed)
     * @param isValid Whether this reading is valid
     */
    void addReading(T value, bool isValid) {
        // Remove the oldest entry from sum and valid count
        if (validBuffer[index]) {
            sum -= buffer[index];
            validCount--;
        }
        
        // Add new entry to buffer
        buffer[index] = isValid ? value : T(0);
        validBuffer[index] = isValid;
        
        // Update sum and valid count if this is a valid reading
        if (isValid) {
            sum += value;
            validCount++;
        }
        
        // Update index (circular buffer)
        index = (index + 1) % SIZE;
        
        // Track how many values we've received (valid or invalid)
        if (count < SIZE) {
            count++;
        }
    }
    
    /**
     * @brief Get the current moving average
     * @return Average of values in the buffer (only valid readings)
     */
    T getAverage() const {
        if (validCount == 0) {
            return 0;
        }
        return sum / validCount;
    }
    
    /**
     * @brief Check if the buffer is full (has SIZE samples)
     * @return true if buffer is full, false otherwise
     */
    bool isFull() const {
        return count >= SIZE;
    }
    
    /**
     * @brief Reset the moving average buffer
     */
    void reset() {
        index = 0;
        count = 0;
        sum = 0;
        validCount = 0;
        for (size_t i = 0; i < SIZE; i++) {
            buffer[i] = 0;
            validBuffer[i] = false;
        }
    }
    
    /**
     * @brief Get the number of samples currently in the buffer (valid + invalid)
     * @return Total number of samples
     */
    size_t getCount() const {
        return count;
    }
    
    /**
     * @brief Get the number of valid samples in the buffer
     * @return Number of valid samples
     */
    size_t getValidCount() const {
        return validCount;
    }
    
    /**
     * @brief Check if more than half the readings in the window are valid
     * @return true if > 50% of readings are valid, false otherwise
     */
    bool hasValidMajority() const {
        if (count == 0) return false;
        return validCount > (count / 2);
    }
    
    /**
     * @brief Get the success rate as a percentage
     * @return Success rate (0.0 to 100.0)
     */
    float getSuccessRate() const {
        if (count == 0) return 0.0f;
        return (float(validCount) / float(count)) * 100.0f;
    }
};

#endif // MOVING_AVERAGE_H
