#ifndef FILE_DROP_H
#define FILE_DROP_H
/**
 * Copyright Copyright (c) 2021-2022 Whist Technologies, Inc.
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

/**
 * @brief                          A packet of data referring to and containing
 *                                 the information for an active file drag.
 */
typedef struct FileDragData {
    // int num_files;             // The number of files being dragged
    size_t file_uri_list_len;  // Number of bytes for the file uri list
    char data[0];              // The file uri list (separated by \0s)
} FileDragData;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the file drop handler if supported.
 *
 * @returns                        true if successful, false otherwise
 */
bool init_file_drop_handler(void);

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
 * @brief                          Create directories to house the file data and metadata,
 *                                 and write necesary metadata (for example, the file size)
 *                                 to the metadata file.
 *
 * @param id                       The unique ID for the file transfer
 * @param file_metadata            The metadata for the file being transferred
 *
 * @returns                        Returns the path to the file data directory on success,
 *                                 NULL on failure
 *
 * @note                           This is only for servers running a parallel FUSE filesystem
 */
const char* file_drop_prepare(int id, FileMetadata* file_metadata);

/**
 * @brief                          Write the ready file that indicates that the
 *                                 file has been fully written and the FUSE node is ready.
 *
 * @param id                       The unique ID for which we are writing the ready file
 *
 * @returns                        Returns -1 on failure, 0 on success
 *
 * @note                           This is only for servers running a parallel FUSE filesystem
 */
int file_drop_mark_ready(int id);

/**
 * @brief                          Clean up the file drop handler
 */
void destroy_file_drop_handler(void);

int file_drag_update(bool is_dragging, int x, int y, FileDragData* file_list);

#endif  // FILE_DROP_H
