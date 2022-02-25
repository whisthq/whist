/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file command_line.h
 * @brief Command line handling API.
 */
#ifndef WHIST_UTILS_COMMAND_LINE_H
#define WHIST_UTILS_COMMAND_LINE_H

#include <stdbool.h>

#include "linked_list.h"

/**
 * @defgroup command_line Command-line Options
 *
 * API to handle defining command-line options and then parsing
 * command-lines using them.
 *
 * Example:
 * @code{.c}
 * static int option_int;
 * static bool option_bool;
 * static const char *option_string;

 * COMMAND_LINE_INT_OPTION(option_int, 'i', "int", -12, 51,
 *                         "An integer between -12 and 51.")
 * COMMAND_LINE_BOOL_OPTION(option_bool, 'b', "bool",
 *                          ""A boolean, which can be either true or false.")
 * COMMAND_LINE_STRING_OPTION(option_string, 's', "string", SIZE_MAX,
 *                            "The string, which can be arbitrarily long just "
 *                            "like this very-padded explanation which will "
 *                            "demonstrate how showing the help text works.")
 *
 * static bool callback(const WhistCommandLineOption *opt, const char *value) {
 *     printf("Hello world!\n");
 *     return true;
 * }
 * COMMAND_LINE_CALLBACK_OPTION(callback, 'c', "callback", WHIST_OPTION_NO_ARGUMENT,
 *                              "Offer a traditional greeting.")
 *
 * int main(int argc, const char **argv) {
 *     bool ret = whist_parse_command_line(argc, argv, NULL);
 *     if (!ret) {
 *         printf("Failed to parse command line!\n");
 *         return 1;
 *     }
 *
 *     printf("int = %d\n", option_int);
 *     printf("bool = %d\n", option_bool);
 *     printf("string = %s\n", option_string);
 *
 *     return 0;
 * }
 * @endcode
 *
 * @{
 */

typedef struct WhistCommandLineOption WhistCommandLineOption;

/**
 * Type for registered command-line option handlers.
 *
 * @param opt    The option to be handled.
 * @param value  String value of the option.  If NULL, indicates that
 *               the option is present with no argument.
 * @return  True on success, false on error and parsing will then fail.
 */
typedef bool (*WhistOptionHandler)(const WhistCommandLineOption *opt, const char *value);

/**
 * Type for anonymous command-line operand handlers.
 *
 * @param position  Position in the command-line that this operand
 *                  appears (array index in argv).
 * @param value     String value of the operand.
 * @return  True on success, false on error and parsing will then fail.
 */
typedef bool (*WhistOperandHandler)(int position, const char *value);

/**
 * Flags which apply to command-line options.
 */
enum {
    /**
     * The option takes no argument.
     */
    WHIST_OPTION_NO_ARGUMENT = 1,
    /**
     * The option may take an argument.
     */
    WHIST_OPTION_OPTIONAL_ARGUMENT = 2,
    /**
     * The option requires an argument.
     */
    WHIST_OPTION_REQUIRED_ARGUMENT = 4,
    /**
     * The option is for internal use only, and should not be visible
     * when querying options.
     *
     * This applies to internally-generated options like "--help".
     */
    WHIST_OPTION_INTERNAL = 8,
};

/**
 * Type defining a command-line option.
 */
struct WhistCommandLineOption {
    /** Header for internal option list. */
    LINKED_LIST_HEADER;
    /** Short option character, or 0 if none. */
    int short_option;
    /** Long option string, or NULL if none. */
    const char *long_option;
    /** Help text to show in --help. */
    const char *help_text;
    /** Flags associated with this option. */
    int flags;
    /** Handler function to call for this option. */
    WhistOptionHandler handler;
    /** Pointer to variable to set. */
    void *variable;
    /** Minimum value for integer types. */
    int range_min;
    /** Maximum value for integer types. */
    int range_max;
    /** Maximum length for array types and strings. */
    size_t length;
};

/**
 * Register a command-line option.
 *
 * This function is normally called from a constructor, since options
 * need to be registered before the command-line is parsed early in
 * main().
 *
 * @param opt  Option structure to register.
 */
void whist_register_command_line_option(WhistCommandLineOption *opt);

/**
 * Register an integer option.
 *
 * This uses strtol() to parse the value, so hexadecimal is usable.
 *
 * @param variable      The variable to place the result in, which must
 *                      have the type "int".
 * @param short_option  Short option character, or 0 for none.
 * @param long_option   Long option string.
 * @param min_value     Minimum allowed value.
 * @param max_value     Maximum allowed value.
 * @param help_text     Help text string.
 */
#define COMMAND_LINE_INT_OPTION(variable, short_option, long_option, min_value, max_value, \
                                help_text)                                                 \
    CONSTRUCTOR(register_command_line_option_##variable) {                                 \
        static WhistCommandLineOption opt = {                                              \
            {NULL, NULL},                                                                  \
            short_option,                                                                  \
            long_option,                                                                   \
            help_text,                                                                     \
            WHIST_OPTION_REQUIRED_ARGUMENT,                                                \
            &whist_handle_int_option_internal,                                             \
            &variable,                                                                     \
            min_value,                                                                     \
            max_value,                                                                     \
            0,                                                                             \
        };                                                                                 \
        whist_register_command_line_option(&opt);                                          \
    }

/**
 * Register a boolean option.
 *
 * @param variable      The variable to place the result in, which must
 *                      have the type "bool".
 * @param short_option  Short option character, or 0 for none.
 * @param long_option   Long option string.
 * @param help_text     Help text string.
 */
#define COMMAND_LINE_BOOL_OPTION(variable, short_option, long_option, help_text) \
    CONSTRUCTOR(register_command_line_option_##variable) {                       \
        static WhistCommandLineOption opt = {                                    \
            {NULL, NULL},                                                        \
            short_option,                                                        \
            long_option,                                                         \
            help_text,                                                           \
            WHIST_OPTION_OPTIONAL_ARGUMENT,                                      \
            &whist_handle_bool_option_internal,                                  \
            &variable,                                                           \
            0,                                                                   \
            0,                                                                   \
            0,                                                                   \
        };                                                                       \
        whist_register_command_line_option(&opt);                                \
    }

/**
 * Register a string option.
 *
 * @param variable      The variable to place the result in, which must
 *                      have the type "const char *".
 * @param short_option  Short option character, or 0 for none.
 * @param long_option   Long option string.
 * @param max_length    Maximum allowed length of the string.
 * @param help_text     Help text string.
 */
#define COMMAND_LINE_STRING_OPTION(variable, short_option, long_option, max_length, help_text) \
    CONSTRUCTOR(register_command_line_option_##variable) {                                     \
        static WhistCommandLineOption opt = {                                                  \
            {NULL, NULL},                                                                      \
            short_option,                                                                      \
            long_option,                                                                       \
            help_text,                                                                         \
            WHIST_OPTION_REQUIRED_ARGUMENT,                                                    \
            &whist_handle_string_option_internal,                                              \
            (void *)&variable,                                                                 \
            0,                                                                                 \
            0,                                                                                 \
            max_length,                                                                        \
        };                                                                                     \
        whist_register_command_line_option(&opt);                                              \
    }

/**
 * Register an option using a callback.
 *
 * Runs the callback with argument (if present) when the option is
 * found, allowing arbitrary handling.
 *
 * @param callback      Callback function to call for the option.
 * @param short_option  Short option character, or 0 for none.
 * @param long_option   Long option string.
 * @param flags         Flags to apply.  This must set whether an
 *                      argument is required.
 * @param help_text     Help text string.
 */
#define COMMAND_LINE_CALLBACK_OPTION(callback, short_option, long_option, flags, help_text) \
    CONSTRUCTOR(register_command_line_option_##callback) {                                  \
        static WhistCommandLineOption opt = {                                               \
            {NULL, NULL},                                                                   \
            short_option,                                                                   \
            long_option,                                                                    \
            help_text,                                                                      \
            WHIST_OPTION_REQUIRED_ARGUMENT,                                                 \
            &callback,                                                                      \
            0,                                                                              \
            0,                                                                              \
            0,                                                                              \
            0,                                                                              \
        };                                                                                  \
        whist_register_command_line_option(&opt);                                           \
    }

/**
 * Set a single option value.
 *
 * @param name   Name of the option to set.  If this is a single
 *               character then it will attempt to interpret it as a
 *               short option, then if that is not found then it will
 *               be interpreted as a long option.
 * @param value  Option value to set.
 * @return  True on success, false if the option is not found or the
 *          value was not valid.
 */
bool whist_set_single_option(const char *name, const char *value);

/**
 * Parse a command line.
 *
 * @param argc  Argument count passed to main().
 * @param argv  Argument vector passed to main().
 * @param operand_handler  If non-NULL, called for each operand
 *                         (non-option) argument in the order that they
 *                         are seen.  If NULL, operand arguments are
 *                         considered an error.
 * @return  True on success, false if parsing failed or if some option
 *          value was invalid.
 */
bool whist_parse_command_line(int argc, const char **argv, WhistOperandHandler operand_handler);

/**
 * @privatesection
 * These types and functions are the handlers used by the macros above,
 * and should not be used directly.
 */

bool whist_handle_int_option_internal(const WhistCommandLineOption *opt, const char *value);
bool whist_handle_bool_option_internal(const WhistCommandLineOption *opt, const char *value);
bool whist_handle_string_option_internal(const WhistCommandLineOption *opt, const char *value);

/** @} */

#endif /* WHIST_UTILS_COMMAND_LINE_H */
