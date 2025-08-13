#include <Wire.h>
#include "terminal_commander.h"


/* BEGIN: FEATURE enable/disable */
// I2C communication bus speed
#define I2C_CLK_RATE  (400000L)

// UART serial console communication baud rate
#define TERM_BAUD_RATE            (115200L)

#define FEATURE_ECHO
/* END: FEATURE enable/disable */

TerminalCommander::Terminal Terminal(&Serial, &Wire);

void setup() {
  // initialize serial console and set baud rate
  Serial.begin(TERM_BAUD_RATE);

  // initialize Wire library and set clock rate to 400 kHz
  Wire.begin();
  Wire.setClock(I2C_CLK_RATE);

  pinMode(LED_BUILTIN, OUTPUT);

  // (optional) enable VT100-style terminal echo
  // set 'false' if using a terminal with local echo
  Terminal.echo(true);

  // Option1: using a lambda expression that matches 
  // type TerminalCommander::user_callback_char_fn_t
  Terminal.onCommand("led",            &led_function);


  // Option2: using a pointer to a function that matches
  // type TerminalCommander::user_callback_char_fn_t
  Terminal.onCommand("MyCommand",      &my_function);
  Terminal.onCommand("digitalWrite",   &digitalWrite_function);
  Terminal.onCommand("help",           &help_function);
#ifdef FEATURE_ECHO
  Terminal.echo(true);
#endif
}


int keyword2value(const char *str) {
    if (strcmp(str, "HIGH") == 0) return 1;
    if (strcmp(str, "LOW") == 0) return 0;
    if (strcmp(str, "1") == 0) return 1;
    if (strcmp(str, "0") == 0) return 0;
    if (strcmp(str, "on") == 0) return 1;
    if (strcmp(str, "off") == 0) return 0;
    if (strcmp(str, "OUTPUT") == 0) return 1;
    if (strcmp(str, "INPUT") == 0) return 0;
    if (strcmp(str, "INPUT_PULLUP") == 0) return 2;
    return -1;
}



void led_function(struct cmd_param param) {
    int val = 0;
  
    val = keyword2value(param.argv[0]);

    if ((param.argc != 1)||(val < 0))
    {
      Serial.println(F("Bad parameter,Example: led off"));
      return;
    }

  digitalWrite(LED_BUILTIN, val);    
}

  
void my_function(struct cmd_param param) {
  // your code goes here
  //list all the command parameters
  Serial.println(param.argc);
  Serial.println(param.argv[0]);
  Serial.println(param.argv[1]);
  Serial.println(param.argv[2]);
  Serial.println(param.argv[3]);
  Serial.println(param.argv[4]);
}

  
void digitalWrite_function(struct cmd_param param) {
  int val = 0;
  
  val = keyword2value(param.argv[1]);

  if ((param.argc != 2) ||(val < 0))
  {
    Serial.println(F("Bad parameter,Example: digitalWrite 13  HIGH"));
    return;
  }
  
  digitalWrite(atoi(param.argv[0]), val);
}


void help_function(struct cmd_param param) {
  Serial.println("help:\n"
                "led on         - led on\n"
                "led off        - led off\n"
                "scan           - i2c scan\n"
                "i2c r          - i2c read\n"
                "i2c w          - i2c write\n"
                "digitalWrite   - digitalWrite pin, HIGH/LOW \n"
                "help           - list command\n"
                "MyCommand      - your test command\n"
                "\n"
                "Example:\n"
                "* read four registers of some device with address 0x31 and starting at register address 0x02\n"
                "  >>i2c r 31 02 00 00 00\n"
                "* test your command\n"
                "  >>MyCommand P1 P2 P3 P4 P5\n"
                "* GPIO13 to HIGH\n"
                "  >>digitalWrite 13  HIGH\n");
}

void loop() {
  Terminal.loop();
}