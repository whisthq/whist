#ifndef FILE_DROP_H
#define FILE_DROP_H
/**
 * Copyright Copyright 2021 Whist Technologies, Inc.
 * @file file_drop.h
 * @brief This file contains all the code for dropping a file into a window.
============================
Usage
============================

init_file_drop_handler();

TransferringFile* drop_file;
--- GET FILE INFO ---
drop_file_into_active_window(drop_file);

destroy_file_drop_handler();

*/

/*
============================
Includes
============================
*/

#include <whist/file/file_synchronizer.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the X11 display and XDND atoms for the file
 *                                 drop handler
 */
void init_file_drop_handler();

/**
 * @brief                          Use the XDND protocol to execute a drag and drop of
 *                                 `drop_file` onto the active X11 window.
 *
 * @param drop_file                The file to be dropped into the active X11 window
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int drop_file_into_active_window(TransferringFile* drop_file);

/**
 * @brief                          Write the ready file that indicates that the
 *                                 file has been fully written and the FUSE node is ready.
 *
 * @param id                       The unique ID for which we are writing the ready file
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int write_fuse_ready_file(int id);

/**
 * @brief                          Clean up the file drop handler
 */
void destroy_file_drop_handler();

#endif  // WINDOW_NAME_H
