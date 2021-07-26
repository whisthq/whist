/**
 * Copyright Fractal Computers, Inc. 2020
 * @file mac_utils.c
 * @brief This file contains the MacOS helper functions for clipboard functions.
============================
Usage
============================

You can use these functions to check if a directory exist, to compare filenames,
or to delete files/folders
*/

#include "mac_utils.h"

#include <stdio.h>
#include <string.h>

#include <fractal/logging/logging.h>

int remove_file(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    int err_code = remove(fpath);
    if (err_code < 0) {
        LOG_WARNING("Error from remove: %d", err_code);
    }
    return err_code;
}

void mac_rm_rf(const char* path) {
    int err_code =
        nftw(path, remove_file, 64 /* number of simultaneously opened fds*/, FTW_DEPTH | FTW_NS);
    if (err_code < 0) {
        LOG_WARNING("Error from nftw, remove recursively: %d", err_code);
    }
}

int dir_exists(const char* path) {
    struct stat sb;
    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        return 1;
    } else {
        return -1;
    }
}
