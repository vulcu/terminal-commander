/*
 * terminal_commander.h - Declaration for the Terminal Commander Class
 * 
 * Created: 3/4/2021
 * Author : Winry Litwa-Vulcu
 */

#ifndef TERMINAL_COMMANDER_H
#define TERMINAL_COMMANDER_H

  #include <Arduino.h>
  #include <Wire.h>

  // UART terminal console communication baud rate and timeout
  #define TERM_LINE_ENDING          ('\n')

  // UART TERM console input, I2C, and 'error' buffer sizes
  #define TERM_CHAR_BUFFER_SIZE    (64U)  // total length in bytes
  #define TERM_COMMAND_PREFIX_SIZE ( 4U)  // command prefix ('i2cw', 'gpio', etc)
  #define TERM_TWOWIRE_BUFFER_SIZE (32U)  // TwoWire read/write buffer length

  #if (TERM_TWOWIRE_BUFFER_SIZE > TERM_CHAR_BUFFER_SIZE)
    #error "TwoWire buffer size must not exceed terminal character buffer size"
  #elif (TERM_COMMAND_PREFIX_SIZE != 4U)
    #error "Command prefix sizes other than 4 characters are unsupported"
  #endif

  namespace TerminalCommanderTypes {
    enum terminal_protocols_t {
      INVALID = 0,
      I2C_READ, 
      I2C_WRITE, 
      I2C_SCAN, 
      EXECUTE, 
      GPIO, 
    };

    enum error_type_t {
      NoError = 0,
      NoInput, 
      UnrecognizedInput, 
      InvalidSerialCmdLength, 
      InvalidTwoWireCharacter, 
      InvalidTwoWireCmdLength, 
      IncomingTwoWireReadLength, 
      InvalidTwoWireReadRegister, 
      InvalidHexValuePair, 
      UnrecognizedProtocol, 
      InvalidDeviceAddr,
      UnrecognizedI2CRequest, 
      UnrecognizedExecRequest, 
      UnrecognizedGPIOSelection, 
      CommandDataEmpty, 
      NonNumeric, 
      NumericFormat, 
      PositiveIntVal,  
      WriteProtectedLock,
    };

    enum twi_error_type_t {
      NO_ERROR = 0,
      TX_BUFFER_OVERFLOW, 
      NACK_ADDRESS, 
      NACK_DATA, 
      OTHER, 
      TIME_OUT
    };

    /**
     * @brief Use this struct to hold error and warning context
     *
     * @details A more elaborate description of the constructor.
     */
    typedef struct error_t {
      /** Fixed array for raw incoming serial rx data */
      bool flag;

      /** Fixed array for raw incoming serial rx data */
      bool warning;

      /** Fixed array for raw incoming serial rx data */
      error_type_t type;

      /** Fixed array for raw incoming serial rx data */
      char message[TERM_CHAR_BUFFER_SIZE] = {'\0'};

      /** Fixed array for raw incoming serial rx data */
      error_t () : flag(false), warning(false), type(NoError) {};

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   void
       * @returns Description of the returned parameter
       */
      error_type_t set(error_type_t error_type) {
        flag = true;
        type = error_type;
        return type;
      }

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   void
       * @returns Description of the returned parameter
       */
      void clear(void) {
        flag = false;
        warning = false;
        type = NoError;
      }

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   void
       * @returns Description of the returned parameter
       */
      void reset(void) {
        clear();
        memset(message,  '\0', sizeof(message));
      }
    };

    /**
     * @brief Use this struct to build and config terminal command data.
     *
     * @details A more elaborate description of the constructor.
     */
    typedef struct terminal_command_t {
      /** Fixed array for raw incoming serial rx data */
      char serialRx[TERM_CHAR_BUFFER_SIZE] = {'\0'};

      /** Fixed array for holding the prefix of the received command */
      char prefix[TERM_COMMAND_PREFIX_SIZE] = {'\0'};

      /** Fixed array for holding the received command data */
      char data[TERM_CHAR_BUFFER_SIZE - TERM_COMMAND_PREFIX_SIZE] = {'\0'};

      /** Fixed array for holding hex values to be sent/received via TwoWire/I2C */
      uint8_t twowire[TERM_TWOWIRE_BUFFER_SIZE] = {0};

      /** Protocol type identified for  */
      terminal_protocols_t protocol;

      /** Total length in char of command after prefix and without spaces */
      uint8_t length;

      /** Index of current character in incomming serial rx data array */
      uint16_t index;

      /** True if the incoming serial data transfer is complete (newline was received) */
      bool complete;

      /** True if the incoming serial rx data overflowed the allocated buffer size */
      bool overflow;

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   void
       * @returns Description of the returned parameter
       */
      terminal_command_t() :
        protocol(INVALID), length(0U), index(0U), complete(false), overflow(false) {}

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   param Description of the input parameter
       * @returns void
       */
      void next(char character) {
        if (character == TERM_LINE_ENDING) {
          complete = true;
          return;
        } 
        
        if (index >= TERM_CHAR_BUFFER_SIZE) {
          overflow = true;
          return;
        }
        
        serialRx[index++] = character;
      }

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   void
       * @returns void
       */
      void flushInput(void) {
        memset(serialRx,  '\0', sizeof(serialRx));
        complete = false;
        overflow = false;
      }

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   param Description of the input parameter
       * @returns void
       */
      void flushTwoWire(void) {
        memset(twowire, 0, sizeof(twowire));
      }

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   void
       * @returns void
       */
      void initialize(void) {
        flushTwoWire();
        memset(prefix,  '\0', sizeof(prefix));
        memset(data, '\0', sizeof(data));
        protocol = INVALID;
        index = 0U;
        length = 0U;
      }

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param   void
       * @returns void
       */
      void reset(void) {
        flushInput();
        initialize();
      }
    };
  }

namespace TerminalCommander {
  class TerminalCommander {
    public:
      /*! @brief Class constructor
       *
       * @details A more elaborate description of the constructor.
       * 
       * @param pSerial   A pointer to an instance of the Stream class
       * @param pWire     A pointer to an instance of the TwoWire class
       */
      TerminalCommander(Stream *pSerial, TwoWire *pWire);

      /*! @brief  Execute incoming serial string by command or protocol type
      *
      * @details Detailed description here.
      */
      void loop(void);

    private:
      TerminalCommanderTypes::error_t lastError;
      TerminalCommanderTypes::terminal_command_t command;
      Stream *pSerial;
      TwoWire *pWire;

      /*! @brief  Execute incoming serial string by command or protocol type
      *
      * @details Detailed description here.
      * 
      * @param   void
      * @returns void
      */
      void serialCommandProcessor(void);

      /*! @brief  Error-check the incoming ASCII command string
      *
      * @details Detailed description here.
      * 
      * @param   param Description of the input parameter
      * @returns void
      */
      void writeErrorMsgToSerialBuffer(TerminalCommanderTypes::error_type_t error, char *message);

      /*! @brief  Return the integer represented by a character array
      *
      * @details Detailed description here.
      * 
      * @param   param    Description of the input parameter
      * @returns uint32_t numeric value represented by input char array
      */
      //void checkCommandCharacterValidity(void) 

      /*! @brief  Error-check the incoming ASCII command string
      *
      * @details Detailed description here.
      * 
      * @param   param    Description of the input parameter
      * @param   param    Description of the input parameter
      * @returns uint16_t Total valid character count of incoming buffer
      */
      int16_t getIntFromCharArray(const char *char_array, size_t array_size);

      /*! @brief  Error-check the incoming ASCII command string
      *
      * @details Detailed description here.
      * 
      * @param   param    Description of the input parameter
      * @param   param    Description of the input parameter
      * @returns uint16_t Total valid character count of incoming buffer
      */
      uint16_t power_uint8(uint8_t base, uint8_t exponent);

      /*! @brief  Error-check the incoming ASCII command string
      *
      * @details Detailed description here.
      * 
      * @param   param    Description of the input parameter
      * @param   param    Description of the input parameter
      * @returns uint16_t Total valid character count of incoming buffer
      */
      void scanTwoWireBus(void);
  };
}
#endif
