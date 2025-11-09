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
    size_t index;
    size_t count;
    T sum;
    
public:
    MovingAverage() : index(0), count(0), sum(0) {
        for (size_t i = 0; i < SIZE; i++) {
            buffer[i] = 0;
        }
    }
    
    /**
     * @brief Add a new value to the moving average
     * @param value New value to add
     */
    void add(T value) {
        // Subtract the oldest value from sum
        sum -= buffer[index];
        
        // Add new value to buffer and sum
        buffer[index] = value;
        sum += value;
        
        // Update index (circular buffer)
        index = (index + 1) % SIZE;
        
        // Track how many values we've received
        if (count < SIZE) {
            count++;
        }
    }
    
    /**
     * @brief Get the current moving average
     * @return Average of values in the buffer
     */
    T getAverage() const {
        if (count == 0) {
            return 0;
        }
        return sum / count;
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
        for (size_t i = 0; i < SIZE; i++) {
            buffer[i] = 0;
        }
    }
    
    /**
     * @brief Get the number of samples currently in the buffer
     * @return Number of samples
     */
    size_t getCount() const {
        return count;
    }
};

#endif // MOVING_AVERAGE_H
