/*
 * terminal_commander.h - Interactive Serial Terminal for Arduino
 * Copyright (C) 2024 Winry Litwa-Vulcu
 * Licensed under the GNU General Public License, Version 3
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

namespace TerminalCommander {
  using namespace TerminalCommanderTypes;

  // put common error messages into Program memory to save SRAM space
  static const char strErrNoError[] PROGMEM = "No Error\n";
  static const char strErrNoInput[] PROGMEM = "Error: No Input\n";
  static const char strErrUndefinedUserFunctionPtr[] PROGMEM = "Error: USER function is not defined (null pointer)\n";
  static const char strErrUnrecognizedInput[] PROGMEM = "Error: Unrecognized Input Character\n";
  static const char strErrInvalidSerialCmdLength[] PROGMEM = "\nError: Serial Command Length Exceeds Limit\n";
  static const char strErrTwoWireNack[] PROGMEM = "Error: I2C transaction attempt recieved NACK\n";
  static const char strErrTwoWireReadLength[] PROGMEM = "Error: Incoming TwoWire Data Exceeds Read Buffer\n";
  static const char strErrInvalidTwoWireCharacter[] PROGMEM = "Error: Invalid TwoWire Command Character\n";
  static const char strErrInvalidTwoWireCmdLength[] PROGMEM = "Error: TwoWire Command requires Address and Register\n";
  static const char strErrInvalidTwoWireWriteData[] PROGMEM = "Error: No data provided for write to I2C registers\n";
  static const char strErrInvalidHexValuePair[] PROGMEM = "Error: Commands must be in hex value pairs\n";
  static const char strErrUnrecognizedCommand[] PROGMEM = "Error: Unrecognized Command\n";
  static const char strErrUnrecognizedI2CTransType[] PROGMEM = "Error: Unrecognized I2C transaction type\n";

  const char *const Error::string_error_table[] PROGMEM = 
  {
    strErrNoError,
    strErrNoInput, 
    strErrUndefinedUserFunctionPtr, 
    strErrUnrecognizedInput, 
    strErrInvalidSerialCmdLength,
    strErrTwoWireNack, 
    strErrTwoWireReadLength,
    strErrInvalidTwoWireCharacter, 
    strErrInvalidTwoWireCmdLength, 
    strErrInvalidTwoWireWriteData, 
    strErrInvalidHexValuePair, 
    strErrUnrecognizedCommand, 
    strErrUnrecognizedI2CTransType
  };

  Error::Error(void):
    flag(false), 
    warning(false), 
    type(NoError) {};

  void Error::set(TerminalCommanderTypes::error_type_t error_type) {
    this->flag = true;
    this->type = error_type;
    memset(message, '\0', TERM_ERROR_MESSAGE_SIZE);
    strcpy_P(message, (char *)pgm_read_ptr(&(this->string_error_table[error_type])));
  }

  void Error::warn(TerminalCommanderTypes::error_type_t error_type) {
    this->warning = true;
    this->set(error_type);
  }

  void Error::clear(void) {
    this->flag    = false;
    this->warning = false;
    this->type    = NoError;
  }

  void Error::reset(void) {
    this->clear();
    memset(this->message,  '\0', sizeof(this->message));
  }

  Command::Command(void): 
    pArgs(nullptr),
    iArgs(0U), 
    cmdLength(0U), 
    argsLength(0U), 
    index(0U), 
    complete(false), 
    overflow(false) {}

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

  void Command::previous(void) {
    if (this->index > 0) {
      serialRx[--this->index] = '\0';
    }
  }

  void Command::flushInput(void) {
    memset(this->serialRx,  '\0', sizeof(this->serialRx));
    this->complete = false;
    this->overflow = false;
  }

  void Command::flushTwoWire(void) {
    memset(this->twowire, 0, sizeof(this->twowire));
  }

  void Command::initialize(void) {
    this->pArgs       = nullptr;
    this->iArgs       = 0U;
    this->cmdLength   = 0U;
    this->argsLength  = 0U;
    this->index       = 0U;
    this->flushTwoWire();
    memset(this->data, '\0', sizeof(this->data));
  }

  void Command::reset(void) {
    this->flushInput();
    this->initialize();
  }

  Terminal::Terminal(Stream* pSerial, 
    TwoWire* pWire) :
    termCommandDelimiter(TERM_DEFAULT_CMD_DELIMITER) {
    this->pSerial = pSerial;
    this->pWire = pWire;
  };

  Terminal::Terminal(Stream* pSerial, 
    TwoWire* pWire,
    const char command_delimiter = TERM_DEFAULT_CMD_DELIMITER) :
    termCommandDelimiter(command_delimiter) {
    this->pSerial = pSerial;
    this->pWire = pWire;
  };

  void Terminal::loop(void) {
    while(this->pSerial->available() > 0) {
      // check for buffer overflow
      if (this->command.overflow) {
        break;
      }      

      // get the new byte
      char c = (char)this->pSerial->read();

      // Could add handling here for ASCII '27' escape sequence indicator e.g.
      // \033[A = VT100 Up Cursor Key      \033[B = VT100 Down Cursor Key
      // \033[C = VT100 Right Cursor Key   \033[D = VT100 Left Cursor Key
      if ((uint8_t)c == 8U) {
        // ASCII character '8' is backspace
        if (this->isEchoEnabled && (this->command.index > 0)) {
          // VT100 destructive backspace (delete from terminal output) is "\b \b"
          this->pSerial->print(F("\b \b"));
        }
        this->command.previous();
      }
      else {
        if (this->isEchoEnabled) {
          this->pSerial->print(c);
        }
        this->command.next(c);
      }
    };

    if (this->command.overflow) {
      // wait for next chararcter in case command is still being transmitted
      delayMicroseconds(TERM_MICROSEC_PER_CHAR);

      // discard incoming data until the serial line ending is received
      while (this->pSerial->available() > 0) {
        if ((char)this->pSerial->read() == TERM_LINE_ENDING) {
          break;
        }
        // wait to see if another new character will arrive
        delayMicroseconds(TERM_MICROSEC_PER_CHAR);
      }
      this->lastError.set(InvalidSerialCmdLength);
      this->pSerial->print(this->lastError.message);
      this->lastError.clear();
      this->command.reset();
      this->isNewTerminalCommandPrompt = true;
    }
    else if (this->command.complete) {
      this->serialCommandProcessor();

      if (this->lastError.flag) {
        this->pSerial->print(this->lastError.message);
        this->lastError.clear();
      }

      // clear the input buffer array and reset serial logic
      this->command.reset();
      this->isNewTerminalCommandPrompt = true;
    }

    if (this->isNewTerminalCommandPrompt) {
      this->isNewTerminalCommandPrompt = false;
      this->pSerial->print(F(">> "));
    }
  }

  void Terminal::init(void) {
    this->lastError.clear();
    this->command.reset();
    this->pSerial->print(F("\n"));
  }

  void Terminal::echo(bool enable_terminal_echo) {
    this->isEchoEnabled = enable_terminal_echo;
  }

  void Terminal::onCommand(const char* command, user_callback_char_fn_t callback) {
    if (this->numUserCharCallbacks >= MAX_USER_COMMANDS) {
      TerminalCommander::Print::println(this->pSerial,
                                        F("Error: User Command count exceeded, maximum is "), 
                                        MAX_USER_COMMANDS);
      TerminalCommander::Print::println(this->pSerial, 
                                        F("! \""), 
                                        command, 
                                        F("\" was not added to user command list"));
      return;
    }

    this->userCharCallbacks[this->numUserCharCallbacks] = { command, callback };
    this->numUserCharCallbacks++;
  }

  bool Terminal::serialCommandProcessor(void) {
    // check validity of incoming buffer data
    if (!this->isRxBufferDataValid()) {
      return false;
    }

    // create a copy of the buffer without whitespace for easier parsing
    if (!this->removeSpaces()) {
      return false;
    }

    // Check for user-defined functions for GPIO, configurations, reinitialization, etc.
    if (this->runUserCallbacks()) {
      return true;
    }

    if ((this->command.data[0] == 'i' || this->command.data[0] == 'I') &&
        (this->command.data[1] == '2') &&
        (this->command.data[2] == 'c' || this->command.data[2] == 'C')) {

      if (this->command.data[3] == 'r' || this->command.data[3] == 'R') {
        return this->readTwoWire();
      }
      else if (this->command.data[3] == 'w' || this->command.data[3] == 'W') {
        return this->writeTwoWire();
      }

      this->lastError.set(UnrecognizedI2CTransType);
      return false;
    }
    else if ((this->command.data[0] == 's' || this->command.data[0] == 'S') &&
             (this->command.data[1] == 'c' || this->command.data[1] == 'C') &&
             (this->command.data[2] == 'a' || this->command.data[2] == 'A') &&
             (this->command.data[3] == 'n' || this->command.data[3] == 'N')) {
      return this->scanTwoWireBus();
    }

    // no terminal commander or user-defined command was identified
    this->lastError.set(UnrecognizedCommand);
    return false;
  }

  bool Terminal::isRxBufferDataValid(void) {
    // check validity of input command characters before parsing commands
    uint16_t idx;
    for (idx = 0; idx < sizeof(this->command.serialRx); idx++) {
      // check input serial buffer only contains permitted ASCII characters
      if (((uint8_t)this->command.serialRx[idx] > 96U) && ((uint8_t)this->command.serialRx[idx] < 122U)) {
        // these are lower-case letters [a-z]
      }
      else if (((uint8_t)this->command.serialRx[idx] > 47U) && ((uint8_t)this->command.serialRx[idx] < 58U)) {
        // these are numbers [0-9]
      }
      else if (((uint8_t)this->command.serialRx[idx] > 64U) && ((uint8_t)this->command.serialRx[idx] < 91U)) {
        // these are upper-case letters [A-Z]
      }
      else if (isSpace(this->command.serialRx[idx])) {
        // this is a whitespace character which could be a delimiter
        // isSpace() returns true for a space, form feed ('\f'), newline ('\n'), 
        // carriage return ('\r'), horizontal tab ('\t'), or vertical tab ('\v')
      }
      else if ((uint8_t)this->command.serialRx[idx] == 44U) {
        // this is the ',' symbol which could be a delimiter
      }
      else if ((uint8_t)this->command.serialRx[idx] == 45U) {
        // this is the '-' symbol which could be indicating a negative value
      }
      else if ((uint8_t)this->command.serialRx[idx] == 46U) {
        // this is the '.' symbol which could be indicating a decimal value
      }
      else if ((uint8_t)this->command.serialRx[idx] == 59U) {
        // this is the ';' symbol which could be a delimiter
      }
      else if (this->command.serialRx[idx] == this->termCommandDelimiter) {
        // by default this is a space but can be a user-defined delimiter
      }
      else if (this->command.serialRx[idx] == '\0') {
        // end of serial input data has been reached
        break;
      }
      else {
        // an input buffer value was unrecognized
        this->lastError.set(UnrecognizedInput);
        return false;
      }
    }

    if (idx == 0) {
      // input serial buffer is empty
      this->lastError.set(NoInput);
      return false;
    }

    return true;
  }

  bool Terminal::removeSpaces(void) {
    uint8_t data_index = 0;

    // create a copy of the serialRx buffer with whitespace removed for easier parsing
    for (uint8_t serialrx_index = 0; serialrx_index < TERM_CHAR_BUFFER_SIZE; serialrx_index++) {
      if (this->command.serialRx[serialrx_index] != this->termCommandDelimiter) {
        if (this->command.serialRx[serialrx_index] == '\0') {
          // end of serial input data has been reached
          if (data_index == 0U) {
            // input serial buffer is empty
            this->lastError.set(NoInput);
            return false;
          }

          if (this->command.cmdLength == 0U) {
            // serial buffer is not empty but command did not have any delimiter
            this->command.cmdLength = data_index;
          }
          break;
        }
        
        if (!isSpace(this->command.serialRx[serialrx_index])){
          this->command.data[data_index] = this->command.serialRx[serialrx_index];
          data_index++;
        }
      }
      else {
        if ((this->command.pArgs == nullptr) && 
               (data_index != 0U) && 
               (serialrx_index != (TERM_CHAR_BUFFER_SIZE - 1U))) {
        // Store pointer to next char character after the 1st space to enable passing user args
        this->command.pArgs = (char*)(&this->command.serialRx[serialrx_index]) + 1;

        // Store index corresponding to pointer location since it is start index of user args
        this->command.iArgs = serialrx_index;

        // First delimiter instance is always treated as the delimiter for a user command
        this->command.cmdLength = data_index;
        }
        else if (!isSpace(this->command.serialRx[serialrx_index])) {
          this->command.data[data_index] = this->command.serialRx[serialrx_index];
          data_index++;
        }
      }
    }
    this->command.argsLength = data_index - this->command.cmdLength;
    return true;
  }

  bool Terminal::runUserCallbacks(void) {
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
          this->parseCmd(this->command.pArgs,&(this->command.cmd_param_value));
          this->userCharCallbacks[k].callback((struct cmd_param)this->command.cmd_param_value);
          return true;
        }
      }
    }
    else {
      for (uint8_t k = 0; k < this->numUserCharCallbacks; k++) {
        if (strcmp(this->command.data, this->userCharCallbacks[k].command) == 0)  {
          this->command.cmd_param_value = {0};
          this->userCharCallbacks[k].callback((struct cmd_param)this->command.cmd_param_value);
          return true;
        }
      }
    }
    return false;
  }

  bool Terminal::parseTwoWireCommand(void) {
    // set correct length for command and args if command sent without spaces or badly formatted
    if (this->command.cmdLength != 4U) {
      this->command.argsLength = this->command.argsLength + this->command.cmdLength - 4U;
      this->command.cmdLength = 4U;
    }
  
    // convert buffer from from ASCII to hex and check if it contains hex value pairs
    uint8_t idx = 0;
    for ( ; idx < ((uint8_t)sizeof(this->command.twowire)); idx++) {
      if (this->command.data[idx + this->command.cmdLength] == '\0') {
        // end of serial input data has been reached
        break;
      }
      else {
        this->command.twowire[idx] = (uint8_t)this->command.data[idx + this->command.cmdLength];
      }

      if ((this->command.twowire[idx] > 96U) && (this->command.twowire[idx] < 103U)) {
        // check for lower-case letters [a-f] and convert to upper-case
        this->command.twowire[idx] -= 32U;
      }

      if ((this->command.twowire[idx] > 47U) && (this->command.twowire[idx] < 58U)) {
        // check for numbers [0-9] and convert ASCII value to numeric
        this->command.twowire[idx] -= 48U;
      }
      else if ((this->command.twowire[idx] > 64U) && (this->command.twowire[idx] < 71U)) {
          // check for upper-case letters [A-F] and convert ASCII to numeric
          this->command.twowire[idx] -= 55U;  //subract 65, but add 10 because A == 10
      }
      else {
        // an command character is invalid or unrecognized
        this->lastError.set(InvalidTwoWireCharacter);
        return false;
      }
    }

    if (idx < 3) {
      // command length does not contain an address and a register
      this->lastError.set(InvalidTwoWireCmdLength);
      return false;
    }
    else if ((idx >> 1) != ((idx + 1) >> 1)) {
      // command buffer does not specify hex values in multiples of 8 bits
      this->lastError.set(InvalidHexValuePair);
      return false;
    }

    return true;
  }

  bool Terminal::readTwoWire(void) {
    // TwoWire commands require more strict validation and parsing
    if (!this->parseTwoWireCommand()) {
      return false;
    }

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
      this->lastError.set(TwoWireNack);
      return false;
    }

    delayMicroseconds(50U);
    this->pWire->requestFrom(i2c_address, (uint8_t)((this->command.argsLength >> 1) - 1));
    delayMicroseconds(50U);
    while(this->pWire->available()) {
      if (twi_read_index >= TERM_TWOWIRE_BUFFER_SIZE) {
        this->lastError.set(TwoWireReadLength);
        return false;
      }
      this->command.twowire[twi_read_index] = (uint8_t)this->pWire->read();
      twi_read_index++;
    }

    this->pSerial->print(F("Read Data:"));
    if (twi_read_index == 0) {
      this->pSerial->print(F(" No Data Received"));
    }
    else {
      for(uint8_t k = 0; k < twi_read_index; k++) {
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
    return true;
  }

  bool Terminal::writeTwoWire(void) {
    // TwoWire commands require more strict validation and parsing
    if (!this->parseTwoWireCommand()) {
      return false;
    }

    if (command.argsLength < 6U) {
      this->lastError.set(InvalidTwoWireWriteData);
      return false;
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
      this->lastError.set(TwoWireNack);
      return false;
    }

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
    return true;
  }

  bool Terminal::scanTwoWireBus(void) {
    // This command does not accept additional arguments
    if ((this->command.argsLength + this->command.cmdLength) > 4U) {
      /** TODO: this could be a warning instead of an error */
      this->lastError.set(UnrecognizedCommand);
      return false;
    }

    this->pSerial->println(F("Scanning for available I2C devices..."));

    twi_error_type_t error;
    uint8_t device_count = 0;

    for(uint8_t address = 1; address <= 127; address++ ) {
      // This uses the return value of Write.endTransmisstion to
      // see if a device acknowledgement occured at the address.
      this->pWire->beginTransmission(address);
      error = (twi_error_type_t)(this->pWire->endTransmission());

      if (error == NO_ERROR) {
        this->pSerial->print(F("I2C device found at "));
        this->printTwoWireAddress(address);
        device_count++;
      }
      else if (error == OTHER) {
        this->pSerial->print(F("Unknown error at "));
        this->printTwoWireAddress(address);
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
    return true;
  }

  void Terminal::printTwoWireAddress(uint8_t i2c_address) {
    if (i2c_address < 0x10) {
      this->pSerial->print(F("Address: 0x0"));
    }
    else {
      this->pSerial->print(F("Address: 0x"));
    }
    this->pSerial->println(i2c_address, HEX);
  }

  void Terminal::printTwoWireRegister(uint8_t i2c_register) {
    if (i2c_register < 0x10) {
      this->pSerial->print(F("Register: 0x0"));
    }
    else {
      this->pSerial->print(F("Register: 0x"));
    }
    this->pSerial->println(i2c_register, HEX);
  }
  
  
int Terminal::parseCmd(const char* input, struct cmd_param* result) {
  int i = 0;
  char buffer[MAX_CMD_ARGC_NUM * MAX_CMD_ARGV_STR_LENGTH] = {0};
  
  if((result == NULL)||(input == NULL))
  {
    return -1;
  }
  result->argc = 0;
  memset(result->argv, 0, sizeof(result->argv));
  
  strncpy(buffer, input, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';
  
  // remove '\n' of string end 
  i = MAX_CMD_ARGC_NUM * MAX_CMD_ARGV_STR_LENGTH -1;
  for( ; i > 0 ;i--)
  {
      if(buffer[i] == '\n')
      {
          buffer[i] = 0;
          break;
      }
      else
      if(buffer[i] !=  0)
      {
        break;
      }
  }

  // remove '\r' of string end 
  for( ;i > 0 ;i--)
  {
      if(buffer[i] == '\r')
      {
          buffer[i] = 0;
          break;
      }
      else
      if(buffer[i] !=  0)
      {
        break;
      }
  }

  char* token = strtok(buffer, " ");
  while (token != NULL && result->argc < MAX_CMD_ARGC_NUM) {
    strncpy(result->argv[result->argc], token, (MAX_CMD_ARGV_STR_LENGTH -1));
    result->argv[result->argc][MAX_CMD_ARGV_STR_LENGTH -1] = '\0';
    
    result->argc++;
    token = strtok(NULL, " ");
  }
  return 0;
  }
}
