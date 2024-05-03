/*
 * terminal_commander.h - Interactive I2C and Hardware Control Terminal for Arduino
 * 
 * Created: 3/4/2021
 * Author : Winry Litwa-Vulcu
 */

#ifndef TERMINAL_COMMANDER_H
#define TERMINAL_COMMANDER_H

  #include <Arduino.h>
  #include <Wire.h>

  // UART terminal console communication baud rate and timeout
  #define TERM_LINE_ENDING            ('\n')
  #define TERM_DEFAULT_CMD_DELIMITER  ( ' ')

  // UART TERM console input, I2C, and 'error' buffer sizes
  #define TERM_CHAR_BUFFER_SIZE       ( 64U)  // terminal buffer length in bytes
  #define TERM_TWOWIRE_BUFFER_SIZE    ( 30U)  // TwoWire read/write buffer length
  #define TERM_ERROR_MESSAGE_SIZE     ( 64U)  // error message buffer length
  #define TERM_MICROSEC_PER_CHAR      (140U)  // assumes 57600 baud minimum

  // Maximum number of unique user-defined commands
  #define MAX_USER_COMMANDS           ( 10U)

  #if (TERM_TWOWIRE_BUFFER_SIZE > TERM_CHAR_BUFFER_SIZE)
    #error "TwoWire buffer size must not exceed terminal character buffer size"
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
  extern int strcmp(const char *s1, const char *s2) __attribute__((weak));

  namespace TerminalCommander {
    namespace TerminalCommanderTypes {
      // the following only works for lambda expressions that do NOT capture local variables, e.g. [](){}
      typedef void (*user_callback_char_fn_t)(char*, size_t);

      // alt: the following works for lambda expressions that capture local variables
      // e.g. [&](){}, but requires #include <functional> which is not supported for AVR cores
      // typedef std::function<void(uint8_t)> cmd_callback_t

      /**
       * @brief Use this struct to hold error and warning context
       *
       * @details A more elaborate description of the constructor.
       */
      struct user_callback_char_t {
        const char* command;
        user_callback_char_fn_t callback; // could be pointer?
      };

      enum error_type_t {
        NoError = 0,
        NoInput, 
        UndefinedUserFunctionPtr,
        UnrecognizedInput, 
        InvalidSerialCmdLength, 
        IncomingTwoWireReadLength, 
        InvalidTwoWireCharacter, 
        InvalidTwoWireCmdLength, 
        InvalidTwoWireWriteData, 
        InvalidHexValuePair, 
        UnrecognizedProtocol, 
        UnrecognizedI2CTransType, 
      };

      enum twi_error_type_t {
        NO_ERROR = 0,
        TX_BUFFER_OVERFLOW, 
        NACK_ADDRESS, 
        NACK_DATA, 
        OTHER, 
        TIME_OUT
      };
    }

    class Error {
      public:
        /** Fixed array for raw incoming serial rx data */
        bool flag;

        /** Fixed array for raw incoming serial rx data */
        bool warning;

        /** Fixed array for raw incoming serial rx data */
        TerminalCommanderTypes::error_type_t type;

        /** Fixed array for raw incoming serial rx data */
        char message[TERM_ERROR_MESSAGE_SIZE + 1] = {'\0'};

        /*! @brief Class constructor
        *
        * @details A more elaborate description of the constructor.
        */
        Error(void);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   void
         * @returns Description of the returned parameter
         */
        void set(TerminalCommanderTypes::error_type_t error_type);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   void
         * @returns Description of the returned parameter
         */
        void warn(TerminalCommanderTypes::error_type_t error_type);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   void
         * @returns Description of the returned parameter
         */
        void clear(void);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   void
         * @returns Description of the returned parameter
         */
        void reset(void);

        private:
          /** Array of char pointers for storing error messages in PROGMEM */
          static const char *const string_error_table[] PROGMEM;
      };

    class Command {
      public:
        /** Fixed array for raw incoming serial rx data */
        char serialRx[TERM_CHAR_BUFFER_SIZE + 1] = {'\0'};

        /** Fixed array for holding the received command data */
        char data[TERM_CHAR_BUFFER_SIZE + 1] = {'\0'};

        /** Fixed array for holding hex values to be sent/received via TwoWire/I2C */
        uint8_t twowire[TERM_TWOWIRE_BUFFER_SIZE] = {0};

        /** Pointer to first non-space character following a space char in the incoming buffer */
        char *pArgs;

        /** Index of the serial buffer element pArgs points to */
        uint8_t iArgs;

        /** Total length in char of buffer preceding first space character*/
        uint8_t cmdLength;

        /** Total length in char and without spaces of buffer following first space character*/
        uint8_t argsLength;

        /** Index of current character in incoming serial rx data array */
        uint8_t index;

        /** True if the incoming serial data transfer is complete (newline was received) */
        bool complete;

        /** True if the incoming serial rx data overflowed the allocated buffer size */
        bool overflow;

        /*! @brief Class constructor
        *
        * @details A more elaborate description of the constructor.
        */
        Command(void);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   param Description of the input parameter
         * @returns void
         */
        void next(char character);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   param Description of the input parameter
         * @returns void
         */
        void previous(void);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   void
         * @returns void
         */
        void flushInput(void);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   param Description of the input parameter
         * @returns void
         */
        void flushTwoWire(void);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   void
         * @returns void
         */
        void initialize(void);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   void
         * @returns void
         */
        void reset(void);

      private:
    };

    class TerminalCommander {
      public:
        /*! @brief Class constructor
        *
        * @details A more elaborate description of the constructor.
        * 
        * @param pSerial   A pointer to an instance of the Stream class
        * @param pWire     A pointer to an instance of the TwoWire class
        */
        TerminalCommander(Stream *pSerial, TwoWire *pWire, const char command_delimiter);

        /*! @brief  Execute incoming serial string by command or protocol type
        *
        * @details Detailed description here.
        */
        void loop(void);

        /*! @brief  Execute incoming serial string by command or protocol type
        *
        * @details Detailed description here.
        */
        void echo(bool);

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
        bool isEchoEnabled = false;
        bool isNewTerminalCommandPrompt = true;
        const char termCommandDelimiter;

        Error lastError;
        Command command;
        Stream *pSerial;
        TwoWire *pWire;

        /*! @brief  Execute incoming serial string by command or protocol type
        *
        * @details Detailed description here.
        * 
        * @param   void
        * @returns void
        */
        bool serialCommandProcessor(void);

        /*! @brief  Error-check the incoming ASCII command string
        *
        * @details Detailed description here.
        * 
        * @param   param Description of the input parameter
        * @returns bool  True if buffer is not empty and all characters are allowed
        */
        bool isRxBufferDataValid(void);

        /**
         * @brief Use this struct to build and config terminal command data.
         *
         * @details A more elaborate description of the constructor.
         * 
         * @param   void
         * @returns void
         */
        bool removeSpaces(void);

        /*! @brief  Error-check the incoming ASCII command string
        *
        * @details Detailed description here.
        * 
        * @param   param    Description of the input parameter
        * @param   param    Description of the input parameter
        * @returns uint16_t Total valid character count of incoming buffer
        */
        bool runUserCallbacks(void);

        /*! @brief  Parse and error-check the incoming TwoWire command string
        *
        * @details Detailed description here.
        * 
        * @param   param Description of the input parameter
        * @returns bool  True if buffer is not empty and all characters are allowed
        */
        bool parseTwoWireData(void);

        /*! @brief  Error-check the incoming ASCII command string
        *
        * @details Detailed description here.
        * 
        * @param   param    Description of the input parameter
        * @param   param    Description of the input parameter
        * @returns uint16_t Total valid character count of incoming buffer
        */
        bool readTwoWire(void);

        /*! @brief  Error-check the incoming ASCII command string
        *
        * @details Detailed description here.
        * 
        * @param   param    Description of the input parameter
        * @param   param    Description of the input parameter
        * @returns uint16_t Total valid character count of incoming buffer
        */
        bool writeTwoWire(void);

        /*! @brief  Error-check the incoming ASCII command string
        *
        * @details Detailed description here.
        * 
        * @param   param    Description of the input parameter
        * @param   param    Description of the input parameter
        * @returns uint16_t Total valid character count of incoming buffer
        */
        bool scanTwoWireBus(void);

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
    };
  }
#endif
