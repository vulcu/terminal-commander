#include <Wire.h>
#include "terminal_commander.h"

// I2C communication bus speed
#define I2C_CLK_RATE  (400000L)

// UART serial console communication baud rate
#define TERM_BAUD_RATE            (115200L)

TerminalCommander::Terminal Terminal(&Serial, &Wire);

void setup() {
  // initialize serial console and set baud rate
  Serial.begin(TERM_BAUD_RATE);

  // initialize Wire library and set clock rate to 400 kHz
  Wire.begin();
  Wire.setClock(I2C_CLK_RATE);

  pinMode(LED_BUILTIN, OUTPUT);

  // (optional) enable VT100-style terminal echo
  Terminal.echo(false);

  // Option1: using a lambda expression that matches 
  // type TerminalCommander::user_callback_char_fn_t
  Terminal.onCommand("led", [](char* args, size_t size) {
    if (args == nullptr || size == 0) {
      Serial.println(F("Error: No LED state provided"));
      return;
    }
    
    // type 'led on' or 'led off' in terminal turn built-in LED on/off
    char cmd[size + 1] = {'\0'};
    memcpy(cmd, args, size);
    if (strcmp(cmd, "on") == 0) {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else if (strcmp(cmd, "off") == 0) {
      digitalWrite(LED_BUILTIN, LOW);
    }
    else {
      Serial.println(F("Error: Unrecognized LED state"));
    }
  });

  // Option2: using a pointer to a function that matches
  // type TerminalCommander::user_callback_char_fn_t
  Terminal.onCommand("MyCommand", &my_function);
}

void my_function(char* args, size_t size) {
  // your code goes here
}

void loop() {
  Terminal.loop();
}