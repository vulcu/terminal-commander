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
      /** @brief User char* callback lambda expression that does not capture local variables */
      typedef void (user_callback_char_fn_t)(char*, size_t);

      // alt: the following works for lambda expressions that capture local variables
      // e.g. [&](){}, but requires #include <functional> which is not supported for AVR cores
      // typedef std::function<void(char*, size_t)> user_callback_char_fn_t

      /**
       * @struct user_callback_char_t "terminal_commander.h"
       * @brief Use this struct to hold user commands and callback fn
       *
       * @details This struct holds a single user command (as created by
       *          TerminalCommander::onCommand() for a callback function 
       *          matching the type user_callback_char_fn_t
       */
      struct user_callback_char_t {
        const char *command;
        user_callback_char_fn_t *callback; // could be pointer?
      };

      /** @brief Index of the string error table array */
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

      /** @brief Error names returned by Wire.endTransmission() */
      enum twi_error_type_t {
        NO_ERROR = 0,
        TX_BUFFER_OVERFLOW, 
        NACK_ADDRESS, 
        NACK_DATA, 
        OTHER, 
        TIME_OUT
      };
    }

    /**
     * @class Error "terminal_commander.h"
     * @brief Terminal Commander error states and messages
     */
    class Error {
      public:
        /** True if an error or warning has been set */
        bool flag;

        /** True if the error set should be treated as a warning */
        bool warning;

        /** Enum indexing the string_error_table array */
        TerminalCommanderTypes::error_type_t type;

        /** Char array for holding the terminal error message */
        char message[TERM_ERROR_MESSAGE_SIZE + 1] = {'\0'};

        /*! @brief Construct an instance of the Error class
        *
        * @details Constructor for Error class, takes no arguments
        */
        Error(void);

        /**
         * @brief Set a new error message and raise the error flag
         *
         * @details Set with set the error message using the string_error_table
         *          and will flag that an error has occured by setting flag = true.
         * 
         * @param   error_type_t TerminalCommanderTypes::error_type_t
         * @returns void
         */
        void set(TerminalCommanderTypes::error_type_t error_type);

        /**
         * @brief Set a new error message and raise error and warning flags
         *
         * @details Warn will set warning = true before calling set()
         * 
         * @param   error_type_t TerminalCommanderTypes::error_type_t
         * @returns void
         */
        void warn(TerminalCommanderTypes::error_type_t error_type);

        /**
         * @brief Clear error type and all flags
         *
         * @details Set error type to 'NO_ERROR' and all flags to 'false'
         * 
         * @param   void
         * @returns void
         */
        void clear(void);

        /**
         * @brief Clear error type, error message, and all flags
         *
         * @details Calls clear() then sets message array to '\0'
         * 
         * @param   void
         * @returns void
         */
        void reset(void);

        private:
          /** Array of char pointers for storing error messages in PROGMEM */
          static const char *const string_error_table[] PROGMEM;
      };

    /**
     * @class Command "terminal_commander.h"
     * @brief Terminal Commander command buffers, pointers, and indicies
     */
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

        /** Total length in char of buffer preceding first command delimiter character*/
        uint8_t cmdLength;

        /** Total length in char and without spaces of buffer after command delimiter character*/
        uint8_t argsLength;

        /** Index of current character in incoming serial rx data array */
        uint8_t index;

        /** True if incoming serial data transfer is complete (line ending was received) */
        bool complete;

        /** True if incoming serial rx data overflowed the size allocated by TERM_CHAR_BUFFER_SIZE */
        bool overflow;

        /*! @brief Construct an instance of the Command class
        *
        * @details Constructor for Command class, takes no arguments
        */
        Command(void);

        /**
         * @brief Add character to buffer and increment buffer index
         *
         * @details Add a single character to the incoming serialRx buffer
         *          and increment the buffer index by 1
         * 
         * @param   char Character to add to the incoming buffer
         * @returns void
         */
        void next(char character);

        /**
         * @brief Decrement buffer index and delete character at the previous index
         *
         * @details Decrement the index of the incoming serialRx buffer and 
         *          reset the character at the previous index back to '\0'
         * 
         * @param   void
         * @returns void
         */
        void previous(void);

        /**
         * @brief Clear incoming buffer contents and reset overflow and complete flags
         *
         * @details Clear the entire serialRx buffer by setting all elements to '\0',
         *          then clear 'overflow' and 'complete' flags by setting to false
         * 
         * @param   void
         * @returns void
         */
        void flushInput(void);

        /**
         * @brief Clear twowire buffer contents
         *
         * @details Clear the entire twowire buffer by setting contents to '\0'
         * 
         * @param   void
         * @returns void
         */
        void flushTwoWire(void);

        /**
         * @brief Clear data and twowire buffers and reset all indicies, pointers, and flags
         *
         * @details Clear contents of the data and twowire buffers by setting all elements 
         *          to '\0', and reset all indicies, pointers, and flags to default value.
         *          Note, this method does NOT clear data from the serialRx buffer.
         * 
         * @param   void
         * @returns void
         */
        void initialize(void);

        /**
         * @brief Clear contents of all buffers and reset all indicies, pointers, and flags
         *
         * @details Reset Command object by calling flushInput() followed by initialize()
         * 
         * @param   void
         * @returns void
         */
        void reset(void);

      private:
    };

    /**
     * @class TerminalCommander "terminal_commander.h"
     * @brief Terminal Commander terminal object
     * 
     * @details The TerminalCommander class is an interactive serial
     *          terminal for Arduino, providing serial buffer parsing
     *          and command-line access to the I2C interface. The
     *          class is intended to simplify the creation of a
     *          simple command-line terminal on any Arduino device.
     * 
     *          Construction requires a pointer to an instance of the
     *          Stream class, and a pointer to an instance of the TwoWire
     *          class. Optionally, a single-character command delimiter 
     *          may be defined for deliniating custom user commands and 
     *          their arguments. The default command delimiter is a space.
     * 
     * @param pSerial   A pointer to an instance of the Stream class
     * @param pWire     A pointer to an instance of the TwoWire class
     * @param char      A single ASCII character
     */
    class TerminalCommander {
      public:
        /*! @brief Construct an instance of the TerminalCommander class
        *
        * @details Constructor for the TerminalCommander class.
        *          Requires a pointer to an instance of the Stream class,
        *          and a pointer to an instance of the TwoWire class.
        *          Optionally, a single-character command delimiter may be 
        *          defined for deliniating custom user commands and their
        *          arguments. The default command delimiter is a space.
        * 
        * @param pSerial   A pointer to an instance of the Stream class
        * @param pWire     A pointer to an instance of the TwoWire class
        * @param char      A single ASCII character
        */
        TerminalCommander(Stream *pSerial, TwoWire *pWire, const char command_delimiter);

        /*! @brief The main TerminalCommander method, place this in Arduino's loop()
         *
         * @details Reset Command object by calling flushInput() followed by initialize()
         * 
         * @param   void
         * @returns void
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
