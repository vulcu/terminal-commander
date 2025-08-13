/*
 * terminal_commander.h - Interactive Serial Terminal for Arduino
 * Copyright (C) 2024 Winry Litwa-Vulcu
 * Licensed under the GNU General Public License, Version 3
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
  
  #define MAX_CMD_ARGC_NUM             (5)
  #define MAX_CMD_ARGV_STR_LENGTH      (10 + 1)   //each parameter max 10 char with '\0'
  

  #if (TERM_TWOWIRE_BUFFER_SIZE > TERM_CHAR_BUFFER_SIZE)
    #error "TwoWire buffer size must not exceed terminal character buffer size"
  #elif (TERM_TWOWIRE_BUFFER_SIZE > 34U)
    // '34' since this buffer size includes address and register bytes
    #warning "Wire library does not support transactions exceeding 32 bytes"
  #endif

struct cmd_param {
  int  argc;                                                  //parsed cmd parameter_cnt(max MAX_CMD_ARGC_NUM)
  char argv[MAX_CMD_ARGC_NUM][MAX_CMD_ARGV_STR_LENGTH];       //parsed cmd parameter
};

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
      typedef void (user_callback_char_fn_t)(struct cmd_param);

      // alt: the following works for lambda expressions that capture local variables
      // e.g. [&](){}, but requires #include <functional> which is not supported for AVR cores
      // typedef std::function<void(char*, size_t)> user_callback_char_fn_t

      /**
       * @struct user_callback_char_t "terminal_commander.h"
       * @brief Use this struct to hold user commands and callback fn
       *
       * @details This struct holds a single user command (as created by
       *          Terminal::onCommand() for a callback function 
       *          matching the type user_callback_char_fn_t
       */
      struct user_callback_char_t {
        const char *command;
        user_callback_char_fn_t *callback;
      };

      /** @brief Index of the string error table array */
      enum error_type_t {
        NoError = 0,
        NoInput, 
        UndefinedUserFunctionPtr,
        UnrecognizedInput, 
        InvalidSerialCmdLength,
        TwoWireNack, 
        TwoWireReadLength, 
        InvalidTwoWireCharacter, 
        InvalidTwoWireCmdLength, 
        InvalidTwoWireWriteData, 
        InvalidHexValuePair, 
        UnrecognizedCommand, 
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
     * @class Print "terminal_commander.h"
     * @brief Terminal Commander template class utilizing variadic template 
     *        recursion for efficient printing of multiple argument types
     */
    class Print {
      public:
        template<typename Stream, typename X>
        static void print(Stream *pSerial, X&& x) {
          pSerial->print(x);
        }

        template<typename Stream, typename X, typename... Args>
        static void print(Stream *pSerial, X&& x, Args&&... args) {
          pSerial->print(x);
          print(pSerial, args...);
        }

        template<typename Stream, typename X>
        static void println(Stream *pSerial, X&& x) {
          pSerial->println(x);
        }

        template<typename Stream, typename X, typename... Args>
        static void println(Stream *pSerial, X&& x, Args&&... args) {
          pSerial->print(x);
          println(pSerial, args...);
        }
    };

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
        
        /* after parsed command parameters */
        struct cmd_param cmd_param_value;

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
     * @class Terminal "terminal_commander.h"
     * @brief Terminal Commander terminal object
     * 
     * @details The Terminal class is an interactive serial terminal
     *          for Arduino, providing serial buffer parsing
     *          and command-line access to the I2C interface. The
     *          class is intended to streamline the creation of a
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
    class Terminal {
      public:
        /*! @brief Construct an instance of the Terminal class
        *
        * @details Constructor for the Terminal class.
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
        Terminal(Stream *pSerial, TwoWire *pWire);
        Terminal(Stream *pSerial, TwoWire *pWire, const char command_delimiter);

        /*! @brief The core Terminal method, place this in Arduino's loop()
         *
         * @details Handle all processing for the serial terminal, including reading
         *          and parsing of the serial buffer and execution of all callback
         *          functions. Place this method in the loop() section of Arduino code.
         * 
         * @param   void
         * @returns void
        */
        void loop(void);

        /*! @brief Initialize the Terminal output, place this in Arduino's setup()
         *
         * @details This is an optional method to reduce visual clutter by initializing
         *          the terminal prompt on a new line during Arduino setup.
         * 
         * @param   void
         * @returns void
        */
        void init(void);

        /*! @brief Enable serial terminal echo
         *
         * @details Echo incoming terminal ASCII back to the source terminal. Useful
         *          for programs such as TeraTerm and PuTTY. Correctly handles the
         *          backspace input and will delete the previous terminal character.
         *          Not recommended for terminals which send the entire line in one
         *          transmission when hitting Enter in the terminal.
         *          Note: VT100-style control characters (^[C, ^[D, etc.) are not 
         *          supported, so Left/Right arrow keys will generate unrecognized
         *          inputs.
         * 
         * @param   bool  Boolean to enable (true) or disable (false) terminal echo.
         * @returns void
        */
        void echo(bool);

        /*! @brief Attach a lambda expression or function pointer to a terminal command
         *
         * @details Call this inside the Arduino 'setup' function. Usage is either with a lamba
         *          expression:
         *            Terminal.onCommand("mycommand", [](char* args, size_t args_size) {
         *              // custom code here
         *            }
         *          or with a function pointer, where myfuction points to the address of a function
         *          which takes (char* args, size_t args_size) as arguments and returns void:
         *            Terminal.onCommand("mycommand", &myfuction);
         * 
         * @param   char*                   Char array with the command name, e.g. 'mycommand'
         * @param   user_callback_char_fn_t Lambda expr. or fn pointer matching 'void (char*, size_t)'
         * @returns void
        */
        void onCommand(const char* command, TerminalCommanderTypes::user_callback_char_fn_t callback);

      private:
        /** A struct array for storing user commands and their corresponding fn pointers */
        TerminalCommanderTypes::user_callback_char_t userCharCallbacks[MAX_USER_COMMANDS] = {};

        /** Increments by one for each user callback added by onCommand() */
        uint8_t numUserCharCallbacks = 0;

        /** True if serial terminal echo is enabled */
        bool isEchoEnabled = false;

        /** True if the terminal object is ready for the next command and should print '>>' prompt */
        bool isNewTerminalCommandPrompt = true;

        /** Terminal command delimiter, defaults to space unless specified during construction */
        const char termCommandDelimiter;

        /** Instance of the Error class for maintaining the terminal error state */
        Error lastError;

        /** Instance of the Command class for maintaining incoming buffers, indicies, and pointers */
        Command command;

        /** Pointer to an instance of the Arduino Stream class, specified when calling constructor */
        Stream *pSerial;

        /** Pointer to an instance of the Arduino Wire class, specified when calling constructor */
        TwoWire *pWire;

        /*! @brief  Process the incoming raw serialRx data buffer
         *
         * @details  Called once when a newline character is received from the terminal.
         *           Handles all processing of the incoming serial buffer and calling of all
         *           associated functionality or user commands based on serial buffer content.
         * 
         * @param   void
         * @returns void
         */
        bool serialCommandProcessor(void);

        /*! @brief Check the validity of the incoming serial buffer
         *
         * @details Check that the incoming serialRx buffer is not empty and contains only
         *          allowed ASCII characters (letters, numbers, some symbols, and delimiter).
         * 
         * @param   param Description of the input parameter
         * @returns bool  True if buffer is not empty and all characters are allowed
         */
        bool isRxBufferDataValid(void);

        /*! @brief Remove all whitespace characters from the incoming serial command
         *
         * @details Removes all whitespace from the incoming serial command, copies this
         *          over to a separate data buffer (original buffer is not modified), and
         *          records the index of and a pointer to the first command delimiter found.
         * 
         * @param   void
         * @returns bool  True if no errors occured during whitespace removal
         */
        bool removeSpaces(void);

        /*! @brief Check for user callbacks and call one if the command matches
         *
         * @details Check the incoming command (as denoted by the command delimiter)
         *          against the array of user commands, if any. This happens prior to
         *          the built-in 'i2c' or 'scan' commands being checked for, allowing
         *          these commands to be overloaded if desired. If a command matches,
         *          execute the user callback and pass any remaining arguments to it.
         * 
         * @param   void
         * @returns bool  True if no errors occured during callback checks and execution
         */
        bool runUserCallbacks(void);

        /*! @brief Parse and error-check the incoming TwoWire command string
         *
         * @details Checks TwoWire data to ensure it only contains hex value pairs,
         *          no additional characters or half-bytes of data.
         * 
         * @param   void
         * @returns bool  True if TwoWire buffer is not empty and has valid contents
         */
        bool parseTwoWireCommand(void);

        /*! @brief  Read bytes from an address on the TwoWire bus
         *
         * @details Read the requested registers from the TwoWire bus (as specified
         *          by the incoming command data). Can perform sequential reads, if
         *          supported by the IC being read from.
         * 
         * @param   void
         * @returns bool  True if the read operation was successful and without errors
         */
        bool readTwoWire(void);

        /*! @brief  Write byte to an address on the TwoWire bus
         *
         * @details Write the requested registers from the TwoWire bus (as specified
         *          by the incoming command data). Can perform sequential writes, if
         *          supported by the IC being read from.
         * 
         * @param   void
         * @returns bool  True if the write operation was successful and without errors
         */
        bool writeTwoWire(void);

        /*! @brief  Scan and report all devices on the TwoWire bus
         *
         * @details Scans the TwoWire bus and prints the address of all devices that
         *          ACK a transmission start operation. Scans all 7-bit addresses
         *          from 0 to 127.
         * 
         * @param   void
         * @returns bool  True if the scan operation was successful and without errors
         */
        bool scanTwoWireBus(void);

        /*! @brief  Print the hexadecimal TwoWire address value to the console
         *
         * @details Automatically prepends an additional zero if the address
         *          value is less than 0x10, such that the terminal output always
         *          contains two hexadecimal digits (e.g. 0x02 vs 0x2)
         * 
         * @param   uint8_t The 7-bit TwoWire address to print to the terminal
         * @returns void
         */
        void printTwoWireAddress(uint8_t i2c_address);

        /*! @brief  Print the hexadecimal TwoWire register value to the console
         *
         * @details Automatically prepends an additional zero if the register
         *          value is less than 0x10, such that the terminal output always
         *          contains two hexadecimal digits (e.g. 0x02 vs 0x2)
         * 
         * @param   uint8_t The TwoWire register address to print to the terminal
         * @returns void
         */
        void printTwoWireRegister(uint8_t i2c_register);
        
        int parseCmd(const char* input, struct cmd_param* result);
    };
  }
#endif
