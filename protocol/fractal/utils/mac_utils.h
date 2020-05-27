/**
Copyright Fractal Computers, Inc. 2020
@file mac_utils.h
@brief MacOS helper functions for clipboard functions.

============================
Usage
============================

*/

#ifndef MAC_UTILS_H
#define MAC_UTILS_H

#include <fts.h>
#include <ftw.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @param fpath path to file to be deleted
 * @param sb ?
 * @param typeflag ?
 * @param ftwbuf ?
 * @return
 */
int remove_file(const char* fpath, const struct stat* sb, int typeflag,
                struct FTW* ftwbuf);

/**
 * @brief recursive delete path
 * @param path path to be deleted
 */
void mac_rm_rf(const char* path);

/**
 * @brief uses strcmp to compare two file names
 * @param first file
 * @param second file
 * @return same return values as strcmp, e.g 0 for equality.
 */
int cmp_files(const FTSENT** first, const FTSENT** second);

/**
 * @param dir list files from dir
 * @param filenames allocated array of char*s to store found file names in
 */
void get_filenames(char* dir, char* filenames[]);

/**
 * @param path to directory to check existence
 * @return 1 if dir exists, -1 otherwise
 */
int dir_exists(const char* path);

#endif  // MAC_UTILS_H
