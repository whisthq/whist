/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file whistgetopt.h
 * @brief Declarations for glibc-compatible getopt_long().
 */
#ifndef WHIST_CORE_WHISTGETOPT_H
#define WHIST_CORE_WHISTGETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

// Structure type for options passed to getopt_long().
struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};

// Values for has_arg field in struct option.
#define no_argument 0
#define required_argument 1
#define optional_argument 2

// standard for POSIX programs
#define WHIST_GETOPT_HELP_CHAR (CHAR_MIN - 2)
#define WHIST_GETOPT_VERSION_CHAR (CHAR_MIN - 3)

// Globals used with getopt_long().
extern char *optarg;
extern int optind;

// glibc-compatible getopt_long().
// See glibc documentation for details.
int getopt_long(int argc, char *const argv[], const char *optstring, const struct option *longopts,
                int *longindex);

#ifdef __cplusplus
}
#endif

#endif /* WHIST_CORE_WHISTGETOPT_H */
