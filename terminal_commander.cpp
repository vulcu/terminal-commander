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
  static const char strErrNoError[] PROGMEM = "No Error\n";
  static const char strErrUndefinedUserFunctionPtr[] PROGMEM = "Error: USER function is not defined (null pointer)\n";
  static const char strErrNoInput[] PROGMEM = "Error: No Input\n";
  static const char strErrUnrecognizedInput[] PROGMEM = "Error: Unrecognized Input Character\n";
  static const char strErrInvalidSerialCmdLength[] PROGMEM = "Error: Serial Command Length Exceeds Limit\n";
  static const char strErrInvalidTwoWireCharacter[] PROGMEM = "Error: Invalid TwoWire Command Character\n";
  static const char strErrInvalidTwoWireCmdLength[] PROGMEM = "Error: TwoWire Command Length Exceeds Limit\n";
  static const char strErrIncomingTwoWireReadLength[] PROGMEM = "Error: Incoming TwoWire Data Exceeds Read Buffer\n";
  static const char strErrInvalidTwoWireReadRegister[] PROGMEM = "Error: Invalid or Undefined TwoWire Read Register\n";
  static const char strErrInvalidHexValuePair[] PROGMEM = "Error: Commands must be in hex value pairs\n";
  static const char strErrUnrecognizedProtocol[] PROGMEM = "Error: Unrecognized Protocol\n";
  static const char strErrInvalidDeviceAddr[] PROGMEM = "Request Error: Invalid device address\n";
  static const char strErrUnrecognizedI2CRequest[] PROGMEM = "Error: Unrecognized I2C request\n";
  static const char strErrUnrecognizedExecRequest[] PROGMEM = "Error: Unrecognized execution request\n";
  static const char strErrUnrecognizedGPIOSelection[] PROGMEM = "Error: Unrecognized GPIO selection\n";
  static const char strErrCommandDataEmpty[] PROGMEM = "Input Error: Protocol specified but command empty\n";
  static const char strErrNonNumeric[] PROGMEM = "Input Error: Input value must be numeric\n";
  static const char strErrNumericFormat[] PROGMEM = "Input Error: Unrecognized numeric formatting\n";
  static const char strErrPositiveIntVal[] PROGMEM = "Input Error: Input value must be positive integer\n";
  static const char strErrWriteProtectedLock[] PROGMEM = "Write Failed: Write-Protected Register Locked\n";

  static const char *const string_error_table[] PROGMEM = 
  {
    strErrNoError,
    strErrUndefinedUserFunctionPtr, 
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
  };

  void TerminalCommander::loop(void) {
    while(this->pSerial->available() > 0) {
      // get the new byte
      this->command.next((char)this->pSerial->read());
    };

    if (this->command.overflow) {
      // discard incoming data until the serial line ending is received
      while(1) {
        if (this->pSerial->available() > 0) {
          char character = (char)this->pSerial->read();
          if (character == TERM_LINE_ENDING) { 
            break;
          }
        }
      }
      writeErrorMsgToSerialBuffer(this->lastError.set(InvalidSerialCmdLength), this->lastError.message);
      for(uint8_t k = 0; k < TERM_CHAR_BUFFER_SIZE; k++) {
        this->pSerial->print(this->lastError.message[k]);
      }
      this->pSerial->print('\n');
      this->lastError.clear();
      this->command.reset();
    }
    else if (this->command.complete) {
      this->serialCommandProcessor();

      if (this->lastError.flag) {
        for(uint8_t k = 0; k < (TERM_CHAR_BUFFER_SIZE - 1); k++) {
          this->pSerial->print(this->lastError.message[k]);
        }
        this->pSerial->print('\n');
        this->lastError.clear();
      }
      else {
        this->pSerial->print('\n');
      }

      // clear the input buffer array and reset serial logic
      this->command.reset();
    }
  }

  void TerminalCommander::onUserCommand(user_callback_t callback) {
    this->userCallbackFn = callback;
  }

  /// TODO: break this up into smaller methods
  void TerminalCommander::serialCommandProcessor(void) {
    // check validity of input command string before parsing commands
    uint16_t idx;
    for (idx = 0; idx < sizeof(this->command.serialRx); idx++) {
      // check input serial buffer is only alpha-numeric characters
      if (((uint8_t)this->command.serialRx[idx] > 96) && ((uint8_t)this->command.serialRx[idx] < 122)) {
        // these are lower-case letters [a-z]
      }
      else if (((uint8_t)this->command.serialRx[idx] > 47) && ((uint8_t)this->command.serialRx[idx] < 58)) {
        // these are numbers [0-9]
      }
      else if (((uint8_t)this->command.serialRx[idx] > 64) && ((uint8_t)this->command.serialRx[idx] < 91)) {
        // these are upper-case letters [A-Z]
      }
      else if ((uint8_t)this->command.serialRx[idx] == 45) {
        // this is the '-' symbol which could be indicating a negative value
      }
      else if ((uint8_t)this->command.serialRx[idx] == 46) {
        // this is the '.' symbol which could be indicating a decimal value
      }
      else if ((uint8_t)this->command.serialRx[idx] == 44) {
        // this is the ',' symbol which could be indicating separated values
      }
      else if (((uint8_t)this->command.serialRx[idx] == 10) || 
              ((uint8_t)this->command.serialRx[idx] == 13) ||  
              ((uint8_t)this->command.serialRx[idx] == 32)) {
        // this is a space character which could be indicating separated parameters
      }
      else if (this->command.serialRx[idx] == '\0') {
        // end of serial input data has been reached
        break;
      }
      else {
        // an input buffer value was unrecognized
        writeErrorMsgToSerialBuffer(this->lastError.set(UnrecognizedInput), this->lastError.message);
        return;
      }
    }

    if (idx == 0) {
      // input serial buffer is empty
      writeErrorMsgToSerialBuffer(this->lastError.set(NoInput), this->lastError.message);
      return;
    }

    // remove spaces from incoming serial command then separate into 'prefix' and 'command'
    uint8_t data_index = 0;
    for (uint8_t serialrx_index = 0; serialrx_index < TERM_CHAR_BUFFER_SIZE; serialrx_index++) {
      // remove whitespace from input character string
      if (((uint8_t)this->command.serialRx[serialrx_index] != 10) && 
          ((uint8_t)this->command.serialRx[serialrx_index] != 13) &&  
          ((uint8_t)this->command.serialRx[serialrx_index] != 32)) {
        if (this->command.serialRx[serialrx_index] == '\0') {
          // end of serial input data has been reached
          break;
        }
        else if (data_index < TERM_COMMAND_PREFIX_SIZE) {
          this->command.prefix[data_index] = this->command.serialRx[serialrx_index];
        }
        else {
          this->command.data[data_index - TERM_COMMAND_PREFIX_SIZE] = this->command.serialRx[serialrx_index];
        }
        data_index++;
      }
    }
    this->command.length = data_index - TERM_COMMAND_PREFIX_SIZE;

    if ((this->command.prefix[0] == 'i' || this->command.prefix[0] == 'I') &&
        (this->command.prefix[1] == '2' || this->command.prefix[1] == '@') &&
        (this->command.prefix[2] == 'c' || this->command.prefix[2] == 'C')) {

      // TwoWire commands require more strict validation and parsing
      for (idx = 0; idx < ((uint8_t)sizeof(this->command.twowire)); idx++) {
        if (this->command.data[idx] == '\0') {
          // end of serial input data has been reached
          break;
        }
        else {
          this->command.twowire[idx] = (uint8_t)this->command.data[idx];
        }

        if ((this->command.twowire[idx] > 96) && (this->command.twowire[idx] < 103)) {
          // check for lower-case letters [a-f] and convert to upper-case
          this->command.twowire[idx] -= 32;
        }

        if ((this->command.twowire[idx] > 47) && (this->command.twowire[idx] < 58)) {
          // check for numbers [0-9] and convert ASCII value to numeric
          this->command.twowire[idx] -= 48;
        }
        else if ((this->command.twowire[idx] > 64) && (this->command.twowire[idx] < 71)) {
            // check for upper-case letters [A-F] and convert ASCII to numeric
            this->command.twowire[idx] -= 55;  //subract 65, but add 10 because A == 10
        }
        else {
          // an command character is invalid or unrecognized
          this->writeErrorMsgToSerialBuffer(this->lastError.set(InvalidTwoWireCharacter), this->lastError.message);
          return;
        }
      }

      if (idx == 0) {
        // command buffer is empty
        this->writeErrorMsgToSerialBuffer(this->lastError.set(NoInput), this->lastError.message);
        return;
      }
      else if ((idx >> 1) != ((idx + 1) >> 1)) {
        // command buffer does not specify hex values in multiples of 8 bits
        this->writeErrorMsgToSerialBuffer(this->lastError.set(InvalidHexValuePair), this->lastError.message);
        return;
      }

      if (this->command.prefix[3] == 'r' || this->command.prefix[3] == 'R') {
        this->command.protocol = I2C_READ;
        this->pSerial->print(F("I2C Read\n"));
        if (this->command.length < 4) {
          this->writeErrorMsgToSerialBuffer(this->lastError.set(InvalidTwoWireReadRegister), this->lastError.message);
          return;
        }
      }
      else if (this->command.prefix[3] == 'w' || this->command.prefix[3] == 'W') {
        this->command.protocol = I2C_WRITE;
        this->pSerial->print(F("I2C Write\n"));
      }
    }
    else if ((this->command.prefix[0] == 's' || this->command.prefix[0] == 'S') &&
             (this->command.prefix[1] == 'c' || this->command.prefix[1] == 'C') &&
             (this->command.prefix[2] == 'a' || this->command.prefix[2] == 'A') &&
             (this->command.prefix[3] == 'n' || this->command.prefix[3] == 'N')) {
      this->command.protocol = I2C_SCAN;
      this->pSerial->print(F("Scan I2C Bus for Available Devices\n"));
    }
    else if ((this->command.prefix[0] == 'u' || this->command.prefix[0] == 'U') &&
             (this->command.prefix[1] == 's' || this->command.prefix[1] == 'S') &&
             (this->command.prefix[2] == 'e' || this->command.prefix[2] == 'E') &&
             (this->command.prefix[3] == 'r' || this->command.prefix[3] == 'R')) {
      this->command.protocol = USER_CALLBACK;
      this->pSerial->print(F("Call User-Defined Function\n"));
    }
    else {
      writeErrorMsgToSerialBuffer(this->lastError.set(UnrecognizedProtocol), this->lastError.message);
      return;
    }

    if ((this->command.length == 0) && (this->command.protocol != I2C_SCAN)) {
      writeErrorMsgToSerialBuffer(this->lastError.set(CommandDataEmpty), this->lastError.message);
      return;
    }

    switch (command.protocol) {
      case I2C_READ: {
        /// TODO: move this to TWI parsing section
        const uint8_t i2c_address =
          (uint8_t)((this->command.twowire[0] << 4) + this->command.twowire[1]);
        const uint8_t i2c_register =
          (uint8_t)((this->command.twowire[2] << 4) + this->command.twowire[3]);
        uint8_t twi_read_index = 4;
        
        if (i2c_address < 0x10) {
          this->pSerial->print(F("Address: 0x0"));
        }
        else {
          this->pSerial->print(F("Address: 0x"));
        }
        this->pSerial->println(i2c_address, HEX);

        if (i2c_register < 0x10) {
          this->pSerial->print(F("Register: 0x0"));
        }
        else {
          this->pSerial->print(F("Register: 0x"));
        }
        this->pSerial->println(i2c_register, HEX);

        this->pWire->beginTransmission(i2c_address);
        this->pWire->write(i2c_register);
        twi_error_type_t error = (twi_error_type_t)(this->pWire->endTransmission());
        if (error == NACK_ADDRESS) {
          this->pSerial->print(F("Error: I2C read attempt recieved NACK"));
          return;
        }
        delayMicroseconds(50);
        this->pWire->requestFrom(i2c_address, (uint8_t)((this->command.length >> 1) - 1));
        delayMicroseconds(50);
        while(this->pWire->available()) {
            if (twi_read_index >= TERM_TWOWIRE_BUFFER_SIZE) {
              this->writeErrorMsgToSerialBuffer(this->lastError.set(IncomingTwoWireReadLength), this->lastError.message);
              return;
            }
            this->command.twowire[twi_read_index] = (uint8_t)this->pWire->read();
            twi_read_index++;
        }

        this->pSerial->print(F("Read Data:"));
        if (twi_read_index == 4) {
          this->pSerial->print(F(" No Data Received"));
        }
        else {
          for(uint8_t k = 4; k < twi_read_index; k++) {
            if (this->command.twowire[k] < 0x10) {
              this->pSerial->print(F(" 0x0"));
            }
            else {
              this->pSerial->print(F(" 0x"));
            }
            this->pSerial->print(this->command.twowire[k], HEX);
          }
        }
        this->pSerial->print('\n');

      }
      break;

      case I2C_WRITE: {
        /// TODO: move this to TWI parsing section
        const uint8_t i2c_address =
          (uint8_t)((this->command.twowire[0] << 4) + this->command.twowire[1]);
        const uint8_t i2c_register =
          (uint8_t)((this->command.twowire[2] << 4) + this->command.twowire[3]);

        if (i2c_address < 0x10) {
          this->pSerial->print(F("Address: 0x0"));
        }
        else {
          this->pSerial->print(F("Address: 0x"));
        }
        this->pSerial->println(i2c_address, HEX);

        if (i2c_register < 0x10) {
          this->pSerial->print(F("Register: 0x0"));
        }
        else {
          this->pSerial->print(F("Register: 0x"));
        }
        this->pSerial->println(i2c_register, HEX);

        this->pWire->beginTransmission(i2c_address);
        this->pWire->write(i2c_register);
        for (uint8_t k = 4; k < this->command.length; k += 2) {
          this->pWire->write((16 * this->command.twowire[k]) + this->command.twowire[k+1]);
        }
        twi_error_type_t error = (twi_error_type_t)(this->pWire->endTransmission());
        if (error == NACK_ADDRESS) {
          this->pSerial->print(F("Error: I2C write attempt recieved NACK"));
          return;
        }
        else {
          this->pSerial->print(F("Write Data:"));
          for(uint8_t k = 4; k < this->command.length; k += 2) {
            if (this->command.twowire[k] < 0x01) {
              this->pSerial->print(F(" 0x0"));
            }
            else {
              this->pSerial->print(F(" 0x"));
            }
            this->pSerial->print((16 * this->command.twowire[k]) + this->command.twowire[k+1], HEX);
          }
          this->pSerial->print('\n');
        }
      }
      break;

      // Scan TwoWire bus to explore and query available devices
      case I2C_SCAN: {
        this->scanTwoWireBus();
      }
      break;

      // Call user-defined functions for GPIO, configurations, reinitialization, etc.
      case USER_CALLBACK: {
        const int8_t case_select = (int8_t)(this->getIntFromCharArray(this->command.data, (size_t)this->command.length));

        if (this->lastError.flag) {
          return;
        }

        if (this->userCallbackFn != nullptr) {
          /// TODO: modify this to pass a pointer to command.data and the size_t of command.data
          this->userCallbackFn(case_select);
        }
        else {
          this->writeErrorMsgToSerialBuffer(this->lastError.set(UndefinedUserFunctionPtr), this->lastError.message);
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
          this->writeErrorMsgToSerialBuffer(this->lastError.set(NumericFormat), this->lastError.message);
          return 0;
        }
        is_negative = true;
        k += 1;
      }
      else if (!((char_array[k] > 47) && (char_array[k] < 58))) {
        this->writeErrorMsgToSerialBuffer(this->lastError.set(NonNumeric), this->lastError.message);
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
      this->pSerial->println(F("Warning: Only integer data values are accepted"));
      this->pSerial->print(F("Requested value rounded towards zero, new value is "));
      this->pSerial->println(numeric_value);
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

    this->pSerial->println(F("Scanning for I2C devices..."));

    for(uint8_t address = 1; address <= 127; address++ ) {
      // This uses the return value of Write.endTransmisstion to
      // see if a device acknowledgement occured at the address.
      this->pWire->beginTransmission(address);
      error = (twi_error_type_t)(this->pWire->endTransmission());

      if (error == NO_ERROR) {
        this->pSerial->print(F("I2C device found at address 0x"));
        if (address < 0x10) {
          this->pSerial->print("0");
        }
        this->pSerial->println(address,HEX);

        device_count++;
      }
      else if (error == OTHER) {
        this->pSerial->print(F("Unknown error at address 0x"));
        if (address < 0x10) {
          this->pSerial->print("0");
        }
        this->pSerial->println(address,HEX);
      }
    }

    if (device_count == 0) {
      pSerial->println(F("No I2C devices found :("));
    }
    else {
      pSerial->print(F("Scan complete, "));
      pSerial->print(device_count);
      pSerial->println(F(" devices found!"));
    }
  }
}
