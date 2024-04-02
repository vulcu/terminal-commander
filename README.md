# terminal-commander
Command-based terminal interface for Arduino

# Usage
```cpp
#include <Wire.h>
#include "terminal_commander.h"

// I2C communication bus speed
#define I2C_CLK_RATE  (400000L)

// UART serial console communication baud rate and timeout
#define TERM_BAUD_RATE            (115200L)
#define TERM_TIMEOUT_MILLISECONDS (1000U)

TerminalCommander Terminal(&Serial, &Wire);

void setup() {
    // initialize serial console and set baud rate
    Serial.setTimeout(TERM_TIMEOUT_MILLISECONDS);
    Serial.begin(TERM_BAUD_RATE);

    // initialize Wire library and set clock rate to 400 kHz
    Wire.begin();
    Wire.setClock(I2C_CLK_RATE);

    // remaining board configuration code goes here
}

void loop() {
    Terminal.loop();
}
```
