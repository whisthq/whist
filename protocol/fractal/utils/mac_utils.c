/*
 * MacOS helper functions for clipboard functions.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "mac_utils.h"

#include <stdio.h>
#include <string.h>

int remove_file(const char* fpath, const struct stat* sb, int typeflag,
                struct FTW* ftwbuf) {
    int errCode = remove(fpath);
    if (errCode < 0) {
        printf("Error from remove: %d", errCode);
    }
    return errCode;
}

void mac_rm_rf(const char* path) {
    int errCode =
        nftw(path, remove_file, 64 /* number of simultaneously opened fds*/,
             FTW_DEPTH | FTW_PHYS);
    if (errCode < 0) {
        printf("Error from nftw, remove recursively: %d", errCode);
    }
}

int cmp_files(const FTSENT** first, const FTSENT** second) {
    return (strcmp((*first)->fts_name, (*second)->fts_name));
}

void get_filenames(char* dir, char* filenames[]) {
    FTS* fhandle = NULL;
    FTSENT* child = NULL;
    FTSENT* parent = NULL;

    char* path[2] = {dir, NULL};
    fhandle = fts_open(path, FTS_COMFOLLOW, &cmp_files);

    int i = 0;

    if (fhandle != NULL) {
        while ((parent = fts_read(fhandle)) != NULL) {
            child = fts_children(fhandle, 0);

            while ((child != NULL)) {
                size_t pathlen = strlen(child->fts_path);
                strncpy(filenames[i], child->fts_path, pathlen);
                strncpy(filenames[i] + pathlen, "/", 1);
                strncpy(filenames[i] + pathlen + 1, child->fts_name,
                        child->fts_namelen);
                strncpy(filenames[i] + pathlen + 1 + child->fts_namelen, "\0",
                        1);
                child = child->fts_link;
                i++;
            }
        }
        fts_close(fhandle);
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
