#ifndef MAC_UTILS_H
#define MAC_UTILS_H
#include <stdio.h>
#include <ftw.h>
#include <fts.h>

int remove_file( const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf );
void mac_rm_rf( const char* path );
int cmp_files( const FTSENT** first, const FTSENT** second );
void get_filenames( char* dir, char* filenames[] );

int dir_exists( const char* path );


#endif // MAC_UTILS_H
