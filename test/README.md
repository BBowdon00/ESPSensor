# Unit Tests

This directory is for unit and integration tests.

PlatformIO supports native (desktop) and embedded testing frameworks.

## Running Tests

To run tests on your development machine:
```bash
pio test
```

To run tests on the ESP32 hardware:
```bash
pio test --environment esp32dev
```

## Writing Tests

Create test files with the naming convention `test_*.cpp`:

```cpp
#include <unity.h>

void test_sensor_initialization(void) {
    // Your test code here
    TEST_ASSERT_TRUE(true);
}

void setUp(void) {
    // Set up code (runs before each test)
}

void tearDown(void) {
    // Clean up code (runs after each test)
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_initialization);
    UNITY_END();
}

void loop() {
    // Empty
}
```

For more information, see: https://docs.platformio.org/page/plus/unit-testing.html
