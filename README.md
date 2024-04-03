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

TerminalCommander::TerminalCommander Terminal(&Serial, &Wire);

void exec_commands(uint8_t cmd) {
    switch (cmd) {
        case 0: {
        Serial.println("EXEC Case 0");
        }
        break;

        case 1: {
        Serial.println("EXEC Case 1");
        }
        break;

        default: {
        Serial.println("EXEC Default Case");
        }
        break;
    }
}

void setup() {
    // initialize serial console and set baud rate
    Serial.setTimeout(TERM_TIMEOUT_MILLISECONDS);
    Serial.begin(TERM_BAUD_RATE);

    // initialize Wire library and set clock rate to 400 kHz
    Wire.begin();
    Wire.setClock(I2C_CLK_RATE);

    // Option1: using a lambda expression that matches type TerminalCommander::cmd_callback_t
    Terminal.onGpio([](uint8_t cmd) {
        switch (cmd) {
        case 0: {
            Serial.println("GPIO Case 0");
        }
        break;
        case 1: {
            Serial.println("GPIO Case 1");
        }
        break;
        default: {
            Serial.println("GPIO Default Case");
        }
        break;
        }
    });

    // Option2: using a pointer to a function that matches type TerminalCommander::cmd_callback_t
    Terminal.onExec(&exec_commands);

    // remaining board configuration code goes here
}

void loop() {
    Terminal.loop();
}
```
