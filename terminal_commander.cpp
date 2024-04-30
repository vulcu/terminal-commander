/*
 * terminal_commander.h - Interactive I2C and Hardware Control Terminal for Arduino
 * 
 * Created: 3/4/2021
 * Author : Winry Litwa-Vulcu
 */

#include "terminal_commander.h"

int strcmp(const char *s1, const char *s2) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;

  while (*p1 != '\0') {
      if (*p2 == '\0') return  1;
      if (*p2 > *p1)   return -1;
      if (*p1 > *p2)   return  1;

      p1++;
      p2++;
  }

  if (*p2 != '\0') return -1;

  return 0;
}

int tolower(int c) {
  return c >= 'A' && c <= 'Z' ? c + 32 : c;
}

int strncasecmp(const char *s1, const char *s2, size_t num) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;

	if (num == 0) return 0;

	while (num-- != 0 && tolower(*p1) == tolower(*p2)) {
		if (num == 0 || *p1 == '\0' || *p2 == '\0')
			break;
		p1++;
		p2++;
	}

	return tolower(*p1) - tolower(*p2);
}

namespace TerminalCommander {
  using namespace TerminalCommanderTypes;

  // put common error messages into Program memory to save SRAM space
  static const char strErrNoError[] PROGMEM = "No Error\n";
  static const char strErrNoInput[] PROGMEM = "Error: No Input\n";
  static const char strErrUndefinedUserFunctionPtr[] PROGMEM = "Error: USER function is not defined (null pointer)\n";
  static const char strErrUnrecognizedInput[] PROGMEM = "Error: Unrecognized Input Character\n";
  static const char strErrInvalidSerialCmdLength[] PROGMEM = "\nError: Serial Command Length Exceeds Limit\n";
  static const char strErrIncomingTwoWireReadLength[] PROGMEM = "Error: Incoming TwoWire Data Exceeds Read Buffer\n";
  static const char strErrInvalidTwoWireCharacter[] PROGMEM = "Error: Invalid TwoWire Command Character\n";
  static const char strErrInvalidTwoWireCmdLength[] PROGMEM = "Error: TwoWire Command requires Address and Register\n";
  static const char strErrInvalidTwoWireWriteData[] PROGMEM = "Error: No data provided for write to I2C registers\n";
  static const char strErrInvalidHexValuePair[] PROGMEM = "Error: Commands must be in hex value pairs\n";
  static const char strErrUnrecognizedProtocol[] PROGMEM = "Error: Unrecognized Protocol\n";
  static const char strErrUnrecognizedI2CTransType[] PROGMEM = "Error: Unrecognized I2C transaction type\n";

  static const char *const string_error_table[] PROGMEM = 
  {
    strErrNoError,
    strErrNoInput, 
    strErrUndefinedUserFunctionPtr, 
    strErrUnrecognizedInput, 
    strErrInvalidSerialCmdLength, 
    strErrIncomingTwoWireReadLength,
    strErrInvalidTwoWireCharacter, 
    strErrInvalidTwoWireCmdLength, 
    strErrInvalidTwoWireWriteData, 
    strErrInvalidHexValuePair, 
    strErrUnrecognizedProtocol, 
    strErrUnrecognizedI2CTransType
  };

  /**
   * @brief Use this struct to build and config terminal command data.
   *
   * @details A more elaborate description of the constructor.
   * 
   * @param   void
   * @returns Description of the returned parameter
   */
  Command::Command() :
    pArgs(nullptr),
    iArgs(0U), 
    cmdLength(0U), 
    argsLength(0U), 
    index(0U), 
    complete(false), 
    overflow(false) {}

  /**
   * @brief Use this struct to build and config terminal command data.
   *
   * @details A more elaborate description of the constructor.
   * 
   * @param   param Description of the input parameter
   * @returns void
   */
  void Command::next(char character) {
    if (character == TERM_LINE_ENDING) {
      this->complete = true;
      return;
    } 
    
    if (index >= TERM_CHAR_BUFFER_SIZE) {
      this->overflow = true;
      return;
    }
    
    serialRx[this->index++] = character;
  }

  /**
   * @brief Use this struct to build and config terminal command data.
   *
   * @details A more elaborate description of the constructor.
   * 
   * @param   param Description of the input parameter
   * @returns void
   */
  void Command::previous(void) {
    if (this->index > 0) {
      serialRx[--this->index] = '\0';
    }
  }

  /**
   * @brief Use this struct to build and config terminal command data.
   *
   * @details A more elaborate description of the constructor.
   * 
   * @param   void
   * @returns void
   */
  void Command::flushInput(void) {
    memset(this->serialRx,  '\0', sizeof(this->serialRx));
    this->complete = false;
    this->overflow = false;
  }

  /**
   * @brief Use this struct to build and config terminal command data.
   *
   * @details A more elaborate description of the constructor.
   * 
   * @param   param Description of the input parameter
   * @returns void
   */
  void Command::flushTwoWire(void) {
    memset(this->twowire, 0, sizeof(this->twowire));
  }

  /**
   * @brief Use this struct to build and config terminal command data.
   *
   * @details A more elaborate description of the constructor.
   * 
   * @param   void
   * @returns void
   */
  void Command::initialize(void) {
    this->pArgs       = nullptr;
    this->iArgs       = 0U;
    this->cmdLength   = 0U;
    this->argsLength  = 0U;
    this->index       = 0U;
    this->flushTwoWire();
    memset(this->data, '\0', sizeof(this->data));
  }

  /**
   * @brief Use this struct to build and config terminal command data.
   *
   * @details A more elaborate description of the constructor.
   * 
   * @param   void
   * @returns void
   */
  void Command::reset(void) {
    this->flushInput();
    this->initialize();
  }

  /**
   * @brief Remove spaces from incoming serial command.
   */
  bool Command::removeSpaces(void) {
    uint8_t data_index = 0;
    for (uint8_t serialrx_index = 0; serialrx_index < TERM_CHAR_BUFFER_SIZE; serialrx_index++) {
      // remove whitespace from input character string
      if (!isSpace(this->serialRx[serialrx_index])) {
        if (this->serialRx[serialrx_index] == '\0') {
          // end of serial input data has been reached
          if (data_index == 0) {
            // input serial buffer is empty
            return false;
          }

          if (this->cmdLength == 0) {
            // serial buffer is not empty but command did not have any spaces
            this->cmdLength = data_index;
          }
          break;
        }
        else {
          this->data[data_index] = this->serialRx[serialrx_index];
        }
        data_index++;
      }
      else if ((this->pArgs == nullptr) && 
               (data_index != 0) && 
               (serialrx_index != (TERM_CHAR_BUFFER_SIZE - 1U))) {
        // Store pointer to next char character after the 1st space to enable passing user args
        this->pArgs = (char*)(&this->serialRx[serialrx_index]) + 1;

        // Store index corresponding to pointer location since it is start index of user args
        this->iArgs = serialrx_index;

        // Space character is treated as delimiter for commands even if not a user command
        this->cmdLength = data_index;
      }
    }
    this->argsLength = data_index - this->cmdLength;

    return true;
  }

  TerminalCommander::TerminalCommander(Stream* pSerial, TwoWire* pWire) {
    this->pSerial = pSerial;
    this->pWire = pWire;
  };

  void TerminalCommander::loop(void) {
    while(this->pSerial->available() > 0) {
      // check for buffer overflow
      if (this->command.overflow) {
        break;
      }

      // get the new byte
      char c = (char)this->pSerial->read();
      if ((uint8_t)c == 0x08) {
        // control character 0x08 = backspace
        this->command.previous();
      }
      // if (c == "[C") {
      //   // control character ESC[C = VT100 Right Cursor Key
      // }
      // if (c == "[D") {
      //   // control character ESC[D = VT100 Left Cursor Key
      // }
      else {
        this->command.next(c);
      }
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
        else {
          break;
        }
      }
      this->writeErrorMsgToSerialBuffer(this->lastError.set(InvalidSerialCmdLength), this->lastError.message);
      this->pSerial->println(this->lastError.message);
      this->lastError.clear();
    }
    else if (this->command.complete) {
      this->serialCommandProcessor();

      if (this->lastError.flag) {
        this->pSerial->print(this->lastError.message);
        this->lastError.clear();
      }
      this->pSerial->print(F(">> "));
    }

    // clear the input buffer array and reset serial logic
    this->command.reset();
  }

  void TerminalCommander::onCommand(const char* command, user_callback_char_fn_t callback) {
    this->userCharCallbacks[this->numUserCharCallbacks] = { command, callback };
    this->numUserCharCallbacks++;
  }

  void TerminalCommander::serialCommandProcessor(void) {
    if (!this->isRxBufferDataValid()) {
      return;
    }    
    if (!this->command.removeSpaces()) {
      this->writeErrorMsgToSerialBuffer(this->lastError.set(NoInput), this->lastError.message);
      return;
    }

    if (strncasecmp(this->command.data, "I2C", 3)) {
      // test if buffer represents hex value pairs and convert these from ASCII to hex
      if (!this->parseTwoWireData()) {
        return;
      }

      // if command was sent without spaces then set correct length for command and args
      // cmdLength will be length of command and args, e.g. I2CWffffff, and should be
      // set to 4 and argsLength to (cmd+args).length - cmd.
      if (this->command.cmdLength > 4U) {
        this->command.argsLength = this->command.cmdLength - 4U;
        this->command.cmdLength = 4U;
      }

      if (this->command.data[3] == 'r' || this->command.data[3] == 'R') {
        this->i2cRead();
        return;
      }
      else if (this->command.data[3] == 'w' || this->command.data[3] == 'W') {
        this->i2cWrite();
        return;
      }
      else {
        this->writeErrorMsgToSerialBuffer(this->lastError.set(UnrecognizedI2CTransType), this->lastError.message);
        return;
      }
    }
    else if (strncasecmp(this->command.data, "SCAN", 4)) {
      this->i2cScan();
      return;
    }

    if (!this->runUserCallbacks()) {
      // no terminal commander or user-defined command was identified
      this->writeErrorMsgToSerialBuffer(this->lastError.set(UnrecognizedProtocol), this->lastError.message);
    }
  }

  bool TerminalCommander::isRxBufferDataValid(void) {
    // check validity of input command characters before parsing commands
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
        this->writeErrorMsgToSerialBuffer(this->lastError.set(UnrecognizedInput), this->lastError.message);
        return false;
      }
    }

    if (idx == 0) {
      // input serial buffer is empty
      this->writeErrorMsgToSerialBuffer(this->lastError.set(NoInput), this->lastError.message);
      return false;
    }

    return true;
  }

  bool TerminalCommander::parseTwoWireData(void) {
    // TwoWire commands require more strict validation and parsing
    uint16_t idx = 4;
    for ( ; idx < ((uint8_t)sizeof(this->command.twowire)); idx++) {
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
        return false;
      }
    }

    if (idx < 7) {
      // command buffer is empty
      this->writeErrorMsgToSerialBuffer(this->lastError.set(InvalidTwoWireCmdLength), this->lastError.message);
      return false;
    }
    else if ((idx >> 1) != ((idx + 1) >> 1)) {
      // command buffer does not specify hex values in multiples of 8 bits
      this->writeErrorMsgToSerialBuffer(this->lastError.set(InvalidHexValuePair), this->lastError.message);
      return false;
    }

    return true;
  }

  void TerminalCommander::printTwoWireAddress(uint8_t i2c_address) {
    if (i2c_address < 0x10) {
      this->pSerial->print(F("Address: 0x0"));
    }
    else {
      this->pSerial->print(F("Address: 0x"));
    }
    this->pSerial->println(i2c_address, HEX);
  }

  void TerminalCommander::printTwoWireRegister(uint8_t i2c_register) {
    if (i2c_register < 0x10) {
      this->pSerial->print(F("Register: 0x0"));
    }
    else {
      this->pSerial->print(F("Register: 0x"));
    }
    this->pSerial->println(i2c_register, HEX);
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

  void TerminalCommander::i2cRead(void) {
    this->pSerial->println(F("I2C Read"));
    const uint8_t i2c_address =
      (uint8_t)((this->command.twowire[0] << 4) + this->command.twowire[1]);
    this->printTwoWireAddress(i2c_address);
    const uint8_t i2c_register =
      (uint8_t)((this->command.twowire[2] << 4) + this->command.twowire[3]);
    this->printTwoWireRegister(i2c_register);
    
    uint8_t twi_read_index = 0;   // start at zero so we can use the entire buffer for read
    this->command.flushTwoWire(); // flush the existing twowire buffer of all data

    this->pWire->beginTransmission(i2c_address);
    this->pWire->write(i2c_register);
    twi_error_type_t error = (twi_error_type_t)(this->pWire->endTransmission());
    if (error == NACK_ADDRESS) {
      this->pSerial->println(F("Error: I2C read attempt recieved NACK"));
      return;
    }

    delayMicroseconds(50);
    this->pWire->requestFrom(i2c_address, (uint8_t)((this->command.argsLength >> 1) - 1));
    delayMicroseconds(50);

    while(this->pWire->available()) {
      if (twi_read_index >= TERM_TWOWIRE_BUFFER_SIZE) {
        this->writeErrorMsgToSerialBuffer(this->lastError.set(IncomingTwoWireReadLength), this->lastError.message);
        return;
      }
      this->command.twowire[twi_read_index] = (uint8_t)this->pWire->read();
      twi_read_index++;
    }

    if (twi_read_index == 0) {
      this->pSerial->println(F("No Data Received"));
      return;
    }

    this->pSerial->print(F("Read Data:"));
    for(uint8_t k = 0; k < twi_read_index; k++) {
      if (this->command.twowire[k] < 0x10) {
        this->pSerial->print(F(" 0x0"));
      }
      else {
        this->pSerial->print(F(" 0x"));
      }
      this->pSerial->print(this->command.twowire[k], HEX);
    }
    this->pSerial->print('\n');
  }

  void TerminalCommander::i2cWrite(void) {
    if (this->command.argsLength < 6U) {
      this->writeErrorMsgToSerialBuffer(this->lastError.set(InvalidTwoWireWriteData), this->lastError.message);
      return;
    }

    this->pSerial->println(F("I2C Write"));
    const uint8_t i2c_address =
      (uint8_t)((this->command.twowire[0] << 4) + this->command.twowire[1]);
    this->printTwoWireAddress(i2c_address);
    const uint8_t i2c_register =
      (uint8_t)((this->command.twowire[2] << 4) + this->command.twowire[3]);
    this->printTwoWireRegister(i2c_register);

    this->pWire->beginTransmission(i2c_address);
    this->pWire->write(i2c_register);
    for (uint8_t k = 4; k < this->command.argsLength; k += 2) {
      this->pWire->write((16 * this->command.twowire[k]) + this->command.twowire[k+1]);
    }
    twi_error_type_t error = (twi_error_type_t)(this->pWire->endTransmission());
    if (error == NACK_ADDRESS) {
      this->pSerial->println(F("Error: I2C write attempt recieved NACK"));
      return;
    }
    else {
      this->pSerial->print(F("Write Data:"));
      for(uint8_t k = 4; k < this->command.argsLength; k += 2) {
        uint8_t write_data = (16 * this->command.twowire[k]) + this->command.twowire[k+1];
        if (write_data < 0x01) {
          this->pSerial->print(F(" 0x0"));
        }
        else {
          this->pSerial->print(F(" 0x"));
        }
        this->pSerial->print(write_data, HEX);
      }
      this->pSerial->print('\n');
    }
  }

  void TerminalCommander::i2cScan(void) {
    if (this->command.argsLength != 0) {
      this->writeErrorMsgToSerialBuffer(this->lastError.set(UnrecognizedProtocol), this->lastError.message);
      return;
    }

    this->pSerial->println(F("Scan I2C Bus for Available Devices"));
    this->scanTwoWireBus();
  }

  bool TerminalCommander::runUserCallbacks(void) {
    // Check for user-defined functions for GPIO, configurations, reinitialization, etc.
    if (this->command.pArgs != nullptr) {
      char user_command[this->command.cmdLength + 1] = {'\0'};
      memcpy(user_command, this->command.data, (size_t)(this->command.cmdLength));
      for (uint8_t k = 0; k < this->numUserCharCallbacks; k++) {
        if (strcmp(user_command, this->userCharCallbacks[k].command) == 0) {
          // remove leading whitespace
          while (*this->command.pArgs != '\0' && isSpace(this->command.pArgs[0])) {
            this->command.pArgs++;
            this->command.iArgs++;
          }

          // get char count for user args not including trailing whitespace/terminators
          uint8_t user_args_length = 0;
          for (uint8_t k = TERM_CHAR_BUFFER_SIZE; k > this->command.iArgs; k--) {
            if ((this->command.serialRx[k] != '\0') && !isSpace(this->command.serialRx[k])) {
              user_args_length = k - this->command.iArgs;
              break;
            }
          }

          this->userCharCallbacks[k].callback(this->command.pArgs, (size_t)(user_args_length));
          return true;
        }
      }
    }
    else {
      for (uint8_t k = 0; k < this->numUserCharCallbacks; k++) {
        if (strcmp(this->command.data, this->userCharCallbacks[k].command) == 0)  {
          this->userCharCallbacks[k].callback((char*)nullptr, (size_t)0U);
          return true;
        }
      }
    }
    return false;
  }

  /// TODO: move as much of this as possible into an error_t member
  void TerminalCommander::writeErrorMsgToSerialBuffer(error_type_t error, char *message) {
    memset(message, '\0', TERM_ERROR_MESSAGE_SIZE);
    strcpy_P(message, (char *)pgm_read_word(&(this->string_error_table[error])));
  }
}
