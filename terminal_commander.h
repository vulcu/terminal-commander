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
  #define TERM_CHAR_BUFFER_SIZE     ( 64U)  // total length in bytes
  #define TERM_TWOWIRE_BUFFER_SIZE  ( 30U)  // TwoWire read/write buffer length

  // Maximum number of unique user-defined commands
  #define MAX_USER_COMMANDS         ( 10U)

  #if (TERM_TWOWIRE_BUFFER_SIZE > TERM_CHAR_BUFFER_SIZE)
    #error "TwoWire buffer size must not exceed terminal character buffer size"
  #elif (TERM_COMMAND_PREFIX_SIZE != 4U)
    #error "Command prefix sizes other than 4 characters are unsupported"
  #elif (TERM_TWOWIRE_BUFFER_SIZE > 34U)
    // '34' since this buffer size includes address and register bytes
    #warning "Wire library does not support transactions exceeding 32 bytes"
  #endif

  /**
   * @brief Compares two null-terminated byte strings lexicographically.
   *
   * @details Returns:
   *   Negative Value: If s1 appears before s2 in lexicographical order. Or, 
   *                   the first not-matching character in s1 has a greater 
   *                   ASCII value than the corresponding character in s2.
   *             Zero: If s1 and s2 compare equal.
   *   Positive Value: If s1 appears after s2 in lexicographical order. 
   *
   * @param  s1  Null-terminated byte string.
   * @param  s2  Null-terminated byte string.
   * @return int 
   */
  int16_t strcmp(const char *s1, const char *s2) __attribute__((weak));

  namespace TerminalCommander {
    namespace TerminalCommanderTypes {
      // the following only works for lambda expressions that do NOT capture local variables, e.g. [](){}
      typedef void (*user_callback_char_fn_t)(char*, size_t);

      // alt: the following works for lambda expressions that capture local variables
      // e.g. [&](){}, but requires #include <functional> which is not supported for AVR cores
      // typedef std::function<void(uint8_t)> cmd_callback_t

      enum terminal_protocols_t {
        INVALID = 0,
        I2C_READ, 
        I2C_WRITE, 
        I2C_SCAN, 
        // USER_CALLBACK,  
      };

      enum error_type_t {
        NoError = 0,
        UndefinedUserFunctionPtr,
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
        UserCommandDataEmpty, 
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
      struct user_callback_char_t {
        const char* command;
        user_callback_char_fn_t callback; // could be pointer?
      };

      struct error_t {
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
          this->flag = true;
          this->type = error_type;
          return this->type;
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
          this->flag    = false;
          this->warning = false;
          this->type    = NoError;
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
          this->clear();
          memset(this->message,  '\0', sizeof(this->message));
        }
      };

      /**
       * @brief Use this struct to build and config terminal command data.
       *
       * @details A more elaborate description of the constructor.
       */
      struct terminal_command_t {
        /** Fixed array for raw incoming serial rx data */
        char serialRx[TERM_CHAR_BUFFER_SIZE] = {'\0'};

        /** Fixed array for holding the received command data */
        char data[TERM_CHAR_BUFFER_SIZE] = {'\0'};

        /** Fixed array for holding hex values to be sent/received via TwoWire/I2C */
        uint8_t twowire[TERM_TWOWIRE_BUFFER_SIZE] = {0};

        /** Protocol type identified for  */
        terminal_protocols_t protocol;

        /** Pointer to first non-space character following a space in the command cstring */
        char *pArgs;

        /** Total length in char and without spaces of arguments following command*/
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
          protocol(INVALID), args(nullptr), length(0U), index(0U), complete(false), overflow(false) {}

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
         * @param   void
         * @returns void
         */
        void flushInput(void) {
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
        void flushTwoWire(void) {
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
        void initialize(void) {
          this->flushTwoWire();
          memset(this->data, '\0', sizeof(this->data));
          this->protocol = INVALID;
          this->index    = 0U;
          this->length   = 0U;
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
          this->flushInput();
          this->initialize();
        }
      };
    }

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

        /*! @brief  Add callback function for specific command.
        *
        * @details Usage:
        *          onCommand("YOUR COMMAND", [](char* args, size_t size){
        *              // do things
        *          });
        */
        void onCommand(const char* command, TerminalCommanderTypes::user_callback_char_fn_t callback);

      private:
        TerminalCommanderTypes::user_callback_char_t userCharCallbacks[MAX_USER_COMMANDS] = {};
        uint8_t numUserCharCallbacks = 0;

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
        * @returns bool  True if buffer is not empty and all characters are allowed
        */
        bool isRxBufferDataValid(void);

        /*! @brief  Parse and error-check the incoming TwoWire command string
        *
        * @details Detailed description here.
        * 
        * @param   param Description of the input parameter
        * @returns bool  True if buffer is not empty and all characters are allowed
        */
        bool parseTwoWireData(void);

        /*! @brief  Return the integer represented by a character array
        *
        * @details Detailed description here.
        * 
        * @param   param    Description of the input parameter
        * @returns void
        */
        void printTwoWireAddress(uint8_t i2c_address);

        /*! @brief  Return the integer represented by a character array
        *
        * @details Detailed description here.
        * 
        * @param   param    Description of the input parameter
        * @returns void
        */
        void printTwoWireRegister(uint8_t i2c_register);

        /*! @brief  Error-check the incoming ASCII command string
        *
        * @details Detailed description here.
        * 
        * @param   param    Description of the input parameter
        * @param   param    Description of the input parameter
        * @returns uint16_t Total valid character count of incoming buffer
        */
        void scanTwoWireBus(void);

        /*! @brief  Error-check the incoming ASCII command string
        *
        * @details Detailed description here.
        * 
        * @param   param Description of the input parameter
        * @returns void
        */
        void writeErrorMsgToSerialBuffer(TerminalCommanderTypes::error_type_t error, char *message);
    };
  }
#endif
