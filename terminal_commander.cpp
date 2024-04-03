/*
 * terminal_commander.h - Defnition for the Terminal Commander Class
 * 
 * Created: 3/4/2021
 * Author : Winry Litwa-Vulcu
 */

#include "terminal_commander.h"

namespace TerminalCommander {
  using namespace TerminalCommanderTypes;

  // put common error messages into Program memory to save SRAM space
  const char strErrNoError[] PROGMEM = "No Error\n";
  const char strErrUndefinedExecFunctionPtr[] PROGMEM = "Error: EXEC function is not defined (null pointer)\n";
  const char strErrUndefinedGpioFunctionPtr[] PROGMEM = "Error: GPIO function is not defined (null pointer)\n";
  const char strErrNoInput[] PROGMEM = "Error: No Input\n";
  const char strErrUnrecognizedInput[] PROGMEM = "Error: Unrecognized Input Character\n";
  const char strErrInvalidSerialCmdLength[] PROGMEM = "Error: Serial Command Length Exceeds Limit\n";
  const char strErrInvalidTwoWireCharacter[] PROGMEM = "Error: Invalid TwoWire Command Character\n";
  const char strErrInvalidTwoWireCmdLength[] PROGMEM = "Error: TwoWire Command Length Exceeds Limit\n";
  const char strErrIncomingTwoWireReadLength[] PROGMEM = "Error: Incoming TwoWire Data Exceeds Read Buffer\n";
  const char strErrInvalidTwoWireReadRegister[] PROGMEM = "Error: Invalid or Undefined TwoWire Read Register\n";
  const char strErrInvalidHexValuePair[] PROGMEM = "Error: Commands must be in hex value pairs\n";
  const char strErrUnrecognizedProtocol[] PROGMEM = "Error: Unrecognized Protocol\n";
  const char strErrInvalidDeviceAddr[] PROGMEM = "Request Error: Invalid device address\n";
  const char strErrUnrecognizedI2CRequest[] PROGMEM = "Error: Unrecognized I2C request\n";
  const char strErrUnrecognizedExecRequest[] PROGMEM = "Error: Unrecognized execution request\n";
  const char strErrUnrecognizedGPIOSelection[] PROGMEM = "Error: Unrecognized GPIO selection\n";
  const char strErrCommandDataEmpty[] PROGMEM = "Input Error: Protocol specified but command empty\n";
  const char strErrNonNumeric[] PROGMEM = "Input Error: Input value must be numeric\n";
  const char strErrNumericFormat[] PROGMEM = "Input Error: Unrecognized numeric formatting\n";
  const char strErrPositiveIntVal[] PROGMEM = "Input Error: Input value must be positive integer\n";
  const char strErrWriteProtectedLock[] PROGMEM = "Write Failed: Write-Protected Register Locked\n";

  const char *const string_error_table[] PROGMEM = 
  {
    strErrNoError,
    strErrUndefinedExecFunctionPtr, 
    strErrUndefinedGpioFunctionPtr,
    strErrNoInput, 
    strErrUnrecognizedInput, 
    strErrInvalidSerialCmdLength, 
    strErrInvalidTwoWireCharacter, 
    strErrInvalidTwoWireCmdLength, 
    strErrIncomingTwoWireReadLength,
    strErrInvalidTwoWireReadRegister, 
    strErrInvalidHexValuePair, 
    strErrUnrecognizedProtocol, 
    strErrInvalidDeviceAddr, 
    strErrUnrecognizedI2CRequest, 
    strErrUnrecognizedExecRequest, 
    strErrUnrecognizedGPIOSelection,
    strErrCommandDataEmpty, 
    strErrNonNumeric, 
    strErrNumericFormat, 
    strErrPositiveIntVal, 
    strErrWriteProtectedLock
  };

  TerminalCommander::TerminalCommander(Stream* pSerial, TwoWire* pWire) {
    this->pSerial = pSerial;
    this->pWire = pWire;

    error_t lastError;
    terminal_command_t command;
  };

  void TerminalCommander::loop(void) {
    while(pSerial->available() > 0) {
      // get the new byte
      command.next((char)pSerial->read());
    };

    if (command.overflow) {
      // discard incoming data until the serial line ending is received
      while(1) {
        if (pSerial->available() > 0) {
          char character = (char)pSerial->read();
          if (character == TERM_LINE_ENDING) { 
            break;
          }
        }
      }
      writeErrorMsgToSerialBuffer(lastError.set(InvalidSerialCmdLength), lastError.message);
      for(uint8_t k = 0; k < TERM_CHAR_BUFFER_SIZE; k++) {
        pSerial->print(lastError.message[k]);
      }
      pSerial->print('\n');
      lastError.clear();
      command.reset();
    }
    else if (command.complete) {
      serialCommandProcessor();

      if (lastError.flag) {
        for(uint8_t k = 0; k < (TERM_CHAR_BUFFER_SIZE - 1); k++) {
          pSerial->print(lastError.message[k]);
        }
        pSerial->print('\n');
        lastError.clear();
      }
      else {
        pSerial->print('\n');
      }

      // clear the input buffer array and reset serial logic
      command.reset();
    }
  }

  void TerminalCommander::onGpio(cmd_callback_t callback) {
    this->gpioCallback = callback;
  }

  void TerminalCommander::onExec(cmd_callback_t callback) {
    this->execCallback = callback;
  }

  /// TODO: break this up into smaller methods
  void TerminalCommander::serialCommandProcessor(void) {
    // check validity of input command string before parsing commands
    uint16_t idx;
    for (idx = 0; idx < sizeof(command.serialRx); idx++) {
      // check input serial buffer is only alpha-numeric characters
      if (((uint8_t)command.serialRx[idx] > 96) && ((uint8_t)command.serialRx[idx] < 122)) {
        // these are lower-case letters [a-z]
      }
      else if (((uint8_t)command.serialRx[idx] > 47) && ((uint8_t)command.serialRx[idx] < 58)) {
        // these are numbers [0-9]
      }
      else if (((uint8_t)command.serialRx[idx] > 64) && ((uint8_t)command.serialRx[idx] < 91)) {
        // these are upper-case letters [A-Z]
      }
      else if ((uint8_t)command.serialRx[idx] == 45) {
        // this is the '-' symbol which could be indicating a negative value
      }
      else if ((uint8_t)command.serialRx[idx] == 46) {
        // this is the '.' symbol which could be indicating a decimal value
      }
      else if ((uint8_t)command.serialRx[idx] == 44) {
        // this is the ',' symbol which could be indicating separated values
      }
      else if (((uint8_t)command.serialRx[idx] == 10) || 
              ((uint8_t)command.serialRx[idx] == 13) ||  
              ((uint8_t)command.serialRx[idx] == 32)) {
        // this is a space character which could be indicating separated parameters
      }
      else if (command.serialRx[idx] == '\0') {
        // end of serial input data has been reached
        break;
      }
      else {
        // an input buffer value was unrecognized
        writeErrorMsgToSerialBuffer(lastError.set(UnrecognizedInput), lastError.message);
        return;
      }
    }

    if (idx == 0) {
      // input serial buffer is empty
      writeErrorMsgToSerialBuffer(lastError.set(NoInput), lastError.message);
      return;
    }

    // remove spaces from incoming serial command then separate into 'prefix' and 'command'
    uint8_t data_index = 0;
    for (uint8_t serialrx_index = 0; serialrx_index < TERM_CHAR_BUFFER_SIZE; serialrx_index++) {
      // remove whitespace from input character string
      if (((uint8_t)command.serialRx[serialrx_index] != 10) && 
          ((uint8_t)command.serialRx[serialrx_index] != 13) &&  
          ((uint8_t)command.serialRx[serialrx_index] != 32)) {
        if (command.serialRx[serialrx_index] == '\0') {
          // end of serial input data has been reached
          break;
        }
        else if (data_index < TERM_COMMAND_PREFIX_SIZE) {
          command.prefix[data_index] = command.serialRx[serialrx_index];
        }
        else {
          command.data[data_index - TERM_COMMAND_PREFIX_SIZE] = command.serialRx[serialrx_index];
        }
        data_index++;
      }
    }
    command.length = data_index - TERM_COMMAND_PREFIX_SIZE;

    if ((command.prefix[0] == 'i' || command.prefix[0] == 'I') &&
        (command.prefix[1] == '2' || command.prefix[1] == '@') &&
        (command.prefix[2] == 'c' || command.prefix[2] == 'C')) {

      // TwoWire commands require more strict validation and parsing
      for (idx = 0; idx < ((uint8_t)sizeof(command.twowire)); idx++) {
        if (command.data[idx] == '\0') {
          // end of serial input data has been reached
          break;
        }
        else {
          command.twowire[idx] = (uint8_t)command.data[idx];
        }

        if ((command.twowire[idx] > 96) && (command.twowire[idx] < 103)) {
          // check for lower-case letters [a-f] and convert to upper-case
          command.twowire[idx] -= 32;
        }

        if ((command.twowire[idx] > 47) && (command.twowire[idx] < 58)) {
          // check for numbers [0-9] and convert ASCII value to numeric
          command.twowire[idx] -= 48;
        }
        else if ((command.twowire[idx] > 64) && (command.twowire[idx] < 71)) {
            // check for upper-case letters [A-F] and convert ASCII to numeric
            command.twowire[idx] -= 55;  //subract 65, but add 10 because A == 10
        }
        else {
          // an command character is invalid or unrecognized
          writeErrorMsgToSerialBuffer(lastError.set(InvalidTwoWireCharacter), lastError.message);
          return;
        }
      }

      if (idx == 0) {
        // command buffer is empty
        writeErrorMsgToSerialBuffer(lastError.set(NoInput), lastError.message);
        return;
      }
      else if ((idx >> 1) != ((idx + 1) >> 1)) {
        // command buffer does not specify hex values in multiples of 8 bits
        writeErrorMsgToSerialBuffer(lastError.set(InvalidHexValuePair), lastError.message);
        return;
      }

      if (command.prefix[3] == 'r' || command.prefix[3] == 'R') {
        command.protocol = I2C_READ;
        pSerial->print(F("I2C Read\n"));
        if (command.length < 4) {
          writeErrorMsgToSerialBuffer(lastError.set(InvalidTwoWireReadRegister), lastError.message);
          return;
        }
      }
      else if (command.prefix[3] == 'w' || command.prefix[3] == 'W') {
        command.protocol = I2C_WRITE;
        pSerial->print(F("I2C Write\n"));
      }
    }
    else if ((command.prefix[0] == 's' || command.prefix[0] == 'S') &&
              (command.prefix[1] == 'c' || command.prefix[1] == 'C') &&
              (command.prefix[2] == 'a' || command.prefix[2] == 'A') &&
              (command.prefix[3] == 'n' || command.prefix[3] == 'N')) {
      command.protocol = I2C_SCAN;
      pSerial->print(F("Scan I2C Bus for Available Devices\n"));
    }
    else if ((command.prefix[0] == 'e' || command.prefix[0] == 'E') &&
              (command.prefix[1] == 'x' || command.prefix[1] == 'X') &&
              (command.prefix[2] == 'e' || command.prefix[2] == 'E') &&
              (command.prefix[3] == 'c' || command.prefix[3] == 'C')) {
      command.protocol = EXECUTE;
      pSerial->print(F("Execute Configuration\n"));
    }
    else if ((command.prefix[0] == 'g' || command.prefix[0] == 'G') &&
              (command.prefix[1] == 'p' || command.prefix[1] == 'P') &&
              (command.prefix[2] == 'i' || command.prefix[2] == 'I') &&
              (command.prefix[3] == 'o' || command.prefix[3] == 'O')) {
      command.protocol = GPIO;
      pSerial->print(F("GPIO Command\n"));
    }
    else {
      writeErrorMsgToSerialBuffer(lastError.set(UnrecognizedProtocol), lastError.message);
      return;
    }

    if ((command.length == 0) && (command.protocol != I2C_SCAN)) {
      writeErrorMsgToSerialBuffer(lastError.set(CommandDataEmpty), lastError.message);
      return;
    }

    switch (command.protocol) {
      case I2C_READ: {
        const uint8_t i2c_address =
          (uint8_t)((command.twowire[0] << 4) + command.twowire[1]);
        const uint8_t i2c_register =
          (uint8_t)((command.twowire[2] << 4) + command.twowire[3]);

        pWire->beginTransmission(i2c_address);
        pWire->write(i2c_register);
        pWire->endTransmission();
        delayMicroseconds(50);
        pWire->requestFrom(i2c_address, (uint8_t)((command.length >> 1) - 2));
        delayMicroseconds(50);
        uint8_t twi_read_index = 0;
        command.flushTwoWire();
        while(pWire->available()) {
            command.twowire[twi_read_index] = (uint8_t)pWire->read();
            twi_read_index++;
            if (twi_read_index >= TERM_TWOWIRE_BUFFER_SIZE) {
              writeErrorMsgToSerialBuffer(lastError.set(IncomingTwoWireReadLength), lastError.message);
              return;
            }
        }

        pSerial->print(F("Address: 0x"));
        pSerial->println(i2c_address, HEX);
        pSerial->print(F("Register: 0x"));
        pSerial->println(i2c_register, HEX);
        pSerial->print(F("Read Data:"));
        if (twi_read_index == 0) {
          pSerial->print(F(" No Data"));
        }
        else {
          for(uint8_t k = 4; k < twi_read_index; k += 2) {
            pSerial->print(F(" 0x"));
            pSerial->print(command.twowire[k], HEX);
            pSerial->print(command.twowire[k+1], HEX);
          }
        }
        pSerial->print('\n');

      }
      break;

      case I2C_WRITE: {
        const uint8_t i2c_address =
          (uint8_t)((command.twowire[0] << 4) + command.twowire[1]);
        const uint8_t i2c_register =
          (uint8_t)((command.twowire[2] << 4) + command.twowire[3]);

        pWire->beginTransmission(i2c_address);
        pWire->write(i2c_register);
        for (uint8_t k = 4; k < command.length; k += 2) {
          pWire->write((16 * command.twowire[k]) + command.twowire[k+1]);
        }
        pWire->endTransmission();

        pSerial->print(F("Address: 0x"));
        pSerial->println(i2c_address, HEX);
        pSerial->print(F("Register: 0x"));
        pSerial->println(i2c_register, HEX);
        pSerial->print(F("Write Data:"));
        for(uint8_t k = 4; k < command.length; k += 2) {
          pSerial->print(F(" 0x"));
          pSerial->print((16 * command.twowire[k]) + command.twowire[k+1], HEX);
        }
        pSerial->print('\n');
      }
      break;

      // Scan TwoWire bus to explore and query available devices
      case I2C_SCAN: {
        scanTwoWireBus();
      }
      break;

      // Exectute commands, modify configurations, reinitialize devices, etc.
      case EXECUTE: {
        const int8_t case_select = (int8_t)(getIntFromCharArray(command.data, (size_t)command.length));

        if (lastError.flag) {
          return;
        }

        if (this->execCallback != nullptr) {
          this->execCallback(case_select);
        }
        else {
          writeErrorMsgToSerialBuffer(lastError.set(UndefinedExecFunctionPtr), lastError.message);
          return;
        }
      }
      break;

      // GPIO commands, events, and sequences go here
      case GPIO: {
        const int8_t case_select = (int8_t)(getIntFromCharArray(command.data, (size_t)command.length));

        if (lastError.flag) {
          return;
        }

        if (this->gpioCallback != nullptr) {
          this->gpioCallback(case_select);
        }
        else {
          writeErrorMsgToSerialBuffer(lastError.set(UndefinedGpioFunctionPtr), lastError.message);
          return;
        }
      }
      break;

      default: { /** TODO: this can probably be removed */ } break;
    }
  }

  void TerminalCommander::writeErrorMsgToSerialBuffer(error_type_t error, char *message) {
    memset(message, '\0', TERM_CHAR_BUFFER_SIZE);
    strcpy_P(message, (char *)pgm_read_word(&(string_error_table[error])));
  }

  /// TODO: fix handling of negative numbers because it's currently broken
  int16_t TerminalCommander::getIntFromCharArray(const char *char_array, size_t array_size) {
    bool is_negative = false;
    bool is_fractional = false;
    int16_t numeric_value = 0;

    for (uint8_t k = 0; k < array_size; k++) {
      if (char_array[k] == 46) {
        is_fractional = true;
        array_size = k;
      }
    }

    for (uint8_t k = 0; k < array_size; k++) {
      if (char_array[k] == 45) {
        if (k != 0) {
          writeErrorMsgToSerialBuffer(lastError.set(NumericFormat), lastError.message);
          return 0;
        }
        is_negative = true;
        k += 1;
      }
      else if (!((char_array[k] > 47) && (char_array[k] < 58))) {
        writeErrorMsgToSerialBuffer(lastError.set(NonNumeric), lastError.message);
        return 0;
      }
      else {
        // math library uses a lot of program space so don't use pow() here
        uint16_t order_of_mag = power_uint8(10, (array_size - (k + 1)));

        /// TODO: Since always multiplying by power of 10 might not need call to power_uint8()
        numeric_value += ((char_array[k] - 48) * order_of_mag);
      }
    }

    /// TODO: Test if this is actually used when number is negative?
    if (is_negative) {
      numeric_value *= -1;
    }

    if (is_fractional) {
      // Input data value was not an integer
      pSerial->println(F("Warning: Only integer data values are accepted"));
      pSerial->print(F("Requested value rounded towards zero, new value is "));
      pSerial->println(numeric_value);
    }

    return numeric_value;
  }

  uint16_t TerminalCommander::power_uint8(const uint8_t base, const uint8_t exponent) {
    if (exponent == 0) {
      return (uint16_t)1;
    }
    else if (exponent == 1) {
      return (uint16_t)base;
    }
    else {
      uint16_t output = base;
      for (uint8_t k = 1; k < exponent; k++) {
        output *= base;
      }
      return output;
    }
  }

  void TerminalCommander::scanTwoWireBus(void) {
    twi_error_type_t error;
    uint8_t device_count = 0;

    Serial.println(F("Scanning for I2C devices..."));

    for(uint8_t address = 1; address <= 127; address++ ) {
      // This uses the return value of Write.endTransmisstion to
      // see if a device acknowledgement occured at the address.
      Wire.beginTransmission(address);
      error = (twi_error_type_t)(Wire.endTransmission());

      if (error == NO_ERROR) {
        Serial.print(F("I2C device found at address 0x"));
        if (address < 0x10) {
          Serial.print("0");
        }
        Serial.println(address,HEX);

        device_count++;
      }
      else if (error == OTHER) {
        Serial.print(F("Unknown error at address 0x"));
        if (address < 0x10) {
          Serial.print("0");
        }
        Serial.println(address,HEX);
      }
    }

    if (device_count == 0) {
      Serial.println(F("No I2C devices found :("));
    }
    else {
      Serial.print(F("Scan complete, "));
      Serial.print(device_count);
      Serial.println(F(" devices found!"));
    }
  }
}
