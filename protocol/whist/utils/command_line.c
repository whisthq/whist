/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file command_line.c
 * @brief Command line handling implementation.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "whist/core/whist.h"
#include "whist/core/error_codes.h"
#include "whist/utils/linked_list.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#include "command_line.h"

// Enable for debugging command-line options.
#define LOG_COMMAND_LINE false

WhistStatus whist_handle_int_option_internal(const WhistCommandLineOption *opt, const char *value) {
    // Do this conversion in long-long to try to better catch extreme
    // out-of-range values.
    char *end;
    long long x = strtoll(value, &end, 0);
    if (end == value || *end != '\0') {
        // String is not a valid integer.
        return WHIST_ERROR_SYNTAX;
    }
    if (x < opt->range_min || x > opt->range_max) {
        // Out of range.
        return WHIST_ERROR_OUT_OF_RANGE;
    }
    *(int *)opt->variable = x;
    return WHIST_SUCCESS;
}

WhistStatus whist_handle_bool_option_internal(const WhistCommandLineOption *opt,
                                              const char *value) {
    bool *variable = opt->variable;

    if (!value) {
        // No argument, set to true.
        *variable = true;
        return WHIST_SUCCESS;
    }

    static const char *const true_strings[] = {
        "true",
        "1",
        "on",
        "yes",
    };
    static const char *const false_strings[] = {
        "false",
        "0",
        "off",
        "no",
    };

    for (unsigned int i = 0; i < ARRAY_LENGTH(true_strings); i++) {
        if (!strcasecmp(value, true_strings[i])) {
            *variable = true;
            return WHIST_SUCCESS;
        }
    }
    for (unsigned int i = 0; i < ARRAY_LENGTH(false_strings); i++) {
        if (!strcasecmp(value, false_strings[i])) {
            *variable = false;
            return WHIST_SUCCESS;
        }
    }

    return WHIST_ERROR_SYNTAX;
}

WhistStatus whist_handle_string_option_internal(const WhistCommandLineOption *opt,
                                                const char *value) {
    if (strlen(value) > opt->length) {
        return WHIST_ERROR_TOO_LONG;
    }
    char **var = opt->variable;
    free(*var);
    *var = strdup(value);
    if (!*var) {
        return WHIST_ERROR_OUT_OF_MEMORY;
    } else {
        return WHIST_SUCCESS;
    }
}

static LinkedList option_list;

void whist_register_command_line_option(WhistCommandLineOption *opt) {
    if (!opt->help_text) {
        opt->help_text = "The naughty developer did not provide any help text for this option.";
    }
    linked_list_add_tail(&option_list, opt);
}

static int compare_options(const void *first, const void *second) {
    const WhistCommandLineOption *opt_a = first, *opt_b = second;

    // Internal options (help/version) always sort to the end.
    if ((opt_a->flags ^ opt_b->flags) & WHIST_OPTION_INTERNAL) {
        if (opt_a->flags & WHIST_OPTION_INTERNAL)
            return +1;
        else
            return -1;
    }

    if (opt_a->long_option && opt_b->long_option) {
        return strcmp(opt_a->long_option, opt_b->long_option);
    } else if (opt_a->long_option) {
        return opt_a->long_option[0] - opt_b->short_option;
    } else if (opt_b->long_option) {
        return opt_a->short_option - opt_b->long_option[0];
    } else {
        return opt_a->short_option - opt_b->short_option;
    }
}

static void print_spaces(int count) {
    for (int i = 0; i < count; i++) {
        putchar(' ');
    }
}

static void print_usage(const char *program, bool accepts_operands) {
    linked_list_sort(&option_list, &compare_options);

    printf("Usage: %s [OPTIONS]", program);
    if (accepts_operands) printf(" [OPERANDS]");
    printf("\n");

    linked_list_for_each(&option_list, WhistCommandLineOption, option) {
        print_spaces(2);
        if (option->short_option)
            printf("-%c, ", option->short_option);
        else
            print_spaces(4);

        int column = 6;
        if (option->long_option) {
            column += 4 + (int)strlen(option->long_option);
            printf("--%s  ", option->long_option);
        }
        if (column < 28) {
            print_spaces(28 - column);
            column = 28;
        }

        int total_columns = 80;
        const char *start, *end;
        start = option->help_text;
        while (1) {
            const char *space;
            end = start;
            for (space = start + 1;; space++) {
                if (*space == '\0') {
                    end = space;
                    break;
                }
                if (*space == ' ') end = space;
                if (end > start && column + space - start > total_columns) break;
            }
            printf("%.*s\n", (int)(end - start), start);
            if (*end == ' ') {
                start = end + 1;
                print_spaces(30);
                column = 30;
            } else {
                break;
            }
        }
    }
}

static void print_version(void) { printf("Whist client revision %s\n", whist_git_revision()); }

static const WhistCommandLineOption *find_short_option(int flag) {
    linked_list_for_each(&option_list, const WhistCommandLineOption, opt) {
        if (opt->short_option == flag) {
            return opt;
        }
    }
    printf("Unknown option '%c'.\n", flag);
    return NULL;
}

static const WhistCommandLineOption *find_long_option(const char *str, size_t len) {
    const WhistCommandLineOption *matched = NULL;
    linked_list_for_each(&option_list, const WhistCommandLineOption, opt) {
        if (!opt->long_option) continue;
        if (!strncmp(opt->long_option, str, len)) {
            if (matched) {
                printf("Ambiguous option \"%s\" matches both \"%s\" and \"%s\".\n", str,
                       matched->long_option, opt->long_option);
                return NULL;
            }
            matched = opt;
        }
    }
    if (!matched) {
        printf("Unknown option \"%s\".\n", str);
    }
    return matched;
}

static const WhistCommandLineOption *find_option(const char *name) {
    size_t len = strlen(name);
    if (len < 1) {
        return NULL;
    }
    const WhistCommandLineOption *opt = NULL;
    if (len == 1) {
        opt = find_short_option(name[0]);
    }
    if (opt == NULL) {
        opt = find_long_option(name, len);
    }
    return opt;
}

static WhistStatus call_option_handler(const WhistCommandLineOption *opt, bool is_short,
                                       const char *value) {
    if (LOG_COMMAND_LINE) {
        if (is_short)
            printf("Setting option '%c'", opt->short_option);
        else
            printf("Setting option \"%s\"", opt->long_option);
        if (value)
            printf(" with value \"%s\".\n", value);
        else
            printf(" with no value.\n");
    }
    if ((opt->flags & WHIST_OPTION_REQUIRED_ARGUMENT) && value == NULL) {
        LOG_WARNING("Argument value missing");
        return WHIST_ERROR_INVALID_ARGUMENT;
    }
    WhistStatus ret = opt->handler(opt, value);
    if (ret != WHIST_SUCCESS) {
        if (is_short)
            printf("Failed to parse option '%c'", opt->short_option);
        else
            printf("Failed to parse option \"%s\"", opt->long_option);
        if (value)
            printf(" with value \"%s\".\n", value);
        else
            printf(".\n");
        return ret;
    }
    return WHIST_SUCCESS;
}

static WhistStatus call_operand_handler(WhistOperandHandler handler, int position,
                                        const char *value) {
    WhistStatus ret = handler(position, value);
    if (ret != WHIST_SUCCESS) {
        printf("Failed to parse argument %d \"%s\".\n", position, value);
        return ret;
    }
    return WHIST_SUCCESS;
}

WhistStatus whist_set_single_option(const char *name, const char *value) {
    size_t len = strlen(name);
    if (len < 1) {
        return WHIST_ERROR_INVALID_ARGUMENT;
    }
    const WhistCommandLineOption *opt = NULL;
    if (len == 1) {
        // Possibly a short option.
        opt = find_short_option(name[0]);
        if (opt) {
            return call_option_handler(opt, true, value);
        }
    }
    // Long option.
    opt = find_long_option(name, len);
    if (opt) {
        return call_option_handler(opt, false, value);
    }
    return WHIST_ERROR_NOT_FOUND;
}

static void free_allocated_options(void) {
    linked_list_for_each(&option_list, WhistCommandLineOption, opt) {
        if (!(opt->flags & WHIST_OPTION_ALLOCATED)) {
            continue;
        }
        char **var = opt->variable;
        free(*var);
        *var = NULL;
    }
}

static void set_option_exit_handler(void) {
    static bool exit_handler_set = false;
    if (!exit_handler_set) {
        atexit(&free_allocated_options);
        exit_handler_set = true;
    }
}

WhistStatus whist_parse_command_line(int argc, const char **argv,
                                     WhistOperandHandler operand_handler) {
    bool show_help = false;
    WhistCommandLineOption help_option = {
        .long_option = "help",
        .help_text = "Show this help and exit.",
        .flags = WHIST_OPTION_INTERNAL | WHIST_OPTION_NO_ARGUMENT,
        .handler = &whist_handle_bool_option_internal,
        .variable = &show_help,
    };
    linked_list_add_tail(&option_list, &help_option);

    bool show_version = false;
    WhistCommandLineOption version_option = {
        .long_option = "version",
        .help_text = "Show version information and exit.",
        .flags = WHIST_OPTION_INTERNAL | WHIST_OPTION_NO_ARGUMENT,
        .handler = &whist_handle_bool_option_internal,
        .variable = &show_version,
    };
    linked_list_add_tail(&option_list, &version_option);

#ifndef NDEBUG
    // In a debug build, validate options.
    bool validation_fail = false;
    linked_list_for_each(&option_list, const WhistCommandLineOption, opt) {
        for (const WhistCommandLineOption *iter = linked_list_next(opt); iter;
             iter = linked_list_next(iter)) {
            if (opt->short_option && opt->short_option == iter->short_option) {
                printf("Conflicting short options '%c'.\n", opt->short_option);
                validation_fail = true;
            }
            if (opt->long_option && iter->long_option &&
                !strcmp(opt->long_option, iter->long_option)) {
                printf("Conflicting long options \"%s\".\n", opt->long_option);
                validation_fail = true;
            }
        }
    }
    if (validation_fail) {
        printf(
            "This build has conflicting command-line options, "
            "please fix and rebuild before continuing.\n");
        exit(EXIT_FAILURE);
    }
#endif

    // Add the exit handler
    set_option_exit_handler();

    const WhistCommandLineOption *opt;
    WhistStatus ret = WHIST_SUCCESS;

    int pos;
    for (pos = 1; pos < argc && ret == WHIST_SUCCESS; pos++) {
        const char *arg = argv[pos];
        if (arg[0] != '-' || arg[1] == '\0') {
            // Operand, including "-".
            ret = call_operand_handler(operand_handler, pos, arg);
        } else if (arg[1] == '-') {
            if (arg[2] == '\0') {
                // Isolated "--" ends options.
                ++pos;
                break;
            }
            // Long option.
            const char *eq = strchr(&arg[2], '=');
            size_t len;
            if (eq) {
                len = eq - &arg[2];
            } else {
                len = strlen(&arg[2]);
            }
            opt = find_long_option(&arg[2], len);
            if (!opt) {
                ret = WHIST_ERROR_NOT_FOUND;
                break;
            }
            if (opt->flags & WHIST_OPTION_REQUIRED_ARGUMENT) {
                if (eq) {
                    ret = call_option_handler(opt, false, eq + 1);
                } else if (pos + 1 == argc) {
                    printf("Option \"%s\" requires an argument.\n", opt->long_option);
                    ret = WHIST_ERROR_SYNTAX;
                } else {
                    ret = call_option_handler(opt, false, argv[pos + 1]);
                    ++pos;
                }
            } else if (opt->flags & WHIST_OPTION_OPTIONAL_ARGUMENT &&
                       (eq || (pos + 1 < argc && argv[pos + 1][0] != '-'))) {
                if (eq) {
                    ret = call_option_handler(opt, false, eq + 1);
                } else {
                    ret = call_option_handler(opt, false, argv[pos + 1]);
                    ++pos;
                }
            } else {
                if (eq) {
                    printf("Option \"%s\" does not take an argument.\n", opt->long_option);
                    ret = WHIST_ERROR_SYNTAX;
                } else {
                    ret = call_option_handler(opt, false, NULL);
                }
            }
        } else {
            // Short option(s).
            for (int i = 1; arg[i]; i++) {
                opt = find_short_option(arg[i]);
                if (!opt) {
                    ret = WHIST_ERROR_NOT_FOUND;
                    break;
                }
                if (opt->flags & WHIST_OPTION_REQUIRED_ARGUMENT) {
                    if (arg[i + 1]) {
                        ret = call_option_handler(opt, true, &arg[i + 1]);
                    } else if (pos + 1 < argc) {
                        ret = call_option_handler(opt, true, argv[pos + 1]);
                        ++pos;
                    } else {
                        printf("Option '%c' requires an argument.\n", opt->short_option);
                        ret = WHIST_ERROR_SYNTAX;
                    }
                    break;
                } else if (opt->flags & WHIST_OPTION_OPTIONAL_ARGUMENT && arg[i + 1]) {
                    ret = call_option_handler(opt, true, &arg[i + 1]);
                    break;
                } else if (opt->flags & WHIST_OPTION_OPTIONAL_ARGUMENT &&
                           (pos + 1 < argc && argv[pos + 1][0] != '-')) {
                    ret = call_option_handler(opt, true, argv[pos + 1]);
                    ++pos;
                    break;
                } else {
                    ret = call_option_handler(opt, true, NULL);
                }
            }
        }
    }

    if (show_help || show_version) {
        if (show_version) print_version();
        if (show_help) print_usage(argv[0], !!operand_handler);
        exit(0);
    }
    // The special options should no longer be visible after this call.
    linked_list_remove(&option_list, &help_option);
    linked_list_remove(&option_list, &version_option);

    for (; pos < argc && ret == WHIST_SUCCESS; pos++) {
        ret = call_operand_handler(operand_handler, pos, argv[pos]);
    }

    if (ret != WHIST_SUCCESS) {
        printf("Try \"%s --help\" for more information.\n", argv[0]);
    }
    return ret;
}

WhistStatus whist_option_get_int_value(const char *name, int *value) {
    const WhistCommandLineOption *opt = find_option(name);
    if (opt) {
        *value = *(int *)opt->variable;
        return WHIST_SUCCESS;
    } else {
        return WHIST_ERROR_NOT_FOUND;
    }
}

WhistStatus whist_option_get_bool_value(const char *name, bool *value) {
    const WhistCommandLineOption *opt = find_option(name);
    if (opt) {
        *value = *(bool *)opt->variable;
        return WHIST_SUCCESS;
    } else {
        return WHIST_ERROR_NOT_FOUND;
    }
}

WhistStatus whist_option_get_string_value(const char *name, const char **value) {
    const WhistCommandLineOption *opt = find_option(name);
    if (opt) {
        *value = *(const char **)opt->variable;
        return WHIST_SUCCESS;
    } else {
        return WHIST_ERROR_NOT_FOUND;
    }
}
