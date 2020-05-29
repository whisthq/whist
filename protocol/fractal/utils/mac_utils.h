#ifndef MAC_UTILS_H
#define MAC_UTILS_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file mac_utils.h
 * @brief This file contains the MacOS helper functions for clipboard functions.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include <fts.h>
#include <ftw.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Delete a file on MacOS
 *
 * @param fpath                    Path to file to be deleted
 * @param sb                       File path stat
 * @param typeflag                 FTW flags
 * @param ftwbuf                   FTW buffer flags, unique to MacOS
 *
 * @returns
 */
int remove_file(const char* fpath, const struct stat* sb, int typeflag,
                struct FTW* ftwbuf);

/**
 * @brief                         Recurisvely delete a path on MacOS
 *
 * @param path                    Path to be deleted
 */
void mac_rm_rf(const char* path);

/**
 * @brief                         Uses strcmp to compare two file names
 *
 * @param first                   First file to compare
 * @param second                  Second file to compare with
 *
 * @returns                       Same return values as strcmp, e.g 0 for
 *                                equality
 */
int cmp_files(const FTSENT** first, const FTSENT** second);

/**
 * @brief                         List the files in a directory
 *
 * @param dir                     The directory to list the files from
 * @param filenames               An allocated array of char*'s to store the
 *                                found file names in
 */
void get_filenames(char* dir, char* filenames[]);

/**
 * @brief                         Check if a directory exists on MacOS
 *
 * @param path                    The directory to check the existence of
 *
 * @returns                       1 if dir exists, -1 otherwise
 */
int dir_exists(const char* path);

#endif  // MAC_UTILS_H
