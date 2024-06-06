# terminal-commander

Terminal Commander is an interactive serial terminal for Arduino, providing serial buffer parsing and command-line access to the I2C interface. The class is intended to streamline the creation of a simple command-line terminal on any Arduino device.

## Table of Contents

- [Installation](#installation)
- [Basic Usage](#basic-usage)
  - [Creating a Terminal Object](#creating-a-terminal-object)
  - [Setup](#setup)
  - [Loop](#loop)
- [Using Built-In Commands](#using-built-in-commands)
  - [Overloading Built-In Commands](#overloading-built-in-commands)
- [Additional Functionality](#additional-functionality)
  - [Enabling VT-100 Style Terminal Echo](#enabling-vt-100-style-terminal-echo)
- [Creating User-Defined Terminal Commands](#creating-user-defined-terminal-commands)
  - [Creating a Function Callback for a Custom Command](#creating-a-function-callback-for-a-custom-command)
  - [Parsing Terminal Arguments in a User Command](#parsing-terminal-arguments-in-a-user-command)
  - [Using a Lambda Expression Instead of a Function](#using-a-lambda-expression-instead-of-a-function)

## Installation

Install from the Arduino Library Manager, where it is listed as 'TerminalCommander'. Alternatively, Terminal Commander can also be installed as a Git submodule:

```shell
git submodule add https://github.com/vulcu/terminal-commander.git 
# or
git submodule add git@github.com:vulcu/terminal-commander.git

# followed by
git submodule --init
```

## Basic Usage

### Creating a Terminal Object

Constructing an instance of Terminal Commander requires a pointer to an instance of the Stream class, and a pointer to an instance of the TwoWire class. Optionally, a single-character command delimiter may be defined for deliniating custom user commands and their arguments. The default command delimiter is a space.

```cpp
// basic instantiation using the default command delimiter
TerminalCommander::Terminal Terminal(&Serial, &Wire);

// alternative instantiation using the custom command delimiter ':'
// and the 'Serial1' hardware serial port for communication
TerminalCommander::Terminal Terminal(&Serial1, &Wire, ':');
```

Terminal Commander will always use the newline character `\n` (also known as `LF`) as the input buffer line ending. If necessary this can be changed by changing the `TERM_LINE_ENDING` definition in the header file, but `LF` is suggested.

### Setup

The above instantiation should go at the top of your Arduino sketch, prior to the 'setup' section of the sketch (see the Terminal-LED-Control example). By default, Terminal Commander does not require any additional code in the 'setup' section of the sketch. However, the Stream and Wire classes each have their own  `begin()` methods and should be used as usual:

```cpp
void setup() {
  // Initialize serial console and set baud rate.
  // See Arduino 'Serial' documentation for details.
  Serial.begin(9600);

  // Initialize Wire library and set clock rate.
  // See Arduino 'Wire' documentation for details.
  Wire.begin();
  Wire.setClock(400000);

  // the rest of your setup code, etc. goes here
}
```

### Loop

To use Terminal Commander interactively, simply add the following to the 'loop' section of your sketch:

```cpp
void loop() {
  Terminal.loop();

  // the rest of your loop code, etc. goes here
}
```

Please note that although Terminal Commander is very lightweight and fast, `Terminal.loop()` is only called once per iteration of the 'loop' section. So, if your 'loop' takes a long while to execute each iteration (e.g. if you are calling a non-asynchronous WiFi or MQTT library), Terminal Commander's responses and terminal echo function may appear sluggish.

## Using Built-In Commands

By default, Terminal Commander has three built-in commands:

- **SCAN**: Scan the I2C bus and return the I2C address of any device that acknowledges.
- **I2C**:  Write (`i2c w`) or read (`i2c r`) the I2C bus directly, using the I2C address, register, and (in the case of a write) value.
  - The I2C command supports sequential reads or writes, if supported by the I2C device.
  - For example, to read four registers of some device with address **0x31** and starting at register address **0x02**, enter `i2c r 31 02 00 00 00` in the terminal.
  - I2C commands must be submitted as two-digit hexadecimal byte values, e.g. `i2c r 31 01`  and not `i2c r 31 1`.
  - Characters other than '**0-9**' and '**A-F**' will not be accepted for I2C reads/writes and will return an error.
  - Spaces character delimiters are not necessary when using this command, so `i2c r 31 01` and `i2cr3101` are parsed the same.
- **HELP**: (Implementation pending, see #5) Return this list of built-in commands and a usage summary for each. Also lists all user-defined commands, although it will not list any arguments to user-defined commands as these are outside the scope of the class.
- All built-in commands are completely case insensitive, e.g. `scan`, `Scan`, and `SCAN` are all treated the same.
  - NB: Only built-in commands are case-insensitive. User-defined commands _are_ case-sensitive (See [Creating User-Defined Terminal Commands](#creating-user-defined-terminal-commands) for more details).

### Overloading Built-In Commands

All built-in commands can be overloaded by the user. Custom user commands are parsed and evaluated prior to evaluating any built-in commands. If you want to write your own `scan`, `i2c`, or `help` commands then simply create a custom command with that name:

```cpp
Terminal.onCommand("scan", &my_custom_scan_fn)
```

See [Creating User-Defined Terminal Commands](#creating-user-defined-terminal-commands) for more details.

## Additional Functionality

### Enabling VT-100 Style Terminal Echo

Terminal Commander supports VT-100 style terminal echo, for use with terminal programs that do not support local echo (e.g. the Arduino IDE Serial Monitor) or for which local echo is optional (e.g. TeraTerm, PuTTY). This feature is not compatible with some terminal programs which send the entire line in one transmission (e.g. VSCode).

To enable terminal echo, add the following lines to the 'setup' section of your Arduino sketch:

```cpp
// Add this inside the setup() block of your sketch
Terminal.echo(true);
```

Enabling this feature will echo incoming terminal ASCII back to the source terminal. Terminal Commander correctly handles the 'backspace' input and will delete the previous terminal character. However, VT100-style control characters (`^[C`, `^[D`, etc.) are not supported, so Left/Right arrow keys will generate unrecognized inputs.

## Creating User-Defined Terminal Commands

User-defined terminal commands can be easily created by calling the `onCommand` method in the 'setup' block of your sketch. All arguments following the user command (as defined [by the delimiter](#creating-a-terminal-object)) are passed directly to the user function with all whitespace, etc. intact.

By default, up to 10 user-defined functions can be created. This value can be modified by changing the `MAX_USER_COMMANDS` definition in the header file. Increasing the value will allow more commands at the expense of more SRAM usage, and conversely decreasing this value will decrease SRAM usage.

### Creating a Function Callback for a Custom Command

To create a command called '**led**' and use it to turn on/off the built-in LED, first define the command and attach it to a callback function:

```cpp
// Add this inside the setup() block of your sketch
Terminal.onCommand("led", &my_led_function);
```

Then define `my_led_function` outside of the 'setup' block, as you would define any other funtion:

```cpp
void my_led_function(char* args, size_t size) {
  // your code goes here
}
```

Please note that functions definitions must match the type `TerminalCommander::user_callback_char_fn_`:

```cpp
typedef void (user_callback_char_fn_t)(char*, size_t);
```

### Parsing Terminal Arguments in a User Command

Terminal Commander will pass all terminal input following the first delimiter to your function as a pointer to a char array, and will pass the length of that char array as an integer of type `size_t`. These arguments can be checked and processed further inside of any user-defined function.

Utilizing this functionality in your code is optional; if used it is highly recommended to include a check for 'no arguments'. If no arguments are passed to the user fuction (e.g. no terminal input followed the delimiter) then Terminal Commander will pass a `nullptr` to the callback function and a size of zero. This can be checked for easily in your code:

```cpp
if (args == nullptr || size == 0) {
  // there was no terminal input following the delimiter
}
```

It is also recommended to copy the character array to a local variable within the function before doing anything with it:

```cpp
char cmd[size + 1] = {'\0'};
memcpy(cmd, args, size);
```

Putting this all together, the `my_led_function` from the previous section could contain the following:

```cpp
void my_led_function(char* args, size_t size) {
  if (args == nullptr || size == 0) {
    Serial.println(F("Error: No LED state provided"));
    return;
  }
    
  // type 'led on' or 'led off' in terminal turn built-in LED on/off
  char cmd[size + 1] = {'\0'};
  memcpy(cmd, args, size);
  if (strcmp(cmd, "on") == 0) {
    // turn on built-in LED
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else if (strcmp(cmd, "off") == 0) {
    // turn off built-in LED
    digitalWrite(LED_BUILTIN, LOW);
  }
  else {
    Serial.println(F("Error: Unrecognized LED state"));
  }
}
```

### Using a Lambda Expression Instead of a Function

In place of a separately defined function, a lambda expression can also be used to define a user command. The lambda expression must also match the type `TerminalCommander::user_callback_char_fn_`:

```cpp
typedef void (user_callback_char_fn_t)(char*, size_t);
```

For example, using a lambda expression to define the `led` command detailed above would look like this:

```cpp
// Add this inside the setup() block of your sketch
Terminal.onCommand("led", [](char* args, size_t size) {
  // your code goes here
});
```

If defining your custom commands using a lamda expression, no additional function definition is necessary. For this application, there are no behavioral or performance differences between these implementations. Both options are available to suit code structure and organizational preferences.
